#pragma once

#include <array>
#include <cstdint>
#include <vector>

#include "gfx/tile.h"

namespace snes {
class SnesRom;
}

namespace gfx {

// Title-screen graphics extracted at runtime from the user-supplied ROM (T021).
// Composes the "flight" title frame from three SNES BG layers:
//   Layer 3 (2bpp) — background stars/atmosphere
//   Layer 1 (4bpp) — city skyline
//   Layer 2 (4bpp sprite bank) — SIMCITY logo / title text
// All CPU addresses come from the SimCity SNES disassembly
// (AssetPointersAndFiles.asm). No proprietary asset bytes are committed here.
struct TitleScreenData {
    static constexpr int kW = 256;
    static constexpr int kH = 224;
    static constexpr int kTileCols = kW / 8;   // 32
    static constexpr int kTileRows = kH / 8;   // 28
    static constexpr int kMaxTileId = 1024;
    static constexpr int kPalCount = 128;      // 8 sub-palettes x 16 colors

    std::vector<uint8_t>  layer1_tiles;        // 4bpp, 8x8 (sprite-count flexible)
    std::vector<uint16_t> layer1_tm;           // 64x64 entries
    std::vector<uint8_t>  layer2_tiles;        // 4bpp sprite bank
    std::vector<uint16_t> layer2_tm;           // 32x64 entries
    std::vector<uint8_t>  layer3_tiles;        // 2bpp
    std::vector<uint16_t> layer3_tm;           // 32x64 entries
    std::array<Color, kPalCount> palette;      // raw BGR555, 8x16 colors

    bool valid = false;
};

// Decompress title GFX + tilemaps and copy the raw title palette. Sets
// `data.valid` only when every packet decodes and palette bytes are present.
bool load_title_screen(const snes::SnesRom& rom, TitleScreenData& data);

// Render the title frame (native 256x224) into `out`, which must be kW x kH.
// Pixels with palette index 0 are treated as transparent (backdrop / lower
// layers show through), matching the SNES BG behaviour.
void render_title_screen(const TitleScreenData& data, Image& out);

} // namespace gfx