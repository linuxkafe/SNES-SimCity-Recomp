/*
 * SimCity SNES Recomp — per-game runtime glue.
 *
 * THIS FILE IS THE PORT. Everything else the scaffold produced is layout,
 * build wiring, and packaging; this is where the actual work happens.
 *
 * The framework does not, and cannot, drive an arbitrary SNES game on its
 * own. Two responsibilities always land on the game host:
 *
 *   1. Deciding what "one frame" means for THIS title, and returning from
 *      run_frame() at that boundary. A real ROM's reset vector never
 *      returns — it enters a main loop that waits on vblank — so somebody
 *      has to choose the yield point.
 *   2. Delivering NMI/IRQ at the hardware edge (see
 *      snesrecomp/docs/FRAME_MODEL_HOSTS.md).
 *
 * What follows is the shape every working port in this ecosystem converged
 * on, built from documented bridge entry points:
 *
 *      boot from the reset vector
 *      -> deliver NMI at the vblank edge, gated on NMITIMEN
 *      -> run the guest in slices until it parks or the frame's clock is out
 *      -> service a raster IRQ whenever the comparator asserts
 *      -> rasterize the field with HDMA per line
 *
 * It is a starting point, not a finished port: a title with unusual timing
 * will need the slice loop tightened. What it will NOT do is sit at a
 * black screen doing nothing, which is what a driver that delivers no
 * interrupt always does.
 */

#include "game_rtl.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common_cpu_infra.h"
#include "host_report.h"
#include "common_rtl.h"
#include "cpu_state.h"
#include "snes/dma.h"
#include "snes/interp_bridge.h"
#include "snes/ppu.h"
#include "snes/snes.h"

extern CpuState g_cpu;
extern Ppu *g_ppu;
extern Snes *g_snes;

/* One NTSC frame: 262 scanlines x 1364 master clocks. Bounds a productive
 * MMIO loop so it cannot run across several vblanks atomically. */
#define GAME_MASTER_CYCLES_PER_FRAME 357368ull

/* How many times a frame may re-enter the guest after it parks on a poll.
 * Each slice ends at a deterministic read-only cycle or at an IRQ; the bound
 * only stops a pathological loop from spinning the host. */
#define GAME_MAX_SLICES_PER_FRAME 64

/* 0 until the first frame has booted from the reset vector. */
static uint32_t g_resume_pc;

static uint32_t read_vector(uint32_t addr)
{
    /* Read through the guest bus so a mapper or coprocessor window resolves
     * the same way the CPU sees it. */
    uint32_t lo = snes_read(g_snes, addr);
    uint32_t hi = snes_read(g_snes, addr + 1u);
    return ((hi & 0xFFu) << 8) | (lo & 0xFFu);
}
static uint32_t reset_vector(void) { return read_vector(0x00FFFCu); }
static uint32_t nmi_vector(void)   { return read_vector(0x00FFEAu); }
static uint32_t irq_vector(void)   { return read_vector(0x00FFEEu); }

/* The resume point is host state, and NOTHING carries it.
 *
 * g_resume_pc is a static in this file. It is not in the Snes, not in the CPU -
 * snes_saveload restores the CPU with pc=0000, verified by logging the
 * restored register right after a load - and not in any blob. So a state
 * restored into a fresh process leaves it at 0, the frame loop reads
 * `booting = (g_resume_pc == 0)` as true, and the guest is answered with the
 * reset vector instead of with where it was.
 *
 * The symptom was a state taken from a running city that came back as a booting
 * machine: resuming at $008000 - the reset vector - with S=01FF and DP=0000,
 * boot-time register values, drawn over WRAM that still held the city. One byte
 * of WRAM then moved in 3600 frames and the picture was an early-boot screen.
 * Two rounds of measurement were spent on that, all of it about a boot.
 *
 * So the resume point goes in the game's own chunk, which is what
 * state_save_extra and state_load_extra exist for. */
static void SimCityStateSaveExtra(SaveLoadInfo *sli) {
    uint32 resume = g_resume_pc;
    sli->func(sli, &resume, sizeof resume);
}

static void SimCityStateLoadExtra(SaveLoadInfo *sli, uint32 version) {
    (void)version;
    uint32 resume = 0;
    sli->func(sli, &resume, sizeof resume);
    if (resume == 0) return;
    g_resume_pc = resume;
    host_report_breadcrumb("state loaded: resume PC restored as $%06X", resume);
}

/* Whether to log the resume point each frame. It is only meaningful at frame
 * boundaries — the bridge syncs the interpreter into g_cpu there, whereas
 * inside an interrupt run g_cpu describes the AOT tier, not the interpreter
 * (the mistake that produced T050's first, refuted hypothesis). The question
 * it answers: on which frame does S leave the main loop's value, and does the
 * NMI of that frame move it. Off unless armed. */
static int frame_slog_enabled(void)
{
    static int on = -1;
    if (on < 0) {
        const char *e = getenv("SNESRECOMP_FRAME_SLOG");
        on = (e && e[0] && e[0] != '0') ? 1 : 0;
    }
    return on;
}

static void frame_slog(const char *tag)
{
    if (!frame_slog_enabled())
        return;
    extern int snes_frame_counter;
    fprintf(stderr, "[fslog] f=%d %-7s S=%04X PB=%02X DP=%04X resume=%06X\n",
            snes_frame_counter, tag, (unsigned)g_cpu.S, (unsigned)g_cpu.PB,
            (unsigned)g_cpu.D, (unsigned)g_resume_pc);
}

/* T050 (SNESRECOMP_NMI_PRESERVE_S=1): make the NMI atomic with respect to the
 * guest's own registers.
 *
 * MEASURED (f878, s3.script, SNESRECOMP_FRAME_SLOG=1):
 *
 *   f=877 slice0 enter  S=1FEA resume=009313
 *   f=877 slice0 exit   S=1FEA resume=009315
 *   f=878 post-nmi      S=1F7F resume=009315   <-- NMI moved S by -107
 *   f=878 slice0 enter  S=1F7F resume=009315   <-- main loop resumes MID-ROUTINE
 *   f=878 slice0 exit   S=1F76 resume=00375E   <-- RTS $9317 popped $0000
 *
 * $9311 INC $C7 / $9313 LDA $B9 / $9315 BEQ / $9317 RTS is the guest's vblank
 * wait AND this host's frame park point. The main loop is inside it with its
 * JSR return frame on the stack; the NMI handler's long path relocates the
 * guest stack to its own base and leaves it there, so the RTS that follows
 * pops a stack the main loop is not using.
 *
 * The 65816 interrupt frame does not carry S, so nothing forces the handler to
 * put it back — and this handler restores a CONSTANT ($1F7F), not the
 * interrupted S ($1FEA). Holding a resume PC inside a subroutine while letting
 * the handler move the stack under it is this host's inconsistency, not the
 * game's: the frame boundary is our artefact, hardware has no such thing
 * mid-subroutine.
 *
 * Env-gated only so it can be A/B'd against a build without it;
 * SNESRECOMP_NMI_PRESERVE_S=0 restores the old behaviour. Default ON: the
 * gates below are the evidence, not the argument.
 *
 *   s3.script, 1200 frames, headless luma of the presented frame
 *     off:  f900..f1200  mean=0.00   (black; $2100 pinned to 00)
 *     on:   f900..f1200  mean=122.8  (city view, live and stable)
 *   s2.script, 1400 frames
 *     off:  f925..f1400  static VRAM garbage (two luma values, scene frozen)
 *     on:   f900..f1400  mean=122.8  (city view, live and stable)
 *   SNESRECOMP_TRAP_BADPB=1, s3.script, 3000 frames
 *     off:  trap at frame 878 ($009317 RTS popped $0000 -> $000001)
 *     on:   0 hits, exit 0
 */
static int nmi_preserve_regs(void)
{
    static int on = 1;
    if (on < 0) {
        const char *e = getenv("SNESRECOMP_NMI_PRESERVE_S");
        on = (e && e[0] && e[0] == '0') ? 0 : 1;
    }
    return on;
}

/* Run one interrupt handler to its RTI, entered as hardware enters it: the
 * frame is pushed at the PC the guest was interrupted AT, so the handler's
 * terminal RTI returns into that instruction stream. */
static void game_run_interrupt(uint32_t vector, uint64_t frame_end)
{
    const int preserve = nmi_preserve_regs();
    const uint16_t s0 = g_cpu.S, d0 = g_cpu.D;
    const uint8_t pb0 = g_cpu.PB, db0 = g_cpu.DB;
    const uint32_t resume0 = g_resume_pc;

    cpu_push_interrupt_frame_at(&g_cpu, g_resume_pc);
    interp_bridge_set_master_deadline(frame_end);
    (void)interp_bridge_run_interrupt(&g_cpu, vector);
    /* Clearing matters: a deadline left armed stays true for every AOT block
     * prologue afterwards, which turns every compiled body into an immediate
     * yield-unwind. */
    interp_bridge_set_master_deadline(0);
    if (preserve) {
        /* The handler's RTI already returns to the PC it was entered from;
         * what it must not do is relocate the stack we are holding a resume
         * PC inside. Restore the interrupted register set wholesale. */
        g_cpu.S = s0; g_cpu.D = d0; g_cpu.PB = pb0; g_cpu.DB = db0;
        g_resume_pc = resume0;
    } else {
        uint32_t resume = interp_bridge_lle_resume_pc();
        if (resume)
            g_resume_pc = resume;
    }
}

void GameRunOneFrame(void)
{
    const uint64_t frame_end = g_cpu.master_cycles + GAME_MASTER_CYCLES_PER_FRAME;
    const int booting = (g_resume_pc == 0);
    int slice;

    if (booting)
        g_resume_pc = reset_vector();

    /* Vblank edge. NMITIMEN gates it: delivering before the guest has enabled
     * NMI would land an interrupt frame in the middle of its SEI boot
     * sequence. Nothing is delivered on the very first frame either — reset
     * has not run yet, so there is no instruction stream to interrupt. */
    if (!booting && g_snes->nmiEnabled) {
        g_snes->inNmi = true;
        frame_slog("pre-nmi");
        game_run_interrupt(nmi_vector(), frame_end);
        frame_slog("post-nmi");
        g_snes->inNmi = false;
    }

    /* Run the guest until it parks on a read-only poll (its vblank wait) or
     * the frame's clock runs out. A single call is not enough: the guest
     * typically parks several times per frame — on HVBJOY, on a DMA-complete
     * flag, on its own state machine — and each park needs either an
     * interrupt or simply more time. */
    for (slice = 0; slice < GAME_MAX_SLICES_PER_FRAME; slice++) {
        if (g_cpu.master_cycles >= frame_end)
            break;
        interp_bridge_set_master_deadline(frame_end);
        {
            /* T050: the resume PC this slice STARTS at is the one that decides
             * whether a guest `RTS` later has a return frame to pop. A slice
             * that starts mid-subroutine must have inherited that subroutine's
             * JSR frame; log it before the run so the two can be compared. */
            extern int snes_frame_counter;
            if (frame_slog_enabled())
                fprintf(stderr, "[fslog] f=%d slice%-2d enter  S=%04X resume=%06X\n",
                        snes_frame_counter, slice, (unsigned)g_cpu.S,
                        (unsigned)g_resume_pc);
        }
        interp_bridge_run_until_quiescent(&g_cpu, g_resume_pc);
        interp_bridge_set_master_deadline(0);
        {
            uint32_t resume = interp_bridge_lle_resume_pc();
            if (resume)
                g_resume_pc = resume;
        }
        {
            extern int snes_frame_counter;
            if (frame_slog_enabled())
                fprintf(stderr, "[fslog] f=%d slice%-2d exit   S=%04X resume=%06X wai=%d\n",
                        snes_frame_counter, slice, (unsigned)g_cpu.S,
                        (unsigned)g_resume_pc,
                        interp_bridge_lle_took_wai() ? 1 : 0);
        }

        /* A raster IRQ asserted while the guest ran: service it before
         * continuing, exactly as the CPU samples it between instructions. */
        if (g_snes->inIrq && !g_cpu._flag_I) {
            game_run_interrupt(irq_vector(), frame_end);
            continue;
        }
        /* Parked with no interrupt pending and clock left over: the guest is
         * waiting for the next vblank. Nothing more happens this frame. */
        if (interp_bridge_lle_took_wai())
            break;
    }
    frame_slog("frame-end");
}

void GameDrawPpuFrame(void)
{
    SimpleHdma hdma_chans[8];
    Dma *dma = g_snes->dma;
    int trigger;
    int line, ch;

    /* Re-arm HDMA from the last $420C (HDMAEN) the guest wrote — typically
     * during the NMI just run. The framework records it for exactly this. */
    dma_startDma(dma, g_snesrecomp_last_hdmaen, true);
    for (ch = 0; ch < 8; ch++)
        SimpleHdma_Init(&hdma_chans[ch], &dma->channel[ch]);

    /* Mid-frame raster split, if the guest programmed the V comparator. */
    trigger = g_snes->vIrqEnabled ? (int)g_snes->vTimer : -1;

    /* From line 0, not line 1: starting at 1 leaves the top scanline holding
     * the previous frame's state, which shows up as a stripe of stale
     * tilemap above a HUD. */
    for (line = 0; line <= 224; line++) {
        /* HDMA runs in the H-blank BEFORE each visible line, and the raster
         * IRQ then selects the register set that line is drawn with — so
         * both must precede ppu_runLine for this line, not follow it. */
        for (ch = 0; ch < 8; ch++)
            SimpleHdma_DoLine(&hdma_chans[ch]);
        if (line == trigger) {
            g_snes->inIrq = true;
            cpu_push_interrupt_frame_at(&g_cpu, g_resume_pc);
            (void)interp_bridge_run_interrupt(&g_cpu, irq_vector());
            g_snes->inIrq = false;
            {
                uint32_t resume = interp_bridge_lle_resume_pc();
                if (resume)
                    g_resume_pc = resume;
            }
            trigger = g_snes->vIrqEnabled ? (int)g_snes->vTimer : -1;
        }
        ppu_runLine(g_ppu, line);
    }
}

const RtlGameInfo kGameInfo = {
    .title = "SimCitySNESRecomp",
    .initialize = NULL,
    .run_frame = &GameRunOneFrame,
    .draw_ppu_frame = &GameDrawPpuFrame,
    .save_name_prefix = "save",
    .state_save_extra = &SimCityStateSaveExtra,
    .state_load_extra = &SimCityStateLoadExtra,
};

void GameSessionReset(void)
{
    /* Rematch / soft-return: clear anything that must not survive a new
     * session. recomp-ai-rules/NETPLAY.md §3 — sticky state that "has always
     * been fine" is the usual desync culprit, because single-player never
     * re-enters the boot path twice in one process. */
    g_resume_pc = 0;
}