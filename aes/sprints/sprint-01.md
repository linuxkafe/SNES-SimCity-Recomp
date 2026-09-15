---
sprint: sprint-01
period: 2026-09-11 → 2026-09-18
status: active
---

# Sprint 01 — Foundation + Core Simulation + Playable Map

## Goal
Establish AES project scaffolding, ROM parser, graphics extraction, game loop, deterministic simulation core, playable map editor, and ROM-driven scenario terrain loading.

## Tickets
| ID | Title | Status |
|----|-------|--------|
| T001 | Project scaffolding + ROM parser | done |
| T002 | Graphics extraction + tile renderer | done |
| T003 | Basic game loop + window | done |
| T005 | City simulation core | done |
| T006 | City map rendering + edit tools | done |
| T004 | ROM map loader (scenario terrain) | in-progress |

## Retrospective
*Filled at end of sprint.*

### What went well
- AES scaffolding established early; all phases documented.
- Two-stage decompressor architecture is clean and testable.
- Scenario table parser works correctly per documentation.
- Graceful fallback keeps game playable when ROM decode fails.
- All tests pass; no regressions.

### What went wrong
- Spent significant time on false-positive empirical offsets (ASCII text).
- Did not validate decompressed output structure before accepting.
- Relied on documentation that described select-screen graphics, not playable maps.
- Tile→terrain mapping incomplete; high bits discarded.

### What to change next sprint
- Trace COP #08 decompression routine in emulator to locate true map packets.
- Add structural validator for decompressed maps (water borders, land contiguity).
- Improve tile→terrain mapping using snescityeditor reference.
- Preserve high bits of tile ID for future zone/power seeding.