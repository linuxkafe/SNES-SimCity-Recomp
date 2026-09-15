---
ticket: T007
phase: verify
date: 2026-09-11
---

# T007 — Verify Artifact

## Quality Gates

### Tests
- `make test`: 5/5 ctest suites pass (test_rom, test_tile, test_timestep, test_city, test_decompress)
- `test_city`: 13/13 individual tests pass (6 power-specific + 7 updated existing)
- Headless smoke test: `SDL_VIDEODRIVER=dummy timeout 3 ./build/simcity` exits cleanly (1262 frames, exit 0)

### Build
- `make build`: Clean build, zero warnings, zero errors

### Lint/Format
- No linter configured for this project (C++17/CMake). Manual review of code style: consistent with existing patterns.

## Acceptance Criteria Verification

| AC | Status | Evidence |
|----|--------|----------|
| Coal 3×3 / Nuclear 4×4 plant zones | ✅ | `test_power_plant_placement`: coal at (10,10) fills (10,10)-(12,12); nuclear at (20,20) fills (20,20)-(23,23); reject water/overlap |
| PowerLine terrain type | ✅ | `Terrain::PowerLine = 4` in city.h; `place_power_line()` sets terrain |
| step_month() computes power | ✅ | `compute_power()` called first in `step_month()`; clears + recomputes monthly |
| Power spreads through lines | ✅ | `test_power_line_conductivity`: 9 lines powered across full chain; gap breaks conductivity |
| Unpowered zones don't grow | ✅ | `test_unpowered_zone_no_jobs`: commercial=0/jobs=0 without power; `test_unpowered_residential_no_growth`: density=0 without power |
| Powered queryable per tile | ✅ | `bool powered(int x, int y) const` in public API |
| All tests pass + new tests | ✅ | 13/13 city tests, 5/5 ctest suites |
| Determinism preserved | ✅ | `test_power_determinism`: identical power state across different placement orders |

## Pre-existing Failures
None detected.

## Regression Check
- `test_budget_burn`: funds correctly burn to 0, never negative
- `test_determinism`: identical operation sequences produce identical state
- Headless smoke test: game boots, sim runs, clean exit
