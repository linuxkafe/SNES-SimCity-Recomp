---
ticket: T026
phase: learn
status: done
created: 2026-09-14
requires:
  - aes/tickets/T026-review.md
produces:
  - aes/tickets/T026-learn.md
  - aes/shadow/SD-META-004.md
  - aes/shadow/SD-CI-001.md
---

# T026 — Learn: Architectural Decision (Reimplementation vs Recompilation)

## Feynman Explanation — For a Child

Imagine two ways to make a copy of a LEGO castle:
1. **Build your own (reimplementation)**: Look at the castle, figure out the rules (red bricks for walls, blue for roof), and build a NEW castle following those rules. You can fix the wobbly tower the original had.
2. **3D scan and print (recompilation)**: Scan every single brick position, then print an exact plastic replica. Including the wobbly tower.

StarFox needed way 2 because it has a special custom chip (Super FX) that does 3D math — too hard to rebuild from scratch. SimCity is just a 2D simulation — no special chips. We already built our own castle (15 tickets done, all tests pass), and we FIXED the wobbly parts (fake earthquakes, wrong colors, missing fonts). Switching to 3D printing now would give us back the wobbles, plus require a fancy printer (Rust toolchain) that only one person knows how to fix, and the printer's license says "no selling."

## Feynman Explanation — For an Expert

**Two approaches compared:**

| Dimension | Clean-Room Reimplementation (Current) | Static Recompilation (snesrecomp) |
|-----------|----------------------------------------|-----------------------------------|
| **Core technique** | Rewrite 65C816 logic in C++17 from specs + disassembly | AOT recompile 65C816 → C via Rust analyzer |
| **Asset handling** | Runtime extraction from ROM (tiles, palettes, maps) | Same (ROM-derived data fed to recompiled code) |
| **Hardware modeling** | None needed (simulation only) | Full SNES runtime (CPU, PPU, APU, DMA, timers) |
| **Coprocessor** | N/A (SimCity has none) | Super FX/GSU (StarFox's raison d'être) |
| **License** | Permissive (SDL2 zlib, CMake BSD) | **PolyForm Noncommercial 1.0.0** |
| **Toolchain** | C++17, CMake, SDL2 | + Rust, Python 3.9+, snesrecomp framework |
| **Bug behavior** | We FIX original bugs (earthquake≠meteor, palette) | Preserves ALL original bugs + timing quirks |
| **Bus factor** | Team owns simulation logic | Depends on mstan/snesrecomp maintenance |
| **Authenticity** | Assets = 100% ROM; Logic = corrected | Assets = 100% ROM; Logic = 100% original (bugs included) |

**Key insight:** The "authenticity" argument is a category error. We achieve **asset authenticity** (runtime ROM extraction, validated by pixel census in T020) without **logic authenticity** (which carries bugs). SimCity SNES has no PPU tricks, HDMA, or coprocessor — the domains where recompilation shines.

## First Principles Analysis

**What problem are we solving?**
- Legal: Zero asset distribution → runtime ROM extraction ✅ (already done)
- Correctness: Simulation matches player expectations → fix original bugs ✅ (reimplementation does this)
- Maintainability: Team can modify/extend → owned C++ code ✅
- Sustainability: License allows commercial path → permissive deps ✅

**Why did StarFox choose recompilation?**
- Super FX (GSU) coprocessor: custom RISC, 21 MHz, does 3D transform/lighting/rasterization
- Reimplementing GSU + all games' microcode = years of work
- snesrecomp makes this tractable: recompile GSU code + model hardware
- **SimCity has no coprocessor** — this entire rationale is absent

**What would switching cost?**
- 3-6 months: integrate snesrecomp, write per-bank config, model SimCity peripherals
- Ongoing: framework updates, Rust toolchain in CI, generated code patches
- License: forever non-commercial
- Benefit: "original logic" including bugs we already fixed

## Hostile Audit

**What could be wrong with this decision?**
1. **Future PPU complexity**: If SimCity uses mid-frame palette swaps or HDMA we haven't found, recompilation handles it automatically. Risk: LOW — disassembly + T020 pixel census show static palettes.
2. **Simulation drift**: Our reimplementation might diverge from original in subtle ways. Mitigation: Deterministic tests + visual regression (T025) + scenario validation.
3. **snesrecomp license change**: If PolyForm → MIT/BSD, re-evaluate. Not a current decision factor.

**Assumptions validated:**
- SimCity = LoROM, no coprocessor ✅ (ROM header + disassembly)
- Asset extraction pattern works uniformly ✅ (SD-META-003, 3× validated)
- License is binding ✅ (PolyForm Noncommercial text explicit)

## Key Insights (Shadow Documents)

### SD-META-004: Architectural Decision Framework
- **Synthesis**: For SNES ports without coprocessors, clean-room reimplementation with runtime asset extraction dominates static recompilation on: license (permissive vs non-commercial), toolchain simplicity (C++ vs Rust+Python), bug control (fix vs preserve), maintainability (owned vs generated), and performance (native vs emulated). Recompilation only wins when coprocessor microcode is infeasible to reimplement (Super FX, SA-1, DSP).
- **Provenance**: T026 two-agent protocol analysis
- **Epistemic State**: SUPORTADA

### SD-CI-001: License Gate for Dependency Adoption
- **Synthesis**: Any dependency with non-commercial license (PolyForm, CC-NC, etc.) is an automatic veto for projects with potential commercialization path. This gate must run in `aes-plan` Phase 00 before technical evaluation. Current project deps: SDL2 (zlib), CMake (BSD-3), C++ stdlib — all pass.
- **Provenance**: T026 license analysis (FAILURE MODE 1)
- **Epistemic State**: SUPORTADA

## Kanban Update

T026 completed — no status change needed (analysis ticket).

Sprint-02 focus remains:
| ID | Title | Status |
|----|-------|--------|
| T022 | Menu system — ROM font + text | **in-progress** |
| T023 | Building sprites — map 1024-tile bank to zones | pending |
| T024 | Disaster UX — earthquake or gate random disasters | pending |
| T025 | Visual regression tests — headless pixel assertions | pending |

## Sprint Retrospective Addition

**Architectural challenge resolved:** User raised valid comparison to StarFox recompilation. AES heavy-ticket protocol (two-agent) produced documented decision in <1 hour. Key differentiator: **coprocessor presence**. SimCity has none → recompilation's killer feature irrelevant. License veto is independent and decisive.