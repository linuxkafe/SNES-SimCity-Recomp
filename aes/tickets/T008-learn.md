---
ticket: T008
phase: learn
status: done
created: 2026-09-12
requires:
  - aes/tickets/T008-review.md
produces:
  - aes/tickets/T008-learn.md
side_effects:
  - updates aes/sprints/sprint-02.md (learning section)
  - updates aes/kanban.md (Learning History)
  - creates aes/shadow/SD-META-*.md (N new insights)
blocked_by: ''
---

# T008 — Learn Artifact

## What Was Done (2 sentences)
Implemented the disaster system with Meteor and Monster disasters for SimCity SNES PC port. Disasters trigger randomly each month (scaling with population) or manually via debug keys, destroy terrain/zones in their path, and provide visual feedback via camera shake and HUD alerts.

---

## Feynman Method

### For a Child
Imagine you're playing SimCity and suddenly — KABOOM! — a space rock crashes into your city, leaving a big round hole (that's a meteor). Or a giant monster walks out of the ocean and stomps through your downtown, smashing buildings as it goes (that's a monster). I built the rules for when these happen, where they hit, and what they destroy. The game rolls dice each month to decide if a disaster strikes — bigger cities get more disasters. When it happens, the game shows a screen shake and a red alert bar so you know something bad happened.

### For an Expert
Implemented two disaster types in the deterministic simulation core:
- **Meteor**: Uniform random valid coordinate (non-water), circular destruction radius 6 using Chebyshev distance, sets `Terrain::Crater`, clears zones/power plants in radius. Instant effect, disaster state clears after 1 month for display.
- **Monster**: Spawns at random map edge, targets map center (60,50), moves 1 tile/month (4-directional), destroys 3x3 area at each step. Multi-month duration until reaching center.
- **Trigger System**: Monthly roll in `process_disasters()` — base 2% + 0.5% per 100 population, capped at 15%. LCG (state = state × 1664525 + 1013904223) seeded by `month_` for full determinism.
- **Visual Feedback**: Camera shake (±2px alternating for 15/30 frames), red HUD bar with disaster name for 60 frames.
- **Manual Triggers**: Keys `6` (meteor) and `7` (monster) for testing.

**Trade-offs:**
- Monster uses straight-line path (not land-only) — simpler, matches MVP scope
- Power grid inconsistency during monster month accepted — disaster rare, recomputes next month
- Crater terrain permanent — no healing mechanic yet
- Disaster RNG state not persisted — save/load (T012) will need to serialize `rng_state_`

---

## First Principles

### Challenged Assumptions
| Assumption | Was it fact or habit? | What we discovered |
|------------|----------------------|-------------------|
| Disasters need complex event queue | Habit (from other engines) | Monthly roll + immediate effect is sufficient for SNES-style disasters |
| Monster needs A* pathfinding | Assumed | Straight-line to center works for MVP; land-only is future polish |
| Disaster RNG must be std::random | Habit | Simple LCG seeded by month_ gives full determinism with zero deps |
| Visual effects need particles | Assumed | Camera shake + HUD bar is sufficient for SNES aesthetic |

### The Real Problem
The real problem was not implementing disaster mechanics — it was doing so without breaking determinism or adding dependencies. The SNES original uses a simple monthly probability check. The key insight: **disasters are just another monthly simulation step**, not a separate event system. By putting logic in `step_month()` and using the existing `month_` counter as RNG seed, we get determinism for free.

### If We Started Today
With current knowledge, I would:
1. Make disaster probability constants `static constexpr` for easier tuning
2. Add separate `target_x`, `target_y` fields to `DisasterState` instead of packing into `radius`
3. Add `// TODO` comments for known limitations (water crossing, crater healing)
4. Consider making meteor radius configurable per disaster tier

---

## Hostile Audit

### Where Our Learnings Fail
- **Assuming straight-line monster is "good enough"**: Players may notice monster walking through water. But SNES monster also ignores water in some versions — needs verification.
- **Ignoring mid-month power grid**: Zones destroyed by monster may show as powered until next month. In practice, disaster is rare and player sees destruction visually first.
- **Hardcoded disaster probabilities**: No configuration file; tuning requires recompile. Acceptable for now.
- **No disaster variety**: Only 2 of 6 SNES disasters. Explicitly scoped to meteor + monster per T008.

### What We Do Not Know That We Do Not Know
- **Exact SNES disaster probability formula**: Used 2% base + scaling. Real formula unknown without disassembly.
- **Whether meteor can hit water in SNES**: Our code avoids water; SNES might allow it.
- **Monster speed in SNES**: We use 1 tile/month. Could be faster.
- **Whether craters affect land value**: SNES craters may reduce land value permanently. Not simulated.

### Experiment to Confirm/Refute
1. **Emulator trace**: Run SNES SimCity, trigger disasters via cheat, log coordinates and destruction patterns.
2. **Probability measurement**: Run 1000 months at various populations, measure disaster frequency.
3. **Visual comparison**: Record SNES disaster screen shake vs our ±2px alternating.

---

## Decisions We Would Change
- **Packed target in radius field** → Would use separate `target_x`, `target_y` fields for clarity
- **Hardcoded constants in process_disasters()** → Would make `static constexpr` for tuning
- **No TODO comments for known gaps** → Would add `// TODO: T018` etc. in code

---

## What Went Well
- Deterministic RNG via LCG + month_ seed — zero dependencies, full reproducibility
- Clean integration: disasters in `step_month()`, visual in `Game::render()`, rendering in `CityView`
- All 4 new tests pass; no regressions in 5 existing test suites
- Manual triggers (keys 6/7) enable instant testing without waiting for random roll
- Camera shake and HUD alert provide clear feedback with minimal code

---

## What Went Wrong
- Spent time debugging test_rom failure (permission issue with /tmp/opencode) — unrelated to disaster code
- Monster target packing into `radius` field is clever but opaque
- Forgot to add TODO comments for known limitations during implementation

---

## Shadow Docs Created
| ID | Title | Category |
|----|-------|----------|
| SD-META-020 | Deterministic LCG pattern for simulation RNG | META |
| SD-META-021 | Disaster system integration in monthly step | META |
| SD-CI-006 | Test infrastructure for random-triggered features | CI |
| SD-META-022 | Camera shake via render offset | META |

---

## Shadow Docs Content

### SD-META-020: Deterministic LCG pattern for simulation RNG
**Synthesis**: For deterministic simulations needing randomness, use a simple LCG (Linear Congruential Generator) seeded by the simulation step counter. `state = state * 1664525 + 1013904223` (Numerical Recipes constants). Same step sequence → same random sequence. Zero dependencies, O(1), trivial to save/restore (just serialize `state`).

### SD-META-021: Disaster system integration in monthly step
**Synthesis**: Disasters that occur monthly (like SNES SimCity) should be processed at the START of `step_month()`, before other monthly updates. This ensures: (1) destruction affects this month's budget/power, (2) disaster state clears after visual display duration, (3) deterministic order: roll → apply → compute_power → growth → budget.

### SD-CI-006: Test infrastructure for random-triggered features
**Synthesis**: For features with random triggers, provide manual trigger API (`trigger_disaster(type)`) for testing. Test determinism by: (1) creating two identical simulations, (2) calling same manual trigger, (3) verifying identical state after N steps. Test random roll by controlling RNG seed (via initial month_).

### SD-META-022: Camera shake via render offset
**Synthesis**: Simple screen shake: alternate ±N pixels per frame for M frames. Implement as `shake_timer` in Game, compute `shake_x/y` in render, pass to `CityView::render(cam_x + shake_x, cam_y + shake_y)`. Zero cost when not shaking, trivial to tune (amplitude, duration, frequency).