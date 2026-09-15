---
ticket: T008
title: Disaster system (meteor, monster)
sprint: sprint-02
priority: medium
status: pending
created: 2026-09-12
---

# T008 — Disaster System (Meteor, Monster)

## Context
SimCity SNES features disaster events that can strike the player's city. The two primary disasters are:
- **Meteor**: A large impact crater destroying terrain/buildings in a radius
- **Monster**: A Godzilla-like creature that walks through the city leaving destruction

These are iconic features that add challenge and variety to gameplay. Currently the simulation core has no disaster mechanics.

## Acceptance Criteria
- [ ] Meteor disaster: random impact at valid coordinates, destroys terrain/buildings in circular radius, leaves crater terrain
- [ ] Monster disaster: spawns at map edge, pathfinds through city destroying tiles, exits map
- [ ] Disasters trigger randomly based on game state (population, time, random factor)
- [ ] Visual feedback: screen shake, sound (placeholder), disaster name display
- [ ] Player can trigger disasters manually via cheat/debug menu for testing
- [ ] All simulation remains deterministic (same seed → same disaster sequence)
- [ ] Headless tests pass for disaster mechanics

## Scope
**In scope:**
- Disaster types: Meteor, Monster (core two from SNES)
- Trigger logic (random + manual)
- Terrain/zone destruction effects
- Crater terrain type addition
- Monster pathfinding (simple A* or direct line to center)
- Integration with monthly step

**Out of scope:**
- Other SNES disasters (earthquake, fire, flood, tornado, nuclear meltdown) - future tickets
- Audio playback - T010
- Disaster advisers/UI panels - T009/T015
- Disaster recovery mechanics (rebuilding incentives) - future

## Dependencies
- T005: City simulation core (done) - provides terrain/zone grid, step_month()
- T006: City map rendering (done) - for visual crater/monster path rendering
- T007: Power grid (done) - monster should destroy power lines/plants

## Rollback
Revert `src/sim/city.h/cpp` disaster additions, remove crater terrain from `sim::Terrain` enum, remove disaster test cases.

## Known Risks
- Monster pathfinding may be complex; start with simple straight-line to city center
- Deterministic RNG needed for disaster rolls (no std::random_device)
- Visual effects need integration with CityView rendering
- Balancing: disasters shouldn't be too frequent or too destructive early game

## Notes
- SNES disaster logic: check each month, probability based on city size/score
- Meteor: single impact, circular destruction radius ~5-8 tiles
- Monster: enters from edge, moves toward city center, destroys 1-2 tiles per step
- Crater terrain: new type, impassable, unzoneable, distinct visual