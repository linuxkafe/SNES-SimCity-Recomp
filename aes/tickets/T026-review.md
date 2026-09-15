---
ticket: T026
phase: review
status: done
created: 2026-09-14
requires:
  - aes/tickets/T026-verify.md
produces:
  - aes/tickets/T026-review.md
---

# T026 — Review: Architectural Decision

## Reviewer: Hostile Senior Engineer

### Lens 1: Correctness

**Finding:** Decision correctly identifies that reimplementation fixes original bugs (meteor≠earthquake, palette indices, missing font) while recompilation would preserve them. The "authenticity" argument conflates asset authenticity (which we have via runtime extraction) with logic authenticity (which we DON'T want — original logic has bugs).

**Verdict:** ✅ Correct decision for product correctness.

### Lens 2: Simplicity

**Finding:** Staying with C++17/CMake/SDL2 avoids adding Rust toolchain, Python analyzer, snesrecomp framework (~50MB), custom CI, framework pin management. Current onboarding: `apt install build-essential cmake libsdl2-dev`. Recompilation path would multiply build complexity 10x.

**Verdict:** ✅ Simplicity strongly favors status quo.

### Lens 3: Maintainability

**Finding:** 
- Current: ~2000 lines of simulation logic owned by team, deterministic, testable, documented in AES
- Recompilation: Generated C code (not owned), framework maintained by ~1 person (mstan), patches to generated code are fragile
- Bus factor: Current = team knows sim rules; Recompilation = team depends on external framework author

**Verdict:** ✅ Maintainability strongly favors reimplementation.

### Lens 4: Security

**Finding:** 
- PolyForm Noncommercial 1.0.0 license is a **hard blocker** for any commercialization path (Patreon, sponsorships, Steam, etc.)
- Current dependencies: SDL2 (zlib), CMake (BSD), C++ stdlib — all permissive
- License risk alone is sufficient to reject snesrecomp

**Verdict:** ✅ Security/legal vetoes recompilation path.

### Lens 5: Performance

**Finding:** 
- Reimplementation: simulation runs at native C++ speed, deterministic, headless-testable
- Recompilation: adds runtime hardware emulation layer (CPU, PPU, APU, DMA) even though SimCity uses none of the complex features (no HDMA, no raster effects, no Super FX)
- Overhead: recompilation adds interpretation/emulation tax for zero benefit

**Verdict:** ✅ Performance favors reimplementation.

## Overall Verdict

**APPROVE DECISION** — Continue clean-room reimplementation. Documented in T026-plan.md with full two-agent protocol.

## Backlog Items (Non-Blocking)

1. **Monitor snesrecomp** — If it adds MIT/BSD license option AND SimCity-like precedent, re-evaluate
2. **Extract common asset pipeline** — SD-META-003 pattern (disassembly→LoROM→LZ5→decode→compose) validated 3×; formalize as `RomAssetExtractor` before T019/T023 (tracked separately)

These are improvements, not pivots. T026 is complete.