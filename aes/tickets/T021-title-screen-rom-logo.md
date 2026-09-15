---
ticket: T021
title: Title Screen — Extract and render ROM logo
sprint: sprint-02
priority: high
status: pending
created: 2026-09-14
source: T020 finding F1
---

# T021 — Title Screen (ROM Logo)

## Context
T020 audit confirmed F1: the title screen is invented (colored rectangles in `game.cpp:330-374`). The original SNES SimCity has a title screen with the game logo extracted from ROM graphics. This ticket replaces the placeholder with ROM-extracted title graphics.

## Acceptance Criteria
- [ ] Locate title screen logo/tilemap in ROM (disassembly reference or brute-force)
- [ ] Extract logo graphics at runtime (LC_LZ5 decompression)
- [ ] Render title screen using extracted tiles + correct palette
- [ ] Remove invented colored rectangles from `render_title()`
- [ ] Title screen auto-advances to menu after delay (existing behavior)
- [ ] 6/6 ctest + headless smoke pass

## Scope
**In scope:** Logo extraction, title screen rendering, palette selection
**Out of scope:** Menu text (T022), building sprites (T023), audio

## Dependencies
- ROM "SimCity (USA).sfc" (user-supplied)
- Existing LC_LZ5 decompressor (`snes/decompress.cpp`)

## Known Risks
- Title graphics may use different compression or format than city tiles
- Palette for title screen may differ from city layer palette
- Logo may be multi-layered (background + foreground)

## Notes
- Reference: Yoshifanatic1/SimCity-SNES-Disassembly for asset pointers
- Title screen likely uses BG Mode 1 with specific tilemap
- May need to extract title-specific palette from CGRAM init code
