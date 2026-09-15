#pragma once

#include <array>
#include <cstdint>
#include <vector>

#include "gfx/tile.h"

namespace snes {
class SnesRom;
}

namespace gfx {

// Graphics assets extracted at runtime from the user-supplied ROM.
// No proprietary asset bytes are ever committed to this repository.
struct RomAssets {
    static constexpr int kTileCount      = 1024;   // Layer1 city simulation tiles
    static constexpr int kTileBytes      = 32;     // SNES 4bpp 8x8 tile
    static constexpr int kPaletteBlocks  = 14;     // BG palettes $058000..$058E00
    static constexpr int kPaletteSubs    = 8;      // 16-color sub-palettes per block

    std::vector<uint8_t> tiles;                    // kTileCount * kTileBytes
    std::array<Palette16, kPaletteBlocks * kPaletteSubs> palettes;
    // Terrain rendering palette (T020): block 5 is the verified city-layer
    // CGRAM block. Land/tree/road cells render with sub 0 (dominant index 12
    // = #315A00 dark land green), water cells with sub 1 (index 12 =
    // #319CFF blue). The per-terrain sub switch is applied by CityView
    // (see sub_palette_for_terrain); this value is the block base.
    int palette_block = 5;
    int palette_sub   = 0;
    bool valid = false;

    Palette16 terrain_palette() const {
        return palettes[palette_block * kPaletteSubs + palette_sub];
    }
};

// Decompress Layer1 city tiles ($07E584-$08C4DB) and copy BG palettes
// ($058000-$058E00, raw BGR555) into `assets`. Returns false and leaves
// `assets.valid == false` on any failure; assets can stay empty then.
bool load_rom_assets(const snes::SnesRom& rom, RomAssets& assets);

} // namespace gfx