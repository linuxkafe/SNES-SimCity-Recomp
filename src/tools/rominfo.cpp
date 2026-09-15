#include <cstdio>
#include <cstring>
#include <string>

#include "snes/rom.h"

static std::string country_name(uint8_t c) {
    switch (c) {
    case 0x00: return "Japan";
    case 0x01: return "USA";
    case 0x02: return "Europe";
    case 0x04: return "France";
    case 0x05: return "Germany";
    case 0x06: return "Italy";
    case 0x07: return "Spain";
    case 0x08: return "Korea";
    default:   return "Other";
    }
}

int main(int argc, char** argv) {
    if (argc < 2) {
        fprintf(stderr, "usage: rominfo <rom.sfc>\n");
        return 1;
    }
    snes::SnesRom rom(argv[1]);
    if (!rom.load()) {
        fprintf(stderr, "error: failed to load ROM '%s'\n", argv[1]);
        return 1;
    }
    const snes::SnesHeader& h = rom.header();
    size_t rom_kb = rom.size() / 1024;

    printf("=== SNES ROM Info ===\n");
    printf("File:            %s\n", rom.path().c_str());
    printf("Size:            %zu KB (%zu bytes)\n", rom_kb, rom.size());
    printf("Title:           %s\n", h.title.c_str());
    printf("Map mode:        %s\n", snes::map_mode_name(h.map_mode).c_str());
    printf("Cart type:       %s (0x%02X)\n", snes::cart_type_name(h.cart_type).c_str(), h.cart_type);
    printf("ROM size:        %u KB\n", h.rom_size_kb_pow2 > 0 ? (1u << h.rom_size_kb_pow2) : 0u);
    printf("SRAM size:       %u KB\n", h.sram_size_kb_pow2 > 0 ? (1u << h.sram_size_kb_pow2) : 0u);
    printf("Country:         %s (0x%02X)\n", country_name(h.country_code).c_str(), h.country_code);
    printf("License:         0x%02X\n", h.license_code);
    printf("Version:         %u\n", h.version);
    printf("Checksum comp:   0x%04X\n", h.checksum_complement);
    printf("Checksum:        0x%04X\n", h.checksum);
    printf("Checksum valid:  %s\n", rom.checksum_valid() ? "YES" : "NO");
    printf("Reset vector:    0x%06X\n", h.native_reset_vector);
    printf("NMI vector:      0x%06X\n", h.native_nmi_vector);
    printf("IRQ vector:      0x%06X\n", h.native_irq_vector);

    return 0;
}