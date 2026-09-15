#pragma once

#include <cstdint>

#include "gfx/font.h"
#include "gfx/rom_assets.h"

struct SDL_Renderer;

namespace ui {

// Renders text using ROM font glyphs (T022). Replaces RectFontRenderer.
class FontRenderer {
public:
    FontRenderer() = default;
    explicit FontRenderer(const gfx::FontData* font, const gfx::RomAssets* assets)
        : font_(font), assets_(assets) {}

    void set_font(const gfx::FontData* font, const gfx::RomAssets* assets) {
        font_ = font; assets_ = assets;
    }

    void draw_text(SDL_Renderer* renderer, int x, int y, const char* text, uint32_t color) const;
    void draw_text_right(SDL_Renderer* renderer, int x, int y, const char* text, uint32_t color) const;
    int text_width(const char* text) const;
    int text_height() const { return gfx::FontData::kGlyphH; }

private:
    const gfx::FontData* font_ = nullptr;
    const gfx::RomAssets* assets_ = nullptr;

    void blit_glyph_sdl(SDL_Renderer* renderer, int tile_id, int x, int y, uint32_t color) const;
};

} // namespace ui