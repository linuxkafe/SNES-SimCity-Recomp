#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include "snes/rom.h"
#include "snes/decompress.h"
#include "snes/scenariomap.h"

static void usage() {
    printf("usage: rommap <rom.sfc> [scenario_idx]\n"
           "\n"
           "  Dump scenario terrain map from SimCity SNES ROM.\n"
           "\n"
           "Arguments:\n"
           "  rom.sfc       Path to SimCity SNES ROM\n"
           "  scenario_idx  Optional scenario index (0-8), default 0\n"
           "\n"
           "Output:\n"
           "  Prints terrain tile counts and ASCII preview.\n"
    );
}

static void print_ascii_preview(const uint16_t* tiles, int w, int h) {
    auto ch = [](uint16_t t) -> char {
        t &= 0x03FF;
        if (t <= 3) return '~';           // water
        if (t >= 4 && t <= 19) return '.'; // shore/land
        if (t >= 20 && t <= 37) return 'T'; // forest
        if (t == 38 || t == 39) return 'o'; // park
        if (t >= 48 && t <= 95) return '+'; // road
        if (t >= 112 && t <= 127) return '='; // rail
        return ' ';
    };

    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            putchar(ch(tiles[y * w + x]));
        }
        putchar('\n');
    }
}

int main(int argc, char** argv) {
    if (argc < 2 || strcmp(argv[1], "--help") == 0) {
        usage();
        return argc < 2 ? 1 : 0;
    }

    int scenario_idx = 0;
    const char* rom_path = nullptr;

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--help") == 0) {
            usage();
            return 0;
        } else if (argv[i][0] != '-') {
            if (rom_path == nullptr) rom_path = argv[i];
            else scenario_idx = atoi(argv[i]);
        }
    }

    if (rom_path == nullptr) {
        usage();
        return 1;
    }

    snes::SnesRom rom(rom_path);
    if (!rom.load()) {
        fprintf(stderr, "error: could not load ROM '%s'\n", rom_path);
        return 1;
    }

    printf("ROM: %s (%zu KB)\n", rom.path().c_str(), rom.size() / 1024);
    printf("Scenario index: %d\n", scenario_idx);

    // Parse and show scenario table
    auto entries = snes::parse_scenario_table(rom);
    printf("Scenario table (%zu entries):\n", entries.size());
    for (size_t i = 0; i < entries.size(); ++i) {
        printf("  [%zu] ROM addr $%06X file off 0x%06zX\n", i, entries[i].rom_addr, entries[i].file_off);
    }

    // Try to load terrain
    uint16_t terrain[120 * 100];
    if (!snes::load_scenario_terrain(rom, scenario_idx, terrain)) {
        fprintf(stderr, "error: failed to load scenario terrain\n");
        return 1;
    }

    // Count tile types
    int counts[1024] = {0};
    for (int i = 0; i < 120 * 100; ++i) {
        ++counts[terrain[i] & 0x03FF];
    }
    printf("\nTile counts (top 10):\n");
    for (int i = 0; i < 1024; ++i) {
        if (counts[i] > 0) {
            printf("  0x%03X (%d): %d\n", i, i, counts[i]);
        }
    }

    printf("\nASCII preview (120x100):\n");
    print_ascii_preview(terrain, 120, 100);

    return 0;
}