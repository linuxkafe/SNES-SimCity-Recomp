#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace snes {

class SnesRom;

struct ScenarioEntry {
    uint32_t rom_addr;
    size_t file_off;
};

// Parse the scenario pointer table at $03:CE70 (split lo/hi/bank arrays).
std::vector<ScenarioEntry> parse_scenario_table(const SnesRom& rom);

// Decode second-stage 16-bit RLE stream.
// Input: decompressed bytes from nintendo_decompress (LE u16 stream).
// Output: fills out_tiles with low-10-bit tile IDs.
// Stops at 0xFFFF or max_tiles.
// Returns number of tiles written, or 0 on error (bad format, truncated).
size_t decode_rle16(const uint8_t* data, size_t len, uint16_t* out_tiles, size_t max_tiles);

// Load scenario terrain into a 120x100 tile array (row-major).
// Decodes the requested scenario packet (0..8) via the $03:CE70 pointer table.
// On success: returns true, tiles filled with low-10-bit tile IDs.
// On failure: returns false, tiles untouched.
bool load_scenario_terrain(const SnesRom& rom, int scenario_idx, uint16_t* out_tiles_120x100);

} // namespace snes