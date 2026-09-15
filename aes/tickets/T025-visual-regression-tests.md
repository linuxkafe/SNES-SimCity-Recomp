---
ticket: T025
title: Visual Regression Tests — Headless pixel assertions
sprint: sprint-02
priority: medium
status: pending
created: 2026-09-14
source: T020 process violation P4
---

# T025 — Visual Regression Tests

## Context
T020 audit detected process violation P4: test suites validate formatters/decompress only — nothing tests the visual output path (title/menu/palette/tile pairing). This ticket adds headless pixel-assertion tests to catch visual regressions.

## Acceptance Criteria
- [ ] Headless rendering test for title screen (non-black, has logo pixels)
- [ ] Headless rendering test for menu (has text pixels, not just rectangles)
- [ ] Headless rendering test for terrain (green land, blue water)
- [ ] Headless rendering test for zones (building sprites, not flat colors)
- [ ] Palette validation test (block 5/sub 0 = green, sub 1 = blue)
- [ ] Tests run in ctest suite
- [ ] 6/6 ctest pass

## Scope
**In scope:** Pixel-assertion tests, headless rendering validation
**Out of scope:** Title screen (T021), menu (T022), building sprites (T023), disasters (T024)

## Dependencies
- T021-T024 should be done first (tests validate their output)
- SDL_VIDEODRIVER=dummy for headless rendering

## Known Risks
- Pixel-perfect tests are fragile (palette differences, rendering order)
- Need reference images or heuristic assertions
- Headless rendering may differ from windowed rendering

## Notes
- Use existing verification binary pattern from T020-verify.md
- Store reference images outside repo (zero asset distribution)
- Consider fuzzy matching (±1 color channel tolerance)
