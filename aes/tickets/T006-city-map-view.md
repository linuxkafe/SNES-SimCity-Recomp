# T006 — City map rendering + edit tools

Status: pending
Priority: high
Depends on: T003, T005

## Goal
Make the game playable: render the `sim::City` grid on screen, add an
edit-tool UX (bulldoze, road, zone R/C/I) with mouse painting and a
scrolling camera, and run the monthly simulation on a pace timer.

## Scope
- `gfx::CityView`: renders the whole 128x64 map into an RGBA SDL texture
  (16 px/tile), re-uploaded only when dirty (edit or sim step). Draws the
  visible window given a camera offset. Maps screen coords to tiles.
- `Game` integration:
  - keys 1-5 select tools, SPACE pauses, arrows/WASD scroll
  - mouse click + drag paints the selected tool
  - one `City::step_month()` per simulated second (pausable)
  - camera clamped to map bounds
- RCI demand drawn as three height bars in the HUD (no font dependency);
  budget line printed to the terminal each month.

## Acceptance criteria
- [AC1] User can paint roads and zones with the mouse; painted cells appear
         on screen within the same frame.
- [AC2] Camera pan (arrows) and clamp verified; reaching map edges does not
         crash.
- [AC3] Population grows over months when zones + road are drawn, and the
         terminal shows funds/population changes.
- [AC4] Headless smoke test: `simcity` runs with `SDL_VIDEODRIVER=dummy` and
         a forced quit, exiting 124 via timeout without crashing.
- [AC5] `ctest` still passes 4/4 (no sim logic regressions).

## Design notes
- Game keeps a `dirty_` flag; CityView rebuilds the map texture only on
  edits/sim steps, never per-frame.
- Text-free HUD: R/C/I demand bars + pause indicator.