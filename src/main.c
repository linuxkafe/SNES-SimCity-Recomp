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
};

int main(int argc, char **argv)
{
    /* Register the game with the framework. */
    RtlRegisterGame(&kGameInfo);

    /* Hand off to the framework's desktop host. */
    return snesrecomp_desktop_main(&kGameHost, argc, argv);
}

/* Presenter callback implementations */

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