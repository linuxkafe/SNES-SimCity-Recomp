---
ticket: T009
title: UI panels (budget, population, RCI graphs)
sprint: sprint-02
priority: high
status: in-progress
created: 2026-09-13
---

# T009 — UI panels (budget, population, RCI graphs)

## Context
Implement the main UI panels for the game: budget panel, population panel, and RCI (Residential-Commercial-Industrial) demand graphs. These are essential for gameplay - players need to see city finances, population stats, and zone demand.

## Acceptance Criteria
- [ ] Budget panel shows income, expenses, treasury balance
- [ ] Population panel shows total population, residential/commercial/industrial breakdown
- [ ] RCI graph shows demand bars for R, C, I zones
- [ ] Panels render correctly at different window sizes
- [ ] Panels update each simulation step
- [ ] All tests pass

## Scope
**In scope:**
- Budget panel implementation (src/ui/budget_panel.cpp)
- Population panel implementation (src/ui/population_panel.cpp)
- RCI graph panel implementation (src/ui/rci_panel.cpp)
- Panel layout/management (src/ui/panel_manager.cpp)
- Integration with sim::City for data
- Unit tests for panel data formatting

**Out of scope:**
- Full scenario UI (T011)
- Save/load UI (T012)
- Keyboard shortcuts (T013)
- Audio (T010)

## Dependencies
- T005 City simulation core (done) - provides City data
- T006 City map rendering (done) - provides rendering infrastructure
- SDL2 TTF for text rendering (need to add dependency)

## Rollback
Remove src/ui/ panel files and revert CMakeLists.txt changes.

## Known Risks
- SDL2_ttf dependency not yet in CMakeLists.txt
- Font rendering may need asset extraction from ROM
- Panel layout needs to match SNES original positioning

## Notes
- Use deterministic data from sim::City for panel content
- Follow existing rendering patterns in src/gfx/cityview.cpp
- Consider using ROM font extraction like tile graphics
