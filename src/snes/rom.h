#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace snes {

enum class MapMode {
    LoROM,
    HiROM,
    Unknown
};

struct SnesHeader {
    std::string title;
    MapMode     map_mode;
    uint8_t     cart_type;
    uint8_t     rom_size_kb_pow2;
    uint8_t     sram_size_kb_pow2;
    uint8_t     country_code;
    uint8_t     license_code;
    uint8_t     version;
    uint16_t    checksum_complement;
    uint16_t    checksum;
    uint32_t    native_reset_vector;
    uint32_t    native_irq_vector;
    uint32_t    native_nmi_vector;
    uint32_t    native_emulation_vector;
};

class SnesRom {
public:
    explicit SnesRom(const std::string& path);

    bool load();
    bool is_loaded() const { return !data_.empty(); }
    size_t size() const { return data_.size(); }
    const std::string& path() const { return path_; }
    const std::vector<uint8_t>& data() const { return data_; }
    const SnesHeader& header() const { return header_; }
    bool checksum_valid() const;
    void parse_header();

    bool translate(uint32_t cpu_addr, size_t& file_off) const;
    uint8_t read8(uint32_t cpu_addr) const;
    uint16_t read16(uint32_t cpu_addr) const;
    uint32_t read32(uint32_t cpu_addr) const;

    static uint32_t bank_of(uint32_t cpu_addr) { return (cpu_addr >> 16) & 0xFF; }
    static uint16_t addr_in_bank(uint32_t cpu_addr) { return static_cast<uint16_t>(cpu_addr & 0xFFFF); }

private:
    bool fill_header_at(size_t off);

    std::string path_;
    std::vector<uint8_t> data_;
    SnesHeader header_{};
    bool parsed_ = false;
};

std::string map_mode_name(MapMode m);
std::string cart_type_name(uint8_t t);

} // namespace snes