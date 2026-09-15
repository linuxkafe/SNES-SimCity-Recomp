---
ticket: T009
phase: build
status: done
created: 2026-09-13
requires:
  - aes/kanban.md
  - aes/tickets/T009-ui-panels.md
  - aes/tickets/T009-plan.md
produces:
  - aes/tickets/T009-build.md
blocked_by: ''
---

# T009 — Build

## Implementation Summary
Implemented three UI panels (Budget, Population, RCI) with a FontRenderer abstraction for text rendering. The MVP uses a RectFontRenderer that draws characters as colored rectangles (6x8 px each). Panels are screen-space widgets that render on top of the city view and update each simulation month. All formatting logic is headless-testable.

## Changed Files
| File | Operation | Lines +/- | Why |
|------|-----------|-----------|-----|
| src/ui/panel.h | created | +42 | Base classes: Panel, FontRenderer |
| src/ui/rect_font_renderer.h | created | +28 | RectFontRenderer interface |
| src/ui/rect_font_renderer.cpp | created | +39 | RectFontRenderer implementation |
| src/ui/budget_panel.h | created | +34 | BudgetPanel declaration |
| src/ui/budget_panel.cpp | created | +120 | BudgetPanel implementation |
| src/ui/population_panel.h | created | +32 | PopulationPanel declaration |
| src/ui/population_panel.cpp | created | +62 | PopulationPanel implementation |
| src/ui/rci_panel.h | created | +39 | RciPanel declaration |
| src/ui/rci_panel.cpp | created | +72 | RciPanel implementation |
| tests/test_ui.cpp | created | +117 | Headless unit tests for formatting |
| CMakeLists.txt | modified | +18/-2 | Add simcity_ui library, test_ui |
| src/engine/game.h | modified | +10/-0 | Include UI headers, add panel members |
| src/engine/game.cpp | modified | +15/-25 | Init panels, replace draw_hud() with panel rendering |

## Diffstory

### What changed?
- Created `src/ui/` directory with 5 new source files implementing the panel system
- Added `FontRenderer` abstract interface with `RectFontRenderer` MVP implementation (colored rects as glyphs)
- Implemented `BudgetPanel` (funds, income, expenses, tax), `PopulationPanel` (pop, R/C/I zones, jobs), `RciPanel` (three horizontal demand bars)
- Refactored `Game::draw_hud()` to delegate to panel system while preserving top status bar and disaster alert
- Added `test_ui.cpp` with 7 headless tests covering number formatting and demand width calculations
- Updated CMakeLists.txt: new `simcity_ui` library linked into `simcity_engine` and `test_ui`

### Why these files?
- `panel.h` — Core abstraction enabling future RomFontRenderer swap (T019)
- `rect_font_renderer.*` — Zero-dependency text rendering for MVP
- `budget_panel.*` / `population_panel.*` / `rci_panel.*` — Three distinct panels matching SNES UI layout
- `test_ui.cpp` — Verifies formatting logic without SDL (fast, deterministic)
- `game.h/.cpp` — Integration point; panels instantiated once in init(), rendered each frame
- `CMakeLists.txt` — Build system wiring

### What was intentionally untouched?
- **Top status bar** (22px black bar at y=0) — preserved from original `draw_hud()`
- **Disaster display rect** (red rect at y=24) — preserved, not moved into panel system
- **Tool indicator** (console stdout) — preserved, not moved to on-screen UI
- **CityView rendering** — unchanged; panels overlay on top
- **SDL_RENDERER_SOFTWARE** — no renderer change
- **RomAssets extraction** — no font extraction yet (deferred to T019)

### What was verified?
- All 6 test suites pass (test_rom, test_tile, test_timestep, test_city, test_decompress, test_ui)
- test_ui covers: funds formatting (positive, negative, millions), income formatting (signs, commas), tax rate, population formatting, RCI demand widths, integration with sim::City
- Headless smoke test: `SDL_VIDEODRIVER=dummy timeout 3 ./build/simcity "SimCity (USA).sfc"` runs 1800+ frames without crash
- No regressions in existing simulation tests

### Remaining risks
- **Font readability**: RectFontRenderer produces 6x8 blocks — barely legible. Real font extraction (T019) needed for shippable UX.
- **Panel positioning**: Hardcoded for 800×600; may overlap on smaller windows. No responsive layout yet.
- **No interaction**: Panels are read-only. Buttons/sliders for tax rate, tool selection deferred to T013.
- **Window resize**: Panels don't reposition on resize; would need `SDL_WINDOWEVENT` handling.

## Decisions Made
| Decision | Rejected Alternative | Reason |
|----------|---------------------|--------|
| FontRenderer abstraction + RectFontRenderer MVP | SDL2_ttf + system font | Zero deps, matches SNES pixel aesthetic, swappable for ROM font |
| Three separate panel classes | Single combined panel | Matches SNES UI regions; independent layout/updates |
| Headless formatting tests | SDL-dependent visual tests | Fast, deterministic, CI-friendly |
| Panels in Game class (not CityView) | Panels in CityView | Panels are screen-space (camera-independent), not world-space |

## Scope Creep Detected
- [ ] None

## Quality Gates (Local)
- [x] Tests pass (6/6)
- [x] Lint passes (no warnings in new code)
- [x] No TODO in source
- [x] No dead code
- [x] No critical files touched without flagging (CLAUDE.md critical files: sim/city.h, sim/city.cpp — untouched)

## Notes for Verify
- Verify panel visibility at different window sizes (800×600, 1280×720)
- Verify RCI bars update correctly when demand changes (Low→Medium→High)
- Verify budget panel shows negative funds correctly (red income color)
- Verify panels don't flicker (rebuild only when dirty_)

---

Build complete for T009.
Output: aes/tickets/T009-build.md
Diffstory: included
Local quality gates: passed
Next step: /aes-verify
