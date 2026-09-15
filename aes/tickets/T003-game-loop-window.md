---
ticket: T003
title: Basic game loop + window
sprint: sprint-01
priority: high
status: pending
created: 2026-09-11
---

# T003 — Basic game loop + window

## Context
The main engine shell: SDL2 window, event pump, fixed-timestep loop,
and a minimal render pipeline. This becomes the skeleton for the full game.

## Acceptance Criteria
- [ ] `./simcity <rom.sfc>` opens an 800x600 window titled "SimCity"
- [ ] ESC closes the window
- [ ] Fixed-timestep game loop (50fps SNES-aligned)
- [ ] Frame counter displayed on terminal on clean exit
- [ ] `--help` flag for CLI usage
- [ ] Unit tests for game loop timing logic (mock tick counter)

## Scope
**In scope:** Engine shell, window management, event system, frame timing.
**Out of scope:** City rendering (T005), sound (T007), save (future).

## Dependencies
- T001 (ROM parser)
- T002 (graphics infrastructure)

## Known Risks
- Could over-engineer; keep it minimal for now