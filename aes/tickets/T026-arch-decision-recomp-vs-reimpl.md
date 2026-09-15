---
ticket: T026
title: Architectural Decision — Reimplementation vs Static Recompilation (snesrecomp)
sprint: sprint-02
priority: high
status: pending
created: 2026-09-14
source: User challenge comparing current approach to StarFox SNES Recomp (snesrecomp)
---

# T026 — Architectural Decision: Reimplementation vs Static Recompilation

## Context
Current SimCity SNES PC port uses **clean-room reimplementation**: deterministic simulation core in C++17, runtime asset extraction from ROM (tiles, palettes, maps), SDL2 rendering. Zero original game logic is reused — all rules (budget, RCI, zoning, power, disasters) reimplemented from observed behavior + disassembly reference.

StarFox SNES Recomp (mstan/StarFoxSNESRecomp) uses **static recompilation via snesrecomp**: 65C816 machine code → native C via ahead-of-time recompilation, SNES hardware modeled by runtime, Super FX coprocessor integrated. Original game logic runs natively; only hardware abstraction layer is new.

These are fundamentally different approaches. Need rigorous AES evaluation before continuing.

## Acceptance Criteria
- [ ] Document both approaches with technical trade-offs
- [ ] Feasibility assessment: can snesrecomp handle SimCity (no Super FX, but complex simulation)?
- [ ] Legal/licensing analysis: snesrecomp is PolyForm Noncommercial 1.0.0
- [ ] Effort estimation: port current reimplementation vs adopt snesrecomp
- [ ] Risk analysis: correctness, maintainability, bus factor, legal
- [ ] Decision with justification recorded in AES

## Scope
**In scope:** Architectural comparison, feasibility proof-of-concept (if needed), decision
**Out of scope:** Full migration implementation (separate ticket(s) if chosen)

## Known Risks
- snesrecomp requires Rust toolchain, adds significant build complexity
- PolyForm Noncommercial license may conflict with project goals
- SimCity uses no coprocessor — snesrecomp advantage (Super FX) irrelevant
- Current reimplementation already passes all tests, headless smoke, deterministic
- Recompilation preserves bugs; reimplementation fixes them (but may introduce new ones)

## Dependencies
- snesrecomp framework (Rust, Python analyzer)
- SimCity (USA) ROM disassembly (Yoshifanatic1/SimCity-SNES-Disassembly)
- Current AES artifacts (kanban, shadow docs, test suite)