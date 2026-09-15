---
ticket: T002
title: Graphics extraction + tile renderer
sprint: sprint-01
priority: high
status: pending
created: 2026-09-11
---

# T002 — Graphics extraction + tile renderer

## Context
SimCity SNES uses SNES Mode 1 with 4bpp tiles. We need to decode 4bpp tile data
to RGB pixels, create a basic SDL2 window, and be able to view tiles from the ROM
at any offset. This is the foundation for displaying game graphics.

## Acceptance Criteria
- [ ] 4bpp tile decoder: 32 bytes → 8x8 RGB pixel array
- [ ] SDL2 window opens and renders tiles from ROM at given offset
- [ ] rominfo extended: `rominfo <rom> --tiles <offset> [count]` renders PNG/raw
- [ ] Unit tests: known tile patterns decode correctly (synthetic 4bpp data)
- [ ] No game ROM assets distributed — only reads from user-provided ROM at runtime

## Scope
**In scope:** 4bpp decode, palette decode (SNES 15-bit RGB), SDL2 window, tile viewer tool.
**Out of full game tilemap/sprites (T006), PPU emulation, game logic.

## Dependencies
- T001 (ROM parser)

## Rollback
Remove src/gfx/, src/viewer/, tests/test_tile.cpp

## Known Risks
- Which ROM offsets contain tiles is unknown without debugger analysis
- Tile viewer is manual discovery tool for now

## Notes
SNES 4bpp tile = 32 bytes, 8 rows × 4 planes interleaved (Lo,Hi pairs).