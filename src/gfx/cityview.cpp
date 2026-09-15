#include "gfx/cityview.h"
#include "gfx/building_sprites.h"

#include <SDL2/SDL.h>
#include <cstdint>

namespace gfx {

namespace {

struct RGB { uint8_t r, g, b; };

RGB operator*(RGB c, double f) {
    auto clamp = [f](int v) -> uint8_t {
        int vv = static_cast<int>(v * f);
        if (vv > 255) vv = 255;
        if (vv < 0) vv = 0;
        return static_cast<uint8_t>(vv);
    };
    return {clamp(c.r), clamp(c.g), clamp(c.b)};
}

uint32_t pack(RGB c) {
    return (0xFFu << 24) | (uint32_t(c.r) << 16) |
           (uint32_t(c.g) << 8) | uint32_t(c.b);
}

RGB cell_color(sim::TileView t) {
    if (t.zone == sim::Zone::None) {
        switch (t.terrain) {
            case sim::Terrain::Grass: return {96, 168, 76};
            case sim::Terrain::Water: return {40, 96, 208};
            case sim::Terrain::Tree:  return {28, 104, 52};
            case sim::Terrain::Road:  return {128, 128, 128};
            case sim::Terrain::PowerLine: return {160, 150, 120};
            case sim::Terrain::Crater: return {40, 40, 40};
        }
        return {96, 168, 76};
    }
    if (t.zone == sim::Zone::Residential)
        return RGB{212, 168, 60} * (0.7 + 0.3 * (t.density / (double)sim::City::kMaxDensity));
    if (t.zone == sim::Zone::Commercial)
        return RGB{116, 128, 208} * (0.7 + 0.3 * (t.density / (double)sim::City::kMaxDensity));
    if (t.zone == sim::Zone::Industrial)
        return RGB{168, 108, 184} * (0.7 + 0.3 * (t.density / (double)sim::City::kMaxDensity));
    return {96, 168, 76};
}

// Representative Layer1 tile id for each sim terrain. These tile ids dominate
// the decoded scenario maps (tile 0 = grass, 1 = water, 20..37 = forest,
// 48..95 = roads/dirt), matching City::get_tile_to_terrain_table().
uint16_t tile_id_for_terrain(sim::Terrain t) {
    switch (t) {
        case sim::Terrain::Grass:     return 0;
        case sim::Terrain::Water:     return 1;
        case sim::Terrain::Tree:      return 20;
        case sim::Terrain::Road:      return 48;
        case sim::Terrain::PowerLine: return 48;
        case sim::Terrain::Crater:    return 1;  // use water tile as crater (dark)
    }
    return 0;
}

// Sub-palette assignment inside the city palette block. Land, forest and road
// cells share sub 0, where the dominant palette index 12 renders as dark land
// green (#315A00). Water cells use sub 1, where the same index 12 renders as
// blue (#319CFF). Index 12 dominates the huge land/water tile ids (53% / 75%),
// so a single sub-palette can never be green AND blue: the real game switches
// BG Mode 1 tilemap palette-attribute bits per terrain exactly like this.
int sub_palette_for_terrain(sim::Terrain t) {
    return t == sim::Terrain::Water ? 1 : 0;
}

// Select building quad for a zone cell based on type and density.
const BuildingSpriteData::Quad* select_quad(const BuildingSpriteData& b,
                                            sim::Zone zone, int density) {
    if (!b.valid) return nullptr;
    BuildingSpriteData::Density d = BuildingSpriteData::Low;
    if (density >= 2) d = BuildingSpriteData::High;
    else if (density >= 1) d = BuildingSpriteData::Medium;
    switch (zone) {
        case sim::Zone::Residential: return &b.residential[d];
        case sim::Zone::Commercial:  return &b.commercial[d];
        case sim::Zone::Industrial:  return &b.industrial[d];
        default: return nullptr;
    }
}

} // namespace

CityView::CityView(SDL_Renderer* renderer, sim::City* city, int window_w,
                   int window_h, const RomAssets* assets)
    : renderer_(renderer), city_(city), window_w_(window_w), window_h_(window_h),
      assets_(assets) {}

CityView::~CityView() {
    if (texture_) SDL_DestroyTexture(texture_);
}

bool CityView::init() {
    texture_ = SDL_CreateTexture(
        renderer_, SDL_PIXELFORMAT_RGBA32,
        SDL_TEXTUREACCESS_STREAMING, world_w(), world_h());
    if (!texture_) return false;

    // Extract building sprites from ROM assets (T023)
    if (assets_ && assets_->valid) {
        extract_building_sprites(*assets_, building_sprites_);
    }
    rebuild();
    return true;
}

void CityView::fill_rect(uint32_t* px, int pitch4, int x0, int y0, uint32_t color) {
    for (int ty = 0; ty < kTilePx; ++ty) {
        uint32_t* row = px + (y0 + ty) * pitch4;
        for (int tx = 0; tx < kTilePx; ++tx) row[x0 + tx] = color;
    }
}

// Blit a ROM tile (8x8 4bpp, upscaled 2x to the 16px city cell) into the
// locked map buffer using sub-palette `sub` of the configured palette block.
// Does nothing unless a valid asset bank is present.
void CityView::blit_tile_cell(uint32_t* px, int pitch4, int cell_x, int cell_y,
                              int tile_index, int sub) {
    if (!assets_ || !assets_->valid) return;
    if (tile_index < 0 || tile_index >= RomAssets::kTileCount) return;

    const uint8_t* d = assets_->tiles.data() + tile_index * RomAssets::kTileBytes;
    int pal_idx = assets_->palette_block * RomAssets::kPaletteSubs + sub;
    if (pal_idx < 0 || pal_idx >= static_cast<int>(assets_->palettes.size())) return;
    const Color* pal = assets_->palettes[pal_idx].colors;
    const int dest_x = cell_x * kTilePx;
    const int dest_y = cell_y * kTilePx;

    for (int ty = 0; ty < kTilePx; ++ty) {
        const int srow = ty >> 1;               // 8x8 -> 16x16 (x2 upscale)
        // SNES 4bpp is two 2bpp tiles: planes 0/1 in bytes 0-15, planes 2/3 in
        // bytes 16-31 (see decode_4bpp_tile).
        const uint8_t p0 = d[srow * 2 + 0];
        const uint8_t p1 = d[srow * 2 + 1];
        const uint8_t p2 = d[16 + srow * 2 + 0];
        const uint8_t p3 = d[16 + srow * 2 + 1];
        uint32_t* row = px + (dest_y + ty) * pitch4 + dest_x;
        for (int tx = 0; tx < kTilePx; ++tx) {
            const int scol = tx >> 1;
            const int bit = 7 - scol;
            uint8_t idx = 0;
            if (p0 & (1 << bit)) idx |= 1;
            if (p1 & (1 << bit)) idx |= 2;
            if (p2 & (1 << bit)) idx |= 4;
            if (p3 & (1 << bit)) idx |= 8;
            const Color& c = pal[idx];
            row[tx] = (0xFFu << 24) | (uint32_t(c.r) << 16) |
                      (uint32_t(c.g) << 8) | uint32_t(c.b);
        }
    }
}

// Blit a 2x2 building quad (4 tiles = 16x16px) for a zone cell.
void CityView::blit_building_cell(uint32_t* px, int pitch4, int cell_x, int cell_y,
                                  const BuildingSpriteData::Quad& quad) {
    if (!assets_ || !assets_->valid) return;

    const int pal_idx = assets_->palette_block * RomAssets::kPaletteSubs + assets_->palette_sub;
    if (pal_idx < 0 || pal_idx >= static_cast<int>(assets_->palettes.size())) return;
    const Color* pal = assets_->palettes[pal_idx].colors;
    const int dest_x = cell_x * kTilePx;
    const int dest_y = cell_y * kTilePx;

    // Quad order: TL, TR, BL, BR
    const int tile_ids[4] = {quad[0], quad[1], quad[2], quad[3]};
    for (int q = 0; q < 4; ++q) {
        int tile_index = tile_ids[q];
        if (tile_index < 0 || tile_index >= RomAssets::kTileCount) continue;
        const uint8_t* d = assets_->tiles.data() + tile_index * RomAssets::kTileBytes;
        int qx = (q & 1) * 8;  // 0 or 8
        int qy = (q >> 1) * 8; // 0 or 8

        for (int ty = 0; ty < 8; ++ty) {
            const uint8_t p0 = d[ty * 2 + 0];
            const uint8_t p1 = d[ty * 2 + 1];
            const uint8_t p2 = d[16 + ty * 2 + 0];
            const uint8_t p3 = d[16 + ty * 2 + 1];
            uint32_t* row = px + (dest_y + qy + ty) * pitch4 + dest_x + qx;
            for (int tx = 0; tx < 8; ++tx) {
                const int bit = 7 - tx;
                uint8_t idx = 0;
                if (p0 & (1 << bit)) idx |= 1;
                if (p1 & (1 << bit)) idx |= 2;
                if (p2 & (1 << bit)) idx |= 4;
                if (p3 & (1 << bit)) idx |= 8;
                const Color& c = pal[idx];
                row[tx] = (0xFFu << 24) | (uint32_t(c.r) << 16) |
                          (uint32_t(c.g) << 8) | uint32_t(c.b);
            }
        }
    }
}

void CityView::rebuild() {
    void* pixels = nullptr;
    int pitch = 0;
    if (SDL_LockTexture(texture_, nullptr, &pixels, &pitch) < 0) return;
    uint32_t* px = static_cast<uint32_t*>(pixels);
    const int pitch4 = pitch / 4;

    for (int y = 0; y < sim::kMapHeight; ++y) {
        for (int x = 0; x < sim::kMapWidth; ++x) {
            const sim::TileView t = city_->tile(x, y);
            if (assets_ && assets_->valid && t.zone == sim::Zone::None) {
                fill_rect(px, pitch4, x * kTilePx, y * kTilePx, 0xFF000000u);
                blit_tile_cell(px, pitch4, x, y, tile_id_for_terrain(t.terrain),
                               sub_palette_for_terrain(t.terrain));
            } else if (assets_ && assets_->valid && building_sprites_.valid) {
                // Zone cell: render building sprite quad
                const auto* quad = select_quad(building_sprites_, t.zone, t.density);
                if (quad) {
                    blit_building_cell(px, pitch4, x, y, *quad);
                } else {
                    // Fallback to flat color
                    struct RGB { uint8_t r, g, b; };
                    auto cell_color = [&](sim::TileView tv) -> RGB {
                        if (tv.zone == sim::Zone::Residential)
                            return RGB{212, 168, 60};
                        if (tv.zone == sim::Zone::Commercial)
                            return RGB{116, 128, 208};
                        if (tv.zone == sim::Zone::Industrial)
                            return RGB{168, 108, 184};
                        return RGB{96, 168, 76};
                    };
                    RGB c = cell_color(t);
                    double k = 0.7 + 0.3 * (t.density / (double)sim::City::kMaxDensity);
                    c.r = static_cast<uint8_t>(c.r * k);
                    c.g = static_cast<uint8_t>(c.g * k);
                    c.b = static_cast<uint8_t>(c.b * k);
                    fill_rect(px, pitch4, x * kTilePx, y * kTilePx,
                              (0xFFu << 24) | (c.r << 16) | (c.g << 8) | c.b);
                }
            } else {
                // No assets: flat color fallback
                struct RGB { uint8_t r, g, b; };
                auto cell_color = [&](sim::TileView tv) -> RGB {
                    if (tv.zone == sim::Zone::None) {
                        switch (tv.terrain) {
                            case sim::Terrain::Grass: return {96, 168, 76};
                            case sim::Terrain::Water: return {40, 96, 208};
                            case sim::Terrain::Tree:  return {28, 104, 52};
                            case sim::Terrain::Road:  return {128, 128, 128};
                            case sim::Terrain::PowerLine: return {160, 150, 120};
                            case sim::Terrain::Crater: return {40, 40, 40};
                        }
                        return {96, 168, 76};
                    }
                    if (tv.zone == sim::Zone::Residential)
                        return RGB{212, 168, 60};
                    if (tv.zone == sim::Zone::Commercial)
                        return RGB{116, 128, 208};
                    if (tv.zone == sim::Zone::Industrial)
                        return RGB{168, 108, 184};
                    return {96, 168, 76};
                };
                RGB c = cell_color(t);
                double k = (t.zone == sim::Zone::None) ? 1.0
                    : (0.7 + 0.3 * (t.density / (double)sim::City::kMaxDensity));
                c.r = static_cast<uint8_t>(c.r * k);
                c.g = static_cast<uint8_t>(c.g * k);
                c.b = static_cast<uint8_t>(c.b * k);
                fill_rect(px, pitch4, x * kTilePx, y * kTilePx,
                          (0xFFu << 24) | (c.r << 16) | (c.g << 8) | c.b);
            }
        }
    }
    SDL_UnlockTexture(texture_);
}

void CityView::render(int cam_x, int cam_y, int shake_x, int shake_y) {
    const int visible_w = world_w() - cam_x < window_w_ ? world_w() - cam_x : window_w_;
    const int visible_h = world_h() - cam_y < window_h_ ? world_h() - cam_y : window_h_;
    SDL_Rect src{cam_x + shake_x, cam_y + shake_y, visible_w, visible_h};
    SDL_Rect dst{0, 0, visible_w, visible_h};
    SDL_RenderCopy(renderer_, texture_, &src, &dst);
}

void CityView::screen_to_tile(int sx, int sy, int cam_x, int cam_y,
                              int& tx, int& ty) const {
    tx = (sx + cam_x) / kTilePx;
    ty = (sy + cam_y) / kTilePx;
}

} // namespace gfx