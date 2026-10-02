/*
 * A windowed Linux frontend for the peer's SimCity recomp core.
 *
 * WHY THIS EXISTS
 * ---------------
 * The peer ships a Windows-only launcher. Its CMakeLists has
 *
 *     if(WIN32) add_subdirectory(frontend/windows) else() <core only> endif()
 *
 * so on Linux you get libsimcity-static-recomp.a and no way to look at it. The
 * core is not the problem: it hands out a finished BGRA framebuffer and takes a
 * 12-bit button mask. All that is missing is a window, a keyboard and a
 * speaker. That is this file.
 *
 * It is OURS, not the peer's. Nothing of theirs is copied here; we only call
 * their public C API, which is the same API their launcher calls.
 *
 * NO MOUSE, AND THAT IS FINE
 * -------------------------
 * I previously wrote that the city-naming screen could not be passed without a
 * mouse. That was wrong, and it was wrong because I tested one direction
 * (`down`), watched the cursor not move, and generalised. The cursor DOES move
 * on the d-pad: ten `right` presses walk the hand off SPACE onto the P /
 * backspace key. The peer's own launcher has no mouse code either - grep for
 * mouse over its whole frontend returns nothing - it is keyboard and XInput
 * only. So a keyboard is sufficient to reach a city, and this frontend needs
 * nothing more.
 *
 * BUILD
 * -----
 *   gcc -O2 -o jjwin jjwin.c -I<build> -I<src>/static-recomp/include \
 *       <build>/static-recomp/libsimcity-static-recomp.a \
 *       -lstdc++ -lm -lpthread $(pkg-config --cflags --libs sdl2)
 *
 * -lstdc++ is not optional. Their public header is C but the implementation is
 * C++, and without it the link fails on undefined operator new[].
 *
 * RUN
 * ---
 * The ROM path must be ABSOLUTE. The core chdirs to its own executable
 * directory, so a relative path will not resolve. The core also wants a 32 KiB
 * SRAM to boot; we create one next to the binary if it is missing.
 */
#include <SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "simcity_static_recomp.h"

#define WINDOW_SCALE 3

/* The core keeps a ring of produced PCM; we hand it to SDL in the callback and
 * drain whatever is ready once per frame. */
#define AUDIO_RING_FRAMES 8192

typedef struct {
    int16_t data[AUDIO_RING_FRAMES * 2];
    size_t  filled;
} AudioRing;

static uint16_t g_input_mask;
static AudioRing g_audio;

/* An optional input schedule, so a route is reproducible instead of being
 * something a human has to remember. Format, one directive per line:
 *
 *   press <button> <frames>   hold a button for N frames
 *   wait <frames>             hold nothing
 *   mouseclick is not supported and never will be: the core has no mouse.
 *
 * Buttons: b y select start up down left right a x l r  (the names the core's
 * own SimCityRecompInput enum uses).
 */
#define MAX_PRESS 512
static struct { unsigned at; uint16_t mask; } g_press[MAX_PRESS];
static unsigned g_n_press;

static uint16_t button_mask(const char *name) {
    static const struct { const char *n; uint16_t m; } t[] = {
        {"b", SIMCITY_INPUT_B}, {"y", SIMCITY_INPUT_Y},
        {"select", SIMCITY_INPUT_SELECT}, {"start", SIMCITY_INPUT_START},
        {"up", SIMCITY_INPUT_UP}, {"down", SIMCITY_INPUT_DOWN},
        {"left", SIMCITY_INPUT_LEFT}, {"right", SIMCITY_INPUT_RIGHT},
        {"a", SIMCITY_INPUT_A}, {"x", SIMCITY_INPUT_X},
        {"l", SIMCITY_INPUT_L}, {"r", SIMCITY_INPUT_R},
    };
    for (size_t i = 0; i < sizeof t / sizeof *t; i++)
        if (!strcmp(name, t[i].n)) return t[i].m;
    fprintf(stderr, "unknown button: %s\n", name);
    exit(2);
}

static int load_script(const char *path) {
    FILE *f = fopen(path, "r");
    if (!f) { fprintf(stderr, "cannot read script %s\n", path); return 0; }
    char line[256];
    unsigned at = 0;
    while (fgets(line, sizeof line, f)) {
        char verb[32], btn[32];
        unsigned n = 0;
        if (line[0] == '#' || line[0] == '\n') continue;
        if (sscanf(line, "%31s %31s %u", verb, btn, &n) == 3 &&
            !strcmp(verb, "press")) {
            if (g_n_press == MAX_PRESS) {
                fprintf(stderr, "script has more than %d presses\n", MAX_PRESS);
                return 0;
            }
            g_press[g_n_press].at = at;
            g_press[g_n_press].mask = button_mask(btn);
            g_n_press++;
            at += n;
        } else if (sscanf(line, "%31s %u", verb, &n) == 2 &&
                   !strcmp(verb, "wait")) {
            at += n;
        } else {
            fprintf(stderr, "unparsed script line: %s", line);
            return 0;
        }
    }
    fclose(f);
    fprintf(stderr, "script: %u frames scheduled, %u presses\n", at, g_n_press);
    return 1;
}

/* The script owns the mask when one is loaded; otherwise the keyboard does. */
static uint16_t script_mask(unsigned frame) {
    uint16_t m = 0;
    for (unsigned i = 0; i < g_n_press; i++)
        if (g_press[i].at <= frame && frame < g_press[i].at + 5u)
            m = g_press[i].mask;
    return m;
}

/* The SRAM path, resolved once, and a helper that writes it only when the core
 * says the save actually changed. */
static char g_sram_buf[4200];
static const char *g_sram_path;
/* Directory holding this executable, resolved once from /proc/self/exe. */
static char g_exe_dir[4096];

/* The default save location must be ABSOLUTE, anchored to this executable.
 *
 * The comment here used to say "the core chdirs to its own directory, so a
 * relative sram path would land somewhere surprising" - and then the default was
 * the bare relative string "jj.srm". So the save landed wherever the process
 * happened to be, which is not the directory we printed, which is not where the
 * player looked, and which cost a round trip to discover. The comment was right
 * and the code contradicted it in the next line.
 *
 * A relative path is only safe if nothing ever changes the working directory,
 * and the core is exactly the thing that might. So: resolve /proc/self/exe and
 * build an absolute path, and print that absolute path so nobody has to guess
 * where the file went. */
static const char *resolve_sram_path(const char *opt) {
    /* Resolve the executable directory FIRST and unconditionally. An earlier
     * version returned early when --sram was given, so g_exe_dir was left empty
     * and every --wram dump was written to "/jjwram.fN.bin" - the filesystem
     * root - and failed silently, reporting zero dumps and no error. A silent
     * failure in a diagnostic is worse than no diagnostic. */
    char exe[4096];
    ssize_t n = readlink("/proc/self/exe", exe, sizeof exe - 1);
    if (n > 0) {
        exe[n] = 0;
        char *slash = strrchr(exe, '/');
        if (slash) { *slash = 0; snprintf(g_exe_dir, sizeof g_exe_dir, "%s", exe); }
    }
    if (!g_exe_dir[0]) snprintf(g_exe_dir, sizeof g_exe_dir, ".");

    if (opt) {
        snprintf(g_sram_buf, sizeof g_sram_buf, "%s", opt);
        return g_sram_buf;
    }
    snprintf(g_sram_buf, sizeof g_sram_buf, "%s/jj.srm", g_exe_dir);
    return g_sram_buf;
}
static SimCityRecomp *g_inst;
static size_t g_sram_size;
static int g_sram_written;

/* Returns 1 if bytes hit the disk. Writing unconditionally would churn the
 * mtime of a save the player did not touch, and - worse for us - it would
 * manufacture a "save from a city that never started" that later looks like
 * evidence. */
static int save_sram_if_dirty(int announce) {
    if (!g_inst || !g_sram_path || !simcity_recomp_sram_dirty(g_inst)) return 0;
    uint8_t *buf = (uint8_t *)malloc(g_sram_size);
    if (!buf) return 0;
    int ok = 0;
    if (simcity_recomp_sram_copy(g_inst, buf, g_sram_size)) {
        FILE *f = fopen(g_sram_path, "wb");
        if (f) { ok = (fwrite(buf, 1, g_sram_size, f) == g_sram_size); fclose(f); }
    }
    free(buf);
    if (ok) {
        g_sram_written = 1;
        if (announce)
            fprintf(stderr, "SRAM written: %s (%zu bytes)\n",
                    g_sram_path, g_sram_size);
    }
    return ok;
}

static void audio_callback(void *userdata, Uint8 *stream, int len_bytes) {
    (void)userdata;
    int16_t *out = (int16_t *)stream;
    size_t want = (size_t)len_bytes / sizeof(int16_t);

    while (want && g_audio.filled) {
        size_t take = want < g_audio.filled ? want : g_audio.filled;
        memcpy(out, g_audio.data, take * sizeof(int16_t));
        out += take;
        want -= take;
        g_audio.filled -= take;
        /* Move the remainder to the front. */
        memmove(g_audio.data, g_audio.data + take,
                g_audio.filled * sizeof(int16_t));
    }
    if (want) memset(out, 0, want * sizeof(int16_t));
}

/* Keyboard to the core's button mask. The mask is the SNES pad bit layout the
 * core itself declares in SimCityRecompInput, so these are literal, not
 * invented. */
static void handle_key(SDL_Keycode key, int down) {
    uint16_t bit = 0;
    switch (key) {
    /* D-pad: arrows and WASD. On the naming screen this is what walks the
     * on-screen keyboard cursor, which is how you reach a city at all. */
    case SDLK_UP:    case SDLK_w: bit = SIMCITY_INPUT_UP;    break;
    case SDLK_DOWN:  case SDLK_s: bit = SIMCITY_INPUT_DOWN;  break;
    case SDLK_LEFT:  case SDLK_a: bit = SIMCITY_INPUT_LEFT;  break;
    case SDLK_RIGHT: case SDLK_d: bit = SIMCITY_INPUT_RIGHT; break;
    /* Face buttons, in the arrangement a SNES pad has. Z is the bottom one. */
    case SDLK_z: bit = SIMCITY_INPUT_A; break;
    case SDLK_x: bit = SIMCITY_INPUT_B; break;
    case SDLK_c: bit = SIMCITY_INPUT_X; break;
    case SDLK_v: bit = SIMCITY_INPUT_Y; break;
    /* Shoulders. */
    case SDLK_q: bit = SIMCITY_INPUT_L; break;
    case SDLK_e: bit = SIMCITY_INPUT_R; break;
    /* Start / Select. */
    case SDLK_RETURN: case SDLK_KP_ENTER: bit = SIMCITY_INPUT_START;  break;
    case SDLK_RSHIFT: case SDLK_LSHIFT:   bit = SIMCITY_INPUT_SELECT; break;
    default: return;
    }
    if (down) g_input_mask |= bit; else g_input_mask &= (uint16_t)~bit;
}

static uint8_t *read_file(const char *path, size_t *size_out) {
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;
    if (fseek(f, 0, SEEK_END) != 0) { fclose(f); return NULL; }
    long n = ftell(f);
    if (n <= 0) { fclose(f); return NULL; }
    rewind(f);
    uint8_t *buf = (uint8_t *)malloc((size_t)n);
    if (!buf) { fclose(f); return NULL; }
    if (fread(buf, 1, (size_t)n, f) != (size_t)n) {
        free(buf); fclose(f); return NULL;
    }
    fclose(f);
    *size_out = (size_t)n;
    return buf;
}

int main(int argc, char **argv) {
    /* Parse flags first, then positionals. An earlier version read the SRAM
     * from argv[2] unconditionally, so `--script foo.script` took "--script" as
     * the save path and cheerfully wrote a file with that name. Positionals
     * after flags, matched by name, is the only ordering that cannot do that. */
    const char *script_path = NULL;
    unsigned wram_every = 0;
    int wram_at = 0;
    const char *rom_path = NULL;
    const char *sram_opt = NULL;
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--rom")    && i + 1 < argc) { rom_path   = argv[++i]; continue; }
        if (!strcmp(argv[i], "--script") && i + 1 < argc) { script_path = argv[++i]; continue; }
        if (!strcmp(argv[i], "--sram")   && i + 1 < argc) { sram_opt  = argv[++i]; continue; }
        if (!strcmp(argv[i], "--save")   && i + 1 < argc) { ++i; continue; }
        if (!strcmp(argv[i], "--date"))  { wram_at = 1; continue; }
        if (!strcmp(argv[i], "--wram")   && i + 1 < argc) {
            wram_every = (unsigned)atoi(argv[++i]); continue;
        }
        if (!strcmp(argv[i], "--help") || !strcmp(argv[i], "-h")) {
            printf("usage: %s --rom <path> [--sram <path>] [--script <path>]\n"
                   "  --rom     absolute path to the SimCity ROM (required)\n"
                   "  --sram    32 KiB SRAM; defaults to jj.srm beside the binary\n"
                   "  --script  input schedule: press/btn/frames and wait/frames\n"
                   "  --save N  autosave every N frames (default 600)\n"
                   "  --date    print the in-game date each autosave\n"
                   "  --wram N  dump 128 KiB WRAM to jjwram.fN.bin every N frames\n",
                   argv[0]);
            return 0;
        }
        if (argv[i][0] == '-') {
            fprintf(stderr, "unknown option: %s (try --help)\n", argv[i]);
            return 2;
        }
        if      (!rom_path) rom_path = argv[i];
        else if (!sram_opt) sram_opt = argv[i];
        else { fprintf(stderr, "unexpected extra argument: %s\n", argv[i]); return 2; }
    }
    if (!rom_path) {
        fprintf(stderr, "no ROM given. The path must be absolute - the core\n"
                        "chdirs to its own directory before opening anything.\n"
                        "Try --help.\n");
        return 2;
    }
    if (script_path && !load_script(script_path)) return 2;

    size_t rom_size = 0;
    uint8_t *rom = read_file(rom_path, &rom_size);
    if (!rom) {
        fprintf(stderr, "cannot read ROM: %s\n", rom_path);
        return 1;
    }
    if (rom_size != SIMCITY_RECOMP_ROM_SIZE) {
        fprintf(stderr, "ROM is %zu bytes, the core wants %u\n",
                rom_size, (unsigned)SIMCITY_RECOMP_ROM_SIZE);
        return 1;
    }

    /* The core chdirs to its own directory, so a relative sram path would land
     * somewhere surprising. A cold SRAM is 32 KiB of zeroes. */
    const char *sram_path = resolve_sram_path(sram_opt);
    size_t sram_capacity = simcity_recomp_sram_size();
    size_t sram_size = 0;
    uint8_t *sram = read_file(sram_path, &sram_size);
    if (!sram || sram_size != sram_capacity) {
        /* Missing, or the wrong size: a cold SRAM is zeroes. Starting cold is
         * better than refusing to start, but say so, because "it loaded my save
         * and showed the title screen" is the exact silent mismatch that has
         * already cost this investigation an hour once. */
        fprintf(stderr, "%s: %s - starting from a cold SRAM (%zu bytes)\n",
                sram_path, sram ? "wrong size" : "not found", sram_capacity);
        free(sram);
        sram = (uint8_t *)calloc(1, sram_capacity);
        sram_size = 0;
    }

    SimCityRecomp *inst = NULL;
    char err[256] = {0};
    if (!simcity_recomp_create(&inst, rom, rom_size, err, sizeof err)) {
        fprintf(stderr, "core refused the ROM: %s\n", err);
        return 1;
    }
    g_sram_path = sram_path;
    g_inst = inst;
    g_sram_size = sram_capacity;
    if (sram && sram_size) {
        char sram_err[256] = {0};
        if (!simcity_recomp_sram_load(inst, sram, sram_size,
                                      sram_err, sizeof sram_err))
            fprintf(stderr, "SRAM load failed (%s); starting cold\n", sram_err);
    }

    fprintf(stderr,
            "core ready: %s  %.3f fps nominal  %u Hz audio\n",
            simcity_recomp_video_standard(),
            simcity_recomp_nominal_fps(),
            (unsigned)simcity_recomp_host_audio_sample_rate());
    fprintf(stderr,
            "keys: arrows/WASD = d-pad  Z=A X=B C=X V=Y  Q=L E=R  "
            "Enter=Start Shift=Select  Esc=quit\n");
    fprintf(stderr, "save: %s (written automatically once the game dirties it)\n",
            sram_path);
    /* Autosave cadence. Periodic rather than exit-only, because an exit-only
     * save is lost to a closed terminal, a kill, or a crash - and the one thing
     * we need from this frontend is a save that survives whatever happens next. */
    if (wram_every)
        fprintf(stderr, "WRAM trace every %u frames -> %s/jjwram.f*.bin\n",
                wram_every, g_exe_dir);

    unsigned autosave = 600u;
    {
        const char *e = getenv("JJSAVE");
        for (int i = 1; i < argc; i++)
            if (!strcmp(argv[i], "--save") && i + 1 < argc) e = argv[i + 1];
        if (e) {
            int v = atoi(e);
            if (v > 0) autosave = (unsigned)v;
        }
    }

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0) {
        fprintf(stderr, "SDL_Init: %s\n", SDL_GetError());
        return 1;
    }

    int w = 256, h = 224;
    SDL_Window *win = SDL_CreateWindow("SimCity SNES (peer core, our frontend)",
                                       SDL_WINDOWPOS_CENTERED,
                                       SDL_WINDOWPOS_CENTERED,
                                       w * WINDOW_SCALE, h * WINDOW_SCALE,
                                       SDL_WINDOW_SHOWN);
    if (!win) { fprintf(stderr, "window: %s\n", SDL_GetError()); return 1; }
    SDL_Renderer *ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED);
    if (!ren) ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_SOFTWARE);
    if (!ren) { fprintf(stderr, "renderer: %s\n", SDL_GetError()); return 1; }

    SDL_Texture *tex = SDL_CreateTexture(ren, SDL_PIXELFORMAT_ARGB8888,
                                         SDL_TEXTUREACCESS_STREAMING, w, h);
    if (!tex) { fprintf(stderr, "texture: %s\n", SDL_GetError()); return 1; }

    SDL_AudioSpec want;
    SDL_zero(want);
    want.freq = (int)simcity_recomp_host_audio_sample_rate();
    want.format = AUDIO_S16SYS;
    want.channels = 2;
    want.samples = 1024;
    want.callback = audio_callback;
    SDL_AudioDeviceID dev = SDL_OpenAudioDevice(NULL, 0, &want, NULL, 0);
    if (dev) SDL_PauseAudioDevice(dev, 0);
    else fprintf(stderr, "no audio device (%s); continuing silent\n",
                 SDL_GetError());

    int running = 1;
    uint64_t frames = 0;
    while (running) {
        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            if (ev.type == SDL_QUIT) running = 0;
            else if (ev.type == SDL_KEYDOWN) {
                if (ev.key.keysym.sym == SDLK_ESCAPE) running = 0;
                handle_key(ev.key.keysym.sym, 1);
            } else if (ev.type == SDL_KEYUP) {
                handle_key(ev.key.keysym.sym, 0);
            }
        }
        if (!running) break;

        /* advance_streamed lets us drain PCM during a long frame, which is what
         * keeps the audio from stuttering on the heavy frames. */
        uint16_t mask = script_path ? script_mask(frames) : g_input_mask;

        SimCityRecompFrameResult res;
        if (!simcity_recomp_advance_streamed(inst, mask, 1u,
                                             NULL, NULL, &res)) {
            fprintf(stderr, "core failed: %s\n",
                    simcity_recomp_last_error(inst));
            break;
        }
        frames++;

        size_t avail = simcity_recomp_audio_available(inst);
        if (avail) {
            size_t room = AUDIO_RING_FRAMES - g_audio.filled;
            size_t take = avail < room ? avail : room;
            if (take) {
                SDL_LockAudioDevice(dev);
                simcity_recomp_audio_read(inst, g_audio.data + g_audio.filled * 2,
                                          take);
                g_audio.filled += take;
                SDL_UnlockAudioDevice(dev);
            }
            if (avail > room) simcity_recomp_audio_discard(inst);
        }

        if (!simcity_recomp_render_current_frame(inst, err, sizeof err)) {
            fprintf(stderr, "render: %s\n", err);
            break;
        }
        const uint32_t *px = simcity_recomp_frame_bgra(inst);
        uint32_t fw = simcity_recomp_frame_width(inst);
        if (px && fw) {
            /* The core hands out BGRA; SDL wants ARGB8888 on the same byte
             * order, so this is a straight copy. Do not "fix" a channel order
             * here without re-checking, or the palette will come out swapped. */
            SDL_UpdateTexture(tex, NULL, px, (int)(fw * 4));
            SDL_RenderSetScale(ren, (float)(w * WINDOW_SCALE) / (float)fw,
                               (float)(h * WINDOW_SCALE) / (float)fw);
            SDL_RenderClear(ren);
            SDL_RenderCopy(ren, tex, NULL, NULL);
            SDL_RenderPresent(ren);
        }

        if ((frames % autosave) == 0) save_sram_if_dirty(1);

        /* The date, read straight out of WRAM. $0B53 is the year and $0B55 the
         * month, both little-endian, and 0/0 is the encoding for 1900 January -
         * not "unset".
         *
         * RETRACTED, and the retraction is load-bearing here: this comment used
         * to say "$0B51 is a free-running counter modulo 4 and reads 0 one
         * frame in four by design". That is R-031, refuted. $0B51 climbs
         * MONOTONICALLY 0000 -> 001B at +1 per ~197 frames, written by
         * `INC.w $0B51` at $03:8026. It is not mod-4 and it does not "read 0
         * one frame in four by design".
         *
         * $0BA5 (population) and $0B9D (funds) are printed alongside the date
         * because a frozen date is not by itself evidence of a frozen city. If
         * the month stops but the population and the treasury keep moving, the
         * city is simulating and only the calendar is stuck - the opposite
         * finding, and one a date-only readout cannot distinguish. Reading
         * $0BA5/$0B9D is what turns "does the reference simulate" into a
         * question with an answer. Measured on the Deck; see
         * docs/measurements/2026-10-02-t101-reference-simulates.md.
         *
         * Do NOT confuse $0B4D with a year. It is a word, not a byte, and
         * printing its low byte alone turns a date into nonsense. */
        if (wram_at) {
            uint8_t d[4] = {0};
            uint8_t city[8] = {0};
            if (simcity_recomp_read_wram(inst, 0x0B53u, d, 3u)) {
                unsigned year = (unsigned)(d[0] | (d[1] << 8));
                unsigned mon  = d[2];
                /* $0BA5 population and $0B9D funds are both WORDS. Printing
                 * one byte of either produces a plausible-looking number that
                 * is simply wrong, which is the same trap as $0B4D below. */
                unsigned pop = 0, funds = 0;
                if (simcity_recomp_read_wram(inst, 0x0BA5u, city, 2u))
                    pop = (unsigned)(city[0] | (city[1] << 8));
                if (simcity_recomp_read_wram(inst, 0x0B9Du, city + 2, 2u))
                    funds = (unsigned)(city[2] | (city[3] << 8));
                /* 1-based, and measured against a rendered HUD rather than
                 * assumed: our own city shows "1900 JAN" on screen while
                 * $0B55 = $01. So $01 is JAN, not FEB. I had this table
                 * zero-based and printed "1900 FEB" for the value that the
                 * screen calls January. */
                static const char *const MN[12] = {
                    "???","JAN","FEB","MAR","APR","MAY",
                    "JUN","JUL","AUG","SEP","OCT","NOV" };
                /* $0B53 is the ABSOLUTE year, not an offset from 1900: a live
                 * city reads 1900 (0x076C), not 0. So year 0 does not mean
                 * "January 1900" - it means there is no city yet, which is what
                 * the title screen shows. I had this backwards and it was
                 * printed as "0 JAN", a reading that looks like a date and is
                 * not one. */
                /* The frame number is printed because "the date never moved"
                 * is only a claim about a window, and a window needs two ends.
                 * Without it a truncated run and a frozen run print the same
                 * last line - which is exactly how a 9000-frame run came to be
                 * compared against a 33700-frame one without the mismatch
                 * being visible. */
                if (year == 0)
                    fprintf(stderr, "[date] f%-7llu no city yet  (raw $0B53=%04X $0B55=%02X "
                            "$0BA5=%04X $0B9D=%04X)\n",
                            (unsigned long long)frames, year, mon, pop, funds);
                else
                    fprintf(stderr, "[date] f%-7llu %u %s  (raw $0B53=%04X $0B55=%02X "
                            "$0BA5=%04X $0B9D=%04X)\n",
                            (unsigned long long)frames, year,
                            (mon >= 1 && mon <= 12) ? MN[mon] : "???",
                            year, mon, pop, funds);
            }
        }

        /* Optional WRAM trace, for when you need to see WHAT moved rather than
         * whether something moved. This is how a working clock is compared with
         * a frozen one. */
        if (wram_every && (frames % wram_every) == 0) {
            /* Anchored to the executable, for the same reason the save is: a
             * relative path lands in whatever directory the player happened to
             * be standing in, which is how a 128 KiB dump ends up in the middle
             * of a git working tree. Measured, not assumed - this was writing
             * into the repository root. */
            char path[4400];
            snprintf(path, sizeof path, "%s/jjwram.f%llu.bin",
                     g_exe_dir, (unsigned long long)frames);
            FILE *f = fopen(path, "wb");
            if (!f) {
                fprintf(stderr, "WRAM DUMP FAILED: cannot write %s - "
                                "dumps are not being recorded\n", path);
            } else {
                uint8_t *w = (uint8_t *)malloc(131072u);
                if (w && simcity_recomp_read_wram(inst, 0u, w, 131072u)) {
                    if (fwrite(w, 1, 131072u, f) != 131072u)
                        fprintf(stderr, "WRAM DUMP SHORT: %s\n", path);
                } else {
                    fprintf(stderr, "WRAM READ FAILED at frame %llu - "
                                    "dumps are not being recorded\n",
                            (unsigned long long)frames);
                }
                free(w);
                fclose(f);
            }
        }

        if ((frames % 600) == 0) {
            fprintf(stderr, "frame %llu  clock %llu  insns %llu\n",
                    (unsigned long long)frames,
                    (unsigned long long)simcity_recomp_master_clock(inst),
                    (unsigned long long)simcity_recomp_instruction_count(inst));
            fflush(stderr);
        }
    }

    if (!save_sram_if_dirty(1)) {
        /* Say which of the two reasons it was, because "no save" from a
         * session that reached a city is confusing and "no save" from a
         * session on the title screen is expected. */
        fprintf(stderr, simcity_recomp_sram_dirty(inst)
                ? "SRAM dirty but the write failed: %s\n" : "SRAM unchanged - nothing written\n",
                sram_path);
    }
    (void)sram;

    if (dev) SDL_CloseAudioDevice(dev);
    SDL_DestroyTexture(tex);
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    SDL_Quit();
    simcity_recomp_destroy(inst);
    free(rom);
    free(sram);
    return 0;
}
