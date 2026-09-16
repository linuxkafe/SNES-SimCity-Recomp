/*
 * SimCity SNES Recomp — generated stubs / HLE hooks / debug stubs.
 *
 * This file provides C implementations for functions that the recompiler
 * replaces with HLE (high-level emulation) stubs. Common uses:
 * - SPC upload protocol (hle_spc_upload in bank cfg)
 * - Custom dispatchers (hle_dispatch in bank cfg)
 * - Hand-optimized replacements for specific functions (hle_func in bank cfg)
 *
 * Also provides no-op implementations for debug hooks that are declared
 * as extern in the runtime but only defined when SNESRECOMP_TRACE=1.
 *
 * Mod globals: variables for mod features that can be accessed by
 * the host's after_config callback and frame callbacks.
 */

#include "common_cpu_infra.h"
#include "cpu_state.h"
#include "debug_server.h"

/* Debug hooks — no-op stubs for production builds (SNESRECOMP_TRACE=0).
 * The debug_server.h header provides static-inline no-op stubs when
 * SNESRECOMP_TRACE=0, but the runtime also calls extern functions
 * that need definitions. These provide those definitions. */

void debug_on_wram_write_byte(uint32_t addr, uint8_t old_val, uint8_t new_val)
{
    (void)addr; (void)old_val; (void)new_val;
}

void debug_on_wram_write_word(uint32_t addr, uint16_t old_val, uint16_t new_val)
{
    (void)addr; (void)old_val; (void)new_val;
}

void debug_on_block_enter(uint32_t pc, uint32_t a, uint32_t x, uint32_t y)
{
    (void)pc; (void)a; (void)x; (void)y;
}

/* ========================================================================
 * SimCity Mod Globals
 *
 * These variables store mod feature state and are accessed by:
 * - Host's after_config callback (reads env vars)
 * - Frame callback in host (applies GodMode/Disaster effects)
 * ======================================================================== */

int g_mod_widescreen_enabled = 0;
int g_mod_godmode_enabled = 0;
int g_mod_disaster_toggle = 0;  /* 0=normal, 1=disable all, 2=force random */

/* ========================================================================
 * Mod Frame Callback
 *
 * Called every frame from host's presenter for persistent mods.
 * This is called from SimCityBeginSimFrame in main.c.
 * ======================================================================== */

void SimCity_ModFrameCallback(void)
{
    if (!g_mod_godmode_enabled)
        return;

    extern uint8_t g_ram[];
    /* Keep money at max (999,999 = 0x0F423F, but game uses 24-bit at $7E:04B7) */
    g_ram[0x04B7] = 0x3F;  /* Low byte */
    g_ram[0x04B8] = 0x42;  /* Mid byte */
    g_ram[0x04B9] = 0x0F;  /* High byte */

    /* Keep instant build flag set */
    g_ram[0x04B6] = 1;

    /* Force disaster trigger if enabled */
    if (g_mod_disaster_toggle == 2) {
        g_ram[0x04C0] = 0xFF;
        g_ram[0x04C1] = 0;
    }
}

/* Placeholder to keep the translation unit non-empty. */
void GenStubs_Dummy(void) {}