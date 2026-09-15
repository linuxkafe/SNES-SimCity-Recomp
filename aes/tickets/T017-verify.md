---
ticket: T017
phase: verify
date: 2026-09-11
---

# T017 — Verify Artifact

## Quality Gates

### Tests
- `make test`: **5/5 ctest suites pass** (test_rom, test_tile, test_timestep,
  test_city, test_decompress) — 100% pass.
- `test_decompress`: new real-ROM regressions pass — `test_scenario_table_parse`
  (9 canonical entries in range), `test_scenario_maps_decode` (all 9 decode exactly
  and chain), `test_city_tiles_decode` (32768 B), `test_rle16_caps_tiles`,
  `test_load_scenario_terrain` (checks ok, 8431 varied tiles).
- `test_tile`: new `test_rom_assets` real-ROM pass — 1024 tiles (32768 B) extracted,
  14x8 palette entries present, tiles 0/1 decode with palette indices ≤15.
- Headless smoke: `SDL_VIDEODRIVER=dummy timeout 3 ./build/simcity "SimCity (USA).sfc"`
  → stderr `Loaded scenario terrain from ROM`, `Loaded ROM asset tiles+palettes`,
  `loaded: SIMCITY (LoROM, ROM + RAM + Battery, 512 KB)`, runs 1200+ frames, exits
  via timeout (exit 124 = expected kill, no crash before it).

### Build
- `make build`: clean, zero warnings, zero errors.

### rommap canonical output
- `./build/rommap "SimCity (USA).sfc" 0` reports the 9 canonical file offsets:
  0x060F27 (Tokyo), 0x0628E8 (Boston), 0x0645A2 (Detroit), 0x06630B (Bern),
  0x06816E (Rio), 0x069F23 (SanFran), 0x06B987 (LasVegas), 0x06CB15 (FreeCity),
  0x06D131 (Practice). No `$71FF66`-style garbage.

### Lint/Format
- No linter configured (C++17/CMake). Manual style review: consistent with existing
  patterns; `rom_assets.h`/`.cpp` follow `tile.h` naming conventions.

## Acceptance Criteria Verification

| AC | Status | Evidence |
|----|--------|----------|
| Decompressor decodes real ROM packets to registered sizes | ✅ | 9/9 scenario maps + 32768 B city tile bank |
| `parse_scenario_table` returns 9 canonical addresses | ✅ | rommap [0..8] all match disassembly |
| `load_scenario_terrain(0)` returns true, sane map | ✅ | 8431 varied tiles, 519 water in top border; smoke prints marker |
| Game seeds from ROM terrain | ✅ | smoke stderr shows `Loaded scenario terrain from ROM` |
| CityView renders ROM tiles + palette, flat fallback kept | ✅ | `test_rom_assets` decode; `game.cpp` passes `&rom_assets_` |
| rommap reports real table | ✅ | canonical offsets verified |
| All ctest suites pass + new regressions | ✅ | 5/5, incl. new real-ROM tests |

## Pre-existing Failures
- None. LSP errors in editor are false positives (no `compile_commands.json`
  generated; `make build` is the source of truth).

## Known Risk (documented, not blocking)
- Palette rendering default (block 4, sub 0) chosen by palette inspection, not
  human visual render. Tunable via `RomAssets::palette_block`/`palette_sub`.
  Flagged in review for a future visual smell-test when a real window render is
  inspected.