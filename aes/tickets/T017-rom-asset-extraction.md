---
ticket: T017
title: ROM asset extraction & game integration
sprint: sprint-02
priority: high
status: done
date: 2026-09-11
created: 2026-09-11
---

# T017 — ROM Asset Extraction & Game Integration

## Context
The `simcity` build currently loads **no assets** from the ROM: the map is rendered
with hardcoded flat colours (`cell_color()` in `gfx/cityview.cpp`), scenario terrain
always fails to load (`Warning: failed to load scenario terrain, using generated`),
and `gfx/tile.cpp`'s decoder is only used by the experimental `tileview` tool.

Root-cause analysis (Phase 0) showed two concrete bugs in the ROM access layer:

1. **`nintendo_decompress()`** in `src/snes/decompress.cpp`:
   - Applies `length += 1` only in the extended-length branch (`mode == 0xE0`);
     in the LC_LZ5 format the low 5 bits are `length − 1`, so **every** mode needs +1.
   - Reads all input through `rom.read8()`/`read16()`, which treat the offset as a
     **CPU address** and re-translate it. The decompressors receive **file offsets**,
     so every byte is read from the wrong location (double translation).

2. **`parse_scenario_table()`** in `src/snes/scenariomap.cpp`:
   - Reads the split lo/hi/bank pointer table at `$03:CE70` through `rom.read8()`
     with file offsets — same double-translation bug → entry addresses are garbage.

Verified against two independent authoritative sources:
- Yoshifanatic1/SimCity-SNES-Disassembly `AssetPointersAndFiles.asm` — canonical asset
  ranges for the USA ROM (MD5 `23715fc7ef700b3999384d5be20f4db5` confirmed).
- bbbradsmith's LC_LZ5 decoder + lytron's map notes.

With raw byte reads and correct length semantics, **all 9 scenario maps decode
correctly** (≈10,000–12,000 tiles each) and packet-end offsets chain exactly against
the disassembly map table (`0x060F27 → 0x0628E8 → … → 0x06D77C`).

## Acceptance Criteria
- [x] `nintendo_decompress` decodes real ROM packets (scenario maps, Layer1 tiles)
      to byte sizes registered by the disassembly (9/9 maps, 32768 B tile bank).
- [x] `parse_scenario_table` returns the 9 canonical scenario addresses (Tokyo,
      Boston, Detroit, Bern, Rio, SanFran, LasVegas, FreeCity, Practice).
- [x] `load_scenario_terrain(rom, 0)` returns true and produces a 120×100 map with
      geographically-sane water/land distribution.
- [x] `simcity` game loads scenario 0 terrain from ROM and the map is seeded from it
      (HEADLESS smoke: `Loaded scenario terrain from ROM` on stderr).
- [x] CityView renders terrain/zone cells using ROM-extracted 4bpp tiles + BGR555
      palette (Layer1 city tiles at `$07E584–$08C4DB`, palette `$058000`-style block),
      replacing the flat-colour fallback for at least terrain tiles.
- [x] `rommap` CLI reports the real scenario table (no more `$71FF66`-style garbage).
- [x] All ctest suites pass (`make test`), including new regression tests for the
      length+1 semantics, file-offset reads, and ROM asset extraction.

## Scope
**In scope:** decompression correctness, scenario terrain loading, ROM tile/palette
extraction and rendering integration in CityView, `rommap` fix, regression tests.

**Out of scope:** audio/SPC (T010), UI screens/tilemaps beyond the city view, sprite
animation, full scenario system (T011). The disassembly points are recorded here so
those tickets can consume them later.

## Dependencies
- `SimCity (USA).sfc` present in repo root (test fixture).
- Yoshifanatic1/SimCity-SNES-Disassembly (read-only reference, cloned to `/tmp/simcdis`).

## Rollback
Revert the commit(s); the flat-colour renderer remains as a compile-time/behavioural
fallback if extraction fails (no regressions to existing tests).

## Known Risks
- Palette block `$058000` (BG palettes, 14 × 0x100) is raw BGR555; which of the 7 CGRAM
  palette sets the city view uses must be confirmed by rendering, not assumed.
- `$07E584` tile packets are LC_LZ5 compressed `.lz5` — need the fixed decompressor.
- Changed `nintendo_decompress` offset semantics (file offset vs CPU address) is a
  public-contract fix; all callers must be audited (rommap, scenariomap, tests).

## Notes
- bbbradsmith US pointers: `TILEDATA = 0x04C0FB`, `TILESCEN = 0x04875C`, maps at
  `$0C8F27…$0DD77C` (LoROM bank→file: `bank*0x8000 + (addr & 0x7FFF)`).