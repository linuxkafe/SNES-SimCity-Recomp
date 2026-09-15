#include "gfx/rom_assets.h"

#include <cstring>

#include "snes/decompress.h"
#include "snes/rom.h"

namespace gfx {

namespace {

// Raw file offsets in the USA ROM (LoROM bank*0x8000 + (addr & 0x7FFF)).
// Layer1 city simulation tiles: $07E584-$08C4DB (LC_LZ5 compressed).
constexpr size_t kCityTilesFileOff = 0x3E584;
// BG palette blocks: $058000-$058E00 (14 blocks x 0x100 bytes, raw BGR555).
constexpr size_t kPalettesFileOff  = 0x28000;
constexpr int    kBlockBytes       = 0x100;

} // namespace

bool load_rom_assets(const snes::SnesRom& rom, RomAssets& assets) {
    assets.valid = false;
    assets.tiles.clear();

    // --- Layer1 city simulation tiles (LC_LZ5 -> 32768 B = 1024 x 32-B) ---
    std::vector<uint8_t> raw;
    size_t end = 0;
    snes::DecompressError err = snes::DecompressError::None;
    if (!snes::nintendo_decompress(rom, kCityTilesFileOff, raw, end, &err)) {
        return false;
    }
    if (raw.size() != RomAssets::kTileCount * RomAssets::kTileBytes) {
        return false;
    }
    assets.tiles = std::move(raw);

    // --- BG palettes (14 blocks x 128 colors, raw BGR555) ---
    const std::vector<uint8_t>& data = rom.data();
    for (int b = 0; b < RomAssets::kPaletteBlocks; ++b) {
        const size_t base = kPalettesFileOff + static_cast<size_t>(b) * kBlockBytes;
        if (base + kBlockBytes > rom.size()) return false;
        for (int sp = 0; sp < RomAssets::kPaletteSubs; ++sp) {
            Color* out = assets.palettes[b * RomAssets::kPaletteSubs + sp].colors;
            for (int i = 0; i < 16; ++i) {
                uint16_t v = static_cast<uint16_t>(data[base + (sp * 16 + i) * 2]) |
                             (static_cast<uint16_t>(data[base + (sp * 16 + i) * 2 + 1]) << 8);
                out[i] = bgr555(v);
            }
        }
    }

    assets.valid = true;
    return true;
}

} // namespace gfx