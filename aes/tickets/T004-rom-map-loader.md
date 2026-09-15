---
ticket: T004
title: ROM map loader (scenario terrain)
sprint: sprint-01
priority: high
status: in-progress
created: 2026-09-11
---

# T004 — ROM map loader (scenario terrain)

## Context
SimCity SNES stores its 120×100 scenario maps in a two-stage compressed format:
1. **Nintendo packet format** (control-byte + modes: copy, byte-repeat, word-repeat, incrementing, back-references) — decompresses ROM→$7E8000.
2. **16-bit RLE** with repeat count in bits 10–13 and tile ID in low 10 bits — expands to 12000 16-bit tile entries.

The scenario map pointer table at LoROM `$03:CE70` (split into low/hi/bank arrays) points to these packets.
Empirical scan confirms valid packets at `$0F:C184` and `$0F:C4B4` producing ~12000 tiles each.

Goal: load scenario 0 map at startup, decode both stages, map SNES tile IDs to our `sim::Terrain`, and seed `sim::City` terrain grid. Fallback to generated terrain if decode fails.

## Acceptance Criteria
- [AC1] Unit test for Nintendo decompressor with known vectors from ROM.
- [AC2] Unit test for 16-bit RLE stage producing exactly 12000 tiles.
- [AC3] Scenario table parser extracts at least 2 valid map pointers.
- [AC4] Game boots, decodes scenario 0, seeds terrain, renders without crash.
- [AC5] Headless smoke test passes: `SDL_VIDEODRIVER=dummy timeout 3 ./build/simcity "SimCity (USA).sfc"` exits 124 (timeout) with frames > 0.
- [AC6] All existing tests (T001–T006) still pass: `ctest` → 4/4 pass.

## Scope
**In scope:**
- `src/snes/decompress.{h,cpp}` — Nintendo packet decompressor.
- `src/snes/scenariomap.{h,cpp}` — 16-bit RLE decode + scenario table parser.
- `src/tools/rommap.cpp` — CLI tool for debugging/verification.
- `src/sim/city.{h,cpp}` — add `apply_terrain_map()` and resize grid to 120×100.
- `src/engine/game.cpp` — call loader at init, seed city before loop.

**Out of scope:**
- Zone/building population from scenario high bits (zones are 0 at start).
- Multi-scenario selection menu.
- RLE compressor (write path).

## Dependencies
- T001 (SnesRom + LoROM translation)
- T005 (sim::City with 120×100 grid)
- T006 (Game init path)

## Rollback
If decode fails or crashes: feature-flag the loader (`--no-rom-map`), fall back to `City()` default terrain.

## Known Risks
- Nintendo format variant may differ from documented bbbradsmith/lytron specs (mode 0xE0 extend vs. 0xC0/0xE0 backrefs).
- Scenario table at `$03:CE70` may point to intro screens not full maps.
- Overlong tile stream (>12000): truncate to 120×100.

## Notes
- Map dimensions must change from 128×64 to 120×100 (SNES canonical).
- Tile ID → Terrain mapping: 0=grass, 1-3=water, 4-13=shore/land, 14-25=forest, 26-27=park, 30-5F=road, 70-7F=rail, else grass.