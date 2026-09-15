---
ticket: T027
phase: review
status: done
created: 2026-09-14
requires:
  - aes/tickets/T027-verify.md
produces:
  - aes/tickets/T027-review.md
---

# T027 — Review: Architectural Re-evaluation

## Reviewer: Hostile Senior Engineer

### Lens 1: Correctness

**Finding:** New requirements analyzed correctly. StarFox widescreen renderer (3D polygon FOV adjustment) does not transfer to SimCity (2D isometric diamond-grid tilemap). Modding via generated C patches is inferior to clean C++ APIs. License veto unchanged.

**Verdict:** ✅ Decision correct for product correctness.

### Lens 2: Simplicity

**Finding:** Staying with reimplementation avoids Rust/Python toolchain. Proposed feature tickets (widescreen renderer, modding API, optimization) are focused, incremental, and don't require framework migration.

**Verdict:** ✅ Simplicity favors status quo + focused tickets.

### Lens 3: Maintainability

**Finding:** 
- Reimplementation: Team owns sim core, renderer, modding API — all in C++
- Recompilation: Depends on snesrecomp framework (1 maintainer), generated code patches fragile
- Feature tickets add capabilities to owned codebase

**Verdict:** ✅ Maintainability strongly favors reimplementation + feature tickets.

### Lens 4: Security/Legal

**Finding:** PolyForm Noncommercial 1.0.0 unchanged. **Hard veto for commercial path.** If project accepts non-commercial-only, this gate passes but with explicit constraint.

**Verdict:** ⚠️ License gate passes conditionally — project must accept non-commercial constraint.

### Lens 5: Performance

**Finding:** Recompilation adds CPU/PPU/APU emulation layer for zero benefit (SimCity uses no complex PPU). Native widescreen renderer on reimplementation = direct tilemap rendering at target resolution — faster than emulation + post-process.

**Verdict:** ✅ Performance favors reimplementation + native renderer.

## Overall Verdict

**APPROVE DECISION** — Stay reimplementation. Create feature tickets for actual requirements.

## Feature Tickets Created (Non-Blocking)

1. **T028**: Native widescreen renderer for isometric tilemap
2. **T029**: Modding API (Lua bindings for sim::City)
3. **T030**: Performance profiling + optimization

These address user's stated goals without architectural pivot.