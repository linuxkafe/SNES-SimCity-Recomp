/* Headless measurement runner for Junior-Jones/SimCity-SNES-Static-Recomp.
 * Uses ONLY the published public API. Writes nothing into the emu state
 * except advance()/sram_load(). Measurement only.
 *
 * Usage: jjhead <rom> <srm_in|-> <srm_out> <frames> <script|-> <dumpdir>
 * Script lines: "wait N" | "press <btn> N"
 */
#include "simcity_static_recomp.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int read_file(const char *path, uint8_t **data, size_t *size) {
    FILE *f = fopen(path, "rb");
    long n;
    if (!f) return 0;
    fseek(f, 0, SEEK_END); n = ftell(f); fseek(f, 0, SEEK_SET);
    *data = (uint8_t *)malloc((size_t)n);
    if (!*data || fread(*data, 1, (size_t)n, f) != (size_t)n) { fclose(f); return 0; }
    fclose(f); *size = (size_t)n;
    return 1;
}

static uint16_t mask_of(const char *b) {
    if (!strcmp(b, "b")) return SIMCITY_INPUT_B;
    if (!strcmp(b, "a")) return SIMCITY_INPUT_A;
    if (!strcmp(b, "x")) return SIMCITY_INPUT_X;
    if (!strcmp(b, "y")) return SIMCITY_INPUT_Y;
    if (!strcmp(b, "l")) return SIMCITY_INPUT_L;
    if (!strcmp(b, "r")) return SIMCITY_INPUT_R;
    if (!strcmp(b, "up")) return SIMCITY_INPUT_UP;
    if (!strcmp(b, "down")) return SIMCITY_INPUT_DOWN;
    if (!strcmp(b, "left")) return SIMCITY_INPUT_LEFT;
    if (!strcmp(b, "right")) return SIMCITY_INPUT_RIGHT;
    if (!strcmp(b, "start")) return SIMCITY_INPUT_START;
    if (!strcmp(b, "select")) return SIMCITY_INPUT_SELECT;
    fprintf(stderr, "unknown button %s\n", b);
    exit(2);
}

int main(int argc, char **argv) {
    uint8_t *rom = 0; size_t rom_size = 0;
    SimCityRecomp *inst = 0; char err[512];
    SimCityRecompFrameResult res;
    uint32_t sram_size; void *sram;
    FILE *log;
    const char *dumpdir;
    uint32_t total_frames, done = 0, dump_every, run_frame = 0;
    char path[1024];
    /* scheduled presses: frame -> mask, sparse table */
    uint32_t *press_at = 0; uint16_t *press_mask = 0; uint32_t n_press = 0, cap = 0;

    if (argc < 7) {
        fprintf(stderr, "usage: %s rom srm_in srm_out frames script dumpdir\n", argv[0]);
        return 2;
    }
    dumpdir = argv[6];
    if (!read_file(argv[1], &rom, &rom_size)) { fprintf(stderr, "cannot read rom %s\n", argv[1]); return 1; }
    total_frames = (uint32_t)strtoul(argv[4], NULL, 0);
    dump_every = 60u;

    /* ---- script: build a per-frame input schedule ---- */
    if (strcmp(argv[5], "-") != 0) {
        FILE *sf = fopen(argv[5], "r");
        char line[256];
        if (!sf) { fprintf(stderr, "cannot read script %s\n", argv[5]); return 1; }
        while (fgets(line, sizeof line, sf)) {
            char verb[32], btn[32]; unsigned n;
            if (line[0] == '#' || line[0] == '\n') continue;
            if (sscanf(line, "%31s %31s %u", verb, btn, &n) == 3 && !strcmp(verb, "press")) {
                if (n_press == cap) { cap = cap ? cap * 2 : 64; press_at = realloc(press_at, cap * sizeof *press_at); press_mask = realloc(press_mask, cap * sizeof *press_mask); }
                press_at[n_press] = done; press_mask[n_press] = mask_of(btn); n_press++;
                for (unsigned i = 0; i < n; i++) { done++; }
            } else if (sscanf(line, "%31s %u", verb, &n) == 2 && !strcmp(verb, "wait")) {
                done += n;
            } else if (strstr(line, "mouseclick")) {
                /* The peer exposes no SNES mouse, but the step our route needs
                 * is the right mouse button, which the soft mouse ultimately
                 * turns into an A press. Drive A for the same frame count the
                 * mouseclick occupied. */
                char *w = strstr(line, "right") ? "a" : "b";
                if (n_press == cap) { cap = cap ? cap * 2 : 64; press_at = realloc(press_at, cap * sizeof *press_at); press_mask = realloc(press_mask, cap * sizeof *press_mask); }
                press_at[n_press] = done; press_mask[n_press] = mask_of(w); n_press++;
                for (unsigned i = 0; i < 6; i++) { done++; }
                fprintf(stderr, "mouseclick %s -> press %s x6 at frame %u\n",
                        strstr(line, "right") ? "right" : "left", w, done);
            } else {
                fprintf(stderr, "unparsed: %s", line);
            }
        }
        fclose(sf);
    }
    printf("script schedule: %u frames, %u presses\n", done, n_press);

    if (!simcity_recomp_create(&inst, rom, rom_size, err, sizeof err)) {
        fprintf(stderr, "CREATE FAILED: %s\n", err); return 1;
    }
    sram_size = (uint32_t)simcity_recomp_sram_size();
    sram = calloc(1, sram_size);
    if (strcmp(argv[2], "-") != 0) {
        uint8_t *s; size_t n;
        if (!read_file(argv[2], &s, &n) || n != sram_size) { fprintf(stderr, "srm_in bad\n"); return 1; }
        if (!simcity_recomp_sram_load(inst, s, sram_size, err, sizeof err)) { fprintf(stderr, "SRAM LOAD FAILED: %s\n", err); return 1; }
        free(s);
        printf("srm_in loaded (%u bytes)\n", sram_size);
    }

    printf("video_standard=%s nominal_fps=%.6f presentation_fps=%.6f avg_master_clocks_per_frame=%u\n",
           simcity_recomp_video_standard(), simcity_recomp_nominal_fps(),
           simcity_recomp_presentation_fps(), simcity_recomp_average_master_clocks_per_frame());

    snprintf(path, sizeof path, "%s/timeline.log", dumpdir);
    log = fopen(path, "w");
    /* Column `d0B55` is the month and column `d0B53lo` is the low byte of the
     * year. The latter was called `raw55` for the whole life of this log while
     * printing $0B53's low byte, not $0B55's - see the clobber note below.
     * `d0B55` likewise printed the year until this revision. */
    fprintf(log, "frame master_clock insns z12 zB9 zC7 d0B51 d0B53 d0B55 d0B5C d0B53lo d0BA5 d0B9D\n");

    uint32_t script_end = done;
    int failed = 0;
    while (run_frame < total_frames && !failed) {
        uint16_t m = 0;
        for (uint32_t i = 0; i < n_press; i++)
            if (press_at[i] <= run_frame && run_frame < press_at[i] + 5u) { m = press_mask[i]; break; }
        if (!simcity_recomp_advance_headless(inst, m, 1u, &res)) {
            fprintf(stderr, "ROUTE FAILED at run frame %u: %s\n", run_frame, simcity_recomp_last_error(inst));
            failed = 1; break;
        }
        run_frame++;
        done = run_frame;
        /* Render every 300 frames so the run is self-documenting: a frozen
         * date in the log means nothing if we cannot see the screen. */
        if (done % 300u == 0u || done == total_frames) {
            char rp[512]; FILE *rf;
            if (simcity_recomp_render_current_frame(inst, err, sizeof err)) {
                uint32_t w = simcity_recomp_frame_width(inst);
                const uint32_t *px = simcity_recomp_frame_bgra(inst);
                uint32_t h = w ? (224u * 239u + w - 1u) / w : 0u;
                snprintf(rp, sizeof rp, "%s/frame.f%u.bgra", dumpdir, done);
                rf = fopen(rp, "wb");
                if (rf && px && w && h) { fwrite(px, 4, (size_t)w * h, rf); }
                if (rf) { fprintf(stderr, "render f%u %ux%u -> %s\n", done, w, h, rp); fclose(rf); }
            } else { fprintf(stderr, "RENDER FAILED f%u: %s\n", done, err); }
        }
        if (done % dump_every == 0u || done == script_end || done == total_frames) {
            uint8_t zp[3]; uint8_t b5[0x10]; uint8_t mon = 0;
            /* $0BA5 (population) and $0B9D (funds) are added because a frozen
             * DATE is not the same finding as a frozen CITY, and a log that
             * carries only the date cannot tell the two apart. Both are WORDS;
             * reading one byte of either yields a plausible wrong number. */
            uint8_t city[4];
            unsigned pop = 0, funds = 0;
            simcity_recomp_read_wram(inst, 0x0012u, &zp[0], 1u);
            simcity_recomp_read_wram(inst, 0x00B9u, &zp[1], 1u);
            simcity_recomp_read_wram(inst, 0x00C7u, &zp[2], 1u);
            simcity_recomp_read_wram(inst, 0x0B51u, b5, sizeof b5);
            /* The date fields, by the disassembly's own names: $0B53
             * CurrentYear, $0B55 CurrentMonth, $0B4D season. These are the
             * numbers that decide whether the peer actually simulates.
             *
             * BUG FIXED HERE, and it was in OUR instrument, not the peer.
             * The line used to read:
             *
             *     simcity_recomp_read_wram(inst, 0x0B53u, b5 + 4, 4);
             *
             * which is correct for $0B53 but CLOBBERS b5[4..7] - and b5[4] is
             * $0B55 in the 16-byte read from $0B51 above. So the column this
             * log has always labelled `d0B55` was printing $0B53's value a
             * second time, and `raw55` (meant to be the month byte) printed
             * $0B53's LOW byte. A city sitting at year 1900 ($0B53 = $076C)
             * therefore logged `d0B55=076C raw55=006C`: a "month" of $6C and
             * a raw month byte of $6C, neither of which is a month.
             *
             * Why it went unnoticed for so long: $0B53 = $076C is a value that
             * *looks* like data, and every claim of the form "the year stays
             * 1900" read the `d0B53` column, which was never wrong. The
             * broken column was the month - the one field whose value would
             * have shown the month advancing. $0B55 now gets its own read, and
             * the old raw byte is kept and relabelled for what it always was:
             * the low byte of $0B53. */
            simcity_recomp_read_wram(inst, 0x0B53u, b5 + 4, 2);
            if (simcity_recomp_read_wram(inst, 0x0B55u, &mon, 1u))
                ; /* mon is $0B55, the month, 1-based: $01 = JAN */
            if (simcity_recomp_read_wram(inst, 0x0BA5u, city, 2u))
                pop = (unsigned)(city[0] | (city[1] << 8));
            if (simcity_recomp_read_wram(inst, 0x0B9Du, city + 2, 2u))
                funds = (unsigned)(city[2] | (city[3] << 8));
            fprintf(log, "%u %llu %llu %02X %02X %02X %04X %04X %04X %04X %04X %04X %04X\n", done,
                    (unsigned long long)simcity_recomp_master_clock(inst),
                    (unsigned long long)simcity_recomp_instruction_count(inst),
                    zp[0], zp[1], zp[2],
                    (unsigned)(b5[0] | (b5[1] << 8)),
                    (unsigned)(b5[2] | (b5[3] << 8)),
                    (unsigned)mon,
                    (unsigned)(b5[11] | (b5[12] << 8)),
                    b5[4],
                    pop, funds);
            fflush(log);
            /* full WRAM snapshot every 600 frames */
            if (done % 120u == 0u || done == total_frames) {
                uint8_t *full = malloc(131072);
                snprintf(path, sizeof path, "%s/wram.f%u.bin", dumpdir, done);
                if (simcity_recomp_read_wram(inst, 0, full, 131072u)) {
                    FILE *f = fopen(path, "wb"); fwrite(full, 1, 131072, f); fclose(f);
                }
                free(full);
            }
        }
    }
    fclose(log);

    printf("RESULT failed=%d frames=%u master_clock=%llu insns=%llu sram_dirty=%d\n",
           failed, done,
           (unsigned long long)simcity_recomp_master_clock(inst),
           (unsigned long long)simcity_recomp_instruction_count(inst),
           simcity_recomp_sram_dirty(inst));
    if (strcmp(argv[3], "-") != 0) {
        FILE *f = fopen(argv[3], "wb");
        simcity_recomp_sram_copy(inst, sram, sram_size);
        fwrite(sram, 1, sram_size, f); fclose(f);
        printf("srm_out written\n");
    }
    simcity_recomp_destroy(inst);
    free(rom); free(sram);
    return failed;
}