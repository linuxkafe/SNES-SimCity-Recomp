---
ticket: T028
title: Native Widescreen Renderer for Isometric Tilemap
sprint: sprint-03
priority: high
status: backlog
created: 2026-09-14
source: T027 decision — widescreen requirement addressed via native renderer
---

# T028 — Native Widescreen Renderer for Isometric Tilemap

## Context
SimCity SNES uses 2D isometric diamond-grid tilemap (120×100 tiles, 16×16 px each = 1920×1600 world). Current renderer shows 4:3 window. Widescreen requires rendering wider viewport with HUD anchored at screen edges.

## Acceptance Criteria
- [ ] Render arbitrary aspect ratios (16:9, 16:10, 21:9, 32:9)
- [ ] Viewport expands horizontally, centered vertically (like StarFox Enhanced adaptive presenter)
- [ ] HUD panels anchor to screen edges (not world coordinates)
- [ ] No distortion — pixel-perfect nearest-neighbor scaling
- [ ] Camera bounds adjust to new viewport width
- [ ] 6/6 ctest + headless smoke pass

## Technical Approach
- Extend `gfx::CityView` with `set_viewport(aspect_ratio)` 
- Calculate visible tile columns from window width / tile_width
- Center vertically: `start_row = max(0, (world_h_tiles - visible_rows) / 2)`
- HUD panels use screen-space coordinates (already implemented in T009)
- Tile culling: only submit visible tiles to GPU
- Config option in `config.ini` + runtime hotkey (F3 cycle aspect)

## Dependencies
- T025 (visual regression tests) for golden images
- T009 (UI panels) already screen-space anchored
- No recompilation dependency

## Estimation
- Complexity: medium (2-8h)
- Risk: low