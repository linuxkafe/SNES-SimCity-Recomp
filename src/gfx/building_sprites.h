#pragma once

#include <array>
#include <cstdint>
#include <vector>
#include <optional>

#include "gfx/rom_assets.h"
#include "gfx/tile.h"
#include "sim/city.h"

namespace gfx {

// Building sprite mapping extracted from ROM city tile bank (T033).
// Buildings are 1x1, 2x2, or 3x3 tiles composed of 8x8 tiles from the
// 1024-tile bank. Maps zone type + density (0-5) to building metadata.
struct BuildingSpriteData {
    enum ZoneType : uint8_t { Residential, Commercial, Industrial, ZoneTypeCount };
    enum Density : uint8_t { D0, D1, D2, D3, D4, D5, DensityCount };

    // Building size classification
    enum class Size : uint8_t { Size1x1, Size2x2, Size3x3, SizeCount };

    // Building metadata for a single density level of a zone type
    struct BuildingMeta {
        Size size = Size::Size1x1;
        // Tile IDs in rendering order (row-major: TL, TR, BL, BR for 2x2;
        // row-major for 3x3: TL, TM, TR, ML, MM, MR, BL, BM, BR)
        std::vector<int> tile_ids;
        // Anchor point within building (0,0 = top-left)
        int anchor_x = 0;
        int anchor_y = 0;
    };

    // Building metadata per zone type and density
    BuildingMeta residential[DensityCount];
    BuildingMeta commercial[DensityCount];
    BuildingMeta industrial[DensityCount];

    // Fallback solid color per zone (RGBA) when sprite not available.
    Color fallback[ZoneTypeCount];

    bool valid = false;
};

// Extract building sprite metadata from the city tile bank (already in RomAssets).
// Uses exact tile IDs from Yoshifanatic1 disassembly (bank 2, tiles ~940-1022).
// Maps zone type + density (0-5) to building metadata with exact tile IDs.
bool extract_building_sprites(const RomAssets& assets, BuildingSpriteData& data);

// Select building metadata for a zone cell based on type and density (0-5).
const BuildingSpriteData::BuildingMeta* select_building(const BuildingSpriteData& b,
                                                         sim::Zone zone, int density);

} // namespace gfx