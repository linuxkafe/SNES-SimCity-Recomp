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
    if (argc < 2) {
        fprintf(stderr,
                "usage: %s <rom> [sram]\n"
                "  rom  absolute path to the SimCity ROM\n"
                "  sram optional path to a 32 KiB SRAM (created if absent)\n",
                argv[0]);
        return 2;
    }

    size_t rom_size = 0;
    uint8_t *rom = read_file(argv[1], &rom_size);
    if (!rom) {
        fprintf(stderr, "cannot read ROM: %s\n", argv[1]);
        return 1;
    }
    if (rom_size != SIMCITY_RECOMP_ROM_SIZE) {
        fprintf(stderr, "ROM is %zu bytes, the core wants %u\n",
                rom_size, (unsigned)SIMCITY_RECOMP_ROM_SIZE);
        return 1;
    }

    /* The core chdirs to its own directory, so a relative sram path would land
     * somewhere surprising. A cold SRAM is 32 KiB of zeroes. */
    const char *sram_path = (argc > 2) ? argv[2] : "jj.srm";
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
        SimCityRecompFrameResult res;
        if (!simcity_recomp_advance_streamed(inst, g_input_mask, 1u,
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

        if ((frames % 600) == 0) {
            fprintf(stderr, "frame %llu  clock %llu  insns %llu\n",
                    (unsigned long long)frames,
                    (unsigned long long)simcity_recomp_master_clock(inst),
                    (unsigned long long)simcity_recomp_instruction_count(inst));
            fflush(stderr);
        }
    }

    if (sram && simcity_recomp_sram_dirty(inst)) {
        /* Only rewrite the file when the core says the save changed, so a clean
         * exit does not churn the mtime of a save the player did not touch. */
        if (!simcity_recomp_sram_copy(inst, sram, sram_capacity)) {
            FILE *f = fopen(sram_path, "wb");
            if (f) { fwrite(sram, 1, sram_capacity, f); fclose(f); }
            fprintf(stderr, "SRAM written: %s\n", sram_path);
        }
    }

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
