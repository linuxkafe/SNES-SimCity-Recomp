#include "ui/rect_font_renderer.h"
#include <SDL2/SDL.h>
#include <cstdio>

namespace ui {

void RectFontRenderer::draw_char_rect(SDL_Renderer* renderer, int x, int y, uint32_t color) const {
    SDL_SetRenderDrawColor(renderer,
                           (color >> 16) & 0xFF,
                           (color >> 8) & 0xFF,
                           color & 0xFF,
                           (color >> 24) & 0xFF);
    SDL_Rect r{x, y, kCharW, kCharH};
    SDL_RenderFillRect(renderer, &r);
}

void RectFontRenderer::draw_text(SDL_Renderer* renderer, int x, int y, const char* text, uint32_t color) {
    if (!text) return;
    int cx = x;
    for (const char* p = text; *p; ++p) {
        draw_char_rect(renderer, cx, y, color);
        cx += kCharW + kSpacing;
    }
}

void RectFontRenderer::draw_text_right(SDL_Renderer* renderer, int x, int y, const char* text, uint32_t color) {
    if (!text) return;
    int w = text_width(text);
    draw_text(renderer, x - w, y, text, color);
}

int RectFontRenderer::text_width(const char* text) const {
    if (!text) return 0;
    int len = 0;
    while (text[len]) ++len;
    return len * (kCharW + kSpacing) - kSpacing;
}

int RectFontRenderer::text_height() const {
    return kCharH;
}

} // namespace ui