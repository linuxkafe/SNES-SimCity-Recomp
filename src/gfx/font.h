#pragma once

#include <array>
#include <cstdint>
#include <vector>

#include "gfx/rom_assets.h"
#include "gfx/tile.h"

namespace gfx {

// Font glyph extracted from ROM city tile bank (T022).
// Uses the two-color (outline+fill) glyphs at tile IDs 680-695.
// Mapping is fixed 8x8 fixed-width for ASCII 32-126.
struct FontData {
    static constexpr int kGlyphW = 8;
    static constexpr int kGlyphH = 8;
    static constexpr int kFirstChar = 32;   // space
    static constexpr int kLastChar  = 126;  // ~
    static constexpr int kGlyphCount = kLastChar - kFirstChar + 1;

    // Maps ASCII code to city tile bank index. 0 = missing (fallback).
    std::array<int, kGlyphCount> glyph_to_tile{};
    // Sub-palette index (0-7) within the active palette block for text color.
    int palette_sub = 1;
    // Which RomAssets palette block to use (default 5 = city block per T020).
    int palette_block = 5;

    bool valid = false;
};

// Extract font glyph mapping from already-loaded RomAssets city tiles.
// Uses known glyph tile IDs (680-695 range) for printable ASCII.
// Returns false if RomAssets not valid.
bool extract_font(const gfx::RomAssets& assets, FontData& font);

// Render a string using font glyphs onto an Image (headless, for tests).
// `color` is the base color; outline uses a darker shade from same sub-palette.
void render_text(const FontData& font, const gfx::RomAssets& assets,
                 gfx::Image& out, int x, int y, const char* text,
                 const gfx::Color& color);

} // namespace gfx