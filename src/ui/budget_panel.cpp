#include "ui/budget_panel.h"
#include <SDL2/SDL.h>
#include <cstdio>
#include "sim/city.h"

namespace ui {

void BudgetPanel::render(SDL_Renderer* renderer, const sim::Stats& stats,
                         FontRenderer* font, int win_w, int win_h) {
    (void)win_w; (void)win_h;
    int x = kMargin;
    int y = 24 + kMargin;  // below status bar (22px) + margin

    draw_bg(renderer, x, y, kPanelW, kPanelH);

    char buf[64];
    int line_y = y + kMargin;

    format_funds(stats.funds, buf, sizeof(buf));
    draw_label_value(renderer, font, x + kMargin, line_y, "FUNDS:", buf, 0xAAAAAAFF, 0xFFFFFFFF);
    line_y += kLineH;

    format_income(stats.income, buf, sizeof(buf));
    uint32_t income_color = (stats.income >= 0) ? 0x88FF88FF : 0xFF8888FF;
    draw_label_value(renderer, font, x + kMargin, line_y, "INCOME:", buf, 0xAAAAAAFF, income_color);
    line_y += kLineH;

    format_income(-stats.upkeep, buf, sizeof(buf));  // show as negative
    draw_label_value(renderer, font, x + kMargin, line_y, "EXPENSES:", buf, 0xAAAAAAFF, 0xFF8888FF);
    line_y += kLineH;

    format_tax_rate(stats.tax_rate, buf, sizeof(buf));
    draw_label_value(renderer, font, x + kMargin, line_y, "TAX:", buf, 0xAAAAAAFF, 0xFFFFFFFF);
}

SDL_Rect BudgetPanel::bounds() const {
    return {kMargin, 24 + kMargin, kPanelW, kPanelH};
}

void BudgetPanel::format_funds(int64_t funds, char* out, size_t out_size) {
    // Format with commas: 20000 -> $20,000, 1234567 -> $1,234,567
    bool negative = funds < 0;
    int64_t abs_funds = negative ? -funds : funds;

    char buf[64];
    int pos = 0;
    if (abs_funds == 0) {
        buf[pos++] = '0';
    } else {
        // Build digits in reverse
        char digits[32];
        int dpos = 0;
        while (abs_funds > 0) {
            digits[dpos++] = '0' + (abs_funds % 10);
            abs_funds /= 10;
        }
        // Add commas every 3 digits from right
        for (int i = dpos - 1; i >= 0; --i) {
            buf[pos++] = digits[i];
            // Add comma if there are more digits and remaining count is multiple of 3
            if (i > 0 && (i % 3) == 0) {
                buf[pos++] = ',';
            }
        }
    }
    buf[pos] = '\0';

    if (negative) {
        std::snprintf(out, out_size, "-$%s", buf);
    } else {
        std::snprintf(out, out_size, "$%s", buf);
    }
}

void BudgetPanel::format_income(int64_t income, char* out, size_t out_size) {
    // Format with sign: +$1,250 or -$500
    char sign = (income >= 0) ? '+' : '-';
    int64_t abs_income = income < 0 ? -income : income;

    char buf[64];
    int pos = 0;
    if (abs_income == 0) {
        buf[pos++] = '0';
    } else {
        char digits[32];
        int dpos = 0;
        while (abs_income > 0) {
            digits[dpos++] = '0' + (abs_income % 10);
            abs_income /= 10;
        }
        for (int i = dpos - 1; i >= 0; --i) {
            buf[pos++] = digits[i];
            if (i > 0 && (i % 3) == 0) {
                buf[pos++] = ',';
            }
        }
    }
    buf[pos] = '\0';

    std::snprintf(out, out_size, "%c$%s", sign, buf);
}

void BudgetPanel::format_tax_rate(int rate, char* out, size_t out_size) {
    std::snprintf(out, out_size, "%d%%", rate);
}

void BudgetPanel::draw_bg(SDL_Renderer* renderer, int x, int y, int w, int h) const {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 200);
    SDL_Rect r{x, y, w, h};
    SDL_RenderFillRect(renderer, &r);
    SDL_SetRenderDrawColor(renderer, 80, 80, 80, 255);
    SDL_RenderDrawRect(renderer, &r);
}

void BudgetPanel::draw_label_value(SDL_Renderer* renderer, FontRenderer* font,
                                   int x, int y, const char* label, const char* value,
                                   uint32_t label_color, uint32_t value_color) const {
    font->draw_text(renderer, x, y, label, label_color);
    int value_x = x + font->text_width(label) + 4;
    font->draw_text(renderer, value_x, y, value, value_color);
}

} // namespace ui