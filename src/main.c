/*
 * SimCity SNES Recomp — desktop host shim.
 *
 * Identity and hooks, nothing else. The host itself is the framework's
 * (snesrecomp/runner/src/desktop/host_main.h): the pre-boot launcher, ROM
 * resolution and digest checks, config.ini and keybinds.ini, the window and
 * the SDL / OpenGL presenters, audio, gamepads, the in-game save-state
 * browser and rewind filmstrip, the OSD, the pacing clock, crash handlers and
 * the post-mortem report, mod packages, Generate & rebuild, and the netplay
 * barrier when the project is built with it. A fix there reaches this
 * project on a submodule pull; nothing in this file needs to change for it.
 *
 * Nothing here ever ships a ROM. The player supplies one; the host makes
 * that easy (launcher, then positional argument, then a copy beside the
 * executable, then the rom.cfg cache, then a file picker) and checks what it
 * is handed against the digests in rom_identity.txt.
 *
 * Grow this file only with what is specific to THIS title: a custom
 * presenter (prepare_frame / draw_frame), an SPC player, a pacing rule
 * (keep_pacing_debt), a Mods provider. See SnesDesktopHostGame for the
 * complete list of hooks and what each is for.
 */

#include "game_rtl.h"
#include "snesrecomp_rom_identity.h"  /* generated from rom_identity.txt */
#include "host_main.h"
#include "widescreen.h"
#include "common_rtl.h"
#include "common_cpu_infra.h"
#include "cpu_state.h"
#include "snes/snes.h"
#include "snes/cpu.h"
#include "snes/interp_bridge.h"
#include "snes/dma.h"
#include <time.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#ifndef SNES_GAME_VERSION
#define SNES_GAME_VERSION "dev"
#endif

/* Forward declarations for presenter callbacks. */
static void SimCityPrepareFrame(int drawable_w, int drawable_h, int *frame_w, int *frame_h);
static void SimCityBeginSimFrame(unsigned number);
static void SimCityEndSimFrame(const uint8_t *field, unsigned number);
static int SimCityDrawFrame(uint8_t *dst, size_t pitch, const uint8_t *field,
                            int frame_w, int frame_h, double alpha);
static double SimCityPresentationHz(double display_refresh);
static int SimCityWindowBaseWidth(int frame_w);
static int SimCityWindowBaseHeight(void);

/* Mod frame callback - declared in gen_stubs.c */
static void SimCity_ModFrameCallback(void);

/* Config callback - called after config.ini is parsed. */
static void SimCityAfterConfig(void);

/* ========================================================================
 * Debug / Watchdog Instrumentation
 * ======================================================================== */


static int g_last_watchdog_frame = -1;

/* Custom watchdog handler - captures context before longjmp */
static void SimCity_WatchdogHandler(void)
{
    if (!g_debug_watchdog) return;
    
    g_watchdog_triggered = 1;
    g_watchdog_frame_start = clock();
    
    extern int snes_frame_counter;
    extern int g_recomp_stack_top;
    extern const char *g_recomp_stack[];
    extern const char *g_last_recomp_func;
    extern int g_watchdog_counter;
    
    double elapsed = (double)(clock() - g_watchdog_frame_start) / CLOCKS_PER_SEC;
    
    fprintf(stderr,
      "\n=== SIMCITY WATCHDOG: Frame %d exceeded %.1fs ===\n"
      "Game mode: %d | WatchdogCheck calls: %d\n"
      "Recomp stack depth: %d\n"
      "Call stack (most recent first):\n",
      snes_frame_counter, elapsed, g_ram[0x100], g_watchdog_counter * 10000,
      g_recomp_stack_top);
    
for (int i = g_recomp_stack_top - 1; i >= 0; i--)
        fprintf(stderr, "  [%d] %s\n", g_recomp_stack_top - 1 - i, g_recomp_stack[i]);
    if (g_recomp_stack_top == 0)
        fprintf(stderr, "  (empty — last was %s)\n", g_last_recomp_func ? g_last_recomp_func : "(none)");
    
    fprintf(stderr, "\n");
    fflush(stderr);
    
    /* Dump CPU state */
    extern struct Cpu *g_snes_cpu;
    fprintf(stderr, "CPU State: PC=%06X A=%04X X=%04X Y=%04X S=%04X D=%04X DB=%02X PB=%02X\n",
            g_snes_cpu->pc, g_snes_cpu->a, g_snes_cpu->x, g_snes_cpu->y, g_snes_cpu->sp, g_snes_cpu->dp, g_snes_cpu->db, g_snes_cpu->k);
    fprintf(stderr, "M=%d X=%d E=%d I=%d C=%d Z=%d V=%d N=%d\n",
            g_snes_cpu->mf, g_snes_cpu->xf, g_snes_cpu->e, g_snes_cpu->i,
            g_snes_cpu->c, g_snes_cpu->z, g_snes_cpu->v, g_snes_cpu->n);
    fflush(stderr);
}

/* Install custom watchdog handler */
extern void SimCity_InstallWatchdogHandler(void) {
    const char *env = getenv("SIMCITY_DEBUG_WATCHDOG");
    g_debug_watchdog = (env && atoi(env) != 0);
    
    env = getenv("SIMCITY_DEBUG_DMA");
    g_debug_dma = (env && atoi(env) != 0);
    
    env = getenv("SIMCITY_DEBUG_APU");
    g_debug_apu = (env && atoi(env) != 0);
    
    if (g_debug_watchdog || g_debug_dma || g_debug_apu) {
        fprintf(stderr, "[SIMCITY DEBUG] Watchdog=%d DMA=%d APU=%d\n",
                g_debug_watchdog, g_debug_dma, g_debug_apu);
    }
}

/* DMA debug logging */
void SimCity_LogDMA(int channel, int fromB, uint8_t aBank, uint16_t aAdr, uint16_t size, uint8_t bAdr)
{
    if (!g_debug_dma) return;
    
    extern Snes *g_snes;
    if (!g_snes || !g_snes->cart) return;
    
    const char *cart_type = "UNKNOWN";
    switch (g_snes->cart->type) {
        case 0: cart_type = "LoROM"; break;
        case 1: cart_type = "HiROM"; break;
        case 2: cart_type = "SA-1"; break;
        case 3: cart_type = "SuperFX"; break;
        case 4: cart_type = "Cx4"; break;
        case 5: cart_type = "DSP1"; break;
        case 6: cart_type = "DSP2"; break;
        case 7: cart_type = "SDD1"; break;
        case 8: cart_type = "OBC1"; break;
    }
    
    fprintf(stderr,
        "[SIMCITY DMA DEBUG] ch=%d fromB=%d aBank=$%02X aAdr=$%04X size=%u bAdr=$%02X cart=%s\n",
        channel, fromB, aBank, aAdr, size, bAdr, cart_type);
    
    /* Check if source is in system area mirror */
    if (!fromB && (aBank & 0x80) && (aAdr & 0x8000) == 0) {
        uint8_t mapped_bank = aBank & 0x7F;
        fprintf(stderr,
            "  [DMA WARNING] Source $%02X:%04X in system area mirror (maps to $%02X:%04X)\n",
            aBank, aAdr, mapped_bank, aAdr);
    }
    fflush(stderr);
}

/* APU sync debug */
void SimCity_LogAPUSync(const char *phase, int cycles)
{
    if (!g_debug_apu) return;
    
    extern CpuState g_cpu;
    fprintf(stderr,
        "[SIMCITY APU DEBUG] %s at master_cycles=%llu (%.3f ms)\n",
        phase, (unsigned long long)g_cpu.master_cycles, (double)g_cpu.master_cycles / 21477272.0 * 1000.0);
    fflush(stderr);
}

/* ========================================================================
 * Config Callback - Mod Settings via Environment Variables
 * ======================================================================== */

static void SimCityAfterConfig(void)
{
    SimCity_InstallWatchdogHandler();
    
    /* Read mod settings from environment variables.
     * Use standard C getenv() since HostGetenv is not exposed in public API. */
    extern int g_mod_widescreen_enabled;
    extern int g_mod_godmode_enabled;
    extern int g_mod_disaster_toggle;
    
    const char *ws = getenv("SIMCITY_WIDESCREEN");
    const char *gm = getenv("SIMCITY_GODMODE");
    const char *dt = getenv("SIMCITY_DISASTER_TOGGLE");
    
    g_mod_widescreen_enabled = (ws && atoi(ws) != 0);
    g_mod_godmode_enabled = (gm && atoi(gm) != 0);
    g_mod_disaster_toggle = (dt ? atoi(dt) : 0);
}

static const SnesDesktopHostGame kGameHost = {
    .display_name        = "SimCity SNES Recomp",
    .window_title        = "SimCity SNES Recomp",
    .region              = SNESRECOMP_ROM_REGION,
    .rom_file            = SNESRECOMP_ROM_FILE,
    .expected_sha256_hex = SNESRECOMP_ROM_EXPECTED_SHA256,
    .expected_crc32_hex  = SNESRECOMP_ROM_EXPECTED_CRC32,
    .game_id             = SNESRECOMP_ROM_GAME_ID,
    .build_version       = SNES_GAME_VERSION,
    .game_info           = &kGameInfo,
    .num_players         = 1,
    /* Battery-backed SRAM shows the launcher's SAVES panel. */
    .sram_path           = "saves/save.srm",
    /* Widescreen support for isometric renderer */
    .widescreen_supported = 1,
    .native_widescreen   = 1,
    .frame_width         = 336,  /* 256 + 2*40 (default 40px margins per side) */
    .frame_height        = 224,
    /* Presenter callbacks for widescreen/isometric rendering */
    .prepare_frame       = SimCityPrepareFrame,
    .begin_sim_frame     = SimCityBeginSimFrame,
    .end_sim_frame       = SimCityEndSimFrame,
    .draw_frame          = SimCityDrawFrame,
    .presentation_hz     = SimCityPresentationHz,
    .window_base_width   = SimCityWindowBaseWidth,
    .window_base_height  = SimCityWindowBaseHeight,
    /* Config callback for mod settings */
    .after_config        = SimCityAfterConfig,
};

int main(int argc, char **argv)
{
    /* Register the game with the framework. */
    RtlRegisterGame(&kGameInfo);

    /* Hand off to the framework's desktop host. */
    return snesrecomp_desktop_main(&kGameHost, argc, argv);
}

/* ========================================================================
 * Presenter Callback Implementations
 * ======================================================================== */

static void SimCityPrepareFrame(int drawable_w, int drawable_h, int *frame_w, int *frame_h)
{
    /* Compute frame width based on widescreen setting.
     * Use the game descriptor's frame_width (336 = 256 + 2*40) directly
     * since g_snes_width is set after prepare_frame returns. */
    *frame_w = 336;  /* 256 + 2*40 (default 40px margins per side) */
    *frame_h = 224;
}

static void SimCityBeginSimFrame(unsigned number)
{
    (void)number;
    /* Configure widescreen margins for isometric tilemap rendering.
     * For SimCity's isometric view, we use symmetric margins on left/right.
     * Bottom margin is 0 since we don't extend vertically. */
    extern bool g_ws_active;
    extern int g_ws_extra;
    if (g_ws_active && g_ws_extra > 0) {
        /* Set symmetric widescreen border */
        PpuSetExtraSpace(g_ppu, (uint16_t)g_ws_extra);
        
        /* For isometric city view: background layers (BG1=terrain, BG2=sprites) 
         * should render into margins. HUD layers should stay clamped. */
        /* BG1 (terrain) and BG2 (buildings/sprites) get widescreen */
        /* BG3 (HUD) stays clamped to native 256 */
        PpuSetWidescreenLayerClamp(g_ppu, 0x04);  /* Clamp BG3 only (bit 2 = layer 3) */
    }
    
    /* Run mod frame callback for persistent mods (GodMode, forced disasters) */
    SimCity_ModFrameCallback();
}

static void SimCityEndSimFrame(const uint8_t *field, unsigned number)
{
    (void)field;
    (void)number;
    /* Nothing special needed at end of frame */
}

static int SimCityDrawFrame(uint8_t *dst, size_t pitch, const uint8_t *field,
                            int frame_w, int frame_h, double alpha)
{
    /* Use the framework's shared widescreen present function.
     * This handles the framebuffer copy with proper pitch/centering. */
    RtlWidescreenPresent(dst, pitch, field, frame_w, frame_h);
    return 1;  /* Frame was presented */
}

static double SimCityPresentationHz(double display_refresh)
{
    /* Present at simulation rate (60.0988 Hz) - no interpolation needed
     * for deterministic simulation. Return 0 to let host present every
     * simulated frame. */
    return 0.0;
}

static int SimCityWindowBaseWidth(int frame_w)
{
    extern bool g_ws_active;
    extern int g_ws_extra;
    /* For widescreen: use the full frame width (including margins).
     * For native: use 4:3 aspect on 224 lines (256 -> 320). */
    if (g_ws_active && g_ws_extra > 0) {
        return frame_w;  /* Full widescreen frame width (336) */
    }
    /* 4:3 on a 224-line window: 256 -> 320. */
    return (frame_w * 5 + 2) / 4;
}

static int SimCityWindowBaseHeight(void)
{
    return 224;  /* Native SNES height */
}

