#include "ui/rci_panel.h"
#include <SDL2/SDL.h>
#include "sim/city.h"

namespace ui {

void RciPanel::render(SDL_Renderer* renderer, const sim::Stats& stats,
                      FontRenderer* font, int win_w, int win_h) {
    (void)win_w;
    int x = kMargin;
    int y = win_h - kPanelH - kMargin;

    draw_bg(renderer, x, y, kPanelW, kPanelH);

    // RCI colors matching SNES: R=yellow, C=blue, I=purple
    struct Bar { sim::Demand d; const char* label; uint32_t color; };
    Bar bars[3] = {
        {stats.res_demand, "R", 0xE8B840FF},  // yellow
        {stats.com_demand, "C", 0x7480D8FF},  // blue
        {stats.ind_demand, "I", 0xB070C0FF},  // purple
    };

    int max_bar_w = kPanelW - kLabelW - kMargin * 2 - kBarGap * 2;
    int bar_x = x + kMargin + kLabelW;
    int bar_y = y + kMargin;

    for (int i = 0; i < 3; ++i) {
        draw_label(renderer, font, x + kMargin, bar_y + 2, bars[i].label, bars[i].color);
        int w = demand_width(bars[i].d, max_bar_w);
        draw_bar(renderer, bar_x, bar_y, w, kBarH, bars[i].color);
        // Draw empty remainder
        if (w < max_bar_w) {
            SDL_SetRenderDrawColor(renderer, 40, 40, 40, 255);
            SDL_Rect r{bar_x + w, bar_y, max_bar_w - w, kBarH};
            SDL_RenderFillRect(renderer, &r);
        }
        bar_y += kBarH + kBarGap;
    }
}

SDL_Rect RciPanel::bounds() const {
    return {kMargin, 0, kPanelW, kPanelH};  // y computed at render time
}

int RciPanel::demand_width(sim::Demand d, int max_width) {
    switch (d) {
        case sim::Demand::Low:    return max_width / 6;    // ~16%
        case sim::Demand::Medium: return max_width / 2;    // 50%
        case sim::Demand::High:   return max_width;        // 100%
    }
    return 0;
}

void RciPanel::draw_bg(SDL_Renderer* renderer, int x, int y, int w, int h) const {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 200);
    SDL_Rect r{x, y, w, h};
    SDL_RenderFillRect(renderer, &r);
    SDL_SetRenderDrawColor(renderer, 80, 80, 80, 255);
    SDL_RenderDrawRect(renderer, &r);
}

void RciPanel::draw_bar(SDL_Renderer* renderer, int x, int y, int w, int h, uint32_t color) const {
    SDL_SetRenderDrawColor(renderer,
                           (color >> 16) & 0xFF,
                           (color >> 8) & 0xFF,
                           color & 0xFF,
                           (color >> 24) & 0xFF);
    SDL_Rect r{x, y, w, h};
    SDL_RenderFillRect(renderer, &r);
}

void RciPanel::draw_label(SDL_Renderer* renderer, FontRenderer* font,
                          int x, int y, const char* label, uint32_t color) const {
    font->draw_text(renderer, x, y, label, color);
}

} // namespace ui