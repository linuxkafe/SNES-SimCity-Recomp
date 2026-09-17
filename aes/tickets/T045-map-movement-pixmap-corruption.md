# T045 — Map movement causes pixmap corruption and freeze

Status: **DONE** — fixed via HDMA ROM bounds check
Resolved: 2026-09-17

## Root Cause

Two separate issues causing map movement corruption:

1. **NMI delivery race (fixed in T044)**: VBlank wait loop at `$9311` ran in AOT while NMI handler at `$80B2` (INC `$00B9`) ran in LLE — memory coherence issue hung game at frame 9-10.

2. **HDMA table pointer OOB reads**: During map scrolling, game sets up HDMA tables with source addresses in banks `$20`, `$30`, `$58`, `$59`, `$5F`, `$79`, `$92`, `$C2`, `$C3`, `$C4`, `$F4`, `$AA`, `$FF` — all beyond the 512KB ROM size. `SimpleHdma_GetPtr` called `RomPtr` which returned `&g_rom[0]` for invalid addresses, corrupting VRAM/CGRAM with garbage data.

## Fixes Applied

| Commit | File | Change |
|--------|------|--------|
| `44ed697` | `recomp/bank00.cfg` | `force_lle 0x009311` — VBlank wait loop to interpreter |
| `b4581ff` (submodule) | `runner/src/snes/cart.c` | ROM size bounds check in `cart_getRomPtr` for all LoROM types |
| `b4581ff` (submodule) | `runner/src/common_rtl.c` | ROM size bounds check in `SimpleHdma_GetPtr` for HDMA table pointers |

## Verification

- ✅ `make test` — deterministic replay passes (2.46s)
- ✅ Headless 2000 frames with `SNESRECOMP_MOUSE=1` — clean exit, no off-rails
- ✅ Headless 500 frames with `PAD_PROBE=1` — clean exit
- ✅ No RomPtr-invalid hits during map movement

## Mouse Status

SNES Mouse (Player 2) enabled via `SNESRECOMP_MOUSE=1` — bsnes-exact 32-bit serial protocol implemented in `runner/src/snes/joypad.c`, per-frame host motion feed in `host_main.c`. Game may only use mouse in specific modes (menus); no read activity detected in gameplay mode via padprobe, but device is present and functional.

## Acceptance Criteria Met

- Map movement works without graphical corruption for 2000+ frames
- No off-rails RomPtr-invalid hits during map movement  
- SNES Mouse available on port 2
- All existing tests pass
