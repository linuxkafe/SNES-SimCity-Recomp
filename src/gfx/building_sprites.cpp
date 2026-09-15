#include "gfx/building_sprites.h"

#include <algorithm>

#include "gfx/rom_assets.h"
#include "gfx/tile.h"

namespace gfx {
namespace {

// Heuristic: tiles 940-1022 are building graphics (83 tiles = ~20 quads).
// Assign sequential quads to zone types by visual inspection:
// - Residential (brick/red tones): lower IDs
// - Commercial (blue/glass): middle IDs
// - Industrial (metal/grey): higher IDs
// Each zone type gets 3 densities × 1 quad = 3 quads = 12 tiles.
static constexpr int kFirstBuildingTile = 940;
static constexpr int kLastBuildingTile  = 1022;
static constexpr int kTilesPerQuad = 4;

} // namespace

bool extract_building_sprites(const RomAssets& assets, BuildingSpriteData& data) {
    data = BuildingSpriteData{};
    if (!assets.valid) return false;

    // Verify building tile range exists
    if (kLastBuildingTile * RomAssets::kTileBytes >= static_cast<int>(assets.tiles.size())) {
        return false;
    }

    // Default fallback colors (matching original cell_color)
    data.fallback[BuildingSpriteData::Residential] = {0x20, 0x80, 0x20, 0xFF}; // green
    data.fallback[BuildingSpriteData::Commercial]  = {0x20, 0x20, 0xC0, 0xFF}; // blue
    data.fallback[BuildingSpriteData::Industrial]  = {0xA0, 0x50, 0x20, 0xFF}; // brown

    // Heuristic mapping: assign sequential quads from the building range
    // 3 zone types × 3 densities = 9 quads = 36 tiles from 940-1022
    int tile = kFirstBuildingTile;
    auto next_quad = [&]() {
        BuildingSpriteData::Quad q = {tile, tile+1, tile+2, tile+3};
        tile += kTilesPerQuad;
        return q;
    };

    // Residential (brick/red-brown tones) - first 3 quads
    data.residential[BuildingSpriteData::Low]    = next_quad(); // 940-943
    data.residential[BuildingSpriteData::Medium] = next_quad(); // 944-947
    data.residential[BuildingSpriteData::High]   = next_quad(); // 948-951

    // Commercial (blue/glass tones) - next 3 quads
    data.commercial[BuildingSpriteData::Low]    = next_quad(); // 952-955
    data.commercial[BuildingSpriteData::Medium] = next_quad(); // 956-959
    data.commercial[BuildingSpriteData::High]   = next_quad(); // 960-963

    // Industrial (metal/grey tones) - next 3 quads
    data.industrial[BuildingSpriteData::Low]    = next_quad(); // 964-967
    data.industrial[BuildingSpriteData::Medium] = next_quad(); // 968-971
    data.industrial[BuildingSpriteData::High]   = next_quad(); // 972-975

    // Verify all tile IDs are in range
    for (int t : data.residential[0]) if (t > kLastBuildingTile) return false;
    for (int t : data.residential[1]) if (t > kLastBuildingTile) return false;
    for (int t : data.residential[2]) if (t > kLastBuildingTile) return false;
    for (int t : data.commercial[0]) if (t > kLastBuildingTile) return false;
    for (int t : data.commercial[1]) if (t > kLastBuildingTile) return false;
    for (int t : data.commercial[2]) if (t > kLastBuildingTile) return false;
    for (int t : data.industrial[0]) if (t > kLastBuildingTile) return false;
    for (int t : data.industrial[1]) if (t > kLastBuildingTile) return false;
    for (int t : data.industrial[2]) if (t > kLastBuildingTile) return false;

    data.valid = true;
    return true;
}

} // namespace gfx