#include "snes/rom.h"

#include <cstring>
#include <fstream>
#include <iostream>

namespace snes {

static constexpr size_t kMaxRomSize = 4ull * 1024 * 1024; // 4MB (max LoROM)

SnesRom::SnesRom(const std::string& path) : path_(path) {}

bool SnesRom::load() {
    std::ifstream f(path_, std::ios::binary | std::ios::ate);
    if (!f) {
        return false;
    }
    std::streamsize sz = f.tellg();
    if (sz <= 0 || static_cast<size_t>(sz) > kMaxRomSize) {
        return false;
    }
    data_.resize(static_cast<size_t>(sz));
    f.seekg(0);
    f.read(reinterpret_cast<char*>(data_.data()), sz);
    if (!f) {
        data_.clear();
        return false;
    }
    parse_header();
    return true;
}

void SnesRom::parse_header() {
    constexpr size_t kLoRomHeaderBank0 = 0x7FC0;
    constexpr size_t kHiRomHeaderEnd   = 0xFFC0;

    if (data_.size() >= kLoRomHeaderBank0 + 0x40 &&
        fill_header_at(kLoRomHeaderBank0) &&
        header_.map_mode == MapMode::LoROM) {
        parsed_ = true;
        return;
    }
    if (data_.size() >= kHiRomHeaderEnd + 0x40 &&
        data_.size() >= 0x100000) {
        size_t hi_off = data_.size() - 0x4000;
        if (fill_header_at(hi_off) && header_.map_mode == MapMode::HiROM) {
            parsed_ = true;
            return;
        }
    }
    for (size_t off = kLoRomHeaderBank0;
         off + 0x40 <= data_.size() && off < 0x400000;
         off += 0x8000) {
        if (fill_header_at(off) && header_.map_mode == MapMode::LoROM) {
            parsed_ = true;
            return;
        }
    }
    parsed_ = false;
}

bool SnesRom::fill_header_at(size_t off) {
    SnesHeader h = {};
    if (off + 21 > data_.size()) return false;
    h.title.assign(reinterpret_cast<const char*>(&data_[off]), 21);
    // SNES title is space-padded to 21 bytes; strip trailing whitespace/NULs.
    size_t last = h.title.find_last_not_of(" \0");
    if (last == std::string::npos) h.title.clear();
    else h.title.erase(last + 1);
    if (off + 32 > data_.size()) return false;
    uint8_t map_mode_raw = data_[off + 21];
    if (map_mode_raw == 0x20)      h.map_mode = MapMode::LoROM;
    else if (map_mode_raw == 0x21) h.map_mode = MapMode::HiROM;
    else                           h.map_mode = MapMode::Unknown;
    if (h.map_mode == MapMode::Unknown) return false;
    h.cart_type         = data_[off + 22];
    h.rom_size_kb_pow2  = data_[off + 23];
    h.sram_size_kb_pow2 = data_[off + 24];
    h.country_code      = data_[off + 25];
    h.license_code      = data_[off + 26];
    h.version           = data_[off + 27];
    h.checksum_complement = static_cast<uint16_t>(data_[off + 28] | (data_[off + 29] << 8));
    h.checksum            = static_cast<uint16_t>(data_[off + 30] | (data_[off + 31] << 8));
    h.native_emulation_vector = 0;
    h.native_irq_vector       = 0;
    h.native_nmi_vector       = 0;
    header_ = h;  // publish before the vector block so translate() sees the map mode
    // Vectors: the native table lives at the end of the header's bank.
    //   NMI  at $X:FFEA-$FFEB, IRQ at $X:FFF4-$FFF5, RESET at $X:FFFC-$FFFD
    //   where X = 0x00 for LoROM, 0xC0 for HiROM.
    uint32_t vec_bank = (h.map_mode == MapMode::HiROM) ? 0xC0 : 0x00;
    auto read_vec16 = [&](uint32_t addr16) -> uint16_t {
        size_t voff = 0;
        if (!translate((vec_bank << 16) | addr16, voff) || voff + 1 >= data_.size()) {
            return 0xFFFF;
        }
        return static_cast<uint16_t>(data_[voff] | (data_[voff + 1] << 8));
    };
    uint16_t emu_nmi  = read_vec16(0xFFE6);  // emulation-mode NMI
    h.native_emulation_vector = (vec_bank << 16) | emu_nmi;
    h.native_nmi_vector       = (vec_bank << 16) | read_vec16(0xFFEA);
    h.native_irq_vector       = (vec_bank << 16) | read_vec16(0xFFF4);
    h.native_reset_vector     = (vec_bank << 16) | read_vec16(0xFFFC);
    header_ = h;
    return true;
}

bool SnesRom::translate(uint32_t cpu_addr, size_t& file_off) const {
    if (data_.empty()) return false;
    uint8_t  bank = static_cast<uint8_t>((cpu_addr >> 16) & 0xFF);
    uint16_t lo   = static_cast<uint16_t>(cpu_addr & 0xFFFF);

    switch (header_.map_mode) {
    case MapMode::LoROM: {
        // $00-3F:8000-FFFF and $80-BF:8000-FFFF map to ROM.
        if (lo < 0x8000) return false;
        uint32_t b = bank & 0x7F;
        size_t off = static_cast<size_t>(b << 15) + (lo & 0x7FFF);
        // Wrap around ROM size (LoROM banks cycle every (size/32KB) banks).
        size_t bank_bytes = (data_.size() / 0x8000) * 0x8000;
        if (bank_bytes == 0) return false;
        off %= bank_bytes;
        if (off >= data_.size()) return false;
        file_off = off;
        return true;
    }
    case MapMode::HiROM: {
        // $C0-FF:8000-FFFF and $40-7D:8000-FFFF (2MB banks).
        if (lo < 0x8000) return false;
        uint32_t b = bank & 0x7F;
        if (b >= 0x40) b -= 0x40;
        size_t off = static_cast<size_t>(b << 16) + (lo & 0xFFFF);
        if (off >= data_.size()) return false;
        file_off = off;
        return true;
    }
    default:
        return false;
    }
}

uint8_t SnesRom::read8(uint32_t cpu_addr) const {
    size_t off = 0;
    if (!translate(cpu_addr, off)) return 0xFF;
    return data_[off];
}

uint16_t SnesRom::read16(uint32_t cpu_addr) const {
    size_t off = 0;
    if (!translate(cpu_addr, off)) return 0xFFFF;
    if (off + 1 >= data_.size()) return 0xFFFF;
    return static_cast<uint16_t>(data_[off] | (data_[off + 1] << 8));
}

uint32_t SnesRom::read32(uint32_t cpu_addr) const {
    uint32_t lo16 = read16(cpu_addr);
    uint32_t hi16 = read16(cpu_addr + 2);
    return lo16 | (hi16 << 16);
}

bool SnesRom::checksum_valid() const {
    if (header_.checksum == 0 && header_.checksum_complement == 0) return false;
    // SimCity (and some other SNES titles) stores checksum = sum(all bytes) mod 0x10000,
    // and complement = 0xFFFF − checksum.  This self-referential scheme is validated
    // by checking the stored checksum against the recomputed sum.
    uint32_t sum = 0;
    for (uint8_t b : data_) sum += b;
    sum &= 0xFFFF;
    return sum == header_.checksum;
}

std::string map_mode_name(MapMode m) {
    switch (m) {
    case MapMode::LoROM: return "LoROM";
    case MapMode::HiROM: return "HiROM";
    default:             return "Unknown";
    }
}

std::string cart_type_name(uint8_t t) {
    switch (t) {
    case 0x00: return "ROM only";
    case 0x01: return "ROM + RAM";
    case 0x02: return "ROM + RAM + Battery";
    case 0x03: return "ROM + DSP";
    case 0x04: return "ROM + DSP + RAM";
    case 0x05: return "ROM + DSP + RAM + Battery";
    case 0x13: return "ROM + SuperFX";
    case 0x1A: return "ROM + SA-1";
    default:   return "Other";
    }
}

} // namespace snes