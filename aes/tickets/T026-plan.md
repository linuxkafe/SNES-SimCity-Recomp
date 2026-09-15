---
ticket: T026
phase: plan
status: in-progress
created: 2026-09-14
tier: heavy
requires:
  - aes/kanban.md
  - aes/tickets/T026-arch-decision-recomp-vs-reimpl.md
produces:
  - aes/tickets/T026-plan.md
blocked_by: ''
---

# T026 — Plan: Architectural Decision (Two-Agent Protocol)

## Two-Agent Analysis

### Critic Phase

## Como se fosse uma criança
Imagine você tem dois jeitos de fazer um bolo:
1. **Receita nova (reimplementação)**: Você prova o bolo original, anota o gosto, e escreve sua própria receita do zero. Pode melhorar (menos açúcar, sem glúten), mas pode errar o ponto.
2. **Máquina de copiar (recompilação)**: Você pega o bolo original, passa num scanner 3D, e a máquina imprime um bolo idêntico molécula por molécula. Inclusive os pedaços queimados no fundo.

O projeto SimCity hoje usa jeito 1. O StarFox usa jeito 2. A pergunta: vale a pena trocar de jeito no meio do caminho?

## Como se fosse um especialista

**Current approach (clean-room reimplementation):**
- ~15 tickets completed, deterministic sim core, 9/9 tests pass, headless smoke works
- Assets extracted at runtime from ROM (legal: zero asset distribution)
- C++17, CMake, SDL2 — standard toolchain, no exotic deps
- Simulation logic: ~2000 lines C++ reimplementing budget, RCI, zoning, power, disasters
- Bus factor: 1-2 people understand the sim rules

**snesrecomp approach (static recompilation):**
- 65C816 → C via Rust-based analyzer (Python fallback), runtime models SNES hardware
- Requires: Rust toolchain, Python 3.9+, SDL3 (or SDL2), CMake 3.20+, Ninja
- License: PolyForm Noncommercial 1.0.0 — **non-commercial only**
- StarFox uses Super FX (GSU); SimCity uses **no coprocessor** — main snesrecomp advantage irrelevant
- Would need: per-bank recompilation config, function metadata, hardware glue for SimCity peripherals
- Preserves ALL original logic including bugs, timing quirks, RNG behavior

[FAILURE MODE 1]
Mechanism: PolyForm Noncommercial license prohibits commercial use. If project ever wants Patreon, sponsorships, or commercial distribution, this is a hard blocker. Current MIT/BSD-style deps have no such restriction.

[FAILURE MODE 2]
Mechanism: snesrecomp adds Rust + Python analyzer to build chain. Current build is pure C++/CMake. CI complexity increases dramatically. Bus factor for Rust/Python analyzer maintenance: ~1 person (mstan).

[FAILURE MODE 3]
Mechanism: Recompilation preserves original binary behavior EXACTLY — including the "random earthquakes" (meteor/monster + red flash), broken palette selection, invented title/menu. Would need to patch generated C or runtime to fix, defeating the purpose.

[ASSUMPTION]
If false: snesrecomp can't handle SimCity's memory map / LoROM banking without major framework changes.
Impact: Months of framework work before game even boots.

[ALTERNATIVE FRAMING]
Instead of binary choice: keep reimplementation for simulation (where we WANT to fix bugs), use recompilation only for obscure hardware behavior (PPU timing, HDMA, DMA). But SimCity uses none of those.

## Porquê? ×5
1. Why consider switching? → User saw StarFox recomp and thinks it's "more authentic"
2. Why is authenticity valued? → Belief that original logic = correct behavior
3. Why is original logic correct? → It ran on real hardware
4. But original logic has bugs? → Yes: meteor≠earthquake, wrong palette, no font
5. Why replicate bugs? → You wouldn't; you'd patch. But then why recompile?

What are we optimizing that we shouldn't be?
Optimizing for "uses original binary" rather than "correct, maintainable, legally clear simulation"

### Implementor Phase

## Como se fosse uma criança
Continuar com a receita própria (reimplementação). Já funciona, os testes passam, podemos corrigir os bugs do original (terremoto que não existe, paleta errada), e não tem problema de licença. O StarFox precisava de recompilação porque tem um chip especial (Super FX) que é impossível de reimplementar direito. SimCity não tem chip especial.

## Como se fosse um especialista
**Decision: Stay with clean-room reimplementation.** Justification:

[ADDRESSING FAILURE MODE 1 - License]
Current deps: SDL2 (zlib), C++17 stdlib, CMake — all permissive. snesrecomp = PolyForm Noncommercial. This alone is decisive for any project that might want sustainability.

[ADDRESSING FAILURE MODE 2 - Toolchain complexity]
Adding Rust + Python analyzer + snesrecomp framework = ~50MB deps, custom CI, framework pin management. Current: `apt install build-essential cmake libsdl2-dev`. Zero friction.

[ADDRESSING FAILURE MODE 3 - Bug preservation]
Recompilation gives you the ORIGINAL bugs. SimCity SNES has: no earthquake disaster (meteor/monster only), broken palette indices, no font in ROM for menus. We'd have to patch the recompiled C or the runtime — same effort as fixing reimplementation, but in generated code you don't own.

[ADDRESSING ASSUMPTION - Feasibility]
SimCity is LoROM, no coprocessor. snesrecomp *could* work technically. But the framework is tuned for Super FX games. No SimCity-like precedent in snesrecomp ecosystem.

[ADDRESSING ALTERNATIVE FRAMING - Hybrid]
Hybrid makes sense for PPU-heavy games (raster effects, HDMA). SimCity is simulation-heavy, PPU-light. Our reimplementation already extracts ROM assets (tiles, palettes, maps) correctly — the "authentic" part. The simulation logic is WHERE bugs live, and we WANT to fix them.

[SOLUTION PROPOSAL]
1. **Close T026 with decision: continue reimplementation**
2. Document why in AES (this ticket)
3. Focus sprint-02 on T022 (menu/ROM font), T023 (building sprites), T024 (disaster UX), T025 (visual regression)
4. If future need arises for cycle-accurate PPU (unlikely for SimCity), evaluate snesrecomp then

## Porquê? ×5
1. Why reimplementation? → Correctness > authenticity; we fix original bugs
2. Why not recompilation? → License, toolchain, bug preservation, no Super FX need
3. Why is toolchain cost decisive? → Bus factor, CI friction, contributor onboarding
4. Why is license decisive? → PolyForm Noncommercial = no commercial path ever
5. Why is this the first principle? → Engineering serves the product; product needs correctness, maintainability, legal clarity

## Reconnaissance Summary
- Current codebase: 15 tickets done, deterministic sim, asset extraction working, all tests pass
- snesrecomp: Rust framework, PolyForm Noncommercial, Super FX focus, no SimCity precedent
- SimCity SNES: LoROM, no coprocessor, simulation-heavy, known bugs in original logic
- Shadow docs: SD-META-003 (extraction pattern validated 3×), SD-DOMAIN-001/002 (sim/PPU knowledge)

## Technical Approach
**No code changes.** This is a decision ticket. Output: documented decision in T026-plan.md and T026-learn.md.

## Affected Files
| File | Operation | Description |
|------|-----------|-------------|
| aes/tickets/T026-plan.md | create | This decision document |
| aes/tickets/T026-learn.md | create | Learning capture (post-review) |

## Verification Criteria
- [ ] Two-agent protocol completed (critic + implementor)
- [ ] Decision recorded with justification
- [ ] No architectural pivot — sprint-02 continues on T022-T025
- [ ] License risk explicitly documented

## Estimation
- Complexity: low (analysis only, no implementation)
- Risk: high (architectural decision)
- Blocking dependencies: no