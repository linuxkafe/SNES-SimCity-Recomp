---
ticket: T017
phase: learn
date: 2026-09-11
---

# T017 — Learn Artifact

## What We Learned

### 1. Nintendo LC_LZ5: `length − 1` applies to ALL modes (Feynman to a child)
Before: "Some packet things add 1 later." After: **every** mode stores value−1 in its
length field — the byte you read as 5 might really mean 6. The compactors packed
"number minus one" because it lets them store 0..31 instead of 1..32 in 5 bits.
Cost of missing it: all real packets refused to decode to their registered sizes.

### 2. Back-references overlap (expert note)
LC_LZ5 back-references copy from the **growing** output buffer (LZSS/LZ77 style),
so `ref + length` may exceed the current output size by design. A naive
`src_end <= out.size()` guard is **wrong** — it rejects valid long matches where the
source and copy regions overlap. Correct handling: byte-at-a-time push from
`out[ref + i]`, letting each byte lengthen the buffer as the source. This was the
final blocker; without it only 6 of 9 maps decoded.

### 3. `read8()/read16()` in SnesRom do CPU-address translation — never call them
for offsets into `data()`. Low-level extraction MUST use `read_byte_raw()/
read_u16_raw()` (pure file offsets) or index `rom.data()` directly. The classic
failure mode: decompressing C000-range packets with CPU-address reads yields garbage
that coincidentally re-syncs — our "empirical offsets matched ASCII text" was a
false positive because of exactly this (see kanban T004 correction).

### 4. Streaming SDL textures cannot be render targets
You cannot lock a surface, then `SDL_RenderCopy` an atlas to it. The atlas/RenderCopy
design fell apart here. The working pattern: CPU-side blit into a locked *streaming*
texture in the loop, `SDL_UnlockTexture` + present once per frame. Streams unlock
fast; lock-unchanged texture once per frame is a non-event on modern GPUs.

### 5. Verify against the disassembly, not against your own decoder
The ground truth for "did the decompressor work" was `AssetPointersAndFiles.asm`:
exact registered bytes for scenario maps + the Layer1 tile bank. Add those as
**unit tests against the real ROM** (skippable if absent) — they are now the
permanent regression net for these two bugs.

## Hostile Audit
- **Introduced files that distribute copyrighted byte-for-byte data?** No — `tiles`
  and `palettes` live only in RAM at runtime; nothing is written to the repo.
- **Did we violate determinism?** No — extraction is pure input→output, no RNG;
  `sim::City` remains decoupled from `gfx`.
- **Could the fallback mask the bug?** The game still starts with flat colours if
  extraction fails; smoke prints explicit markers so a silent regression is unlikely.
- **Test fragility:** ROM-dependent tests self-skip when no ROM is present, so CI
  without the ROM still passes (as designed in NFR-1).