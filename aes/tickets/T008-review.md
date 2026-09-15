---
ticket: T008
phase: review
status: done
created: 2026-09-12
requires:
  - aes/tickets/T008-verify.md
produces:
  - aes/tickets/T008-review.md
blocked_by: ''
---

# T008 — Review (Critical Review)

## Reviewer: Hostile Senior Engineer

---

## Correctness

### ✅ Disaster Mechanics
- **Meteor**: Correctly picks random non-water coordinate, destroys circular radius 6, sets Crater terrain, clears zones/plants. Disaster state clears after 1 month.
- **Monster**: Spawns at random edge, steps toward center (60,50) at 1 tile/month, destroys 3x3 path. Clears on arrival.
- **Monthly Roll**: 2% base + 0.5% per 100 population, capped at 15%. Randomly picks meteor/monster.
- **Determinism**: LCG seeded by `month_` ensures same sequence → same disasters. Verified by `test_disaster_determinism`.

### ⚠️ Issues Found
1. **Monster crosses water**: Straight-line path ignores terrain. SNES monster walks on land. Minor for MVP.
2. **Power grid mid-month inconsistency**: Monster destroys power lines but `compute_power()` only runs at month start. Zones appear powered until next month. Acceptable for MVP — disaster is rare.
3. **Crater never heals**: Permanent terrain change. Future ticket for regrowth.
4. **Disaster RNG not saved**: Save/load (T012) will reset disaster sequence. Documented as known limitation.

---

## Simplicity

### ✅ Good
- Disaster logic lives in `sim::City` — single responsibility, no new classes
- LCG is 1 line, no external RNG dependency
- Camera shake is ±2px alternating — minimal visual effect
- Manual triggers (keys 6/7) for testing only

### ⚠️ Could Be Simpler
- `DisasterState` repurposes `radius` field for monster target (packed x|y<<16). Slightly opaque. Could use separate fields.
- `update_monster()` recomputes target from packed radius each call. Minor overhead.

---

## Maintainability

### ✅ Good
- Clear separation: simulation (City) vs visual (Game/CityView)
- New `Terrain::Crater` integrates with existing rendering pipeline
- Tests cover all disaster behaviors
- No magic numbers exposed (radius=6, shake_frames=15/30, display_frames=60 are local constants)

### ⚠️ Concerns
- Disaster probability formula hardcoded in `process_disasters()`. Should be configurable for tuning.
- Monster target hardcoded to map center (60,50). Could be more varied.

---

## Security

### ✅ No Issues
- No user input parsing in disaster logic
- No buffer overflows (bounds checked via `in_bounds()`)
- No external data dependencies

---

## Performance

### ✅ Negligible Impact
- `process_disasters()` called once/month — O(1) for meteor, O(1) for monster step
- Meteor: iterates ~113 tiles (π·6²) — trivial
- Monster: destroys 9 tiles/month — trivial
- LCG: 1 multiply + 1 add per random call
- Camera shake: 2 integer ops per frame

---

## Backlog Tickets Created

| Ticket | Description | Priority |
|--------|-------------|----------|
| T018 | Monster land-only pathfinding (avoid water) | low |
| T019 | Crater healing/regrowth over time | low |
| T020 | Configurable disaster probability curves | medium |
| T021 | Save/load disaster RNG state | high (part of T012) |
| T022 | Varied monster targets (not always center) | low |
| T023 | Smoother screen shake (sine wave) | low |

---

## Approval Decision

**APPROVED WITH CONDITIONS**

Conditions:
1. Document known limitations in code comments (power grid mid-month, monster water crossing)
2. Add `// TODO: T018` comment in `update_monster()` for land pathfinding
3. Add `// TODO: T019` comment in `trigger_meteor()` for crater healing
4. Disaster probability constants should be `static constexpr` for easier tuning

These are minor and can be addressed in follow-up tickets. The implementation is correct, deterministic, and tested.