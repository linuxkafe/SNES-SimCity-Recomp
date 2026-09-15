---
ticket: T031
phase: verify
status: done
created: 2026-09-14
requires:
  - aes/tickets/T031-plan.md
produces:
  - aes/tickets/T031-verify.md
---

# T031 — Verify: Migration to Static Recompilation (Phase 1 Plan)

## Plan Verification

### Two-Agent Protocol Completeness
- ✅ Critic phase: 5 failure modes identified + assumption + alternative framing
- ✅ Implementor phase: All 5 failure modes addressed + assumption validated + alternative rejected
- ✅ Feynman explanations (child + expert) in both phases
- ✅ Porquê ×5 in both phases

### Technical Specification Completeness
- ✅ 5-phase incremental migration defined
- ✅ Phase 1 files enumerated (7 files)
- ✅ Specification for `tools/regen.sh` and build flow
- ✅ Testing strategy per phase
- ✅ Verification criteria per phase

### Risk Acknowledgment
- ✅ License: PolyForm Noncommercial accepted (user directive)
- ✅ Toolchain: Docker CI mitigation documented
- ✅ Disassembly: Audit plan in Phase 1
- ✅ PPU: Golden master validation (T020 pixel census)
- ✅ Migration: Incremental with gates

### Baseline Established (Current Reimplementation)
- 9/9 ctest pass
- Headless smoke: 688 frames clean
- Title screen: ROM-extracted, validated (T021)
- Asset extraction: 3× validated pattern (SD-META-003)
- Deterministic sim core: `sim::City` headless-testable

## Golden Master Reference (for Phase 2+ validation)

| Metric | Current Value | Use for Validation |
|--------|---------------|-------------------|
| Title screen pixels | T021 test_title.cpp census | Phase 2 visual diff |
| Scenario terrain | 9/9 maps decode | Phase 3 map load |
| Sim step determinism | `test_city` passes | Phase 3 replay test |
| Palette mapping | Block 5, sub 0 (T020) | Phase 2+ PPU accuracy |
| Performance | 221 FPS headless | Phase 5 benchmark |

## Verdict
**PLAN APPROVED FOR PHASE 1 EXECUTION** — Specification complete, risks acknowledged, baseline established. Ready for `/aes-build` on Phase 1 tasks.

## Next Steps
1. Execute Phase 1: Framework integration + `tools/regen.sh` + host-check CI
2. Validate: `make build` (host-check) passes in CI without ROM
3. Proceed to Phase 2: Boot + Title screen