/*
 * SimCity SNES Recomp — host contract stubs.
 *
 * The framework's desktop host expects certain C functions to exist for
 * launcher integration, config, save states, etc. This file provides
 * minimal stubs; the framework's common_rtl.c supplies the real
 * implementations for most of them.
 */

#include "common_cpu_infra.h"
#include "snesrecomp_rom_identity.h"
#include "snes/snes.h"

extern Snes *g_snes;

/* ROM identity — populated by snesrecomp_rom_identity() at CMake time. */
const char *SnesRomExpectedSha256(void) { return SNESRECOMP_ROM_EXPECTED_SHA256; }
const char *SnesRomExpectedCrc32(void)  { return SNESRECOMP_ROM_EXPECTED_CRC32; }

/* Optional: per-game config overrides. Return NULL to use framework defaults. */
const char *SnesConfigIniPath(void) { return NULL; }

/* Optional: per-game save state validation. Return 0 to accept any state. */
int SnesSaveStateValidate(const void *data, size_t size)
{
    (void)data; (void)size;
    return 1;
}

/* Optional: per-game netplay sync check. Return 0 to use framework default. */
int SnesNetplaySyncCheck(const void *state_a, const void *state_b, size_t size)
{
    (void)state_a; (void)state_b; (void)size;
    return 0;
}

/* Optional: per-game input remapping. Return 0 to use framework default. */
int SnesInputRemap(int player, int btn, int *out_btn)
{
    (void)player; (void)btn; (void)out_btn;
    return 0;
}

/* Optional: per-game video mode override. Return 0 for framework default. */
int SnesVideoModeOverride(int *out_width, int *out_height, int *out_refresh_num, int *out_refresh_den)
{
    (void)out_width; (void)out_height; (void)out_refresh_num; (void)out_refresh_den;
    return 0;
}

/* Optional: per-game audio sample rate override. Return 0 for default (32000). */
int SnesAudioSampleRateOverride(void) { return 0; }