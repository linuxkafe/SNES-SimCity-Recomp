#pragma once

#include "ui/panel.h"
#include <cstdint>

namespace ui {

// Population panel: shows total population, R/C/I breakdown, jobs.
// Positioned top-right.
class PopulationPanel : public Panel {
public:
    PopulationPanel() = default;

    void render(SDL_Renderer* renderer, const sim::Stats& stats,
                FontRenderer* font, int win_w, int win_h) override;
    SDL_Rect bounds() const override;

    // Static helper for headless testing.
    static void format_population(int pop, char* out, size_t out_size);

private:
    static constexpr int kPanelW = 200;
    static constexpr int kPanelH = 100;
    static constexpr int kMargin = 8;
    static constexpr int kLineH = 14;

    void draw_bg(SDL_Renderer* renderer, int x, int y, int w, int h) const;
    void draw_label_value(SDL_Renderer* renderer, FontRenderer* font,
                          int x, int y, const char* label, const char* value,
                          uint32_t label_color, uint32_t value_color) const;
};

} // namespace ui