#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <string>
#include <vector>

#include "snes/rom.h"

static int failures = 0;

#define CHECK(cond)                                                         \
    do {                                                                    \
        if (!(cond)) {                                                      \
            printf("FAIL: %s (line %d)\n", #cond, __LINE__);                \
            ++failures;                                                     \
        }                                                                   \
    } while (0)

static std::string simcity_path() {
    // Tests run from build/ directory. Look in parent dirs for the ROM.
    const char* candidates[] = {
        "../SimCity (USA).sfc",
        "SimCity (USA).sfc",
        "../../SimCity (USA).sfc",
    };
    for (const char* c : candidates) {
        std::string p = c;
        std::ifstream f(p, std::ios::binary);
        if (f.good()) return p;
    }
    return "";
}

static void test_synthetic_lorom() {
    printf("-- test_synthetic_lorom\n");
    // Build a synthetic 32KB LoROM ROM with header at 0x7FC0.
    std::vector<uint8_t> rom(0x8000, 0xFF);
    // 21-byte title field (9 chars + 12 pad)
    char title[21];
    memset(title, ' ', 21);
    memcpy(title, "SYNTHTEST", 9);
    memcpy(&rom[0x7FC0], title, 21);
    // Header fields at $FFC0+21 = $FFD5 (LoROM 32KB bank 0 → file 0x7FD5)
    rom[0x7FD5] = 0x20;  // LoROM map mode
    rom[0x7FD6] = 0x02;  // ROM+RAM+Battery
    rom[0x7FD7] = 0x09;  // ROM size 512KB declared
    rom[0x7FD8] = 0x05;  // SRAM 32KB
    rom[0x7FD9] = 0x01;  // USA

    // Patch reset vector at $00:FFFC → file offset 0x7FFC
    rom[0x7FFC] = 0x00;
    rom[0x7FFD] = 0x80;

    // Write to temp file and load.
    const char* tmp = "/home/seyon/tmp_test/test_synthetic.sfc";
    {
        std::ofstream f(tmp, std::ios::binary);
        f.write(reinterpret_cast<const char*>(rom.data()), rom.size());
    }
    snes::SnesRom r(tmp);
    CHECK(r.load());
    if (!r.is_loaded()) return;

    CHECK(r.header().title == std::string("SYNTHTEST"));
    CHECK(r.header().map_mode == snes::MapMode::LoROM);
    CHECK(r.header().cart_type == 0x02);
    CHECK(r.size() == 0x8000);

    // Address translation: $00:8000 → 0, $00:FFFF → 0x7FFF
    size_t off = 0;
    CHECK(r.translate(0x8000, off) && off == 0x0000);
    CHECK(r.translate(0xFFFF, off) && off == 0x7FFF);
    CHECK(r.translate(0x088000, off) && off == 0x0000);  // bank 0x08 → 0
    CHECK(!r.translate(0x007FFF, off));                   // < 0x8000 = RAM
CHECK(r.read8(0x00FFD5) == 0x20);  // map-mode byte via CPU address
    CHECK(r.read16(0x00FFFC) == 0x8000);                  // reset vector
}

static void test_simcity_real_rom() {
    printf("-- test_simcity_real_rom\n");
    std::string path = simcity_path();
    if (path.empty()) {
        printf("SKIP: SimCity ROM not found\n");
        return;
    }
    snes::SnesRom r(path);
    CHECK(r.load());
    if (!r.is_loaded()) return;

    printf("  title: '%s' size=%zu\n", r.header().title.c_str(), r.size());

    CHECK(r.header().title == std::string("SIMCITY"));
    CHECK(r.header().map_mode == snes::MapMode::LoROM);
    CHECK(r.size() == 512 * 1024);

    // First bytes should be 65816 native-mode code ($18 $FB $78 ... = CLC XCE SEI)
    CHECK(r.read8(0x008000) == 0x18);
    CHECK(r.read8(0x008001) == 0xFB);
    CHECK(r.read8(0x008002) == 0x78);

    // Reset vector points into code, header map byte is 0x20.
    CHECK(r.read8(0x00FFD4) == 0x20);

    // Header fields on real ROM
    CHECK(r.header().rom_size_kb_pow2 == 0x09);   // 512KB
    CHECK(r.header().sram_size_kb_pow2 == 0x05);  // 32KB
    CHECK(r.header().country_code == 0x01);       // USA
    CHECK(r.header().cart_type == 0x02);          // ROM+RAM+Battery
}

int main() {
    printf("ROM test suite\n");
    test_synthetic_lorom();
    test_simcity_real_rom();

    if (failures == 0) {
        printf("ALL TESTS PASSED\n");
        return 0;
    }
    printf("%d test(s) FAILED\n", failures);
    return 1;
}