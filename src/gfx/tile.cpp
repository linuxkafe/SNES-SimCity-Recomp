#include "gfx/tile.h"

namespace gfx {

Palette16 default_palette() {
    Palette16 p;
    p.colors[0]  = {0,   0,   0,   255};
    p.colors[1]  = {0,   0,   0,   0};
    p.colors[2]  = {50,  50,  50,  255};
    p.colors[3]  = {80,  80,  80,  255};
    p.colors[4]  = {120, 120, 120, 255};
    p.colors[5]  = {160, 160, 160, 255};
    p.colors[6]  = {200, 200, 200, 255};
    p.colors[7]  = {255, 255, 255, 255};
    p.colors[8]  = {30,  80,  30,  255};
    p.colors[9]  = {60, 140,  60,  255};
    p.colors[10] = {100, 200, 100, 255};
    p.colors[11] = {150, 220, 150, 255};
    p.colors[12] = {40,  40, 120,  255};
    p.colors[13] = {80,  80, 200,  255};
    p.colors[14] = {150, 150, 255, 255};
    p.colors[15] = {220, 220, 255, 255};
    return p;
}

Tile8x8 decode_4bpp_tile(const uint8_t* d) {
    // SNES 4bpp is two 2bpp tiles: bit planes 0/1 in bytes 0-15, planes 2/3 in
    // bytes 16-31. Plane p for row r is d[p*16 + r*2 + (p & 1)].
    Tile8x8 t;
    for (int row = 0; row < 8; ++row) {
        const uint8_t plane0 = d[row * 2 + 0];
        const uint8_t plane1 = d[row * 2 + 1];
        const uint8_t plane2 = d[16 + row * 2 + 0];
        const uint8_t plane3 = d[16 + row * 2 + 1];
        for (int col = 0; col < 8; ++col) {
            int bit = 7 - col;
            uint8_t idx = 0;
            if (plane0 & (1 << bit)) idx |= 1;
            if (plane1 & (1 << bit)) idx |= 2;
            if (plane2 & (1 << bit)) idx |= 4;
            if (plane3 & (1 << bit)) idx |= 8;
            t.pixels[row][col] = {idx, 0, 0, 255};  // idx stored in r for lookup
        }
    }
    return t;
}

void blit_tile(Image& dst, int dst_x, int dst_y,
               const uint8_t* rom_data, const Palette16& pal) {
    Tile8x8 tile = decode_4bpp_tile(rom_data);
    for (int row = 0; row < 8; ++row) {
        for (int col = 0; col < 8; ++col) {
            int px = dst_x + col;
            int py = dst_y + row;
            if (px < 0 || px >= dst.width || py < 0 || py >= dst.height) continue;
            uint8_t idx = tile.pixels[row][col].r;  // r holds the palette index
            dst.at(px, py) = pal.colors[idx];
        }
    }
}

Image make_image(int width, int height) {
    Image img;
    img.width = width;
    img.height = height;
    img.pixels.resize(width * height, kTransparent);
    return img;
}

} // namespace gfx