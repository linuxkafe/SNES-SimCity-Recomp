#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

#include "gfx/building_sprites.h"
#include "gfx/rom_assets.h"
#include "snes/rom.h"

static int failures = 0;
#define CHECK(cond)                                                         \
    do {                                                                    \
        if (!(cond)) {                                                      \
            printf("FAIL: %s (line %d)\n", #cond, __LINE__);                \
            ++failures;                                                     \
        }                                                                   \
    } while (0)

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

static void test_building_sprites_extract() {
    printf("-- test_building_sprites_extract\n");
    std::string path = simcity_path();
    CHECK(!path.empty());
    if (path.empty()) return;

    snes::SnesRom rom(path);
    CHECK(rom.load());
    if (!rom.load()) return;

    gfx::RomAssets assets;
    CHECK(gfx::load_rom_assets(rom, assets));
    CHECK(assets.valid);

    gfx::BuildingSpriteData data;
    CHECK(gfx::extract_building_sprites(assets, data));
    CHECK(data.valid);

    // Verify all 9 quads have valid tile IDs in building range
    const int kMin = 940, kMax = 1022;
    int total_tiles = 0;
    auto check_quads = [&](const gfx::BuildingSpriteData::Quad arr[3]) {
        for (int d = 0; d < 3; ++d) {
            for (int t : arr[d]) {
                CHECK(t >= kMin && t <= kMax);
                ++total_tiles;
            }
        }
    };
    check_quads(data.residential);
    check_quads(data.commercial);
    check_quads(data.industrial);

    // Fallback colors should be non-zero
    for (int z = 0; z < gfx::BuildingSpriteData::ZoneTypeCount; ++z) {
        const gfx::Color& c = data.fallback[z];
        CHECK(c.r || c.g || c.b);
    }

    printf("  building tiles: %d (9 quads x 4)\n", total_tiles);
    CHECK(total_tiles == 36); // 9 quads x 4 tiles
}

int main() {
    printf("Building sprites test suite\n");
    test_building_sprites_extract();
    if (failures == 0) {
        printf("ALL TESTS PASSED\n");
        return 0;
    }
    printf("%d test(s) FAILED\n", failures);
    return 1;
}