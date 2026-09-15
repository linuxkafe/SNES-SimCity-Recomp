---
ticket: T007
phase: build
date: 2026-09-11
---

# T007 — Build Artifact (Diffstory)

## What Changed

### `src/sim/city.h`
- Added `Terrain::PowerLine = 4` enum value
- Added `Zone::PowerPlant = 4` enum value
- Added `enum class PowerPlantType { None, Coal, Nuclear }`
- Added `bool powered` and `PowerPlantType plant_type` to `Tile` struct
- Added public API: `powered()`, `place_power_plant()`, `remove_power_plant()`, `place_power_line()`, `remove_power_line()`
- Added private: `compute_power()`, `power_radius()`, `is_power_line()`, `flood_fill_power()`

### `src/sim/city.cpp`
- Added upkeep constants (kUpkeepCoalPlant=50, kUpkeepNuclearPlant=100, kCountyUpkeep=500, etc.)
- `step_month()`: calls `compute_power()` first; filters all zone/growth/budget logic by `powered` flag; counts power_plant/power_line tiles for upkeep
- `stats()`: filters zones by `powered` flag (power lines no longer counted as road tiles)
- New methods: `place_power_plant()` (footprint validation + zone placement), `remove_power_plant()`, `place_power_line()`, `remove_power_line()`, `powered()`
- `compute_power()`: clears all powered flags, collects plant centers (top-left dedup), calls `flood_fill_power()` per plant
- `flood_fill_power()`: 3-step algorithm:
  1. Power all tiles within plant radius (Chebyshev/square distance) + queue connected power lines
  2. Flood fill through power lines (indefinite 4-connected BFS)
  3. Power all tiles adjacent to powered power lines (zones, roads)

### `tests/test_city.cpp`
- 6 new power tests: `test_power_plant_placement`, `test_power_line_conductivity`, `test_power_coverage_radius`, `test_unpowered_zone_no_jobs`, `test_unpowered_residential_no_growth`, `test_power_determinism`
- 7 existing tests updated with correct power layouts (zones adjacent to powered power lines)

## Why These Files
- `city.h`/`city.cpp`: Core simulation — power is a simulation concern
- `test_city.cpp`: Validation — all power semantics verified

## What Was NOT Touched
- `gfx/` — rendering (T008+ will add power overlay)
- `snes/rom.*` — ROM parsing (unrelated)
- `docs/` — documentation (not in scope)

## Remaining Risks
- `flood_fill_power()` uses static arrays (12000 queue, 100×120 visited) — safe for 120×100 map but not heap-allocated
- No power line rendering yet — users won't see power state visually
- Upkeep uses `power_plant_tiles * kUpkeepCoalPlant` for ALL plant types (simplified)
