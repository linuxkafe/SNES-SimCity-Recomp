#include "ui/rom_font_renderer.h"

#include <SDL2/SDL.h>
#include <algorithm>

#include "gfx/font.h"
#include "gfx/rom_assets.h"

namespace ui {
namespace {

void decode_tile_indices(const gfx::RomAssets& assets, int tile_id, uint8_t idx[8][8]) {
    const int tile_bytes = gfx::RomAssets::kTileBytes; // 32
    const uint8_t* tile_data = assets.tiles.data() + tile_id * tile_bytes;
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
}

} // namespace

void RomFontRenderer::blit_glyph_sdl(SDL_Renderer* renderer, int tile_id, int x, int y, uint32_t color) const {
    if (!font_ || !assets_ || !font_->valid) return;
    if (tile_id <= 0 || tile_id >= 1024) return;

    uint8_t idx[8][8];
    decode_tile_indices(*assets_, tile_id, idx);

    const gfx::Palette16& pal = assets_->palettes[font_->palette_block * gfx::RomAssets::kPaletteSubs + font_->palette_sub];

    uint8_t r = (color >> 16) & 0xFF;
    uint8_t g = (color >> 8) & 0xFF;
    uint8_t b = color & 0xFF;
    uint8_t a = (color >> 24) & 0xFF;

    for (int row = 0; row < 8; ++row) {
        for (int col = 0; col < 8; ++col) {
            int px = x + col;
            int py = y + row;
            uint8_t id = idx[row][col];
            if (id == 0) continue; // transparent

            gfx::Color c = pal.colors[id];
            if (id == 3 || id == 4) {
                c.r = r; c.g = g; c.b = b; c.a = a;
            }
            SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, c.a);
            SDL_Rect rct{px, py, 1, 1};
            SDL_RenderFillRect(renderer, &rct);
        }
    }
}

void RomFontRenderer::draw_text(SDL_Renderer* renderer, int x, int y, const char* text, uint32_t color) {
    if (!font_ || !text) return;
    int cx = x;
    for (const char* p = text; *p; ++p) {
        unsigned char ch = static_cast<unsigned char>(*p);
        if (ch >= gfx::FontData::kFirstChar && ch <= gfx::FontData::kLastChar) {
            int tile_id = font_->glyph_to_tile[ch - gfx::FontData::kFirstChar];
            blit_glyph_sdl(renderer, tile_id, cx, y, color);
        }
        cx += gfx::FontData::kGlyphW;
    }
}

void RomFontRenderer::draw_text_right(SDL_Renderer* renderer, int x, int y, const char* text, uint32_t color) {
    int w = text_width(text);
    draw_text(renderer, x - w, y, text, color);
}

int RomFontRenderer::text_width(const char* text) const {
    if (!text) return 0;
    int len = 0;
    while (text[len]) ++len;
    return len * gfx::FontData::kGlyphW;
}

} // namespace ui