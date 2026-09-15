---
ticket: T009
phase: review
status: done
created: 2026-09-13
requires:
  - aes/tickets/T009-ui-panels.md
  - aes/tickets/T009-plan.md
  - aes/tickets/T009-build.md
  - aes/tickets/T009-verify.md
produces:
  - aes/tickets/T009-review.md
verdict: approved-with-conditions
blocked_by: ''
---

# T009 — Review

## Decision: APPROVED WITH CONDITIONS

## Summary
The UI panel system is correctly implemented with a clean FontRenderer abstraction that enables future ROM font extraction. All acceptance criteria met: Budget, Population, and RCI panels render and update each simulation step. Headless tests verify all formatting logic. No regressions. Two IMPORTANT items found for next sprint.

## Problems Found

### Blocking
None.

### Important

1. **Panel layout not responsive** — Panels use hardcoded positions for 800×600. On window resize, panels don't reposition. At smaller sizes they may overlap or be cut off.
   - **Backlog ticket**: T020 — Responsive panel layout (priority: medium)
   - **Why not blocking**: MVP scope explicitly excluded resize handling; game currently runs at fixed 800×600 default.

2. **RectFontRenderer legibility** — 6×8 colored rectangles per character are barely readable. Users cannot easily read numbers.
   - **Backlog ticket**: T019 — ROM bitmap font extraction (priority: high, already in backlog)
   - **Why not blocking**: FontRenderer abstraction was designed specifically for this swap; RectFontRenderer is explicitly MVP placeholder.

### Suggestions

1. **Extract panel constants to config** — Panel dimensions (200×110, 200×100, 300×60) and colors are hardcoded. Consider a `PanelConfig` struct or constants header for easier theming.

2. **Add panel bounds clamping** — `PopulationPanel::bounds()` returns x=0 because it computes at render time. Consider computing once in render and storing for potential hit-testing.

3. **Consider `draw_text_center`** — FontRenderer has left/right alignment but not center. May be useful for future centered labels.

## Highlights
- **FontRenderer abstraction** is well-designed: single interface with 4 methods, two implementations planned (Rect + ROM), zero coupling to SDL_ttf.
- **Headless-testable formatting**: All number formatting, demand width, population formatting tested without SDL in `test_ui.cpp`. Fast, deterministic.
- **Surgical integration**: `Game::draw_hud()` refactored to delegate to panels while preserving status bar, disaster alert, tool indicator. No simulation logic touched.
- **Zero new dependencies**: No SDL2_ttf, no external fonts. RectFontRenderer uses only SDL_Renderer.
- **Deterministic simulation preserved**: Panels read from `sim::City::stats()` — pure data, no side effects.

## Backlog Tickets Created
| ID | Title | Priority |
|----|-------|----------|
| T019 | ROM bitmap font extraction | high |
| T020 | Responsive panel layout (window resize) | medium |

## Context for Learn
- **Trade-off accepted**: Chose RectFontRenderer MVP over SDL2_ttf to avoid external dependency and match SNES pixel aesthetic. The abstraction makes the future swap clean.
- **Panel architecture**: Three separate classes rather than one monolithic HUD enables independent layout and testing. Matches SNES UI regions.
- **What would change with more time**: Responsive layout system, animated RCI bar transitions, tax rate slider in budget panel.
- **Process note**: The `panel.h` base class with `FontRenderer` interface was the key architectural decision that enables T019 without refactoring panel rendering logic.

---

Review complete for T009: APPROVED WITH CONDITIONS
Next step: /aes-learn
