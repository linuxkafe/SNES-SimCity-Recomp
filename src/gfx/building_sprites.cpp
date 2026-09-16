#include "gfx/building_sprites.h"

#include <algorithm>

#include "gfx/rom_assets.h"
#include "gfx/tile.h"
#include "sim/city.h"

namespace gfx {

// Building size classification
enum class BuildingSize { Size1x1, Size2x2, Size3x3 };

// Building definition with exact tile IDs from disassembly (bank 2, 0x03AC-0x03FE)
struct BuildingDef {
    BuildingSpriteData::Size size;
    std::vector<int> tile_ids;  // row-major order
    int anchor_x = 0;
    int anchor_y = 0;
};

// Pre-defined tile ID arrays (all within 0x03AC-0x03FE = 940-1022)
static const int kRes_D01_Tiles[4] = {0x03AC, 0x03AD, 0x03AE, 0x03AF};
static const int kRes_D23_Tiles[8] = {0x03B0, 0x03B1, 0x03AE, 0x03AF, 0x03B4, 0x03AF, 0x03AE, 0x03AF};
static const int kRes_D45_Tiles[9] = {0x03C0, 0x03C1, 0x03AE, 0x03AF, 0x03B4, 0x03AF, 0x03AE, 0x03AF, 0x03AF};

static const int kCom_D01_Tiles[4] = {0x03D0, 0x03D1, 0x03E0, 0x03E1};
static const int kCom_D23_Tiles[8] = {0x03D0, 0x03D1, 0x03E0, 0x03E1, 0x03EF, 0x03F0, 0x03EE, 0x03EF};
static const int kCom_D45_Tiles[4] = {0x03D0, 0x03D1, 0x03E0, 0x03E1};

static const int kInd_D01_Tiles[4] = {0x03F0, 0x03F1, 0x03F2, 0x03F3};
static const int kInd_D23_Tiles[8] = {0x03F4, 0x03F1, 0x03F2, 0x03F3, 0x03FC, 0x03FD, 0x03FE, 0x03FE};
static const int kInd_D45_Tiles[4] = {0x03F0, 0x03F1, 0x03F2, 0x03F3};

// Building definitions per zone type and density (0-5)
// All tile IDs within 940-1022 (0x03AC-0x03FE)
static const BuildingDef kResidentialBuildings[3] = {
    {BuildingSpriteData::Size::Size1x1, {kRes_D01_Tiles, kRes_D01_Tiles + 4}, 0, 0},
    {BuildingSpriteData::Size::Size2x2, {kRes_D23_Tiles, kRes_D23_Tiles + 8}, 0, 0},
    {BuildingSpriteData::Size::Size3x3, {kRes_D45_Tiles, kRes_D45_Tiles + 9}, 1, 1},
};

static const BuildingDef kCommercialBuildings[3] = {
    {BuildingSpriteData::Size::Size1x1, {kCom_D01_Tiles, kCom_D01_Tiles + 4}, 0, 0},
    {BuildingSpriteData::Size::Size2x2, {kCom_D23_Tiles, kCom_D23_Tiles + 8}, 0, 0},
    {BuildingSpriteData::Size::Size1x1, {kCom_D45_Tiles, kCom_D45_Tiles + 4}, 0, 0},
};

static const BuildingDef kIndustrialBuildings[3] = {
    {BuildingSpriteData::Size::Size1x1, {kInd_D01_Tiles, kInd_D01_Tiles + 4}, 0, 0},
    {BuildingSpriteData::Size::Size2x2, {kInd_D23_Tiles, kInd_D23_Tiles + 8}, 0, 0},
    {BuildingSpriteData::Size::Size1x1, {kInd_D45_Tiles, kInd_D45_Tiles + 4}, 0, 0},
};

// Special buildings (reuse existing tiles)
static const struct {
    const char* name;
    int tiles[4];
} kSpecialBuildings[] = {
    {"Power_Coal",    {0x03F4, 0x03F1, 0x03F2, 0x03F3}},
    {"Power_Nuclear", {0x03E0, 0x03E1, 0x03E2, 0x03EF}},
    {"Stadium",       {0x03D0, 0x03D1, 0x03E2, 0x03E1}},
    {"Airport",       {0x03E4, 0x03E5, 0x03E2, 0x03E3}},
    {"Seaport",       {0x03E8, 0x03ED, 0x03EA, 0x03EB}},
    {"Police_HQ",     {0x03F4, 0x03F1, 0x03FA, 0x03F3}},
    {"Fire_HQ",       {0x03FC, 0x03FD, 0x03FE, 0x03F3}},
    {"Mayor_House",   {0x03B0, 0x03B1, 0x03AE, 0x03AF}},
};

bool extract_building_sprites(const RomAssets& assets, BuildingSpriteData& data) {
    data = BuildingSpriteData{};
    if (!assets.valid) return false;

    // Default fallback colors
    data.fallback[BuildingSpriteData::Residential] = {0x20, 0x80, 0x20, 0xFF}; // green
    data.fallback[BuildingSpriteData::Commercial]  = {0x20, 0x20, 0xC0, 0xFF}; // blue
    data.fallback[BuildingSpriteData::Industrial]  = {0xA0, 0x50, 0x20, 0xFF}; // brown

    // Helper lambda to copy building def to BuildingMeta
    auto copy_def = [](const BuildingDef& def, BuildingSpriteData::BuildingMeta& meta) {
        meta.size = def.size;
        meta.tile_ids = def.tile_ids;
        meta.anchor_x = def.anchor_x;
        meta.anchor_y = def.anchor_y;
    };

    // Residential (D0=D1, D2=D3, D4=D5)
    for (int d = 0; d < 6; ++d) {
        copy_def(kResidentialBuildings[d % 3], data.residential[d]);
    }
    // Commercial
    for (int d = 0; d < 6; ++d) {
        copy_def(kCommercialBuildings[d % 3], data.commercial[d]);
    }
    // Industrial
    for (int d = 0; d < 6; ++d) {
        copy_def(kIndustrialBuildings[d % 3], data.industrial[d]);
    }

    data.valid = true;
    return true;
}

const BuildingSpriteData::BuildingMeta* select_building(const BuildingSpriteData& b,
                                                         sim::Zone zone, int density) {
    if (!b.valid) return nullptr;
    int d = std::clamp(density, 0, 5);
    switch (zone) {
        case sim::Zone::Residential: return &b.residential[d];
        case sim::Zone::Commercial:  return &b.commercial[d];
        case sim::Zone::Industrial:  return &b.industrial[d];
        default: return nullptr;
    }
}

} // namespace gfx