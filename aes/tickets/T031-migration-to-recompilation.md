---
ticket: T031
title: Migration to Static Recompilation (snesrecomp) — Project Purpose
sprint: sprint-03
priority: critical
status: pending
created: 2026-09-14
source: User directive — recompilation is the intended project purpose
---

# T031 — Migration to Static Recompilation (snesrecomp)

## Context
User has clarified: **recompilation IS the project purpose**. Previous analyses (T026, T027) evaluated against different premises. This ticket accepts the premise and plans the migration.

Goals enabled by recompilation:
- Cycle-accurate original logic (including bugs as baseline for mods)
- Native widescreen via adapted renderer (StarFox Enhanced model)
- Modding via generated C patches + runtime hooks
- Optimization via native recompiled 65C816

## Acceptance Criteria
- [ ] snesrecomp framework integrated as submodule (pinned commit)
- [ ] SimCity (USA) ROM disassembly mapped to per-bank recompilation config
- [ ] Recompilation generates C from ROM (src/gen/) — excluded from git
- [ ] Runtime models SNES hardware: CPU, PPU, APU, DMA, timers
- [ ] Original game boots to title screen via recompiled code
- [ ] Native widescreen renderer integrated (adapted from StarFox Enhanced)
- [ ] Modding hooks: config.ini toggles + generated C patch points
- [ ] Build system: Rust toolchain + Python analyzer + CMake + Ninja
- [ ] License: PolyForm Noncommercial 1.0.0 accepted (project is non-commercial)
- [ ] 6/6 ctest pass (adapted for recompiled runtime)
- [ ] Headless smoke test passes

## Scope
**In scope:** Full migration to snesrecomp-based architecture
**Out of scope:** Keeping current reimplementation (will be archived)

## Known Risks
- **License**: PolyForm Noncommercial — project must accept non-commercial-only
- **Toolchain**: Rust + Python analyzer + snesrecomp framework — significant CI complexity
- **Disassembly completeness**: SimCity SNES disassembly must cover all banks
- **PPU modeling**: Cycle-accurate PPU needed for raster effects (if any)
- **Migration effort**: ~3-6 months; current 15 tickets archived
- **Bus factor**: snesrecomp maintained by ~1 person (mstan)

## Dependencies
- snesrecomp framework (submodule)
- SimCity (USA) ROM disassembly (Yoshifanatic1/SimCity-SNES-Disassembly)
- StarFox Enhanced renderer reference (for widescreen adaptation)
- Current AES artifacts (for comparison/validation)