---
ticket: T021
phase: plan
status: done
created: 2026-09-14
tier: standard
requires:
  - aes/kanban.md
  - aes/tickets/T021-title-screen-rom-logo.md
produces:
  - aes/tickets/T021-plan.md
blocked_by: ''
---

# T021 — Plan: Title Screen ROM Logo Extraction

## Reconnaissance Summary

The codebase already contains a complete implementation for extracting and rendering the SNES SimCity title screen from ROM:

**Existing files relevant to T021:**
- `src/gfx/titlescr.h` / `src/gfx/titlescr.cpp` — Title screen data structures, LC_LZ5 decompression, and rendering (3 layers: atmosphere/stars, skyline, logo)
- `src/engine/game.cpp` — Integration: loads title screen at startup (lines 74-84), builds SDL texture (lines 367-381), renders in TitleScreen state (lines 355-365)
- `tests/test_title.cpp` — Comprehensive regression test validating packet sizes, palette integrity, tilemap bounds, and rendered frame content

**Disassembly references used (from Yoshifanatic1/SimCity-SNES-Disassembly):**
- Layer1 GFX: 0x07C9E0 (4bpp, 384 tiles)
- Layer2 GFX: 0x07A680 (4bpp sprite bank, 512 tiles)
- Layer3 GFX: 0x07C930 (2bpp, 32 tiles)
- Layer1 TM: 0x0B966B (64x64)
- Layer2 TM: 0x0B942B (32x64)
- Layer3 TM: 0x0B9224 (32x64)
- Palette: 0x0C89D8 (raw BGR555, 128 colors = 8 sub-palettes × 16)

## Hostile Analysis

**ASSUMPTIONS I AM MAKING:**
- [KNOWN] ROM addresses from disassembly are correct — validated by successful decompression and pixel census in test
- [KNOWN] LC_LZ5 decompressor works for title packets — same code path as scenario maps (T004/T017)
- [KNOWN] 4bpp tile format = two 2bpp planes interleaved (bytes 0-15 planes 0/1, 16-31 planes 2/3) — confirmed by entropy analysis in T020
- [INFERRED] Title screen uses SNES BG Mode 1 with 3 layers + fixed palettes — matches hardware constraints
- [ASSUMED] Palette at 0x0C89D8 is the complete title screen CGRAM — no runtime palette swaps during title
- [UNKNOWN] Whether title screen has animated elements (star twinkle) — SNES version may use HDMA, not implemented

**WHAT WAS NOT SPECIFIED (that matters):**
- Animated title elements (stars twinkle) — not in acceptance criteria
- Transition effect to menu — not specified, current simple timer is acceptable
- Sound/music for title screen — T010 (audio) is backlog

**ALTERNATIVES NOT CHOSEN:**
- Brute-force search for title assets — rejected because disassembly provides exact addresses
- Separate title palette per layer — rejected; single 128-color palette matches CGRAM layout

**RISKS AND SIDE EFFECTS:**
- If user supplies wrong ROM (non-USA), addresses may differ → graceful fallback to black screen (current behavior)
- Palette index 0 treated as transparent — matches SNES BG behavior where backdrop color is palette[0]

**COST OF BEING WRONG: medium** — title screen is first user impression; failure shows black screen but doesn't crash

**SCOPE BOUNDARY:**
In scope: Logo extraction, 3-layer composition, palette application, SDL texture creation, render integration
Out of scope: Title animation, audio, menu text (T022), font extraction (T019)

## Technical Approach

**Chosen approach:** Runtime extraction from user-supplied ROM using known CPU addresses from disassembly. Decompress three graphics packets + three tilemap packets + raw palette. Compose three BG layers back-to-front with index-0 transparency. Build SDL texture once at startup, stretch to window.

**Why this approach:**
- Zero asset distribution (legal requirement)
- Deterministic: same ROM always produces same title frame
- Leverages existing LC_LZ5 decompressor (no new code)
- Matches SNES BG Mode 1 rendering semantics

## Affected Files

| File | Operation | Description |
|------|-----------|-------------|
| src/gfx/titlescr.h | create | TitleScreenData struct, load/render declarations |
| src/gfx/titlescr.cpp | create | LC_LZ5 decompression, 4bpp/2bpp decode, layer blitting, palette load |
| src/engine/game.cpp | modify | Load title screen in init(), build_title_texture(), render_title() uses texture |
| tests/test_title.cpp | create | Full regression: packet sizes, palette distinctness, tilemap bounds, rendered content |

## Specification

**TitleScreenData struct:**
- layer1_tiles: 384 × 32-byte 4bpp tiles (skyline)
- layer1_tm: 64×64 tilemap entries
- layer2_tiles: 512 × 32-byte 4bpp tiles (logo/sprites)
- layer2_tm: 32×64 tilemap entries
- layer3_tiles: 32 × 16-byte 2bpp tiles (stars/atmosphere)
- layer3_tm: 32×64 tilemap entries
- palette[128]: raw BGR555 colors (8 sub-palettes × 16)
- valid: boolean flag set only when all packets decode

**Interface:**
```cpp
bool load_title_screen(const snes::SnesRom& rom, TitleScreenData& data);
void render_title_screen(const TitleScreenData& data, Image& out);
```

**Game integration:**
- Called in Game::init() when cfg.use_rom_map && !cfg.rom_path.empty()
- On success: build_title_texture() creates SDL_TEXTUREACCESS_STATIC RGBA32 texture
- render_title() stretches texture to window with nearest-neighbor (SDL_HINT_RENDER_SCALE_QUALITY=0)

## Testing Strategy

**Unit tests (test_title.cpp):**
- ROM loads and all 6 packets decompress to expected byte counts
- Palette contains non-zero colors with >64 distinct entries
- Tilemap tile IDs stay within their respective tile bank bounds
- Rendered frame: lower quarter has >2000 non-background pixels (proves skyline renders)

**Integration:**
- Headless smoke test: `SDL_VIDEODRIVER=dummy timeout 3 ./build/simcity "SimCity (USA).sfc"` exits cleanly
- All 9 ctest suites pass

**Edge cases covered:**
- Missing ROM → graceful fallback (black screen, no crash)
- Corrupted packets → load_title_screen returns false, valid=false
- Palette out of bounds → bounds check before copy

## Verification Criteria

- [ ] Locate title screen logo/tilemap in ROM (disassembly reference used)
- [ ] Extract logo graphics at runtime (LC_LZ5 decompression implemented)
- [ ] Render title screen using extracted tiles + correct palette (3-layer composition done)
- [ ] Remove invented colored rectangles from render_title() (replaced with texture blit)
- [ ] Title screen auto-advances to menu after delay (existing timer behavior kept)
- [ ] 9/9 ctest pass (6 original + test_title + test_building_sprites + test_ui + test_timestep)
- [ ] Headless smoke test passes

## Estimation

- Complexity: medium (2-8h) — implementation was done in T021 scope
- Risk: medium — depends on ROM addresses from disassembly
- Blocking dependencies: no — all dependencies (decompressor, ROM parser) already existed