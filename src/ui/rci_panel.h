#pragma once

#include "ui/panel.h"
#include "sim/city.h"
#include <cstdint>

namespace ui {

// RCI panel: three horizontal demand bars for Residential, Commercial, Industrial.
// Positioned bottom-left.
class RciPanel : public Panel {
public:
    RciPanel() = default;

    void render(SDL_Renderer* renderer, const sim::Stats& stats,
                FontRenderer* font, int win_w, int win_h) override;
    SDL_Rect bounds() const override;

    // Static helper for headless testing: demand level -> bar width (0..max).
    static int demand_width(sim::Demand d, int max_width);

private:
    static constexpr int kPanelW = 300;
    static constexpr int kPanelH = 60;
    static constexpr int kMargin = 8;
    static constexpr int kBarH = 18;
    static constexpr int kBarGap = 6;
    static constexpr int kLabelW = 24;

    void draw_bg(SDL_Renderer* renderer, int x, int y, int w, int h) const;
    void draw_bar(SDL_Renderer* renderer, int x, int y, int w, int h, uint32_t color) const;
    void draw_label(SDL_Renderer* renderer, FontRenderer* font,
                    int x, int y, const char* label, uint32_t color) const;
};

} // namespace ui