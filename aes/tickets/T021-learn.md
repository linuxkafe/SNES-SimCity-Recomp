---
ticket: T021
phase: learn
status: done
created: 2026-09-14
requires:
  - aes/tickets/T021-review.md
produces:
  - aes/tickets/T021-learn.md
  - aes/shadow/SD-DOMAIN-002.md
  - aes/shadow/SD-META-003.md
---

# T021 — Learn: Title Screen ROM Logo Extraction

## Feynman Explanation — For a Child

Imagine the SNES game stores its title screen like a puzzle: the pictures are squished (compressed) and the colors are in a separate box. Our job is to un-squish the pictures, read the color box, and put the puzzle pieces together in the right order — first the starry sky, then the city skyline, then the big "SIMCITY" logo on top. We do this once when the game starts, save the finished picture, and then just show it like a poster. No magic, just careful unpacking.

## Feynman Explanation — For an Expert

The SNES SimCity title screen uses BG Mode 1 with three background layers: Layer 3 (2bpp, 32 tiles) for stars/atmosphere, Layer 1 (4bpp, 384 tiles) for the skyline, Layer 2 (4bpp, 512 tiles) for the logo sprite bank. All graphics are Nintendo LZ5-compressed packets at known CPU addresses (from disassembly). The palette is raw BGR555 at 0x0C89D8 (128 entries = 8 sub-palettes × 16 colors). At startup we: (1) translate CPU addresses to file offsets via LoROM mapping, (2) decompress 6 packets via `nintendo_decompress()`, (3) decode 4bpp (two interleaved 2bpp planes) and 2bpp tiles to palette indices, (4) blit three layers back-to-front respecting tilemap attributes (tile ID, sub-palette, hflip/vflip) with index-0 transparency, (5) upload to SDL texture, (6) stretch to window with nearest-neighbor. The entire pipeline is deterministic, allocation-free after startup, and secured by bounds checks at every boundary.

## First Principles Analysis

**What problem are we actually solving?**
- Legal constraint: Zero asset distribution → must extract at runtime from user ROM
- Fidelity constraint: Title screen must match SNES pixel-for-pixel
- Determinism constraint: Same ROM → same output every time

**Why these specific addresses?**
- Disassembly (Yoshifanatic1/SimCity-SNES-Disassembly) is ground truth
- Empirical validation: all 6 packets decompress to expected sizes, palette has >64 distinct colors, rendered skyline occupies lower quarter of frame

**Why LC_LZ5?**
- Nintendo's standard compression for SNES; same algorithm as scenario maps (T004) and city tiles (T017)
- Single decompressor handles all packets — no format fragmentation

**Why three layers composed on CPU not GPU?**
- SNES BG layers have per-tile sub-palette and flip attributes that don't map cleanly to GPU
- One-time CPU composite is simpler and bug-free; GPU just stretches final texture

## Hostile Audit

**What could still be wrong?**
1. **Region dependence**: Addresses from USA ROM; JP/EU may differ → silent wrong title screen
2. **Animated stars**: SNES may use HDMA for twinkling stars; we render static frame
3. **Sub-palette assignment**: Assumes tilemap bits 10-12 map directly to our 8 sub-palettes; verified by visual inspection but not exhaustively tested
4. **Palette index 0 transparency**: Correct for BG layers, but what if logo uses index 0 intentionally? (Unlikely — index 0 is backdrop color)

**What did we assume that might be false?**
- Disassembly addresses are 100% correct — validated by successful decompression but not byte-for-byte compared to hardware capture
- No runtime palette animation on title screen — plausible but unverified

## Key Insights (Shadow Documents)

### SD-DOMAIN-002: SNES BG Mode 1 Title Screen Architecture
- **Synthesis**: The SimCity SNES title screen uses BG Mode 1 with three layers: L3 (2bpp atmosphere), L1 (4bpp skyline), L2 (4bpp logo). All graphics LZ5-compressed; palette is raw BGR555 CGRAM snapshot. Tilemaps use standard SNES attributes (10-bit tile ID, 3-bit sub-palette, hflip/vflip). This architecture is distinct from the city view (which uses Mode 1 with different layer assignments).
- **Provenance**: Derived from T021 implementation + disassembly cross-reference
- **Epistemic State**: SUPORTADA (empirically validated by test_title.cpp)

### SD-META-003: Runtime Asset Extraction Pattern
- **Synthesis**: The pattern "disassembly address → LoROM translate → LC_LZ5 decompress → format decode → compose → cache" works uniformly for: scenario maps (T004), city tiles (T017), title screen (T021). Each adds domain-specific decode (4bpp/2bpp, tilemap dimensions) but shares the extraction pipeline. This pattern should be formalized as a reusable `RomAssetExtractor` for future assets (font T019, building sprites T023, audio T010).
- **Provenance**: Pattern recognized across T004, T017, T021 implementations
- **Epistemic State**: HIPÓTESE (pattern observed 3×, not yet abstracted)

## Kanban Update

Move T021 from **In Progress** → **Done**

| ID | Title | Status |
|----|-------|--------|
| T021 | Title screen — extract ROM logo | **done** |

## Sprint Retrospective (sprint-02)

**What worked:**
- Disassembly-driven addresses eliminated guesswork
- Reusing `nintendo_decompress()` gave high confidence
- Comprehensive test_title.cpp caught tilemap bounds issues early
- Headless smoke test validates full integration path

**What to improve:**
- No ROM region detection — technical debt for multi-region support
- Pattern not yet abstracted — T019/T023 will repeat similar code
- No visual regression test (T025) for title screen yet

**Action items for next sprint:**
- T022: Menu system with ROM font/text (depends on T019 font extraction)
- T025: Visual regression tests — add title screen golden image
- Consider `RomAssetExtractor` abstraction before T019/T023