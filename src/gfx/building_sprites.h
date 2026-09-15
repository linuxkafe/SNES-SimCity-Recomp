#pragma once

#include <array>
#include <cstdint>
#include <vector>

#include "gfx/rom_assets.h"
#include "gfx/tile.h"

namespace gfx {

// Building sprite mapping extracted from ROM city tile bank (T023).
// Buildings are 2x2 tiles (16x16px) composed of 4 8x8 tiles from the
// 1024-tile bank at IDs ~940-1022. Maps zone type + density to tile quads.
struct BuildingSpriteData {
    enum ZoneType : uint8_t { Residential, Commercial, Industrial, ZoneTypeCount };
    enum Density : uint8_t { Low, Medium, High, DensityCount };

    // 4 tile IDs forming a 2x2 building (TL, TR, BL, BR order)
    using Quad = std::array<int, 4>;

    Quad residential[DensityCount];
    Quad commercial[DensityCount];
    Quad industrial[DensityCount];
    // Fallback solid color per zone (RGBA) when sprite not available.
    Color fallback[ZoneTypeCount];

    bool valid = false;
};

// Extract building sprite quads from the city tile bank (already in RomAssets).
// Uses heuristic: tiles 940-1022 contain building graphics. Groups sequential
// quads by visual pattern (Residential=brick, Commercial=glass, Industrial=metal).
bool extract_building_sprites(const RomAssets& assets, BuildingSpriteData& data);

} // namespace gfx