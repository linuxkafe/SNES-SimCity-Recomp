#include "snes/scenariomap.h"
#include "snes/rom.h"
#include "snes/decompress.h"

#include <cstddef>
#include <cstdint>

namespace snes {

namespace {

// LoROM file offset from bank:addr (addr in 0x8000-0xFFFF range)
size_t lrom_file_off(uint8_t bank, uint16_t addr) {
    return static_cast<size_t>(bank) * 0x8000 + (addr & 0x7FFF);
}

// Try to decompress and decode a scenario packet at given file offset.
// Returns number of tiles decoded (0 on failure).
size_t try_decode_scenario(const SnesRom& rom, size_t file_off, uint16_t* out_tiles, size_t max_tiles) {
    std::vector<uint8_t> stage1;
    size_t end_off;
    snes::DecompressError err;
    if (!nintendo_decompress(rom, file_off, stage1, end_off, &err)) {
        return 0;
    }
    return decode_rle16(stage1.data(), stage1.size(), out_tiles, max_tiles);
}

} // namespace

std::vector<ScenarioEntry> parse_scenario_table(const SnesRom& rom) {
    std::vector<ScenarioEntry> entries;
    const size_t base = lrom_file_off(3, 0xCE70);
    if (base + 27 > rom.size()) return entries;

    // $03:CE70 is a split lo/hi/bank pointer table: lo at base+i, hi at base+9+i,
    // bank at base+18+i. These are raw file offsets — index rom.data() directly
    // instead of read8(), which would re-translate as CPU addresses.
    const std::vector<uint8_t>& data = rom.data();
    for (int i = 0; i < 9; ++i) {
        uint8_t lo   = data[base + i];
        uint8_t hi   = data[base + 9 + i];
        uint8_t bank = data[base + 18 + i];
        uint32_t addr = (static_cast<uint32_t>(bank) << 16) | (static_cast<uint32_t>(hi) << 8) | lo;
        size_t file_off = lrom_file_off(bank, static_cast<uint16_t>((hi << 8) | lo));
        entries.push_back({addr, file_off});
    }
    return entries;
}

size_t decode_rle16(const uint8_t* data, size_t len, uint16_t* out_tiles, size_t max_tiles) {
    size_t written = 0;
    size_t i = 0;
    while (i + 1 < len && written < max_tiles) {
        uint16_t v = static_cast<uint16_t>(data[i]) | (static_cast<uint16_t>(data[i + 1]) << 8);
        i += 2;
        if (v == 0xFFFF) break;
        uint16_t tile_id = v & 0x03FF;
        uint16_t repeat = ((v & 0x3C00) >> 10) + 1;
        for (uint16_t r = 0; r < repeat && written < max_tiles; ++r) {
            out_tiles[written++] = tile_id;
        }
    }
    return written;
}

bool load_scenario_terrain(const SnesRom& rom, int scenario_idx, uint16_t* out_tiles_120x100) {
    constexpr size_t kMapTiles = 120 * 100;
    const auto entries = parse_scenario_table(rom);

    // First try the requested scenario from the table
    if (scenario_idx >= 0 && scenario_idx < static_cast<int>(entries.size())) {
        size_t decoded = try_decode_scenario(rom, entries[scenario_idx].file_off, out_tiles_120x100, kMapTiles);
        if (decoded >= kMapTiles * 8 / 10) { // at least 80% filled
            return true;
        }
    }

    // Fallback: try all table entries
    for (const auto& e : entries) {
        size_t decoded = try_decode_scenario(rom, e.file_off, out_tiles_120x100, kMapTiles);
        if (decoded >= kMapTiles * 8 / 10) return true;
    }

    return false;
}

} // namespace snes