---
ticket: T030
title: Performance Profiling + Optimization
sprint: sprint-03
priority: medium
status: backlog
created: 2026-09-14
source: T027 decision — optimization requirement addressed via profiling + targeted fixes
---

# T030 — Performance Profiling + Optimization

## Context
Current sim runs at ~60 FPS headless (664 frames in 3s = 221 FPS). Need to ensure 60 FPS at 4K with full rendering, and identify any bottlenecks before they become problems.

## Acceptance Criteria
- [ ] Profile `sim::City::step_month()` with `perf` / `VTune` / `instruments`
- [ ] Identify hot paths: power grid flood-fill, RCI calculation, zone growth, disaster processing
- [ ] Optimize top 3 bottlenecks with measurable improvement
- [ ] Benchmark: 1000 months headless < 5 seconds (200 FPS sustained)
- [ ] Memory: no leaks, stable RSS over 10k months
- [ ] 6/6 ctest + headless smoke pass

## Technical Approach
- Add `--benchmark` CLI mode: runs N months headless, prints ms/month
- Instrument with `Tracy` (C++ profiler, MIT) or manual `chrono` markers
- Power grid: spatial hash for power lines → O(1) neighbor lookup
- RCI: cache demand indices, dirty flag on zone change
- Zone growth: precompute neighbor lists, avoid repeated bounds checks
- Disaster: early-out if no valid spawn locations
- Renderer: frustum culling already in CityView; verify tile submission count

## Dependencies
- Tracy profiler (header-only, MIT) — optional, can use manual timing
- T005 (sim core), T006 (city view)
- No recompilation dependency

## Estimation
- Complexity: low (< 2h profiling, 2-8h optimization)
- Risk: low