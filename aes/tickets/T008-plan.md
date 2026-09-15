---
ticket: T008
phase: plan
status: in-progress
created: 2026-09-12
requires:
  - aes/kanban.md
  - aes/tickets/T008-disaster-system.md
  - src/sim/city.h
  - src/sim/city.cpp
  - src/gfx/cityview.h
  - src/gfx/cityview.cpp
  - src/engine/game.h
  - src/engine/game.cpp
  - tests/test_city.cpp
produces:
  - aes/tickets/T008-plan.md
blocked_by: ''
---

# T008 — Plan: Disaster System (Meteor, Monster)

## Phase 0 — Reconnaissance Summary

**Files Consulted:**
- `src/sim/city.h` (132 lines) — Simulation core: Terrain/Zone enums, City class, step_month()
- `src/sim/city.cpp` (461 lines) — Simulation implementation: budget, RCI, power grid, terrain mapping
- `src/gfx/cityview.h` (50 lines) — Renderer interface: CityView with streaming texture
- `src/gfx/cityview.cpp` (158 lines) — Rendering implementation: ROM tiles + flat fallback, HUD
- `src/engine/game.h` (73 lines) — Game loop: Config, Tools, State
- `src/engine/game.cpp` (255 lines) — Main loop: init, events, update, render, step_simulation
- `tests/test_city.cpp` (343 lines) — Test patterns: deterministic tests, power grid tests

**Key Patterns Observed:**
1. **Deterministic simulation**: `month_` counter drives growth; same ops → same state
2. **Terrain enum**: 5 types (Grass, Water, Tree, Road, PowerLine) — need to add Crater
3. **Zone enum**: 5 types (None, Residential, Commercial, Industrial, PowerPlant)
4. **step_month()**: Single entry point for monthly simulation — ideal place for disaster logic
5. **CityView::rebuild()**: Rebuilds full-map texture — can add crater rendering
6. **Game::handle_events()**: Key handling — can add debug disaster triggers
6. **Tests**: Headless, no SDL, test City directly — disaster tests should follow pattern

---

## Phase 1 — Hostile Analysis

### INSIGHTS CONSULTED:
- SD-META-017 (Nintendo packet decompressor) — not directly relevant
- SD-META-018 (Scenario map format) — not directly relevant
- SD-CI-005 (False positive scan) — not directly relevant
- SD-META-019 (Graceful fallback) — pattern for optional features

### ASSUMPTIONS I'M MAKING (with uncertainty classification):

- **[KNOWN]** `sim::City::step_month()` is the single monthly tick — disaster logic belongs here
  - Justification: All monthly updates (power, growth, budget) happen here; disasters are monthly events in SNES

- **[KNOWN]** `Terrain` enum has 5 values, `Zone` has 5 values — adding Crater terrain is straightforward
  - Justification: Enum values are contiguous uint8_t; Crater = 5 fits without breaking ABI

- **[INFERRED]** SNES disaster probability scales with city population/score
  - Evidence: Original SimCity increases disaster frequency as city grows; prevents early-game frustration
  - Impact if false: Need to implement simpler fixed-probability model first

- **[INFERRED]** Meteor creates circular crater ~5-8 tile radius; Monster walks from edge to center
  - Evidence: SNES behavior per player memory and gameplay videos
  - Impact if false: Radius/pathfinding may need tuning

- **[ASSUMED]** Deterministic RNG needed — will use existing `month_` as seed via simple LCG
  - Justification: No `std::random` allowed (NFR-3); `month_` already tracks simulation time
  - Impact if false: Disaster sequence won't be reproducible across runs

- **[ASSUMED]** Visual feedback: screen shake (camera offset) + disaster name in HUD text
  - Justification: Minimal implementation; matches SNES screen flash + text
  - Impact if false: May need particle effects later (out of scope)

- **[UNKNOWN]** Exact SNES disaster trigger formulas (population thresholds, base probabilities)
  - Why outside reliable knowledge: No disassembly docs for disaster logic found yet

### WHAT WASN'T SPECIFIED (that matters):
- Disaster frequency curve (early vs late game)
- Whether disasters can chain (meteor then monster same month)
- Monster speed (tiles per month) and destruction width
- Whether crater terrain is permanent or fades over time
- Score/penalty impact beyond physical destruction

### ALTERNATIVES I DIDN'T CHOOSE (and why):

| Option | Description | Rejected Because |
|--------|-------------|------------------|
| A: Separate `DisasterManager` class | Own class handling all disaster logic | Over-engineering; disaster logic is simple enough to live in `City::step_month()` |
| B: Event queue system | Queue disasters with delays, types, parameters | YAGNI; SNES disasters are immediate monthly rolls |
| C: Full A* pathfinding for Monster | Proper pathfinding around obstacles | Complexity; straight-line to city center is sufficient for MVP |
| D: All 6 SNES disasters at once | Earthquake, fire, flood, tornado, nuclear, monster | Scope creep; T008 explicitly targets meteor + monster only |
| E: Crater as Zone type | Zone::Crater instead of Terrain::Crater | Craters are terrain, not zoning; zones can be built on terrain later |

### INVITE CONTRADICTION:
- **What would disprove my reasoning?** If SNES disaster logic uses a separate RNG state not derivable from `month_`, determinism across save/load would require storing that state. 
- **Critical flaw I might be missing:** Monster pathfinding through power lines/plants — does it destroy them? If so, power grid recomputation must happen after monster passes, not just at month start.

### DISTINGUISH CLAIM TYPES / QUESTION TYPE DISTINCTION:

**Empirical (what is):**
- SNES disaster trigger formula (needs disassembly or empirical testing)
- Exact meteor radius and monster speed in original
- Whether crater terrain is traversable/zoneable in SNES

**Normative (what should be):**
- Initial disaster probability (balancing decision)
- Whether manual trigger should be cheat-only or always available
- Whether craters should be permanent or healable

### RISKS & SIDE EFFECTS:
1. **Power grid inconsistency**: Monster destroying power lines mid-month won't reflect until next `compute_power()` — zones may appear powered when they're not
2. **Determinism breach**: If RNG uses system time or non-deterministic source
3. **Visual glitches**: Crater rendering must work with both ROM tiles and flat fallback
4. **Test complexity**: Disaster tests need controlled RNG seed, may be flaky if probability-based

### COST OF BEING WRONG: **MEDIUM**
- Disaster system is self-contained; wrong balancing can be tuned later
- Core simulation (budget, RCI, power) unaffected if disasters are buggy
- Visual bugs only affect rendering, not simulation correctness

---

### REASONING SKELETON FOR KEY CLAIMS:

**Claim: Disaster logic belongs in `City::step_month()`**
→ Premise 1: `step_month()` is the single monthly simulation tick
→ Premise 2: SNES disasters occur monthly
→ Premise 3: Disasters affect terrain/zones/power — all managed by `City`
→ Inference: Keep disaster state and logic in `City` for encapsulation
→ Conclusion: Add `DisasterState` struct to `City`, process in `step_month()`

**Claim: Simple LCG seeded by `month_` is sufficient for determinism**
→ Premise 1: `month_` increments once per simulation month
→ Premise 2: Same `month_` sequence → same disaster rolls
→ Premise 3: LCG (e.g., `state = state * 1664525 + 1013904223`) is deterministic
→ Inference: No external RNG dependency needed
→ Conclusion: Implement `next_random()` method in `City` using `month_` as seed

**Claim: Crater should be a Terrain type, not Zone**
→ Premise 1: Craters are ground features (like Water, Tree)
→ Premise 2: Zones represent development (Residential, Commercial, Industrial)
→ Premise 3: Player may later bulldoze crater → build on it
→ Inference: Terrain::Crater allows future zoning; Zone::Crater would not
→ Conclusion: Add `Terrain::Crater = 5`

---

### SCOPE BOUNDARIES DECLARATION:

**This analysis covers:**
- Meteor disaster: trigger, circular destruction, crater terrain
- Monster disaster: spawn, straight-line path to center, tile destruction
- Deterministic RNG integration with `month_`
- Visual feedback: camera shake + HUD text
- Manual debug triggers (keys 6/7)
- Unit tests for disaster mechanics

**This analysis deliberately excludes:**
- Other SNES disasters (earthquake, fire, flood, tornado, nuclear) — future tickets
- Audio — T010
- Disaster adviser UI — T009/T015
- Crater healing/fading over time — future
- Save/load of disaster RNG state — T012
- Monster pathfinding around obstacles — MVP uses straight line

---

## Phase 2 — Solution Proposal

### Chosen Approach

**1. Extend `sim::City` (sim/city.h/.cpp):**
- Add `Terrain::Crater = 5` to enum
- Add `DisasterType { None, Meteor, Monster }` enum
- Add `DisasterState` struct: `type`, `active`, `timer`, `params` (x,y,radius for meteor; path for monster)
- Add `next_random()` method using LCG seeded by `month_`
- Add `trigger_disaster(DisasterType)` for manual testing
- Add `process_disasters()` called at start of `step_month()`
- Meteor: pick random valid coordinate, destroy circle radius 6, set Crater terrain
- Monster: pick random edge, step toward center each month, destroy 2-tile wide path

**2. Extend `gfx::CityView` (gfx/cityview.h/.cpp):**
- Add `Terrain::Crater` color: dark gray {40, 40, 40}
- Add ROM tile mapping for crater (reuse Water tile or find closest)
- Camera shake: add `shake_offset_x/y` to render, decay over frames

**3. Extend `engine::Game` (engine/game.h/.cpp):**
- Add debug keys: `6` = trigger meteor, `7` = trigger monster
- Add disaster name display in HUD (below tool bar)
- Pass shake offset to CityView render

**4. Tests (tests/test_city.cpp):**
- `test_meteor_destruction()`: verify circle of Crater terrain, zones cleared
- `test_monster_path()`: verify path from edge to center, destruction width
- `test_disaster_determinism()`: same month_ sequence → same disaster locations
- `test_disaster_power_grid()`: verify power lines destroyed, recompute works

---

### What Will NOT Change

- `sim::City` public API (existing methods unchanged)
- `sim::Terrain` existing values (Grass=0..PowerLine=4 preserved)
- `CityView` streaming texture architecture
- Game loop fixed timestep (50fps)
- No new external dependencies

---

### Verification Criteria

- [ ] `make build` succeeds
- [ ] `make test` passes (all existing + new disaster tests)
- [ ] Headless smoke test runs 3+ months without crash
- [ ] Manual meteor trigger (key 6) creates visible crater
- [ ] Manual monster trigger (key 7) creates path across map
- [ ] Determinism test: two Cities with same ops have identical disaster sequence
- [ ] No memory leaks (valgrind if available)
- [ ] Clang-tidy clean on new code
- [ ] Clang-format clean

---

## Phase 3 — Implementation Plan (Surgical Changes)

### Files to Modify (in order):

1. **src/sim/city.h** — Add Crater terrain, DisasterType, DisasterState, next_random(), trigger_disaster()
2. **src/sim/city.cpp** — Implement disaster logic in step_month(), process_disasters(), meteor(), monster()
3. **src/gfx/cityview.cpp** — Add crater color/tile mapping, camera shake support
4. **src/engine/game.h** — Add shake offset, disaster display state
5. **src/engine/game.cpp** — Handle debug keys 6/7, pass shake to render, draw disaster text
6. **tests/test_city.cpp** — Add disaster test cases

### Estimated Diff Count: ~300 lines across 6 files

---

## Phase 4 — Validation Plan

Run after implementation:
1. `make build` — compile check
2. `make test` — all unit tests pass
3. `SDL_VIDEODRIVER=dummy timeout 3 ./build/simcity "SimCity (USA).sfc"` — headless smoke
4. Manual test: run game, press 6/7, verify visual effects
5. `clang-tidy src/sim/city.cpp src/gfx/cityview.cpp src/engine/game.cpp` — lint
6. `clang-format --dry-run --Werror src/ tests/` — format check

---

## Remaining Risks

- Monster straight-line path may cross water (SNES monster walks on land); may need simple land-check
- Disaster probability tuning requires playtesting
- Screen shake implementation may need frame counter in Game, not City