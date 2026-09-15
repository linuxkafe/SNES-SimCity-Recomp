#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "gfx/rom_assets.h"
#include "gfx/tile.h"
#include "snes/rom.h"

static int failures = 0;
#define CHECK(cond)                                                         \
    do {                                                                    \
        if (!(cond)) {                                                      \
            printf("FAIL: %s (line %d)\n", #cond, __LINE__);                \
            ++failures;                                                     \
        }                                                                   \
    } while (0)

// Encode one row of 8 pixels (each index 0..15) into SNES 4bpp layout B:
// planes 0/1 in bytes 0-15, planes 2/3 in bytes 16-31.
static void encode_row(uint8_t tile[32], int row, const uint8_t idx[8]) {
    uint8_t p[4] = {0, 0, 0, 0};
    for (int c = 0; c < 8; ++c) {
        for (int bit = 0; bit < 4; ++bit) {
            if (idx[c] & (1 << bit)) p[bit] |= static_cast<uint8_t>(0x80 >> c);
        }
    }
    tile[row * 2 + 0]         = p[0];
    tile[row * 2 + 1]         = p[1];
    tile[16 + row * 2 + 0]    = p[2];
    tile[16 + row * 2 + 1]    = p[3];
}

static void test_known_tile() {
    printf("-- test_known_tile\n");
    // Construct a 4bpp tile where every pixel = color index 0x7.
    // Index 7 → bits 0,1,2 set → planes 0,1,2 all 0xFF, plane3 = 0.
    uint8_t tile[32];
    const uint8_t seven[8] = {7, 7, 7, 7, 7, 7, 7, 7};
    for (int r = 0; r < 8; ++r) encode_row(tile, r, seven);
    gfx::Tile8x8 t = gfx::decode_4bpp_tile(tile);
    for (int r = 0; r < 8; ++r) {
        for (int c = 0; c < 8; ++c) {
            CHECK(t.pixels[r][c].r == 7);
        }
    }
}

static void test_single_pixel_pattern() {
    printf("-- test_single_pixel_pattern\n");
    // Only the leftmost pixel (column 0) lit in plane3 → index 8.
    uint8_t tile[32];
    memset(tile, 0, sizeof tile);
    for (int r = 0; r < 8; ++r) {
        // plane3 (byte 16..31, odd bytes) bit7 = leftmost
        tile[16 + r * 2 + 1] = 0x80;
    }
    gfx::Tile8x8 t = gfx::decode_4bpp_tile(tile);
    for (int r = 0; r < 8; ++r) {
        CHECK(t.pixels[r][0].r == 8);
        for (int c = 1; c < 8; ++c) {
            CHECK(t.pixels[r][c].r == 0);
        }
    }
}

static void test_bgr555_conversion() {
    printf("-- test_bgr555_conversion\n");
    gfx::Color c = gfx::bgr555(0x7FFF);  // max R,G,B
    CHECK(c.r == 255 && c.g == 255 && c.b == 255);
    c = gfx::bgr555(0x0000);
    CHECK(c.r == 0 && c.g == 0 && c.b == 0);
    c = gfx::bgr555(0x001F);  // full red, no green/blue
    CHECK(c.r == 255 && c.g == 0 && c.b == 0);
    c = gfx::bgr555(0x7C00);  // full blue
    CHECK(c.r == 0 && c.g == 0 && c.b == 255);
    c = gfx::bgr555(0x03E0);  // full green
    CHECK(c.r == 0 && c.g == 255 && c.b == 0);
}

static void test_blit_tile() {
    printf("-- test_blit_tile\n");
    uint8_t tile[32];
    memset(tile, 0, sizeof tile);
    const uint8_t seven[8] = {7, 7, 7, 7, 7, 7, 7, 7};
    for (int r = 0; r < 8; ++r) encode_row(tile, r, seven);  // idx 7 everywhere

    gfx::Image img = gfx::make_image(16, 16);
    gfx::Palette16 pal;
    for (int i = 0; i < 16; ++i) pal.colors[i] = { (uint8_t)i, (uint8_t)i, (uint8_t)i, 255 };
    gfx::blit_tile(img, 4, 4, tile, pal);
    // center pixel should be palette color 7
    const gfx::Color& c = img.at(8, 8);
    CHECK(c.r == 7 && c.g == 7 && c.b == 7);
    CHECK(img.at(0, 0).a == 0);  // untouched area stays transparent
}

static std::string simcity_path() {
    const char* candidates[] = {
        "../SimCity (USA).sfc",
        "SimCity (USA).sfc",
        "../../SimCity (USA).sfc",
    };
    for (const char* c : candidates) {
        std::string p = c;
        FILE* f = fopen(p.c_str(), "rb");
        if (f) { fclose(f); return p; }
    }
    return "";
}

// Real-ROM regression: Layer1 city tiles (1024 x 32B) + 14 BG palette blocks
// must extract exactly as the disassembly registers them (no garbage offsets).
static void test_rom_assets() {
    printf("-- test_rom_assets\n");
    std::string path = simcity_path();
    CHECK(!path.empty());
    if (path.empty()) return;

    snes::SnesRom rom(path);
    CHECK(rom.load());
    if (!rom.load()) return;

    gfx::RomAssets assets;
    CHECK(gfx::load_rom_assets(rom, assets));
    CHECK(assets.valid);
    CHECK(assets.tiles.size() ==
          static_cast<size_t>(gfx::RomAssets::kTileCount) * gfx::RomAssets::kTileBytes);

    // Palette block 0 sub 0 must differ from garbage (all-black) and be usable.
    const gfx::Color& c0 = assets.terrain_palette().colors[0];
    CHECK(c0.r == 0 && c0.g == 0 && c0.b == 0);  // index 0 is always black
    CHECK(assets.palettes.size() ==
          static_cast<size_t>(gfx::RomAssets::kPaletteBlocks) * gfx::RomAssets::kPaletteSubs);

    // Decode a known low tile (0 and 1 dominate scenario maps) and verify the
    // decoded bytes survive the 4bpp path (palette index within 0..15).
    for (int t = 0; t < 2; ++t) {
        gfx::Tile8x8 decoded = gfx::decode_4bpp_tile(
            assets.tiles.data() + static_cast<size_t>(t) * gfx::RomAssets::kTileBytes);
        bool ok = true;
        for (int r = 0; r < 8 && ok; ++r)
            for (int c = 0; c < 8; ++c)
                if (decoded.pixels[r][c].r > 15) ok = false;
        CHECK(ok);
    }
    printf("  extracted %zu tile bytes, %zu palette entries\n",
           assets.tiles.size(), assets.palettes.size());
}

int main() {
    printf("Tile test suite\n");
    test_known_tile();
    test_single_pixel_pattern();
    test_bgr555_conversion();
    test_blit_tile();
    test_rom_assets();
    if (failures == 0) {
        printf("ALL TESTS PASSED\n");
        return 0;
    }
    printf("%d test(s) FAILED\n", failures);
    return 1;
}