#pragma once

#include "ui/panel.h"
#include <cstdint>

struct SDL_Renderer;

namespace ui {

// MVP font renderer using colored rectangles as "glyphs".
// Each character is a 6x8 pixel block. Colors the rect based on the text content.
// This is a placeholder until RomFontRenderer extracts the real font from ROM.
class RectFontRenderer : public FontRenderer {
public:
    RectFontRenderer() = default;

    void draw_text(SDL_Renderer* renderer, int x, int y, const char* text, uint32_t color) override;
    void draw_text_right(SDL_Renderer* renderer, int x, int y, const char* text, uint32_t color) override;
    int text_width(const char* text) const override;
    int text_height() const override;

private:
    static constexpr int kCharW = 6;
    static constexpr int kCharH = 8;
    static constexpr int kSpacing = 1;

    void draw_char_rect(SDL_Renderer* renderer, int x, int y, uint32_t color) const;
};

} // namespace ui