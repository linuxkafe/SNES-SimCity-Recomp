---
ticket: T021
phase: review
status: done
created: 2026-09-14
requires:
  - aes/tickets/T021-verify.md
produces:
  - aes/tickets/T021-review.md
---

# T021 — Review: Title Screen ROM Logo Extraction

## Reviewer: Hostile Senior Engineer

### Lens 1: Correctness

**Findings:**
- ✅ Disassembly addresses match Yoshifanatic1/SimCity-SNES-Disassembly (AssetPointersAndFiles.asm)
- ✅ LC_LZ5 decompression reuses proven `nintendo_decompress()` from T004/T017
- ✅ 4bpp decode matches SNES format: two 2bpp planes interleaved (bytes 0-15 planes 0/1, 16-31 planes 2/3) — validated by T020 entropy analysis
- ✅ 2bpp decode correct for Layer 3 (stars/atmosphere)
- ✅ Tilemap entry parsing: bits 0-9 = tile ID, 10-12 = sub-palette, 14 = hflip, 15 = vflip — matches SNES BG Mode 1
- ✅ Palette index 0 treated as transparent (backdrop) — matches SNES behavior
- ✅ Layer composition order correct: L3 (back) → L1 (middle) → L2 (front/logo)
- ✅ All 9 ctest pass including comprehensive `test_title.cpp` regression

**Minor concern:** No test for hflip/vflip in tilemap entries — but test validates tile IDs stay in bounds, and rendered skyline pixels confirm correct orientation.

### Lens 2: Simplicity

**Findings:**
- ✅ Single responsibility: `titlescr.cpp` only handles title screen extraction/rendering
- ✅ No unnecessary abstractions — direct decompression, decode, blit
- ✅ Reuses existing `DecompressError` enum and `SnesRom::translate()`
- ✅ `Image` and `Color` types from `gfx/tile.h` (shared with city view)
- ⚠️ `decode_4bpp` and `decode_2bpp` duplicated logic — could share a template, but 2 functions × 40 lines is acceptable for clarity
- ⚠️ `blit_layer` handles both 4bpp and 2bpp via `tile_bytes` parameter — slightly magic, but documented

**Verdict:** Simplicity appropriate for domain; no over-engineering.

### Lens 3: Maintainability

**Findings:**
- ✅ All ROM addresses as `constexpr` at top of file — easy to update if disassembly corrected
- ✅ Clear struct `TitleScreenData` with `valid` flag for error propagation
- ✅ No global state; pure functions `load_title_screen()` and `render_title_screen()`
- ✅ Test covers packet sizes, palette distinctness, tilemap bounds, rendered output
- ⚠️ No version check for ROM region (USA vs JP/EU) — addresses may differ; graceful fallback exists but silent
- ⚠️ Hardcoded layer dimensions (384/512/32 tiles, 64×64/32×64 tilemaps) — if disassembly wrong, silent corruption

**Recommendation:** Add ROM region detection and address table per region (future ticket).

### Lens 4: Security

**Findings:**
- ✅ No user input in decompression path — offsets from constants
- ✅ `nintendo_decompress()` has bounds checking (`DecompressError` enum)
- ✅ `rom.translate()` validates CPU address → file offset
- ✅ Palette copy bounds-checked: `pal_off + kPalCount * 2 > data.size()`
- ✅ Tilemap tile ID bounds-checked before blit: `tile * tile_bytes >= tiles.size()`
- ✅ No buffer overflows possible — all vectors sized by decompressor output
- ✅ SDL texture creation uses known dimensions (256×224)

**Verdict:** Secure by construction; defense-in-depth at every boundary.

### Lens 5: Performance

**Findings:**
- ✅ One-time cost at startup: 6 decompressions + 3× tile decode + palette copy
- ✅ Render: single `SDL_RenderCopy` of pre-built texture (stretched by GPU)
- ✅ No per-frame allocation or decompression
- ✅ Nearest-neighbor scaling (`SDL_HINT_RENDER_SCALE_QUALITY=0`) — pixel-perfect
- ⚠️ CPU-side 3-layer blit into `Image` at startup (~256×224×3 layers = ~170k pixels) — trivial (<1ms)
- ⚠️ `std::fill` of entire frame with palette[0] each render — could skip if texture covers all, but negligible

**Verdict:** Performance excellent; startup cost amortized, runtime cost zero.

## Overall Verdict

**APPROVE** — Implementation is correct, simple, maintainable, secure, and performant.

## Backlog Items Created (Non-Blocking)

1. **ROM region support** — Add address tables for JP/EU ROMs; detect region from header
2. **Animated title elements** — Star twinkle via HDMA simulation (if SNES original has it)
3. **Flip test** — Add test case for hflip/vflip tilemap entries

These are enhancements, not defects. T021 is complete and ready for learn phase.