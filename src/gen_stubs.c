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
 * For now, all stubs are empty — the recompiler will emit LLE (low-level
 * emulation) fallbacks for everything. Add HLE hooks here as needed
 * during bring-up.
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

/* Example HLE stub for SPC upload (if bank cfg declares hle_spc_upload):
 *
 * int RtlUploadSpcImageFromDp(CpuState *cpu) {
 *     // Read DP+0..2 for 24-bit ROM pointer to block stream
 *     // Walk stream: length / target / data blocks
 *     // Write directly to apu->ram
 *     // Jump apu->spc->pc to terminator target
 *     return 0;
 * }
 */

/* Example HLE stub for custom dispatcher (if bank cfg declares hle_dispatch):
 *
 * void MmxSchedulerTick(CpuState *cpu) {
 *     // Host-side task scheduler selects next PC
 *     // Tail-call into selected task body
 * }
 */

/* Example HLE function replacement (if bank cfg declares hle_func):
 *
 * void MyOptimizedFunction_M1X1(CpuState *cpu) {
 *     // Hand-written C replacement for a specific (m,x) variant
 * }
 */

/* Placeholder to keep the translation unit non-empty. */
void GenStubs_Dummy(void) {}