#include "ui/population_panel.h"
#include <SDL2/SDL.h>
#include <cstdio>
#include "sim/city.h"

namespace ui {

void PopulationPanel::render(SDL_Renderer* renderer, const sim::Stats& stats,
                             FontRenderer* font, int win_w, int win_h) {
    (void)win_h;
    int x = win_w - kPanelW - kMargin;
    int y = 24 + kMargin;  // below status bar

    draw_bg(renderer, x, y, kPanelW, kPanelH);

    char buf[64];
    int line_y = y + kMargin;

    format_population(stats.population, buf, sizeof(buf));
    draw_label_value(renderer, font, x + kMargin, line_y, "POP:", buf, 0xAAAAAAFF, 0xFFFFFFFF);
    line_y += kLineH;

    std::snprintf(buf, sizeof(buf), "R:%d  C:%d  I:%d",
                  stats.resident_tiles, stats.commercial_tiles, stats.industrial_tiles);
    draw_label_value(renderer, font, x + kMargin, line_y, "ZONES:", buf, 0xAAAAAAFF, 0xFFFFFFFF);
    line_y += kLineH;

    format_population(stats.jobs, buf, sizeof(buf));
    draw_label_value(renderer, font, x + kMargin, line_y, "JOBS:", buf, 0xAAAAAAFF, 0xFFFFFFFF);
}

SDL_Rect PopulationPanel::bounds() const {
    // Will be computed at render time with actual window width
    return {0, 24 + kMargin, kPanelW, kPanelH};
}

void PopulationPanel::format_population(int pop, char* out, size_t out_size) {
    // Format with commas: 12345 -> 12,345
    if (pop >= 1000) {
        int thousands = pop / 1000;
        int remainder = pop % 1000;
        std::snprintf(out, out_size, "%d,%03d", thousands, remainder);
    } else {
        std::snprintf(out, out_size, "%d", pop);
    }
}

void PopulationPanel::draw_bg(SDL_Renderer* renderer, int x, int y, int w, int h) const {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 200);
    SDL_Rect r{x, y, w, h};
    SDL_RenderFillRect(renderer, &r);
    SDL_SetRenderDrawColor(renderer, 80, 80, 80, 255);
    SDL_RenderDrawRect(renderer, &r);
}

void PopulationPanel::draw_label_value(SDL_Renderer* renderer, FontRenderer* font,
                                       int x, int y, const char* label, const char* value,
                                       uint32_t label_color, uint32_t value_color) const {
    font->draw_text(renderer, x, y, label, label_color);
    int value_x = x + font->text_width(label) + 4;
    font->draw_text(renderer, value_x, y, value, value_color);
}

} // namespace ui