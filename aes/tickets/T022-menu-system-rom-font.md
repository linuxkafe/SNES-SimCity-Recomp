---
ticket: T022
title: Menu System — ROM font extraction + text rendering
sprint: sprint-02
priority: high
status: done
created: 2026-09-14
source: T020 finding F2
---

# T022 — Menu System (ROM Font + Text)

## Context
T020 audit confirmed F2: the main menu is invented (placeholder rectangles in `game.cpp:376-422`). `rect_font_renderer.cpp:17-24` paints every character as identical rectangles. This ticket extracts the ROM font and renders menu text.

## Research Summary
- No dedicated "FONT" asset in AssetPointersAndFiles.asm
- City tile bank (1024 tiles at $07E584) contains two-color glyph-like patterns at IDs 680-689 (indices 3+4 = outline+fill, classic SNES font style)
- Sprite bank ($07A680, 512 tiles) has no clean alphabet
- BankLoanText tilemap (Layer3) uses tile IDs 657-728; frame=704-728, low-count tiles (24,44,80-107,657,675,685) may be text glyphs but appear solid in isolation
- Will Wright message screens are large pre-rendered sprites, not a reusable font
- Practical approach: Use city tile bank IDs 680-695 as font glyphs; they're already loaded in RomAssets

## Acceptance Criteria
- [ ] Extract font glyph data from city tile bank (already in RomAssets)
- [ ] Implement FontRenderer using RomAssets tiles + palette
- [ ] Replace placeholder rectangles in `render_menu()` with actual text
- [ ] Menu options: New City, Load City, Scenario, Practice, Quit
- [ ] Selection indicator works (keyboard navigation)
- [ ] 7/7 ctest + headless smoke pass

## Plan (AES Phase 1 → 2)
1. **FontData struct** in `src/gfx/font.h`: holds glyph mapping (ASCII → tile ID), uses existing RomAssets
2. **FontRenderer** in `src/ui/font_renderer.h/cpp`: blits glyph tiles using CityView's pattern
3. **Wire into Game**: replace `rect_font_renderer_` with `FontRenderer`, update `render_menu()`
4. **Tests**: add `test_font.cpp` headless verification
5. **Cleanup**: remove `rect_font_renderer.*` if unused elsewhere

## Known Risks
- Glyph→ASCII mapping may be incomplete (fallback to generated for missing)
- Menu text palette may differ from terrain palette (block 5, sub 0)
- Variable-width vs fixed-width: SNES SimCity uses fixed 8×8

## Dependencies
- T021 done (title screen extraction pattern established)
- RomAssets already loads city tiles + palettes

---

## Phase 2 Spec (Build)
### Files to create
- `src/gfx/font.h` / `src/gfx/font.cpp` — FontData + extraction
- `src/ui/font_renderer.h` / `src/ui/font_renderer.cpp` — FontRenderer
- `tests/test_font.cpp` — headless test
### Files to modify
- `CMakeLists.txt` — add font.cpp, font_renderer.cpp, test_font
- `src/engine/game.h` — replace RectFontRenderer with FontRenderer
- `src/engine/game.cpp` — init FontRenderer, render_menu() uses it
- `src/ui/rect_font_renderer.h/cpp` — deprecate/remove