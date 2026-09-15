---
ticket: T017
phase: review
date: 2026-09-11
---

# T017 — Review Artifact

## Verdict
**APPROVE** (with one documented follow-up: palette visual smell-test).

## Friendly Senior Engineer Review — Five Lenses

### Correctness
- ⚠️→✅ Root cause was two interacting bugs, both now fixed and regression-tested:
  - `length += 1` previously skipped for most modes → the PR "byte sizes" only matched
    *after* the fix. New tests assert exact final sizes (9726/10326/…/2814, 32768).
  - Back-ref guard `ref + length > out.size()` rejected legitimate overlapping LZ77
    refs → removed; byte-at-a-time copy relies on the known-correct decompressor.
- `read8/read16` double-translation (file offset treated as CPU address) was the
  sharpest trap; `rommap` now proves the output side with canonical addresses.
- Correct: scenario table order in ROM is SanFran, Bern, Tokyo, Detroit, Boston,
  Rio, LasVegas, FreeCity, Practice — not Tokyo-first. Code doesn't assume order.

### Simplicity
- ✅ `RomAssets` is a plain data struct (`tiles`, `palettes`, `terrain_palette()`);
  no abstraction layered over it. `load_rom_assets` is a single extraction function.
- ✅ Fallback path retained: no assets / failed extraction → flat colours. The game
  still runs without a ROM (headless and windowed), preserving prior behaviour.
- ✅ Rejected over-engineering: SDL atlas texture → CPU-side blit (also the *only*
  option — streaming textures cannot be render targets).

### Maintainability
- ✅ Constants (file offsets, tile/palette sizes) live in `rom_assets.h`/`decompress.h`
  with doc comments; the disassembly source for the pointers is referenced.
- ✅ Test regressions are self-contained against the real ROM (skippable if absent),
  so the suite stays green in CI without proprietary assets.

### Security
- ✅ No new attack surface: extraction reads only within `rom.size()`; decompressor
  has bounds checks; no user-controlled lengths outside ROM data.

### Performance
- ✅ 32768 B decompressed once at startup; per-frame work is a single locked-texture
  blit (2x scale of 8x8 tiles → 1920x1600 equivalent). No per-frame allocations in
  the hot path.

### Hostile Scenarios Considered
- ROM missing → `load_rom_assets` returns `valid=false`; CityView falls back flat.
- Decompressor truncated input → returns false → skip asset load (no crash).
- Scenario decode failure → `ok=false` → fallback terrain, smoke still labels source.
- Palette block/sub invalid → clamped to bounds in `terrain_palette()`.

## Findings
| Severity | Finding | Disposition |
|----------|---------|-------------|
| MINOR | Palette (block 4, sub 0) chosen by palette-inspection, not human render | Follow-up visual smell-test when windowed render is inspected; field documented in `rom_assets.h` |

## Backlog Tickets Created
- None (follow-up absorbed as documented risk; no new functional gap).

## Evidence
- `make build` clean; `make test` 5/5; smoke markers on stderr; rommap canonical
  offsets; verify artifact `T017-verify.md`.