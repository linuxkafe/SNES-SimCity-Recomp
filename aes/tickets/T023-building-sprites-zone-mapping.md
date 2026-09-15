---
ticket: T023
title: Building Sprites — Map 1024-tile bank to zone cells
sprint: sprint-02
priority: high
status: done
created: 2026-09-14
source: T020 finding F4
---

# T023 — Building Sprites (Zone Mapping)

## Context
T020 audit confirmed F4: zone cells use invented flat colors (`cityview.cpp:39-45` `cell_color()`). The 1024-tile bank contains building sprites for residential/commercial/industrial zones. This ticket maps building tiles to zone cells based on density and type.

## Research Summary
- City tile bank: 1024 tiles at $07E584 (already loaded in RomAssets)
- Tiles are 8x8 4bpp. Buildings in SimCity SNES are multi-tile (typically 2x2 or 3x3 tiles = 16x16 or 24x24 px)
- Zone rendering in CityView scales 1 terrain cell = 16px. Building sprites need to fit this.
- Need to identify tile ID ranges for:
  - Residential: low/med/high density
  - Commercial: low/med/high density
  - Industrial: low/med/high density
  - Power plants, ports, stadiums, landmarks
- SNES SimCity uses tile animation for some buildings (blinking lights, etc.)

## Acceptance Criteria
- [ ] Identify building tile ranges for R/C/I zones in 1024-tile bank
- [ ] Map tile IDs to zone type + density level
- [ ] Update CityView to render zone cells with building sprites
- [ ] Keep flat color fallback for zones without mapped sprites
- [ ] Verify visual coherence (buildings look like buildings, not random tiles)
- [ ] 8/8 ctest + headless smoke pass

## Plan (AES Phase 1 → 2)
1. **Scan city tiles** for building-like patterns (multi-tile structures with windows, roofs)
2. **Cross-reference** with disassembly / scenario maps to identify zone-building correlations
3. **Create BuildingSpriteData** struct in `src/gfx/building_sprites.h` with mapping tables
4. **Update CityView** `blit_tile_cell` to use building sprites for zone cells
5. **Tests**: add `test_building_sprites.cpp` headless verification
6. **Cleanup**: remove `cell_color()` fallback from CityView

## Known Risks
- Building sprites may span multiple tiles (2x2, 3x3)
- Density levels may have different sprite sets
- Power plant tiles need separate mapping
- Animation frames may be needed for some buildings

## Dependencies
- ROM "SimCity (USA).sfc"
- Existing tile extraction pipeline (T017/T020)

## Phase 2 Spec (Build)
### Files to create
- `src/gfx/building_sprites.h` / `.cpp` — BuildingSpriteData + extraction
- `tests/test_building_sprites.cpp` — headless test
### Files to modify
- `CMakeLists.txt` — add building_sprites.cpp, test
- `src/gfx/cityview.cpp` — replace `cell_color()` with sprite blitting
- `src/gfx/cityview.h` — add building sprite data member