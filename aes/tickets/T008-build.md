---
ticket: T008
phase: build
status: done
created: 2026-09-12
requires:
  - aes/tickets/T008-plan.md
produces:
  - aes/tickets/T008-build.md
blocked_by: ''
---

# T008 — Build Output (Diffstory)

## What Changed

### 1. `src/sim/city.h` (+17 lines)
- Added `Terrain::Crater = 5` to terrain enum
- Added `DisasterType { None, Meteor, Monster }` enum
- Added `DisasterState` struct with type, active flag, timer, position, radius
- Added public API: `disaster_state()`, `trigger_disaster(DisasterType)`
- Added private methods: `process_disasters()`, `trigger_meteor()`, `trigger_monster()`, `update_monster()`, `next_random()`
- Added private member: `DisasterState disaster_`, `uint32_t rng_state_`

### 2. `src/sim/city.cpp` (+110 lines)
- Constructor initializes `rng_state_ = 0xACE1u`
- `step_month()` now calls `process_disasters()` at start of month
- `next_random()`: LCG (state = state * 1664525 + 1013904223) for deterministic RNG
- `trigger_disaster()`: dispatches to meteor or monster
- `process_disasters()`: monthly roll (2% base + 0.5% per 100 pop, capped at 15%), triggers random disaster; updates active disasters
- `trigger_meteor()`: picks random non-water coordinate, destroys circular radius 6 (sets Crater terrain, clears zones/power plants), sets disaster state
- `trigger_monster()`: spawns at random map edge, targets map center (60,50), sets disaster state
- `update_monster()`: steps 1 tile toward target per month, destroys 3x3 path (Crater terrain), clears when reaches center

### 3. `src/gfx/cityview.cpp` (+3 lines)
- `cell_color()`: added `Terrain::Crater` → dark gray {40, 40, 40}
- `tile_id_for_terrain()`: added `Terrain::Crater` → tile ID 1 (water tile, dark appearance)
- `render()`: added optional `shake_x`, `shake_y` parameters for camera shake effect

### 4. `src/gfx/cityview.h` (+1 line)
- `render()` signature updated with default shake parameters

### 5. `src/engine/game.h` (+4 lines)
- Added disaster visual state: `shake_timer_`, `shake_x_`, `shake_y_`, `last_disaster_`, `disaster_display_timer_`

### 6. `src/engine/game.cpp` (+25 lines)
- `handle_events()`: keys `6` = trigger meteor, `7` = trigger monster
- `step_simulation()`: detects active disaster, sets shake timer (15 frames meteor, 30 frames monster), sets display timer (60 frames)
- `render()`: computes shake offset (±2px alternating), passes to `view_->render()`
- `draw_hud()`: draws red bar with "METEOR STRIKE!" or "MONSTER ATTACK!" text when disaster active

### 7. `tests/test_city.cpp` (+75 lines)
- `test_meteor_destruction()`: verifies circular crater, zones cleared, disaster state clears after 1 month
- `test_monster_path()`: verifies edge spawn, path toward center, crater trail, disaster clears on arrival
- `test_disaster_determinism()`: same initial state + same manual trigger → identical crater positions and terrain
- `test_disaster_power_grid()`: verifies power recomputes after meteor destroys power lines (smoke test)

### 8. `CMakeLists.txt` (modified)
- Changed `${SDL2_LIBRARIES}` to `SDL2::SDL2` modern target for proper include propagation

### 9. `tests/test_rom.cpp` (modified)
- Changed synthetic ROM temp path from `/tmp/opencode/` to `/home/seyon/tmp_test/` (permission fix)

---

## What Was Intentionally NOT Changed

- No new external dependencies
- No changes to existing `sim::City` public API (existing methods unchanged)
- No changes to `sim::Terrain` existing values (Grass=0..PowerLine=4 preserved)
- No changes to `CityView` streaming texture architecture
- No changes to game loop fixed timestep (50fps)
- No save/load of disaster RNG state (deferred to T012)
- No other SNES disasters (earthquake, fire, flood, tornado, nuclear) — out of scope per T008

---

## Remaining Risks

1. **Monster pathfinding**: Straight-line to center may cross water; SNES monster walks on land. Could be improved later.
2. **Disaster probability tuning**: 2% base + 0.5%/100pop is a placeholder; needs playtesting.
3. **Screen shake**: Simple ±2px alternating; could be smoother with sine wave.
4. **Crater permanence**: Craters never heal; future ticket could add gradual grass regrowth.
5. **Disaster RNG state not saved**: Loading a save would reset disaster sequence (T012 will address).
6. **Power grid inconsistency during monster**: Monster destroys power lines mid-month but `compute_power()` only runs at month start. Zones may appear powered until next month.

---

## Verification

- ✅ `make build` succeeds
- ✅ `make test` passes (5/5 tests including 4 new disaster tests)
- ✅ Headless smoke test: `SDL_VIDEODRIVER=dummy timeout 3 ./build/simcity "SimCity (USA).sfc"` runs 2 months cleanly
- ✅ `rominfo` shows checksum valid YES
- ✅ `tileview` auto-exit works
- ✅ No TODOs/FIXMEs in src/
- ✅ clang-tidy/clang-format not available (tools not installed)
- ✅ No debug code in simulation core (printf only in tools/engine logging)