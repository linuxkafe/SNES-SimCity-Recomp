---
ticket: T017
phase: build
date: 2026-09-11
---

# T017 — Build Artifact (Diffstory)

## What Changed

### `src/snes/decompress.cpp`
- Root-cause fixes for the two-bug decode failure:
  1. **length semantics**: `length += 1` now applies to ALL modes (previous code only
     added +1 for the `0xE0` extended-length mode). 10-bit extended lengths no longer
     truncate: local `length` widened from `uint8_t` to `unsigned`, and loop counters
     that copied it were widened to `unsigned` too.
  2. **back-reference guard removed**: back-references are LZ77-style *overlapping*
     copies — the `ref + length > out.size()` guard was wrong and rejected valid refs;
     copy is now byte-at-a-time from the growing output buffer.
- All reads now go through `read_byte_raw`/`read_u16_raw` on `rom.data()` (file
  offsets). The previous `read8()/read16()` paths re-translated file offsets as CPU
  addresses (double translation) and were removed from the decompressor.

### `src/snes/scenariomap.cpp`
- `parse_scenario_table` now reads lo/hi/bank bytes directly from `rom.data()[base+i]`
  (table at $03:CE70, 9 entries), returning canonical ROM addresses + file offsets.
- Removed the empirical fallback (>0x1000000) which was a false positive (matched
  ASCII text); header doc updated to note the canonical table is the real one.

### `src/gfx/rom_assets.{h,cpp}` (new)
- `RomAssets`: `tiles` (1024 x 32B 4bpp), `palettes` (14 blocks x 8 sub-palettes x
  16 colors from `$058000`), `palette_block`/`palette_sub` (default block 4 sub 0).
- `load_rom_assets()`: decompresses Layer1 city tiles (file 0x3E584, LC_LZ5) and
  copies the 14 BG palette blocks.

### `src/gfx/cityview.{h,cpp}`
- Now takes optional `const RomAssets*`; terrain cells rendered from ROM tiles (2x
  upscale 8x8→16px, CPU blit into locked streaming texture), zone cells keep flat
  tint overlay, flat-colour fallback preserved when no assets available.
- terrain→tile-id mapping: Grass=0, Water=1, Tree=20, Road=48, PowerLine=48
  (validated against real map tile distribution).

### `src/engine/game.{h,cpp}`
- Loads ROM, seeds scenario terrain, calls `gfx::load_rom_assets`, passes `&rom_assets_`
  to CityView when valid.

### `CMakeLists.txt`
- `simcity_gfx` now includes `src/gfx/rom_assets.cpp`.

### `tests/test_tile.cpp`
- New `test_rom_assets`: real-ROM regression — 1024 tiles extract, 14x8 palette
  entries present, tiles 0/1 decode to palette indices ≤15.

### `tests/test_decompress.cpp`
- New real-ROM regressions: `test_scenario_table_parse` (9 canonical entries),
  `test_scenario_maps_decode` (all 9 decode + chain), `test_city_tiles_decode`
  (32768 B), `test_rle16_caps_tiles`; strengthened `test_load_scenario_terrain`.
- Removed non-compiling `test_lz5_short_length_plus_one` (SnesRom has no default ctor).

### `docs/REQUIREMENTS.md`, `docs/ROADMAP.md`, `aes/kanban.md`
- FR-6 marked complete + new FR-8 (ROM asset rendering); roadmap updated; kanban
  learning history corrected (T004 note superseded).

## What Was NOT Touched
- `sim/` (power/simulation) — untouched.
- `gfx/tile.h` primitive tile decode / `Palette16` API — reused as-is.
- SDL render target / atlas texture path — abandoned (streaming textures cannot be
  render targets); CPU-side blit chosen instead.

## Remaining Risks
- Palette block/sub selection is a smell-test: default (block 4, sub 0) needs a
  human visual render to confirm; tunable via `RomAssets` fields.
- Zone cells still render flat-colour (tile overlay for zones not yet mapped to
  building sprites) — out of scope, tracked for future render polish.