---
ticket: T027
title: Architectural Re-evaluation — Recompilation for Optimization, Modding, Widescreen
sprint: sprint-02
priority: high
status: pending
created: 2026-09-14
source: User challenge to T026 decision — new requirements: optimization, modding, widescreen
---

# T027 — Architectural Re-evaluation: Recompilation for Optimization/Modding/Widescreen

## Context
T026 decided "continue reimplementation" based on: no coprocessor, license veto, bug preservation, toolchain simplicity.
User now specifies **new requirements** that change the trade-off:
1. **Optimization** — native speed for simulation-heavy game
2. **Modding** — community can patch game logic, not just assets
3. **Widescreen** — native rendering at arbitrary aspect ratios (like StarFox Enhanced)

These are legitimate product goals that recompilation + native renderer can enable better than reimplementation.

## Acceptance Criteria
- [ ] Re-evaluate with new requirements as first-class constraints
- [ ] Two-agent protocol (critic + implementor) with updated premises
- [ ] Feasibility: can snesrecomp + native renderer achieve widescreen/modding for SimCity?
- [ ] License: PolyForm Noncommercial — is there a path (dual license, fork, alternative)?
- [ ] Effort estimate: migration cost vs new features enabled
- [ ] Decision with updated justification

## Scope
**In scope:** Full architectural re-evaluation with new requirements
**Out of scope:** Implementation (separate tickets if chosen)

## Dependencies
- T026 artifacts (previous analysis)
- snesrecomp framework capabilities (widescreen renderer, modding hooks)
- SimCity SNES disassembly completeness