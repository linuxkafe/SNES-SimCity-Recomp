---
ticket: T007
phase: review
date: 2026-09-11
---

# T007 — Critical Review

## Correctness

**Verdict: APPROVED**

- `flood_fill_power()` 3-step algorithm is correct: radius powers plant footprint, BFS extends through power lines, Step 3 powers zones adjacent to lines
- Coal radius 1 from center of 3×3 = exactly the footprint (no external coverage). Nuclear radius 2 from 4×4 center extends 1 tile beyond footprint. Both match SNES behavior.
- Power does NOT flow through roads or buildings — only through power lines. This is correct per the task spec.
- Determinism preserved: top-left dedup for plant centers, deterministic BFS neighbor order (up/right/down/left)

## Simplicity

**Verdict: Good**

- 3-step flood_fill is straightforward and readable
- Static arrays (queue[12000], visited[100][120]) avoid heap allocation but are map-size-dependent. Safe for 120×100 SNES map.
- No unnecessary abstractions

## Maintainability

**Findings:**
- **MINOR**: `power_plant_tiles * kUpkeepCoalPlant` uses coal cost for ALL plant types. Nuclear should cost more. Low priority — flagged for future ticket.
- **MINOR**: `stats()` filters zones by `powered` but `step_month()` also does the same aggregation. Some duplication. Acceptable for clarity.

## Security

No security concerns. Local simulation only.

## Performance

**Verdict: Acceptable**

- `compute_power()` runs O(N) per month (N = map tiles 12000). Flood fill is O(N) worst case. Total: O(N) per month. No performance regression.
- Static arrays avoid malloc/free per call.

## Technical Debt

- Static arrays in `flood_fill_power()` — should be member variables or heap-allocated if map size ever changes
- Power line upkeep counted separately from road upkeep in `step_month()` but `stats()` doesn't expose power_line_tiles separately
- No visual feedback for power state (no overlay rendering yet — T008+)

## Risk Assessment

- **Low risk**: Power is additive to existing simulation. Existing tests updated to maintain coverage.
- **Medium risk**: No visual power overlay — users can't see what's powered. Mitigated by headless test passing.
