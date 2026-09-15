---
ticket: T021
phase: verify
status: done
created: 2026-09-14
requires:
  - aes/tickets/T021-plan.md
produces:
  - aes/tickets/T021-verify.md
---

# T021 — Verify: Title Screen ROM Logo Extraction

## Test Results

### Unit Tests (ctest)
All 9 test suites pass:
- test_rom: ✓
- test_tile: ✓
- **test_title: ✓** (validates packet sizes, palette integrity, tilemap bounds, rendered content)
- test_font: ✓
- test_building_sprites: ✓
- test_timestep: ✓
- test_city: ✓
- test_decompress: ✓
- test_ui: ✓

### Acceptance Criteria Verification

| AC | Status | Evidence |
|----|--------|----------|
| Locate title screen logo/tilemap in ROM | ✅ | Disassembly addresses used: GFX 0x07C9E0/0x07A680/0x07C930, TM 0x0B966B/0x0B942B/0x0B9224, PAL 0x0C89D8 |
| Extract logo graphics at runtime (LC_LZ5) | ✅ | `load_title_screen()` decompresses 6 packets via `nintendo_decompress()` |
| Render title screen using extracted tiles + correct palette | ✅ | `render_title_screen()` composites 3 layers (L3→L1→L2) with index-0 transparency |
| Remove invented colored rectangles from render_title() | ✅ | `game.cpp:355-365` now blits SDL texture built from ROM data |
| Title screen auto-advances to menu after delay | ✅ | `update_title()` advances after 150 frames (~3s) |
| 9/9 ctest pass | ✅ | All test suites pass |
| Headless smoke test passes | ✅ | `SDL_VIDEODRIVER=dummy timeout 3 ./build/simcity "SimCity (USA).sfc"` exits cleanly after 649 frames |

### Headless Smoke Test Output
```
Loaded ROM asset tiles+palettes (palette block=5 sub=0)
Loaded ROM title screen
Loaded ROM font
loaded: SIMCITY (LoROM, ROM + RAM + Battery, 512 KB)
frames: 649
```

### Lint / Static Analysis
- Build completes with no errors (fixed missing `trigger_earthquake` declaration in city.h)
- No compiler warnings introduced

## Verification Bundle
- Test output logged above
- No pre-existing failures masked
- All acceptance criteria objectively verified

## Verdict
**PASS** — T021 implementation meets all acceptance criteria and quality gates.