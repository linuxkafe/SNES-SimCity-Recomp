#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

#include "gfx/tile.h"
#include "gfx/titlescr.h"
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

// Real-ROM regression (T021): a valid extraction must load the canonical
// packet sizes for each title layer and a full, coloured render.
static void test_title_screen_extract() {
    printf("-- test_title_screen_extract\n");
    std::string path = simcity_path();
    CHECK(!path.empty());
    if (path.empty()) return;

    snes::SnesRom rom(path);
    CHECK(rom.load());
    if (!rom.load()) return;

    gfx::TitleScreenData d;
    CHECK(gfx::load_title_screen(rom, d));
    CHECK(d.valid);

    // Layer1: 384 x 32-byte 4bpp tiles, 64x64 tilemap.
    CHECK(d.layer1_tiles.size() == 384 * 32);
    CHECK(d.layer1_tm.size() == 64 * 64);
    // Layer2: sprite bank, 512 x 32-byte tiles, 32x64 tilemap.
    CHECK(d.layer2_tiles.size() == 512 * 32);
    CHECK(d.layer2_tm.size() == 32 * 64);
    // Layer3: 2bpp, 32 x 16-byte tiles, 32x64 tilemap.
    CHECK(d.layer3_tiles.size() == 32 * 16);
    CHECK(d.layer3_tm.size() == 32 * 64);

    // Palette is raw BGR555 (not a packet); must hold non-zero, distinct colors.
    bool any_nonzero = false;
    int distinct = 0;
    for (int i = 0; i < gfx::TitleScreenData::kPalCount; ++i) {
        const gfx::Color& c = d.palette[i];
        if (c.r || c.g || c.b) any_nonzero = true;
        for (int j = i + 1; j < gfx::TitleScreenData::kPalCount; ++j) {
            const gfx::Color& cj = d.palette[j];
            if (c.r != cj.r || c.g != cj.g || c.b != cj.b) { ++distinct; break; }
        }
    }
    CHECK(any_nonzero);
    CHECK(distinct > 64);

    // Tilemap tile IDs must stay within their tile banks (sanity vs garbage).
    const int l1_tiles = static_cast<int>(d.layer1_tiles.size()) / 32;
    const int l2_tiles = static_cast<int>(d.layer2_tiles.size()) / 32;
    const int l3_tiles = static_cast<int>(d.layer3_tiles.size()) / 16;
    for (uint16_t v : d.layer1_tm) CHECK((v & 0x3FF) < l1_tiles);
    for (uint16_t v : d.layer2_tm) CHECK((v & 0x3FF) < l2_tiles);
    for (uint16_t v : d.layer3_tm) CHECK((v & 0x3FF) < l3_tiles);

    // Renders a 256x224 frame; skyline (L1) must cover a good share of the
    // lower half and use the skyline sub-palette (the blue window colours).
    gfx::Image img = gfx::make_image(gfx::TitleScreenData::kW, gfx::TitleScreenData::kH);
    gfx::render_title_screen(d, img);

    int lower_non_bg = 0;
    for (int y = img.height * 3 / 4; y < img.height; ++y) {
        for (int x = 0; x < img.width; ++x) {
            const gfx::Color& c = img.at(x, y);
            const gfx::Color& bg = d.palette[0];
            if (c.r != bg.r || c.g != bg.g || c.b != bg.b) ++lower_non_bg;
        }
    }
    CHECK(lower_non_bg > 2000);   // genuine skyline pixels in the lower quarter
    printf("  L1=%zuB tm=%zu L2=%zuB tm=%zu L3=%zuB tm=%zu lower_non_bg=%d\n",
           d.layer1_tiles.size(), d.layer1_tm.size(), d.layer2_tiles.size(),
           d.layer2_tm.size(), d.layer3_tiles.size(), d.layer3_tm.size(),
           lower_non_bg);
}

int main() {
    printf("Title screen test suite\n");
    test_title_screen_extract();
    if (failures == 0) {
        printf("ALL TESTS PASSED\n");
        return 0;
    }
    printf("%d test(s) FAILED\n", failures);
    return 1;
}