# T005 — City simulation core

Status: pending
Priority: high
Depends on: T001, T003

## Goal
Implement the simulation engine of SimCity SNES as a graphics-free, testable
library (`src/sim/`): a tile grid, terrain/zone model, population growth,
RCI demand and a monthly budget loop. This is the heart of the game and must
be deterministic so unit tests are meaningful.

## Scope
- `sim::Terrain`, `sim::Zone` enums.
- `sim::City` with a 128x64 tile grid, terrain/zone accessors and
  per-tile residential population cells.
- Monthly `City::step_month()`:
  - aggregate residents, jobs (commercial/industrial), roads
  - compute RCI demand from population vs jobs/capacity
  - grow/decay residential density (road adjacency boosts growth)
  - collect taxes and subtract upkeep, updating `funds`
- `City::stats()` exposing population, demand, funds for the UI.

## Acceptance criteria
- [AC1] Unit tests cover: zone/terrain placement, residential growth over
         repeated months, budget income/upkeep, funds cannot go below zero.
- [AC2] Simulation is fully deterministic: two `City` instances fed the same
         sequence of operations produce identical `stats()`.
- [AC3] `City` has no dependency on SDL or ROM data (pure game logic).
- [AC4] `ctest` passes 4/4 with new `test_city`.

## Design notes
- Budget constants placed in `city.cpp` (kept in one place).
- Road adjacency is the only positional growth factor in T005; power and
  services are separate tickets.
- Zone density modelled per tile as occupancy 0..kMaxDensity.