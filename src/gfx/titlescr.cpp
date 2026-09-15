#include "gfx/titlescr.h"

#include <algorithm>
#include <cstdio>

#include "snes/decompress.h"
#include "snes/rom.h"

namespace gfx {
namespace {

constexpr uint32_t kGfxLayer1 = 0x07C9E0;   // 4bpp, 384 tiles
constexpr uint32_t kGfxLayer2 = 0x07A680;   // 4bpp sprite bank, 512 tiles
constexpr uint32_t kGfxLayer3 = 0x07C930;   // 2bpp, 32 tiles
constexpr uint32_t kTMLayer1  = 0x0B966B;   // 64x64 entries
constexpr uint32_t kTMLayer2  = 0x0B942B;   // 32x64 entries
constexpr uint32_t kTMLayer3  = 0x0B9224;   // 32x64 entries
constexpr uint32_t kPalTitle  = 0x0C89D8;   // raw BGR555, 128 colors

// 8x8 tile pixels as palette indices, [row][col].
struct Idx8 {
    uint8_t p[8][8];
};

Idx8 decode_4bpp(const uint8_t* d) {
    Idx8 t;
    for (int row = 0; row < 8; ++row) {
        const uint8_t p0 = d[row * 2 + 0];
        const uint8_t p1 = d[row * 2 + 1];
        const uint8_t p2 = d[16 + row * 2 + 0];
        const uint8_t p3 = d[16 + row * 2 + 1];
        for (int col = 0; col < 8; ++col) {
            const int bit = 7 - col;
            uint8_t idx = 0;
            if (p0 & (1 << bit)) idx |= 1;
            if (p1 & (1 << bit)) idx |= 2;
            if (p2 & (1 << bit)) idx |= 4;
            if (p3 & (1 << bit)) idx |= 8;
            t.p[row][col] = idx;
        }
    }
    return t;
}

Idx8 decode_2bpp(const uint8_t* d) {
    Idx8 t;
    for (int row = 0; row < 8; ++row) {
        const uint8_t p0 = d[row * 2 + 0];
        const uint8_t p1 = d[row * 2 + 1];
        for (int col = 0; col < 8; ++col) {
            const int bit = 7 - col;
            uint8_t idx = 0;
            if (p0 & (1 << bit)) idx |= 1;
            if (p1 & (1 << bit)) idx |= 2;
            t.p[row][col] = idx;
        }
    }
    return t;
}

// Blit one BG layer onto `out`. `tile_bytes` is 32 (4bpp) or 16 (2bpp);
// `tm_w` is the tilemap width in entries; `scx/scy` are whole-tile scrolls.
// Palette index 0 is transparent (shows what is behind / the backdrop).
void blit_layer(Image& out, const std::vector<uint8_t>& tiles, int tile_bytes,
                const std::vector<uint16_t>& tm, int tm_w, int scx, int scy,
                const std::array<Color, TitleScreenData::kPalCount>& pal) {
    if (tiles.empty() || tm.empty()) return;
    const int tm_h = static_cast<int>(tm.size()) / tm_w;
    for (int ty = 0; ty < TitleScreenData::kTileRows; ++ty) {
        const int mrow = scy + ty;
        if (mrow < 0 || mrow >= tm_h) continue;
        for (int tx = 0; tx < TitleScreenData::kTileCols; ++tx) {
            const int mcol = scx + tx;
            if (mcol < 0 || mcol >= tm_w) continue;
            const uint16_t v = tm[static_cast<size_t>(mrow) * tm_w + mcol];
            const int tile = v & 0x3FF;
            const int sub  = (v >> 10) & 7;
            const bool hflip = (v & 0x4000) != 0;
            const bool vflip = (v & 0x8000) != 0;
            if (tile * tile_bytes >= static_cast<int>(tiles.size())) continue;

            const Idx8 idx = (tile_bytes == 32)
                ? decode_4bpp(tiles.data() + tile * 32)
                : decode_2bpp(tiles.data() + tile * 16);
            for (int yy = 0; yy < 8; ++yy) {
                for (int xx = 0; xx < 8; ++xx) {
                    const int srow = vflip ? 7 - yy : yy;
                    const int scol = hflip ? 7 - xx : xx;
                    const uint8_t i = idx.p[srow][scol];
                    if (i == 0) continue;
                    out.at(tx * 8 + xx, ty * 8 + yy) = pal[sub * 16 + i];
                }
            }
        }
    }
}

} // namespace

bool load_title_screen(const snes::SnesRom& rom, TitleScreenData& d) {
    d = TitleScreenData{};
    const auto& data = rom.data();
    size_t end = 0;
    snes::DecompressError err = snes::DecompressError::None;

    auto decompress_cpu = [&](uint32_t cpu, std::vector<uint8_t>& out) {
        size_t off = 0;
        if (!rom.translate(cpu, off)) return false;
        if (!snes::nintendo_decompress(rom, off, out, end, &err)) return false;
        return true;
    };
    auto tm_vec = [](std::vector<uint8_t>& bytes) {
        std::vector<uint16_t> tm(bytes.size() / 2);
        for (size_t i = 0; i < tm.size(); ++i)
            tm[i] = static_cast<uint16_t>(bytes[i * 2] | (bytes[i * 2 + 1] << 8));
        return tm;
    };

    std::vector<uint8_t> raw;
    if (!decompress_cpu(kGfxLayer1, d.layer1_tiles)) return false;
    if (!decompress_cpu(kGfxLayer2, d.layer2_tiles)) return false;
    if (!decompress_cpu(kGfxLayer3, d.layer3_tiles)) return false;
    if (!decompress_cpu(kTMLayer1, raw)) return false;
    d.layer1_tm = tm_vec(raw);
    if (!decompress_cpu(kTMLayer2, raw)) return false;
    d.layer2_tm = tm_vec(raw);
    if (!decompress_cpu(kTMLayer3, raw)) return false;
    d.layer3_tm = tm_vec(raw);

    size_t pal_off = 0;
    if (!rom.translate(kPalTitle, pal_off)) return false;
    if (pal_off + static_cast<size_t>(d.kPalCount) * 2 > data.size()) return false;
    for (size_t i = 0; i < static_cast<size_t>(d.kPalCount); ++i) {
        const uint16_t v = static_cast<uint16_t>(data[pal_off + i * 2]
                                                 | (data[pal_off + i * 2 + 1] << 8));
        d.palette[i] = bgr555(v);
    }

    d.valid = true;
    return true;
}

void render_title_screen(const TitleScreenData& d, Image& out) {
    if (out.width != TitleScreenData::kW || out.height != TitleScreenData::kH) return;
    std::fill(out.pixels.begin(), out.pixels.end(), d.palette[0]);

    // Back to front: atmosphere (L3), skyline (L1), logo (L2).
    blit_layer(out, d.layer3_tiles, 16, d.layer3_tm, 32, 0, 0, d.palette);
    blit_layer(out, d.layer1_tiles, 32, d.layer1_tm, 64, 0, 0, d.palette);
    blit_layer(out, d.layer2_tiles, 32, d.layer2_tm, 32, 0, 0, d.palette);
}

} // namespace gfx