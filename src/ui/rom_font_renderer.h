#pragma once

#include <cstdint>

#include "gfx/font.h"
#include "gfx/rom_assets.h"
#include "ui/panel.h"

struct SDL_Renderer;

namespace ui {

// ROM font renderer (T022). Implements the abstract FontRenderer interface.
class RomFontRenderer : public FontRenderer {
public:
    RomFontRenderer() = default;
    explicit RomFontRenderer(const gfx::FontData* font, const gfx::RomAssets* assets)
        : font_(font), assets_(assets) {}

    void set_font(const gfx::FontData* font, const gfx::RomAssets* assets) {
        font_ = font; assets_ = assets;
    }

    void draw_text(SDL_Renderer* renderer, int x, int y, const char* text, uint32_t color) override;
    void draw_text_right(SDL_Renderer* renderer, int x, int y, const char* text, uint32_t color) override;
    int text_width(const char* text) const override;
    int text_height() const override { return gfx::FontData::kGlyphH; }

private:
    const gfx::FontData* font_ = nullptr;
    const gfx::RomAssets* assets_ = nullptr;

    void blit_glyph_sdl(SDL_Renderer* renderer, int tile_id, int x, int y, uint32_t color) const;
};

} // namespace ui