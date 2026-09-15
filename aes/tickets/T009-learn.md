---
ticket: T009
phase: learn
status: done
created: 2026-09-13
requires:
  - aes/kanban.md
  - aes/tickets/T009-ui-panels.md
  - aes/tickets/T009-plan.md
  - aes/tickets/T009-build.md
  - aes/tickets/T009-verify.md
  - aes/tickets/T009-review.md
produces:
  - aes/tickets/T009-learn.md
side_effects:
  - updates aes/sprints/sprint-02.md (learning section)
  - updates aes/kanban.md (Learning History)
  - creates aes/shadow/SD-META-001.md, SD-META-002.md, SD-DOMAIN-001.md
blocked_by: ''
---

# T009 — Learn

## What Was Done (2 sentences)
Implemented three UI panels (Budget, Population, RCI) with a FontRenderer abstraction. The MVP uses RectFontRenderer (6×8 colored rectangles per character); all formatting logic is headless-testable.

---

## Feynman Method

### For a Child
Imagine you're playing SimCity and you want to know how much money your city has, how many people live there, and whether people want more houses, shops, or factories. The game needs to show you this information on the screen. We built three "info boards" that float on top of the city map: one shows your money (Budget), one shows your population (Population), and one shows demand bars for houses/shops/factories (RCI). To draw the numbers and letters on these boards, we made a "letter drawer" that uses tiny colored squares (like Lite-Brite pegs) because we don't have a real font yet. Later, we'll swap in the real SimCity font from the game cartridge.

### For an Expert
Three-panel UI system with `FontRenderer` strategy pattern. `RectFontRenderer` provides zero-dependency glyph rendering (6×8 rects/char, 1px spacing). Panels (`BudgetPanel`, `PopulationPanel`, `RciPanel`) inherit `Panel` base class, render in screen-space (camera-independent), consume `sim::Stats` read-only. All formatting (comma-separated thousands, signed income, demand→bar width) tested headless in `test_ui.cpp`. Integration via `Game::draw_hud()` delegating to panel instances; preserves existing status bar, disaster alert, tool indicator. No SDL2_ttf dependency. FontRenderer abstraction enables T019 (ROM bitmap font) swap without panel code changes.

### Chain of Whys
*For the most important decision: FontRenderer abstraction + RectFontRenderer MVP*

- Why FontRenderer abstraction?
  → To decouple panel rendering from font implementation, enabling ROM font swap (T019) without touching panel logic.
- Why decouple?
  → Because font extraction from ROM is a separate, complex task (1bpp/4bpp format, unknown offsets, palette mapping) that should not block UI panel implementation.
- Why not SDL2_ttf?
  → Adds external dependency, requires distributing a font file, won't match SNES pixel aesthetic. Zero-dep MVP aligns with project constraints.
- Why RectFontRenderer specifically?
  → Simplest possible implementation: 6×8 rects = 48px glyph area, legible enough for development, zero deps, matches "programmer art" aesthetic.
- Why this glyph size?
  → Fits within panel line height (14px) with room for descenders; 6px width allows ~33 chars in 200px panel.

---

## First Principles

### Challenged Assumptions
| Assumption | Was it fact or habit? | What we discovered |
|-----------|----------------------|-------------------|
| Panels need SDL2_ttf for text | Habit (common in SDL projects) | False — rect glyphs work for MVP; abstraction enables ROM font later |
| RCI demand bars must be vertical | Habit (original SNES HUD) | Horizontal bars work better for screen-space layout; more readable at 60px height |
| Panels must be in CityView (world-space) | Assumption | False — panels are UI, not map features; screen-space avoids camera shake issues |
| Format functions need SDL to test | Habit | False — pure string formatting is 100% headless-testable |

### The Real Problem
Not "draw text on screen" but "display simulation state to player in a way that's testable, swappable, and doesn't couple to rendering details." The font is an implementation detail; the panel data contract (`sim::Stats`) is the stable interface.

### If We Started Today
- Would extract `PanelLayout` concept for responsive positioning (currently hardcoded).
- Would add `draw_text_center` to FontRenderer for future centered labels.
- Would make panel dimensions configurable via `Game::Config` instead of hardcoded constants.

---

## Hostile Audit

### Where Our Learnings Fail
- **FontRenderer abstraction assumes all fonts are monospace** — RectFontRenderer is monospace; SNES font is likely fixed-width but may have variable glyph widths. If ROM font has kerning, the interface needs extension.
- **RectFontRenderer legibility threshold unknown** — "Barely legible" is subjective. At 6×8, numbers 0-9 are distinguishable but 8 vs B, 5 vs S may confuse. No user testing done.
- **Panel layout assumes landscape orientation** — Portrait or ultra-wide would break hardcoded positions.
- **Demand width mapping (Low=1/6, Medium=1/2, High=1) is arbitrary** — Based on SNES visual memory, not measured from ROM. May not match original.

### What We Do Not Know That We Do Not Know
- Exact SNES font format (1bpp? 4bpp? interleaved with palettes?)
- Whether ROM font includes Latin-1 accented chars (for translations)
- How SNES handled panel background transparency (alpha? mask? opaque?)
- If RCI bar colors match original exactly (used memory: yellow/blue/purple)

### Experiments Needed
- Run `./build/tileview` on ROM to locate font tiles and inspect format
- Compare RCI bar colors against SNES screenshots (pixel-perfect)
- Test RectFontRenderer at 1280×720 — does 6×8 scale acceptably?

---

## Decisions We Would Change
- `PopulationPanel::bounds()` returns x=0 (computed at render time) → would return computed bounds or add `update_bounds(win_w, win_h)` call before render.
- Hardcoded panel dimensions → would extract to `PanelConfig` struct in `Game::Config`.
- Missing `test_panel_bounds()` test (planned but not implemented) → would add.

## What Went Well
- FontRenderer abstraction: clean, minimal, enables T019 without panel refactor.
- Headless tests: 7 tests cover all formatting edge cases, run in <0.01s, zero flakiness.
- Surgical integration: `draw_hud()` refactor preserved all existing HUD elements.
- Zero new dependencies: builds on existing SDL2 only.

## What Went Wrong
- Forgot `test_panel_bounds()` test (listed in plan, not implemented).
- `PopulationPanel::bounds()` is misleading (returns x=0).
- No responsive layout consideration — known risk accepted but not documented in code (no TODO).

## Shadow Docs Created
| ID | Title | Category |
|----|-------|----------|
| SD-META-001 | FontRenderer abstraction enables zero-dep MVP with future ROM font swap | META |
| SD-META-002 | Headless-testable formatting logic should be standard for all UI data display | META |
| SD-DOMAIN-001 | RCI demand bars work better horizontal at 60px than vertical at 200px | DOMAIN |

