#pragma once

#include "ui/panel.h"
#include <cstdint>

namespace ui {

// Budget panel: shows city funds, income, expenses, tax rate.
// Positioned top-left, below the status bar.
class BudgetPanel : public Panel {
public:
    BudgetPanel() = default;

    void render(SDL_Renderer* renderer, const sim::Stats& stats,
                FontRenderer* font, int win_w, int win_h) override;
    SDL_Rect bounds() const override;

    // Static helpers for headless testing (format numbers for display).
    static void format_funds(int64_t funds, char* out, size_t out_size);
    static void format_income(int64_t income, char* out, size_t out_size);
    static void format_tax_rate(int rate, char* out, size_t out_size);

private:
    static constexpr int kPanelW = 200;
    static constexpr int kPanelH = 110;
    static constexpr int kMargin = 8;
    static constexpr int kLineH = 14;

    void draw_bg(SDL_Renderer* renderer, int x, int y, int w, int h) const;
    void draw_label_value(SDL_Renderer* renderer, FontRenderer* font,
                          int x, int y, const char* label, const char* value,
                          uint32_t label_color, uint32_t value_color) const;
};

} // namespace ui