#pragma once

#include "gfx/building_sprites.h"
#include "gfx/rom_assets.h"
#include "sim/city.h"

struct SDL_Renderer;
struct SDL_Texture;

namespace gfx {

// Renders a sim::City grid into an SDL texture and draws the visible window
// given a camera offset. The full-map texture is only rebuilt on demand, so
// per-frame cost is a single RenderCopy.
//
// When RomAssets are provided (ROM path present and extraction succeeded) the
// terrain cells are rendered with the real Layer1 city tiles + BG palette,
// scaled up x2 from 8x8 to the 16px city cell. Zone cells are rendered with
// ROM building sprites (T023) based on zone type and density. If assets are
// absent or invalid, the flat-colour fallback is used.
class CityView {
public:
    static constexpr int kTilePx = 16;

    CityView(SDL_Renderer* renderer, sim::City* city, int window_w, int window_h,
             const RomAssets* assets = nullptr);
    ~CityView();

    bool init();
    void rebuild();   // re-upload the whole map texture
    void render(int cam_x, int cam_y, int shake_x = 0, int shake_y = 0);

    void screen_to_tile(int sx, int sy, int cam_x, int cam_y,
                        int& tx, int& ty) const;

    int world_w() const { return sim::kMapWidth * kTilePx; }
    int world_h() const { return sim::kMapHeight * kTilePx; }

private:
    void fill_rect(uint32_t* px, int pitch4, int x0, int y0, uint32_t color);
    void blit_tile_cell(uint32_t* px, int pitch4, int cell_x, int cell_y,
                        int tile_index, int sub);
    void blit_building_cell(uint32_t* px, int pitch4, int cell_x, int cell_y,
                            const BuildingSpriteData::BuildingMeta& meta);

    SDL_Renderer*  renderer_;
    sim::City*     city_;
    int            window_w_;
    int            window_h_;
    const RomAssets* assets_ = nullptr;
    SDL_Texture*   texture_  = nullptr;           // full-map streaming texture
    BuildingSpriteData building_sprites_;         // extracted building quads (T023)
};

} // namespace gfx