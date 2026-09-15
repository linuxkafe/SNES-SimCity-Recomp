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
 */

#include "game_rtl.h"
#include "snesrecomp_rom_identity.h"  /* generated from rom_identity.txt */
#include "host_main.h"

#ifndef SNES_GAME_VERSION
#define SNES_GAME_VERSION "dev"
#endif

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
    /* Widescreen support for isometric renderer (Phase 4) */
    .widescreen_supported = 1,
    .frame_width         = 0,  /* 0 = default 256 */
    .frame_height        = 0,  /* 0 = default 224 */
};

int main(int argc, char **argv)
{
    /* Register the game with the framework. */
    RtlRegisterGame(&kGameInfo);

    /* Hand off to the framework's desktop host. */
    return snesrecomp_desktop_main(&kGameHost, argc, argv);
}