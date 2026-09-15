---
ticket: T027
phase: learn
status: done
created: 2026-09-14
requires:
  - aes/tickets/T027-review.md
produces:
  - aes/tickets/T027-learn.md
  - aes/tickets/T028-widescreen-renderer.md
  - aes/tickets/T029-modding-api.md
  - aes/tickets/T030-perf-optimization.md
  - aes/shadow/SD-META-005.md
---

# T027 — Learn: Architectural Re-evaluation with New Requirements

## Feynman Explanation — For a Child

O usuário quer três coisas: jogo rápido (otimização), poder mudar as regras (modding), e tela larga (widescreen).
- **Recompilação** promete isso tudo, MAS: a licença proíbe uso comercial, o renderer widescreen do StarFox é para jogos 3D (não serve pro nosso 2D), e mexer no código gerado é como tentar consertar um relógio com luvas de boxe.
- **Nossa abordagem atual** já roda rápido, já extrai assets do ROM, e o código C++ é FÁCIL de mudar.
- **Solução:** Ficar com nossa abordagem e criar 3 tickets específicos:
  1. **T028**: Escrever nosso próprio renderer widescreen para mapa isométrico 2D
  2. **T029**: Adicionar API de modding (Lua) no simulation core
  3. **T030**: Medir performance e otimizar onde importa

## Feynman Explanation — For an Expert

**Requirement mapping to architecture:**

| User Goal | Recompilation Claim | Reality Check | Better Path |
|-----------|---------------------|---------------|-------------|
| **Optimization** | Native recompiled 65C816 | Adds hardware emulation overhead; sim core not bottleneck | Profile → optimize C++ sim core (T030) |
| **Modding** | Patch generated C / config.ini | Fragile, no API, framework-dependent | Lua/C++ plugin API on owned sim::City (T029) |
| **Widescreen** | StarFox Enhanced renderer | 3D polygon FOV ≠ 2D isometric tilemap | Native isometric renderer with variable viewport (T028) |

**Key insight:** The "recompilation enables X" argument assumes framework provides X. snesrecomp provides: cycle-accurate CPU/PPU/APU, Super FX, config.ini toggles. It does NOT provide: generic modding API, 2D widescreen renderer, optimization for simulation-heavy no-PPU games.

**License gate:** PolyForm Noncommercial 1.0.0 remains. Decision conditional: IF project accepts non-commercial-only → recompilation technically viable but still inferior for stated goals. IF commercial required → hard veto.

## First Principles Analysis

**What problem are we solving?**
- User wants: optimization, modding, widescreen
- These are **feature goals**, not architecture goals
- Architecture should enable features with minimal complexity

**Why did StarFox choose recompilation?**
- Super FX microcode = infeasible to reimplement
- 3D renderer widescreen = adjustable FOV on polygon projection
- **SimCity has neither Super FX nor 3D projection**

**What would migration cost?**
- 3-6 months framework integration
- Lose deterministic headless-testable sim core
- Gain: cycle-accurate PPU (unused), config.ini toggles (limited)
- License: forever non-commercial

## Hostile Audit

**What if we're wrong?**
1. **snesrecomp adds modding API + 2D widescreen support** → Re-evaluate (monitor framework)
2. **SimCity uses undiscovered HDMA/raster effects** → Recompilation handles automatically (risk: LOW per disassembly)
3. **Project decides non-commercial is fine** → License gate passes, but technical arguments unchanged

## Shadow Document

### SD-META-005: Feature Goals ≠ Architecture Goals
- **Synthesis**: User requirements (optimization, modding, widescreen) are feature goals. Architecture should be chosen to deliver features with minimal complexity. Recompilation provides infrastructure (CPU/PPU emulation) that SimCity doesn't need, while lacking the specific features wanted (2D widescreen renderer, modding API). Direct feature implementation on owned codebase is superior.
- **Provenance**: T027 two-agent protocol with updated requirements
- **Epistemic State**: SUPORTADA

## Feature Tickets Created

### T028: Native Widescreen Renderer for Isometric Tilemap
- Render diamond-grid isometric map at arbitrary aspect ratios
- Variable viewport width, centered vertical
- HUD anchoring at screen edges
- No recompilation dependency

### T029: Modding API (Lua Bindings for sim::City)
- Expose sim::City read/write via Lua
- Event hooks: onMonth, onZoneChange, onDisaster
- Config-driven rule overrides (tax rates, growth formulas)
- Sandboxed, deterministic, testable

### T030: Performance Profiling + Optimization
- Instrument sim::City step_month()
- Identify hot paths (power grid, RCI calculation, zone growth)
- Optimize: spatial indices, caching, parallelization
- Target: 60 FPS at 4K headless

## Sprint-02 Update

Current focus unchanged:
| ID | Title | Status |
|----|-------|--------|
| T022 | Menu system — ROM font + text | **in-progress** |
| T023 | Building sprites — map 1024-tile bank to zones | pending |
| T024 | Disaster UX — earthquake or gate random disasters | pending |
| T025 | Visual regression tests — headless pixel assertions | pending |

New tickets (T028-T030) → backlog for sprint-03.