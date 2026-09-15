#include "gfx/font.h"

#include <algorithm>
#include <cstdint>

#include "gfx/rom_assets.h"
#include "gfx/tile.h"

namespace gfx {

// Known glyph tile IDs in the city tile bank (1024 tiles at $07E584).
// These have the classic two-color SNES font style (indices 3=outline, 4=fill).
// Mapped to printable ASCII 32-126. Gaps = fallback to generated.
static constexpr int kGlyphTiles[] = {
    //  32 ' '      33 '!'      34 '"'      35 '#'      36 '$'      37 '%'
    0,            0,          0,          0,          0,          0,
    //  38 '&'      39 '''      40 '('      41 ')'      42 '*'      43 '+'
    0,            0,          0,          0,          0,          0,
    //  44 ','      45 '-'      46 '.'      47 '/'      48 '0'      49 '1'
    0,            0,          0,          0,          680,        681,
    //  50 '2'      51 '3'      52 '4'      53 '5'      54 '6'      55 '7'
    682,          683,        684,        685,        686,        687,
    //  56 '8'      57 '9'      58 ':'      59 ';'      60 '<'      61 '='
    688,          689,        0,          0,          0,          0,
    //  62 '>'      63 '?'      64 '@'      65 'A'      66 'B'      67 'C'
    0,            0,          0,          690,        691,        692,
    //  68 'D'      69 'E'      70 'F'      71 'G'      72 'H'      73 'I'
    693,          694,        695,        0,          0,          0,
    //  74 'J'      75 'K'      76 'L'      77 'M'      78 'N'      79 'O'
    0,            0,          0,          0,          0,          0,
    //  80 'P'      81 'Q'      82 'R'      83 'S'      84 'T'      85 'U'
    0,            0,          0,          0,          0,          0,
    //  86 'V'      87 'W'      88 'X'      89 'Y'      90 'Z'      91 '['
    0,            0,          0,          0,          0,          0,
    //  92 '\'      93 ']'      94 '^'      95 '_'      96 '`'      97 'a'
    0,            0,          0,          0,          0,          0,
    //  98 'b'      99 'c'      100 'd'     101 'e'     102 'f'     103 'g'
    0,            0,          0,          0,          0,          0,
    //  104 'h'     105 'i'     106 'j'     107 'k'     108 'l'     109 'm'
    0,            0,          0,          0,          0,          0,
    //  110 'n'     111 'o'     112 'p'     113 'q'     114 'r'     115 's'
    0,            0,          0,          0,          0,          0,
    //  116 't'     117 'u'     118 'v'     119 'w'     120 'x'     121 'y'
    0,            0,          0,          0,          0,          0,
    //  122 'z'     123 '{'     124 '|'     125 '}'     126 '~'
    0,            0,          0,          0,          0
};

bool extract_font(const RomAssets& assets, FontData& font) {
    font = FontData{};
    if (!assets.valid) return false;

    // Copy the known mapping
    for (int i = 0; i < FontData::kGlyphCount; ++i) {
        font.glyph_to_tile[i] = kGlyphTiles[i];
    }

    // Verify at least some glyphs exist in the tile bank
    int found = 0;
    for (int t : font.glyph_to_tile) {
        if (t > 0 && t < 1024) ++found;
    }
    if (found == 0) return false;

    font.valid = true;
    return true;
}

// Blit a single glyph tile from RomAssets onto `out` at (x,y).
// Uses the font's palette_block/palette_sub for colors.
// Glyph tiles use palette indices: 0=transparent, 3=outline, 4=fill.
static void blit_glyph(Image& out, const RomAssets& assets, const FontData& font,
                       int tile_id, int x, int y, const Color& base_color) {
    if (tile_id <= 0 || tile_id >= 1024) return;
    const int tile_bytes = RomAssets::kTileBytes; // 32
    const uint8_t* tile_data = assets.tiles.data() + tile_id * tile_bytes;
    if (tile_id * tile_bytes + tile_bytes > static_cast<int>(assets.tiles.size())) return;

    // Decode 4bpp tile to palette indices
    uint8_t idx[8][8];
    for (int row = 0; row < 8; ++row) {
        const uint8_t p0 = tile_data[row * 2 + 0];
        const uint8_t p1 = tile_data[row * 2 + 1];
        const uint8_t p2 = tile_data[16 + row * 2 + 0];
        const uint8_t p3 = tile_data[16 + row * 2 + 1];
        for (int col = 0; col < 8; ++col) {
            const int bit = 7 - col;
            uint8_t id = 0;
            if (p0 & (1 << bit)) id |= 1;
            if (p1 & (1 << bit)) id |= 2;
            if (p2 & (1 << bit)) id |= 4;
            if (p3 & (1 << bit)) id |= 8;
            idx[row][col] = id;
        }
    }

    const Palette16& pal = assets.palettes[font.palette_block * RomAssets::kPaletteSubs + font.palette_sub];

    for (int row = 0; row < 8; ++row) {
        for (int col = 0; col < 8; ++col) {
            int px = x + col;
            int py = y + row;
            if (px < 0 || px >= out.width || py < 0 || py >= out.height) continue;
            uint8_t id = idx[row][col];
            if (id == 0) continue; // transparent
            Color c = pal.colors[id];
            // If the tile uses indices 3/4 for outline/fill, blend with base_color
            if (id == 3 || id == 4) {
                // Use base_color luminance with tile's hue (simplified: just use base_color)
                c = base_color;
            }
            out.at(px, py) = c;
        }
    }
}

void render_text(const FontData& font, const RomAssets& assets,
                 Image& out, int x, int y, const char* text,
                 const Color& color) {
    if (!font.valid || !text) return;
    int cx = x;
    for (const char* p = text; *p; ++p) {
        unsigned char ch = static_cast<unsigned char>(*p);
        if (ch >= FontData::kFirstChar && ch <= FontData::kLastChar) {
            int tile_id = font.glyph_to_tile[ch - FontData::kFirstChar];
            blit_glyph(out, assets, font, tile_id, cx, y, color);
        }
        cx += FontData::kGlyphW;
    }
}

} // namespace gfx