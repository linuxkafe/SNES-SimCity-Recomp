#pragma once

#include <cstdint>
#include <memory>

struct SDL_Renderer;
struct SDL_Rect;

namespace sim {
struct Stats;
} // namespace sim

namespace ui {

// Abstract font rendering interface.
// Implementations: RectFontRenderer (MVP, no deps), RomFontRenderer (future, extracts from ROM).
class FontRenderer {
public:
    virtual ~FontRenderer() = default;

    // Draw left-aligned text at (x, y) in color (0xRRGGBBAA).
    virtual void draw_text(SDL_Renderer* renderer, int x, int y, const char* text, uint32_t color) = 0;

    // Draw right-aligned text ending at (x, y).
    virtual void draw_text_right(SDL_Renderer* renderer, int x, int y, const char* text, uint32_t color) = 0;

    // Pixel width of the given text.
    virtual int text_width(const char* text) const = 0;

    // Line height in pixels.
    virtual int text_height() const = 0;
};

// Base class for all UI panels.
// Panels are screen-space (camera-independent), read-only display widgets.
class Panel {
public:
    virtual ~Panel() = default;

    // Render the panel using the provided font renderer.
    // stats: current simulation stats from sim::City::stats()
    // font: font renderer for text (never null)
    // win_w, win_h: window dimensions for positioning
    virtual void render(SDL_Renderer* renderer, const sim::Stats& stats,
                        FontRenderer* font, int win_w, int win_h) = 0;

    // Return the screen-space bounds of this panel (for layout/collision).
    virtual SDL_Rect bounds() const = 0;
};

} // namespace ui