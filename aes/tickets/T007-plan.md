---
ticket: T007
phase: plan
status: done
created: 2026-09-11
requires:
  - aes/kanban.md
  - aes/tickets/T007-power-grid-simulation.md
  - src/sim/city.h
  - src/sim/city.cpp
  - tests/test_city.cpp
produces:
  - aes/tickets/T007-plan.md
side_effects: []
blocked_by: ''
---

# T007 — Plan

## Phase 0 — Reconnaissance Summary

**Files examined:**
- `src/sim/city.h` — City class, Tile struct, Terrain/Zone enums, step_month(), stats()
- `src/sim/city.cpp` — Implementation: budget, RCI demand, residential growth, terrain map application
- `tests/test_city.cpp` — 7 tests covering placement, growth, budget, determinism, RCI
- `aes/tickets/T004-learn.md` — Learnings: deterministic sim core works; tile→terrain mapping uses low 10 bits; high bits ignored

**Current simulation state:**
- 120×100 tile grid with Terrain (Grass/Water/Tree/Road) and Zone (None/Res/Com/Ind)
- Monthly step: population/jobs calc → RCI demand → residential growth → budget
- Deterministic: identical operations → identical state (verified by test_determinism)
- No power concept exists yet

**SNES power mechanics (from reverse engineering notes):**
- Coal plant: 3×3 tile footprint, powers 3×3 area centered on plant (radius 1)
- Nuclear plant: 4×4 tile footprint, powers 4×4 area centered on plant (radius 1.5 → effectively 2-tile radius)
- Power lines: extend power radius along connected line network (4-connected)
- Unpowered zones: residential density stalls, commercial produces 0 jobs, industrial produces 0 jobs

---

## Phase 1 — Hostile Analysis

### INSIGHTS CONSULTED
- SD-META-017: Nintendo packet decompressor notes (not directly relevant)
- SD-META-018: SimCity SNES map format — confirms tile IDs encode zone/power flags in high bits
- SD-CI-005: False positive scan heuristic — not relevant here
- SD-META-019: Graceful fallback pattern — applicable for power plant placement failures

### ASSUMPTIONS I'M MAKING (with uncertainty classification)

| Classification | Assumption | Justification | Impact if False |
|---|---|---|---|
| **KNOWN** | 120×100 grid, deterministic step_month | Verified by code + tests | N/A |
| **KNOWN** | Tile struct has terrain, zone, density | city.h lines 90-94 | N/A |
| **INFERRED** | Coal plant = 3×3 zone, Nuclear = 4×4 zone | SNES docs, Lytron notes | Wrong footprint → visual mismatch, power radius wrong |
| **INFERRED** | Power radius: coal 3×3, nuclear 4×4 centered | SNES mechanics | Balance off; zones too easy/hard to power |
| **INFERRED** | Power lines conduct indefinitely along 4-connected path | Standard SimCity mechanic | If limited range, need distance tracking |
| **ASSUMED** | Power state = binary per tile (powered/unpowered) | SimCity classic model | If graded voltage, major redesign |
| **ASSUMED** | Power recomputed monthly in step_month() | Fits existing monthly tick | If event-driven, breaks determinism |
| **UNKNOWN** | Exact tile IDs for power plants in SNES | Not in current tile→terrain table | Need to discover or assign new IDs |
| **UNKNOWN** | Whether power lines are terrain or zone | SNES uses terrain for lines | Affects Tile struct design |

### WHAT WASN'T SPECIFIED (that matters)
- Power plant construction cost (funds deduction) — defer to UI ticket
- Whether power plants themselves consume power (they don't in SNES)
- Visual distinction: power line vs road rendering
- Power overlay UI — separate ticket (T009)
- Save/load of power state — T012

### ALTERNATIVES I DIDN'T CHOOSE (and why)

| Alternative | Rejected Because |
|---|---|
| Power as separate grid overlay (not in Tile) | Adds memory, complicates determinism; Tile already has spare bits |
| Event-driven power updates (only on plant/line placement) | Breaks monthly determinism; monthly recompute is simpler |
| Voltage/graded power (0-100%) | SNES uses binary powered/unpowered; overengineering |
| Power lines as Zone type | They're terrain in SNES (graphically distinct from roads); Zone reserved for RCI |
| BFS per plant each month | O(plants × grid) = acceptable for 120×100; flood-fill from all plants simultaneously is O(grid) |

### INVITE CONTRADICTION
- **What would disprove my approach?** If SNES power lines have limited length (e.g., max 10 tiles from plant), my unlimited conductivity is wrong. If power plants require fuel (coal consumption), my model is incomplete.
- **Critical flaw I might be missing:** The Tile struct currently has no "powered" bit. Adding one increases memory from 4 bytes/tile → 5+ bytes (alignment). 120×100×5 = 60KB, still trivial. But if we need per-tile voltage, that's more.

### DISTINGUISH CLAIM TYPES
- **Empirical (what is):** Tile struct layout, step_month() flow, test coverage, SNES power mechanics
- **Normative (what should be):** Power line as Terrain (not Zone), binary powered state, monthly recompute

### RISKS & SIDE EFFECTS
- **Performance:** Flood-fill 120×100 = 12,000 tiles/month. Trivial (<0.1ms).
- **Memory:** Adding `bool powered` to Tile → padding to 6 or 8 bytes. 120×100×8 = 96KB. Acceptable.
- **Determinism:** Flood-fill order must be deterministic (fixed plant iteration order, fixed neighbor order).
- **Rendering:** gfx/renderer.cpp needs PowerLine terrain type for visual distinction.
- **Tooling:** Edit tools (T006) need new tool modes for power plant placement.

### COST OF BEING WRONG
- **MEDIUM-HIGH**: Power grid is core gameplay. Wrong radius/connectivity breaks city growth balance. But: isolated to sim::City, no rendering/audio coupling. Revert is clean.

### REASONING SKELETON FOR KEY CLAIMS

**Claim: Power lines should be Terrain, not Zone**
- Premise 1: SNES graphics show power lines as distinct terrain tiles (like roads)
- Premise 2: Zone enum is for RCI (Residential/Commercial/Industrial) — power plant is a zone, line is infrastructure
- Premise 3: Tile struct already has terrain + zone; using terrain for lines keeps zone for RCI
- Inference: PowerLine ∈ Terrain, PowerPlant ∈ Zone (with sub-type)
- Conclusion: Add Terrain::PowerLine, Zone::PowerPlant with coal/nuclear variants

**Claim: Monthly flood-fill from all plants is correct algorithm**
- Premise 1: step_month() already iterates full grid for growth/budget
- Premise 2: Power state depends only on plant locations + line connectivity (no temporal state)
- Premise 3: Deterministic flood-fill (fixed order) produces identical results
- Inference: Compute power coverage as first pass in step_month(), before growth/budget
- Conclusion: Add `compute_power()` called at start of `step_month()`

**Claim: Unpowered zones produce 0 jobs / stall growth**
- Premise 1: SNES mechanics — commercial/industrial need power to operate
- Premise 2: Current code uses `commercial_tiles * kComJobsPerTile` unconditionally
- Inference: Job calculation must filter by `powered(x,y)`
- Conclusion: Modify job/population aggregation to respect power state

### SCOPE BOUNDARIES DECLARATION
This plan covers: power plant zones, power line terrain, monthly power computation, powered-query API, job/growth gating by power. Deliberately excludes: construction costs (UI), disaster destruction (T008), pollution, save/load (T012), power overlay rendering (T009).

---

## Phase 2 — Solution Proposal

### Data Model Changes

**city.h — New types:**
```cpp
enum class Terrain : uint8_t {
    Grass = 0,
    Water = 1,
    Tree  = 2,
    Road  = 3,
    PowerLine = 4,        // NEW
};

enum class Zone : uint8_t {
    None        = 0,
    Residential = 1,
    Commercial  = 2,
    Industrial  = 3,
    PowerPlant  = 4,      // NEW
};

enum class PowerPlantType : uint8_t {
    None     = 0,
    Coal     = 1,  // 3x3 footprint, powers 3x3 (radius 1)
    Nuclear  = 2,  // 4x4 footprint, powers 4x4 (radius 2)
};
```

**Tile struct — add power state:**
```cpp
struct Tile {
    Terrain terrain = Terrain::Grass;
    Zone    zone    = Zone::None;
    uint8_t density = 0;
    bool    powered = false;        // NEW: powered this month
    PowerPlantType plant_type = PowerPlantType::None;  // NEW: for plant tiles
};
```

**City class — new public API:**
```cpp
// Query power state for UI overlay
bool powered(int x, int y) const;

// Power plant placement (validates footprint clear)
bool place_power_plant(int x, int y, PowerPlantType type);
bool remove_power_plant(int x, int y);

// Power line placement
bool place_power_line(int x, int y);
bool remove_power_line(int x, int y);
```

**City class — new private:**
```cpp
void compute_power();  // called at start of step_month()
int power_radius(PowerPlantType type) const;
bool is_power_line(int x, int y) const;
```

### Algorithm: `compute_power()`

```cpp
void City::compute_power() {
    // 1. Clear all powered flags
    for (all tiles) tiles_[y][x].powered = false;

    // 2. Collect all power plant centers (deterministic order: y then x)
    struct Plant { int x, y; PowerPlantType type; int radius; };
    Plant plants[max_plants];
    int plant_count = 0;
    for (y=0..99) for (x=0..119) {
        if (tiles_[y][x].zone == Zone::PowerPlant) {
            plants[plant_count++] = {x, y, tiles_[y][x].plant_type, 
                                     power_radius(tiles_[y][x].plant_type)};
        }
    }

    // 3. Flood-fill from each plant through power lines
    // Use a single visited array to merge overlapping regions
    bool visited[100][120] = {false};
    for (int p = 0; p < plant_count; ++p) {
        flood_fill_power(plants[p].x, plants[p].y, plants[p].radius, visited);
    }
}

void City::flood_fill_power(int px, int py, int radius, bool visited[100][120]) {
    // BFS queue: (x, y, dist_from_plant)
    // Start from plant center, spread through power lines + powered tiles
    // Power lines conduct indefinitely; non-line tiles only within radius
    // Deterministic neighbor order: up, right, down, left
}
```

### Integration Points in `step_month()`

```cpp
void City::step_month() {
    compute_power();  // FIRST: establish power coverage

    // ... existing aggregation, but filter by power:
    // Commercial jobs only if powered
    // Industrial jobs only if powered
    // Residential growth only if powered
    // Budget: power plants have upkeep cost
}
```

### Tile→Terrain Mapping (city.cpp)
Add to `get_tile_to_terrain_table()`:
- Power line tile IDs (TBD from SNES or assign new)
- Power plant tile IDs handled via Zone, not Terrain

### Tests to Add (test_city.cpp)
1. `test_power_plant_placement()` — coal/nuclear placement, footprint validation
2. `test_power_line_conductivity()` — lines connect, gaps block
3. `test_power_coverage_radius()` — coal 3×3, nuclear 4×4 verified
4. `test_unpowered_zone_no_jobs()` — commercial/industrial produce 0 jobs when unpowered
5. `test_unpowered_residential_no_growth()` — density stalls without power
6. `test_power_determinism()` — identical operations → identical power state

---

## Phase 3 — Implementation Plan (Surgical)

**Files to modify (only these):**
1. `src/sim/city.h` — Add enums, Tile fields, public API declarations
2. `src/sim/city.cpp` — Implement compute_power(), flood_fill, integrate into step_month()
3. `tests/test_city.cpp` — Add 6 new test functions

**Files NOT to touch:**
- `src/gfx/*` — Rendering handles new Terrain::PowerLine separately
- `src/engine/*` — Tool modes for plant/line placement separate ticket
- `src/snes/*` — No ROM changes needed

**Verification criteria:**
- `make test` passes (all existing + new tests)
- `make build` succeeds
- Headless smoke test runs 3 seconds without crash
- Determinism test passes

---

## Phase 4 — Validation Gates

| Gate | Check |
|---|---|
| Compile | `make build` exits 0 |
| Unit tests | `make test` exits 0 (13 tests: 7 existing + 6 new) |
| Determinism | `test_determinism` + new `test_power_determinism` pass |
| Headless | `SDL_VIDEODRIVER=dummy timeout 3 ./build/simcity "SimCity (USA).sfc"` |
| No new LSP errors | Clean build |

---

## Phase 5 — Critical Review (Pre-approval)

**Simplicity check:** Flood-fill is ~50 lines. Tile struct grows by 2 fields. No new classes. Minimal diff.

**Correctness check:** 
- Power computed before growth/budget ✓
- Deterministic neighbor order (up/right/down/left) ✓
- Plant iteration order fixed (y-major) ✓
- Unpowered zones correctly gated ✓

**Maintainability:** Power logic isolated in `compute_power()` and `flood_fill_power()`. Easy to adjust radius/rules later.

**Remaining risks:** 
- Tile ID mapping for power lines (need SNES tile IDs or assign)
- Power plant upkeep cost not yet tuned (add constants)
- Nuclear 4×4 footprint: is it 4×4 centered (impossible on even grid) or offset? → Assume 4×4 with plant at (1,1) offset within footprint.

**Decision:** Proceed with implementation. Tile IDs for power lines will use unused range in tile→terrain table (e.g., 128-135). Power plant footprint validation uses Zone::PowerPlant + plant_type.