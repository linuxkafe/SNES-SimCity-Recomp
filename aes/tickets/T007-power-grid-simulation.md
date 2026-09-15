---
ticket: T007
title: Power grid simulation
sprint: sprint-01
priority: high
status: done
created: 2026-09-11
---

# T007 — Power Grid Simulation

## Context
SimCity SNES requires power plants (coal/nuclear) and power lines to distribute electricity. Zones only develop when powered. Currently the simulation has no concept of power — all zones grow regardless of connectivity. This ticket implements the power grid: power plants as power sources, power lines as conductors, and a deterministic power flow algorithm that marks zones as powered/unpowered each month.

## Acceptance Criteria
- [ ] Power plant zones: Coal (3×3) and Nuclear (4×4) placed via tool
- [ ] Power line terrain type added (distinct from Road)
- [ ] `sim::City::step_month()` computes power coverage from all plants
- [ ] Power spreads through power lines (4-connected, max distance from plant)
- [ ] Unpowered zones do not grow (residential density stalls, commercial/industrial produce no jobs)
- [ ] Power coverage queryable per tile for UI overlay
- [ ] All existing tests pass + new tests for power logic
- [ ] Determinism preserved: identical operations → identical power state

## Scope
**In scope:**
- Add `Terrain::PowerLine` and `Zone::PowerPlant` (with sub-type coal/nuclear)
- Add power radius: coal 3×3 area of effect, nuclear 4×4 (per SNES)
- Power line conductivity: unlimited length, 4-connected
- Power state cached per tile, recomputed monthly
- UI hook: `bool powered(int x, int y) const` for overlay
- Extend `TileView` with `powered` field or separate query

**Out of scope:**
- Power plant construction cost UI (separate ticket)
- Disaster destruction of power plants (T008)
- Power plant pollution effects
- Save/load of power state (T012)

## Dependencies
- T006 (City map rendering + edit tools) — provides zone/terrain placement tools
- T004 (ROM map loader) — provides terrain base (done)

## Rollback
Revert `city.h`, `city.cpp`, `test_city.cpp` changes. No database migrations.

## Known Risks
- Power flow algorithm must be O(N) per month to not degrade simulation performance
- Must not break deterministic simulation guarantee
- Power line vs Road distinction in rendering (gfx/renderer.cpp)

## Notes
Reference: SimCity SNES power mechanics — coal plant powers 3×3 tiles centered on plant, nuclear 4×4. Power lines extend this radius. Unpowered zones: residential won't grow, commercial produces 0 jobs, industrial produces 0 jobs. The "powered" state is a binary per-tile flag recomputed each month from all plants + lines.