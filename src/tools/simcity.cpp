#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

#include "engine/game.h"
#include "snes/rom.h"

static void usage() {
    printf("usage: simcity <rom.sfc> [--help] [--no-rom-map] [--palette-block N] [--palette-sub N] [--skip-menu]\n"
           "\n"
           "  SimCity SNES PC Port\n"
           "  Users must supply their own SNES ROM file.\n"
           "\n"
           "Options:\n"
           "  --help            show this help\n"
           "  --no-rom-map      disable ROM scenario terrain loading\n"
           "  --palette-block N BG palette block for city layer (0-13, default 4)\n"
           "  --palette-sub N   BG sub-palette within block (0-7, default 0)\n"
           "  --skip-menu       skip title screen and main menu (for headless testing)\n"
           "\n"
           "Controls:\n"
           "  ESC          quit (or return to menu during play)\n"
           "  F1           cycle sub-palette (0-7)\n"
           "  F2           cycle palette block (0-13)\n"
           "\n"
           "The ROM must be a US region SimCity SNES dump (.sfc).\n"
           );
}

int main(int argc, char** argv) {
    bool use_rom_map = true;
    bool skip_menu = false;
    const char* rom_path = nullptr;
    int palette_block = 5;
    int palette_sub = 0;

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--help") == 0) {
            usage();
            return 0;
        } else if (strcmp(argv[i], "--no-rom-map") == 0) {
            use_rom_map = false;
        } else if (strcmp(argv[i], "--skip-menu") == 0) {
            skip_menu = true;
        } else if (strcmp(argv[i], "--palette-block") == 0 && i + 1 < argc) {
            palette_block = atoi(argv[++i]);
            if (palette_block < 0) palette_block = 0;
            if (palette_block > 13) palette_block = 13;
        } else if (strcmp(argv[i], "--palette-sub") == 0 && i + 1 < argc) {
            palette_sub = atoi(argv[++i]);
            if (palette_sub < 0) palette_sub = 0;
            if (palette_sub > 7) palette_sub = 7;
        } else if (argv[i][0] != '-') {
            if (rom_path == nullptr) rom_path = argv[i];
        }
    }

    if (rom_path == nullptr) {
        usage();
        return 1;
    }

    // Validate ROM loads and has expected header
    snes::SnesRom rom(rom_path);
    if (!rom.load()) {
        fprintf(stderr, "error: could not load ROM '%s'\n", rom_path);
        return 1;
    }
    if (rom.header().map_mode == snes::MapMode::Unknown) {
        fprintf(stderr, "error: ROM does not appear to be a valid SNES ROM\n");
        return 1;
    }
    printf("loaded: %s (%s, %s, %zu KB)\n",
           rom.header().title.c_str(),
           snes::map_mode_name(rom.header().map_mode).c_str(),
           snes::cart_type_name(rom.header().cart_type).c_str(),
           rom.size() / 1024);

    Game::Config cfg;
    cfg.rom_path = rom_path;
    cfg.use_rom_map = use_rom_map;
    cfg.skip_menu = skip_menu;
    cfg.palette_block = palette_block;
    cfg.palette_sub = palette_sub;
    Game game(cfg);
    if (!game.init()) {
        fprintf(stderr, "error: failed to initialise game\n");
        return 1;
    }
    int rc = game.run();
    printf("frames: %u\n", rc);
    return rc;
}