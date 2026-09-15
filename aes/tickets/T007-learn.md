---
ticket: T007
phase: learn
date: 2026-09-11
---

# T007 — Learn Artifact

## Key Insights

### Power Grid Architecture
- **3-step flood fill** is the cleanest mental model: (1) radius powers footprint, (2) BFS through conductors, (3) power neighbors of conductors. This separates concerns cleanly.
- **Top-left dedup for multi-tile plants**: A 3×3 plant is a single logical entity; computing from center避免s counting 9 separate sources. Correct approach: iterate all tiles, store center coordinates, dedup.
- **Chebyshev distance** is the right metric for square radii (max(|dx|,|dy|) ≤ radius), NOT Manhattan distance which gives diamond shapes.

### Test Design
- **Zone-growth tests need a specific adjacency chain**: Plant → powered line → zone. Every test that checks "does a zone grow" must have an explicit power line adjacent to the zone, with that line connected back to a plant. The original tests placed zones within "radius" of the plant — but coal radius 1 from a 3×3 center covers exactly the footprint, nothing outside.
- **Gap tests are critical for verifying BFS termination**: The conductivity test must include an unconnected power line to prove BFS stops at gaps. Without this, you can't distinguish BFS from "power everything."

### Simulation Integration
- **Power must be computed BEFORE growth**: In `step_month()`, `compute_power()` runs first, then all zone logic uses `powered` flags. This ordering is essential — you can't check if a zone is powered if power hasn't been computed yet.
- **Upkeep for power plants must be per-tile**: A 3×3 coal plant has 9 tiles, each costing 50/month → 450/month. If you charge per-plant instead of per-tile, the economics are wrong.
