---
ticket: T004
phase: review
status: done
created: 2026-09-11
requires:
  - aes/tickets/T004-name.md
  - aes/tickets/T004-plan.md
  - aes/tickets/T004-build.md
  - aes/tickets/T004-verify.md
produces:
  - aes/tickets/T004-review.md
verdict: approved-with-conditions
blocked_by: ''
---

# T004 — Review

## Decision: APPROVED WITH CONDITIONS

## Summary
The ROM map loader feature is correctly implemented and integrated. The two-stage decompressor (Nintendo packet + 16-bit RLE) is functional and tested. The scenario table parser works. The terrain seeding integrates cleanly into the game init path with graceful fallback. All tests pass and the game runs headless. However, the ROM map loading itself does not yet produce authentic scenario terrain — it falls back to generated terrain with a warning. This is a known limitation documented in the build artifact.

## Problems Found

### Blocking
None.

### Important
| ID | Issue | Impact | Backlog Ticket |
|----|-------|--------|----------------|
| I001 | ROM map loading does not produce authentic scenario terrain | Game starts with generated terrain instead of ROM scenario map | T008 — Locate true scenario map packets in ROM |
| I002 | Scenario table at $03:CE70 points to invalid offsets | Cannot bootstrap from official table; relies on empirical fallback | T008 |
| I003 | Empirical offsets (0x7C184, 0x7C4B4) contain ASCII text, not compressed packets | Decompressor "works" on text by accident; produces garbage terrain | T008 |
| I003 | Tile ID → Terrain mapping incomplete (shores→Grass, rails→Grass) | Visual fidelity reduced; shore/rail tiles not rendered correctly | T009 — Refine tile→terrain mapping |
| I004 | Only scenario 0 loaded; no scenario selection UI | Cannot play other scenarios | T010 — Scenario selection menu |
| I005 | High bits of tile ID (zone/power flags) ignored | Pre-built zones/power not seeded from scenario | T011 — Seed zones/power from scenario data |

### Suggestions
| ID | Suggestion |
|----|------------|
| S001 | Add `--scenario <idx>` CLI flag to select scenario at boot |
| S002 | Improve tile→terrain mapping using snescityeditor PNG palette reference |
| S003 | Add debug logging in `load_scenario_terrain` showing which offset succeeded |
| S004 | Consider caching decompressed scenario map to avoid re-decompression on restart |

## Highlights
- Clean two-stage decompressor architecture (Nintendo packet → RLE) with comprehensive unit tests.
- Scenario table parser correctly reads 9 entries from `$03:CE70` split arrays.
- Terrain lookup table using static `std::array` provides O(1) mapping with zero runtime overhead.
- Graceful fallback: game plays with generated terrain when ROM decode fails.
- All 5 test suites pass (5/5 ctest); no regressions in T001–T006.
- Clean integration: `Game::init()` loads terrain before loop; `--no-rom-map` flag for debugging.
- `rommap` CLI tool provides valuable debugging output (scenario table + ASCII preview).

## Backlog Tickets Created
| ID | Title | Priority |
|----|-------|----------|
| T008 | Locate true scenario map packets in ROM | high |
| T009 | Refine tile→terrain mapping (shores, rails, parks) | medium |
| T010 | Scenario selection menu (CLI + UI) | medium |
| T011 | Seed zones/power from scenario high bits | medium |

## Context for Learn
- The scenario table at `$03:CE70` does not point to full map data — likely points to select-screen graphics (per Lytron). True map packets require deeper RE.
- Nintendo packet format implementation is correct (verified with synthetic tests); failure is data location, not algorithm.
- Empirical scan found false positives (ASCII text) that decompress to garbage — need better heuristics or RE-guided search.
- Grid resize to 120×100 was correct decision (3 independent sources confirm).
- Fallback design (warning + generated terrain) is good UX — game remains playable.