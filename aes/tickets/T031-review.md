---
ticket: T031
phase: review
status: done
created: 2026-09-14
requires:
  - aes/tickets/T031-verify.md
produces:
  - aes/tickets/T031-review.md
---

# T031 — Review: Migration to Static Recompilation (Phase 1 Plan)

## Reviewer: Hostile Senior Engineer

### Lens 1: Correctness

**Finding:** Plan correctly identifies that behavioral equivalence to current reimplementation is the validation standard. Golden master baseline established (9 tests, pixel census, determinism). Phase gates prevent big-bang failure.

**Concern:** Disassembly completeness audit not yet done — Phase 1 must validate before Phase 2.

**Verdict:** ✅ Conditionally correct — Phase 1 must include disassembly coverage check.

### Lens 2: Simplicity

**Finding:** 5-phase incremental approach reduces complexity vs big-bang. However, toolchain complexity (Rust + Python + snesrecomp) is real and documented. Docker CI mitigates but adds operational overhead.

**Verdict:** ⚠️ Simplicity traded for capability — accepted per user directive.

### Lens 3: Maintainability

**Finding:** 
- Framework dependency (snesrecomp, 1 maintainer) = bus factor risk
- Generated code (src/gen/) not owned — patches fragile
- Mitigation: Contribute fixes upstream; fork if needed
- Current reimplementation archived with git tag for rollback

**Verdict:** ⚠️ Maintainability reduced — accepted for authenticity goals.

### Lens 4: Security/Legal

**Finding:** PolyForm Noncommercial 1.0.0 explicitly accepted. Project is non-commercial. No commercial path ever. Documented in plan.

**Verdict:** ✅ Legal gate passed with explicit acceptance.

### Lens 5: Performance

**Finding:** Recompiled 65C816 + runtime overhead vs native C++ sim. Current: 221 FPS headless. Target: 60 FPS at 4K with rendering. Phase 5 benchmark will validate.

**Verdict:** ✅ Performance target defined; validation in Phase 5.

## Overall Verdict

**APPROVE PHASE 1 PLAN** — Proceed with framework integration, `tools/regen.sh`, host-check CI.

## Conditions for Phase 2
1. Disassembly coverage audit complete (all banks mapped)
2. Host-check CI passes (framework builds, regen runs)
3. `src/gen/` produces valid C from SimCity ROM
4. No blocker findings in Phase 1 verification

## Backlog Items (Track Separately)
- Upstream snesrecomp fixes for SimCity-specific needs
- Fork decision criteria if upstream unresponsive
- Performance regression budget (Phase 5)