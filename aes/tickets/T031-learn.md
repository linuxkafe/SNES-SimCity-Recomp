---
ticket: T031
phase: learn
status: done
created: 2026-09-14
requires:
  - aes/tickets/T031-review.md
produces:
  - aes/tickets/T031-learn.md
  - aes/shadow/SD-META-006.md
  - aes/shadow/SD-CI-002.md
---

# T031 — Learn: Migration to Static Recompilation (Plan Phase)

## Feynman Explanation — For a Child

O projeto vai mudar completamente de estratégia. Em vez de escrever nosso próprio SimCity, vamos usar uma ferramenta (snesrecomp) que pega o código original do SNES e transforma em código C que roda no PC. É como pegar a receita original do bolo e usar uma máquina que lê a receita e faz o bolo automaticamente — incluindo os erros da receita original.

Vamos fazer em 5 etapas, cada uma testada:
1. Preparar a ferramenta e fazer ela ler o ROM
2. Fazer o jogo ligar e mostrar a tela de título
3. Fazer o jogo rodar completo (simulação original)
4. Adicionar tela larga e mods
5. Testar tudo e aposentar o código antigo

## Feynman Explanation — For an Expert

**Architectural pivot:** Clean-room reimplementation → Static recompilation (snesrecomp).

**Why now:** User directive — recompilation IS the project purpose (authenticity + widescreen + modding + optimization).

**Migration strategy:** 5-phase incremental with golden master validation:
1. **Framework + bootstrap**: snesrecomp submodule, CMake, `tools/regen.sh`, Docker CI
2. **Boot + Title**: CPU + minimal PPU, title screen pixel-perfect vs T021
3. **Gameplay**: Full PPU/DMA/timers, deterministic replay vs current sim
4. **Widescreen + Modding**: Native isometric renderer (StarFox Enhanced adapted), config.ini hooks
5. **Parity + Archive**: All tests pass, old reimplementation tagged `reimplementation-final`

**Key technical decisions:**
- Golden master = current reimplementation (9 tests, T020 pixel census, deterministic sim)
- Disassembly audit in Phase 1 (Yoshifanatic1 coverage)
- PolyForm Noncommercial accepted — project explicitly non-commercial
- Docker CI for reproducible Rust/Python/CMake toolchain
- `src/gen/` gitignored — user runs `tools/regen.sh` locally

## First Principles Analysis

**What problem are we solving?**
- User purpose: Run original SimCity SNES logic natively on PC with modern features
- Recompilation delivers: Original logic (bugs + all), cycle-accurate hardware, patchable generated C
- Reimplementation delivers: Fixed logic, no hardware model, owned code

**Why accept the costs?**
- License: Non-commercial accepted
- Toolchain: One-time Docker CI investment
- Migration: 14 weeks phased, each gated
- Bus factor: Fork snesrecomp if needed

**What could invalidate this?**
- Disassembly gaps in critical banks → Phase 1 audit catches
- PPU features unknown → Golden master validation catches
- snesrecomp unmaintained → Fork threshold defined

## Hostile Audit

**What if we're wrong about Phase 1 feasibility?**
- snesrecomp may not support LoROM without SA-1 → Check framework issues
- Python analyzer too slow → Rust backend required (default)
- ROM mapping errors → `tools/regen.sh` validates output

**Assumptions to validate in Phase 1:**
1. Yoshifanatic1 disassembly covers all executable banks
2. snesrecomp PPU handles Mode 1 + 3 BGs + palette + scroll
3. No HDMA/raster effects in SimCity (per disassembly)
4. LC_LZ5 decompression works in runtime (already validated 3×)

## Shadow Documents

### SD-META-006: Architectural Pivot Protocol
- **Synthesis**: When user directive changes project purpose fundamentally, AES heavy-ticket protocol (two-agent) must re-evaluate from new premise. Previous decisions (T026, T027) invalidated by new premise. Migration plan must include: golden master baseline, incremental phases, explicit risk acceptance, rollback tag.
- **Provenance**: T031 two-agent protocol with user directive as premise
- **Epistemic State**: SUPORTADA

### SD-CI-002: Multi-Language Toolchain CI Pattern
- **Synthesis**: Adding Rust + Python to C++ project requires: Docker build image (pinned versions), `tools/regen.sh` for local generation, GitHub Actions host-check (no ROM), framework pin via git submodule. User runs regeneration locally; CI validates framework + host code only.
- **Provenance**: T031 Phase 1 toolchain design
- **Epistemic State**: HIPÓTESE (not yet implemented)

## Kanban Update

| ID | Title | Status |
|----|-------|--------|
| T031 | Migration to Static Recompilation | **Phase 1: In Progress** |
| T022 | Menu system — ROM font + text | **Paused (superseded)** |
| T023 | Building sprites — map 1024-tile bank | **Paused** |
| T024 | Disaster UX | **Paused** |
| T025 | Visual regression tests | **Paused** |
| T028 | Widescreen renderer | → **Phase 4** |
| T029 | Modding API | → **Phase 4** |
| T030 | Performance optimization | → **Phase 5** |

Current reimplementation archived at git tag `reimplementation-final` after Phase 5.

## Sprint Retrospective Addition

**Pivot executed:** User directive clarified project purpose = recompilation. AES protocol handled pivot cleanly: new heavy ticket (T031), two-agent protocol, 5-phase plan with golden master validation. Previous sprint-02 tickets paused — they solved the wrong problem for the new purpose.