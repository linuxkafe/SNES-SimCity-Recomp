#pragma once

#include <cstdint>
#include <cstddef>
#include <vector>

namespace gfx {

struct Color { uint8_t r, g, b, a; };
static constexpr Color kTransparent = {0, 0, 0, 0};

struct Tile8x8 {
    Color pixels[8][8];  // [row][col], row 0 = top
};

struct Palette16 {
    Color colors[16];
};

struct Image {
    std::vector<Color> pixels;
    int width;
    int height;
    Color& at(int x, int y) { return pixels[y * width + x]; }
    const Color& at(int x, int y) const { return pixels[y * width + x]; }
};

// SNES 15-bit RGB (one BGR555 value) → Color.
inline Color bgr555(uint16_t c) {
    uint8_t r = static_cast<uint8_t>(((c >> 0)  & 0x1F) * 255 / 31);
    uint8_t g = static_cast<uint8_t>(((c >> 5)  & 0x1F) * 255 / 31);
    uint8_t b = static_cast<uint8_t>(((c >> 10) & 0x1F) * 255 / 31);
    return {r, g, b, 255};
}

// Default palette: NES-like palette used when no game palette loaded.
Palette16 default_palette();

// Decode 32 bytes of SNES 4bpp tile data → Tile8x8. SNES 4bpp is packed as two
// 2bpp tiles: planes 0/1 in bytes 0-15, planes 2/3 in bytes 16-31.
Tile8x8 decode_4bpp_tile(const uint8_t* data32);

// Decode a tile into an Image at (dst_x, dst_y). Palette maps indices 0-15.
void blit_tile(Image& dst, int dst_x, int dst_y,
               const uint8_t* rom_data, const Palette16& pal);

// Create an Image wide × high and fill with checkerboard (alpha placeholder).
Image make_image(int width, int height);

} // namespace gfx