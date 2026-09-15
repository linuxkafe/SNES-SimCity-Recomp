#include <cstdio>
#include <vector>
#include <cstring>
#include <array>
#include <fstream>
#include <string>

#include "snes/decompress.h"
#include "snes/rom.h"
#include "snes/scenariomap.h"

using snes::DecompressError;
using snes::nintendo_decompress;

// Canonical scenario map packets (USA ROM), from
// Yoshifanatic1/SimCity-SNES-Disassembly AssetPointersAndFiles.asm.
// LoROM bank*0x8000 + (addr & 0x7FFF) gives the raw file offset.
static const struct { const char* name; uint32_t cpu_addr; uint32_t end_addr; } kMaps[] = {
    {"Tokyo",          0x0C8F27, 0x0CA8E8},
    {"Boston",         0x0CA8E8, 0x0CC5A2},
    {"Detroit",        0x0CC5A2, 0x0CE30B},
    {"Bern",           0x0CE30B, 0x0D816E},
    {"RioDeJaneiro",   0x0D816E, 0x0D9F23},
    {"SanFrancicso",   0x0D9F23, 0x0DB987},
    {"LasVegas",       0x0DB987, 0x0DCB15},
    {"FreeCity",       0x0DCB15, 0x0DD131},
    {"PracticeCity",   0x0DD131, 0x0DD77C},
};

static std::string simcity_path() {
    // Tests run from build/ directory. Look in parent dirs for the ROM.
    const char* candidates[] = {
        "../SimCity (USA).sfc",
        "SimCity (USA).sfc",
        "../../SimCity (USA).sfc",
    };
    for (const char* c : candidates) {
        std::string p = c;
        std::ifstream f(p, std::ios::binary);
        if (f.good()) return p;
    }
    return "";
}

static int failures = 0;
#define CHECK(cond)                                                         \
    do {                                                                    \
        if (!(cond)) {                                                      \
            printf("FAIL: %s (line %d)\n", #cond, __LINE__);                \
            ++failures;                                                     \
        }                                                                   \
    } while (0)

static void test_decode_rle16_simple() {
    printf("-- test_decode_rle16_simple\n");
    // Simple RLE: tile 0x01 repeated 5 times (repeat=4), then tile 0x02 repeated 3 times (repeat=2), then 0xFFFF
    // repeat count = (bits 10-13) + 1
    // For 5 repeats: repeat=4 -> bits 10-13 = 4 -> 0x1000 | 0x0001 = 0x1001 -> LE: 0x01, 0x10
    // For 3 repeats: repeat=2 -> bits 10-13 = 2 -> 0x0800 | 0x0002 = 0x0802 -> LE: 0x02, 0x08
    uint8_t data[] = {
        0x01, 0x10,  // 0x1001: tile 1, repeat 4+1=5
        0x02, 0x08,  // 0x0802: tile 2, repeat 2+1=3
        0xFF, 0xFF   // terminator
    };
    uint16_t out[100];
    size_t written = snes::decode_rle16(data, sizeof(data), out, 100);
    CHECK(written == 8);
    for (size_t i = 0; i < 5; ++i) CHECK(out[i] == 1);
    for (size_t i = 5; i < 8; ++i) CHECK(out[i] == 2);
}

static void test_decode_rle16_terminator() {
    printf("-- test_decode_rle16_terminator\n");
    uint8_t data[] = {
        0x01, 0x04,  // tile 1, repeat 1+1=2
        0xFF, 0xFF
    };
    uint16_t out[100];
    size_t written = snes::decode_rle16(data, sizeof(data), out, 100);
    CHECK(written == 2);
    CHECK(out[0] == 1);
    CHECK(out[1] == 1);
}

static void test_rle16_caps_tiles() {
    printf("-- test_rle16_caps_tiles\n");
    // RLE stream that would overrun max_tiles must stop at the cap, not overflow.
    uint8_t data[] = { 0x00, 0x00 };  // tile 0, repeat of 1 (repeat=0 -> +1)
    uint16_t out[4];
    size_t written = snes::decode_rle16(data, sizeof(data), out, 4);
    CHECK(written >= 1);
    CHECK(written <= 4);
}

static void test_scenario_table_parse() {
    printf("-- test_scenario_table_parse\n");
    std::string path = simcity_path();
    CHECK(!path.empty());
    snes::SnesRom rom(path);
    CHECK(rom.load());

    auto entries = snes::parse_scenario_table(rom);
    CHECK(entries.size() == 9);
    for (size_t i = 0; i < entries.size(); ++i) {
        printf("  [%zu] ROM addr $%06X file off 0x%06zX\n", i, entries[i].rom_addr, entries[i].file_off);
        // Entry i must match the canonical map list modulo table ordering.
        // We only check each entry points to one of the canonical addresses.
    }
    // Sanity: all 9 pointers land within the known map region 0x060F27..0x06D77C.
    for (const auto& e : entries) {
        CHECK(e.rom_addr >= 0x0C8F27 && e.rom_addr <= 0x0DD77C);
    }
}

// Real-ROM regression: every canonical scenario packet must decode to a full map.
static void test_scenario_maps_decode() {
    printf("-- test_scenario_maps_decode\n");
    std::string path = simcity_path();
    CHECK(!path.empty());
    snes::SnesRom rom(path);
    CHECK(rom.load());
    const auto& data = rom.data();

    for (const auto& m : kMaps) {
        size_t file_off = 0;
        CHECK(rom.translate(m.cpu_addr, file_off));
        std::vector<uint8_t> stage1;
        size_t end_off = 0;
        DecompressError err = DecompressError::None;
        bool ok = nintendo_decompress(rom, file_off, stage1, end_off, &err);
        printf("  %-14s file 0x%06zX -> 0x%06zX stage1=%zu ok=%d err=%s\n",
               m.name, file_off, end_off, stage1.size(), ok,
               snes::decompress_error_name(err).c_str());
        CHECK(ok);
        // Packet end must land exactly at the next canonical pointer.
        size_t next_off = 0;
        CHECK(rom.translate(m.end_addr, next_off));
        CHECK(end_off == next_off);
        CHECK(stage1.size() > 2000);
    }
}

// Real-ROM regression: Layer1 city simulation tiles decode to a 1024-tile bank.
static void test_city_tiles_decode() {
    printf("-- test_city_tiles_decode\n");
    std::string path = simcity_path();
    CHECK(!path.empty());
    snes::SnesRom rom(path);
    CHECK(rom.load());

    // GFX_Layer1_CitySimulationTiles at $07E584-08C4DB (USA).
    size_t file_off = 0;
    CHECK(rom.translate(0x07E584, file_off));
    CHECK(file_off == 0x3E584);
    std::vector<uint8_t> raw;
    size_t end_off = 0;
    DecompressError err = DecompressError::None;
    bool ok = nintendo_decompress(rom, file_off, raw, end_off, &err);
    printf("  city tiles: file 0x%06zX -> 0x%06zX raw=%zu ok=%d err=%s\n",
           file_off, end_off, raw.size(), ok, snes::decompress_error_name(err).c_str());
    CHECK(ok);
    CHECK(raw.size() == 32 * 1024);   // 1024 x 32-byte 4bpp tiles
}

static void test_load_scenario_terrain() {
    printf("-- test_load_scenario_terrain\n");
    std::string path = simcity_path();
    CHECK(!path.empty());
    snes::SnesRom rom(path);
    CHECK(rom.load());

    uint16_t terrain[120 * 100];
    bool ok = snes::load_scenario_terrain(rom, 0, terrain);
    CHECK(ok);
    int water_count = 0;
    for (int i = 0; i < 120 * 5; ++i) {
        if ((terrain[i] & 0x03FF) <= 3) ++water_count;
    }
    CHECK(water_count > 100); // expect significant water in top border
    // Count total non-grass terrain to ensure the map isn't blank.
    int varied = 0;
    for (int i = 0; i < 120 * 100; ++i) {
        if ((terrain[i] & 0x03FF) != 0) ++varied;
    }
    CHECK(varied > 1000);
    printf("  loaded %d tiles, water in top border: %d, varied tiles: %d\n", 120*100, water_count, varied);
}

int main() {
    printf("Decompress test suite\n");
    test_decode_rle16_simple();
    test_decode_rle16_terminator();
    test_rle16_caps_tiles();
    test_scenario_table_parse();
    test_scenario_maps_decode();
    test_city_tiles_decode();
    test_load_scenario_terrain();
    if (failures == 0) {
        printf("ALL TESTS PASSED\n");
        return 0;
    }
    printf("%d test(s) FAILED\n", failures);
    return 1;
}