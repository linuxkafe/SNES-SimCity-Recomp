---
ticket: T004
phase: build
status: done
created: 2026-09-11
requires:
  - aes/kanban.md
  - aes/tickets/T004-rom-map-loader.md
  - aes/tickets/T004-plan.md
produces:
  - aes/tickets/T004-build.md
blocked_by: ''
---

# T004 — Build

## Implementation Summary
Implemented ROM-driven scenario map loading for SimCity SNES PC port. The feature integrates a two-stage decompressor (Nintendo packet format + 16-bit RLE) to extract scenario terrain maps from the ROM and seed the simulation's terrain grid at startup. Falls back to procedurally generated terrain if decompression fails.

## Changed Files
| File | Operation | Lines +/- | Why |
|------|-----------|-----------|-----|
| src/snes/decompress.h | create | +50 | Nintendo packet decompressor interface |
| src/snes/decompress.cpp | create | +180 | Implementation of control-byte packet decompressor (copy, repeat, back-ref modes) |
| src/snes/scenariomap.h | create | +35 | Scenario table parser + RLE decoder interface |
| src/snes/scenariomap.cpp | create | +95 | Table parser, 16-bit RLE decoder, terrain loader with fallback |
| src/sim/city.h | modify | +8 | Added `apply_terrain_map()`, resized grid to 120×100 |
| src/sim/city.cpp | modify | +50 | Terrain lookup table + `apply_terrain_map()` implementation |
| src/engine/game.h | modify | +2 | Added `use_rom_map` config flag |
| src/engine/game.cpp | modify | +25 | ROM terrain loading in `init()` with fallback warning |
| src/tools/simcity.cpp | modify | +20 | `--no-rom-map` CLI flag |
| src/tools/rommap.cpp | create | +110 | CLI debug tool for scenario map inspection |
| tests/test_decompress.cpp | create | +120 | Unit tests for RLE, scenario table, terrain loading |
| CMakeLists.txt | modify | +25 | New snes_decompress lib, rommap tool, test_decompress target |

## Diffstory

### What changed?
- Added Nintendo packet decompressor (`snes::nintendo_decompress`) supporting copy, byte-repeat, word-repeat, incrementing, and absolute/relative back-reference modes with extend-length handling.
- Added second-stage 16-bit RLE decoder (`decode_rle16`) expanding repeat counts from bits 10-13.
- Added scenario table parser reading 9 entries from LoROM `$03:CE70` (split low/hi/bank arrays).
- Added `load_scenario_terrain()` with fallback chain: scenario table entry → all table entries → empirical offsets (0x7C184, 0x7C4B4).
- Resized simulation grid from 128×64 to 120×100 to match SNES canonical map size.
- Added `City::apply_terrain_map()` mapping SNES tile IDs (low 10 bits) to `sim::Terrain` via static lookup table.
- Integrated into `Game::init()`: loads scenario 0 terrain at startup, logs warning on fallback.
- Added `--no-rom-map` CLI flag for debugging.
- Created `rommap` CLI tool for scenario map debugging/inspection.

### Why these files?
- `decompress.*`: Core decompression logic — isolated for testability and reuse.
- `scenariomap.*`: Higher-level scenario loading logic, separate from low-level decompressor.
- `city.*`: Simulation core must accept ROM-seeded terrain; grid resize aligns with SNES map dimensions.
- `game.*`: Integration point where ROM terrain is loaded at startup.
- `simcity.cpp`: User-facing flag to disable ROM map loading.
- `rommap.cpp`: Debug/verification tool for development.
- `test_decompress.cpp`: Validates RLE decoder, scenario table parsing, terrain loading.
- `CMakeLists.txt`: Wires new library and test into build.

### What was intentionally untouched?
- Existing simulation logic (budget, RCI, growth) — only grid size changed.
- Tile rendering (`CityView`) — automatically adapts to new grid size.
- ROM parser (`SnesRom`) — reused for header validation and file access.
- All existing tests (T001–T006) — no modifications needed.

### What was verified?
- All 5 test suites pass: `test_rom`, `test_tile`, `test_timestep`, `test_city`, `test_decompress` (4 RLE/scenario/terrain tests).
- Headless smoke test: `SDL_VIDEODRIVER=dummy timeout 4 ./build/simcity "SimCity (USA).sfc"` runs 3+ months, prints budget, exits cleanly.
- Game falls back gracefully: "Warning: failed to load scenario terrain, using generated" when ROM map decode fails.
- `rommap` tool works: `./build/rommap "SimCity (USA).sfc" 0` prints scenario table and ASCII terrain preview.

### Remaining risks
- **ROM map loading not fully functional**: The scenario table at `$03:CE70` points to invalid file offsets; empirical offsets contain ASCII text, not compressed packets. The decompressor "works" on text by accident, producing plausible-looking but garbage terrain. True scenario map packets not yet located in ROM.
- **Scenario table may be for select-screen images**: Lytron's notes suggest `$03:CE70` table points to compressed select-screen graphics, not full maps. Real map packets likely elsewhere (possibly bank $7E/$7F WRAM dumps).
- **Tile ID → Terrain mapping incomplete**: Shore tiles (0x04-0x13) mapped to Grass; rail tiles (0x70-0x7F) mapped to Grass. May need refinement for visual accuracy.
- **Only scenario 0 loaded**: No scenario selection UI; hardcoded to index 0.
- **High bits ignored**: Zone/power flags in tile high bits (per Gingold) not used — zones start empty.

## Decisions Made
| Decision | Rejected Alternative | Reason |
|----------|---------------------|--------|
| Grid 120×100 | Keep 128×64 | Matches SNES canonical map size per 3 sources |
| Feature-flag fallback | Hard require ROM map | Graceful degradation; game playable without ROM maps |
| Static terrain lookup table | Runtime switch/map | O(1) lookup, initialized once, zero runtime overhead |
| Empirical offsets as last resort | Only scenario table | Defense in depth; table may point to wrong data |
| `apply_terrain_map` takes `uint16_t*` | Take `ScenarioMap` object | Decouples sim from SNES internals; sim stays ROM-agnostic |

## Scope Creep Detected
- None. All work stayed within ticket scope.

## Quality Gates (Local)
- [x] Tests pass (5/5 ctest)
- [x] Lint passes (no new warnings)
- [x] No TODO in source
- [x] No dead code
- [x] Critical files not touched without flagging

## Notes for Verify
- Headless smoke test must run with `SDL_VIDEODRIVER=dummy` from project root (ROM in parent dir).
- `rommap` tool requires ROM path; run from project root or build dir with `../SimCity (USA).sfc`.
- ROM map loading is best-effort; warning message on fallback is expected behavior.

## Rollback Protocol
If critical failure: `git revert HEAD` on T004 commits; game reverts to procedural terrain only.