#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

#include "gfx/font.h"
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

static void test_font_extract() {
    printf("-- test_font_extract\n");
    std::string path = simcity_path();
    CHECK(!path.empty());
    if (path.empty()) return;

    snes::SnesRom rom(path);
    CHECK(rom.load());
    if (!rom.load()) return;

    gfx::RomAssets assets;
    CHECK(gfx::load_rom_assets(rom, assets));
    CHECK(assets.valid);

    gfx::FontData font;
    CHECK(gfx::extract_font(assets, font));
    CHECK(font.valid);

    // Check known glyph mappings (digits 0-9 at tiles 680-689)
    CHECK(font.glyph_to_tile['0' - gfx::FontData::kFirstChar] == 680);
    CHECK(font.glyph_to_tile['9' - gfx::FontData::kFirstChar] == 689);
    // A-F at 690-695
    CHECK(font.glyph_to_tile['A' - gfx::FontData::kFirstChar] == 690);
    CHECK(font.glyph_to_tile['F' - gfx::FontData::kFirstChar] == 695);

    // Render a test string headless
    gfx::Image img = gfx::make_image(200, 16);
    gfx::render_text(font, assets, img, 0, 0, "0123456789ABCDEF", {255, 255, 255, 255});

    // Verify non-background pixels exist
    int non_bg = 0;
    const gfx::Color bg = {0, 0, 0, 255};
    for (int y = 0; y < img.height; ++y)
        for (int x = 0; x < img.width; ++x) {
            const gfx::Color& c = img.at(x, y);
            if (c.r != bg.r || c.g != bg.g || c.b != bg.b) ++non_bg;
        }
    CHECK(non_bg > 500); // expect substantial glyph pixels
    printf("  font glyphs=%d non_bg_px=%d\n", 95, non_bg);
}

int main() {
    printf("Font test suite\n");
    test_font_extract();
    if (failures == 0) {
        printf("ALL TESTS PASSED\n");
        return 0;
    }
    printf("%d test(s) FAILED\n", failures);
    return 1;
}