---
ticket: T027
phase: plan
status: in-progress
created: 2026-09-14
tier: heavy
requires:
  - aes/kanban.md
  - aes/tickets/T027-arch-reeval-recomp.md
produces:
  - aes/tickets/T027-plan.md
blocked_by: ''
---

# T027 — Plan: Architectural Re-evaluation (Two-Agent Protocol)

## Two-Agent Analysis

### Critic Phase

## Como se fosse uma criança
O usuário diz: "queremos o bolo E queremos poder mudar a receita DEPOIS que ele estiver assado, E queremos que ele caiba em formas maiores (widescreen)."
- Reimplementação: você escreveu sua própria receita. Para mudar, você reescreve o código C++. Para widescreen, você reescreve o renderer.
- Recompilação: o bolo original roda nativamente. Mods = patch no C gerado ou hook no runtime. Widescreen = renderer nativo separado (como StarFox Enhanced).
O usuário está dizendo: o valor de "moddable + widescreen nativo" vale a pena a complexidade extra.

## Como se fosse um especialista

**New requirements change the utility function:**

| Requirement | Reimplementation | Recompilation + Native Renderer |
|-------------|------------------|--------------------------------|
| **Optimization** | Native C++ sim ~same speed | Native recompiled 65C816 + hardware model overhead |
| **Modding (logic)** | Edit C++ source, rebuild | Patch generated C, or runtime hooks (like StarFox GodMode) |
| **Modding (assets)** | Already runtime ROM extraction | Same |
| **Widescreen** | Rewrite renderer + camera logic | Adapt StarFox Enhanced renderer model (separate native render path) |
| **Authenticity** | Assets ✅, Logic ❌ (fixed bugs) | Assets ✅, Logic ✅ (original + patches) |

[FAILURE MODE 1 - License unchanged]
Mechanism: PolyForm Noncommercial 1.0.0 still blocks commercial path. No evidence of dual-license or MIT fork.
Mitigation: If commercial is hard requirement, this remains veto. If "non-commercial community project" is acceptable, license passes.

[FAILURE MODE 2 - Widescreen renderer not transferable]
Mechanism: StarFox Enhanced renderer is for 3D Super FX games (polygon projection, camera FOV). SimCity is 2D tilemap + isometric projection. Different math entirely.
Mitigation: Would need to write SimCity native renderer from scratch anyway — same effort as reimplementation path.

[FAILURE MODE 3 - Modding on generated code is fragile]
Mechanism: Patching generated C = patches break on recompile. Runtime hooks need framework support (snesrecomp doesn't have generic mod API yet).
Mitigation: StarFox uses config.ini toggles (GodMode, crosshair color) — limited, not general modding.

[FAILURE MODE 4 - Migration cost underestimated]
Mechanism: 15 tickets of reimplementation → throw away? Or hybrid? Hybrid = maintain both.
Mitigation: Could keep sim core reimplemented, only recompile PPU/input? But SimCity has no complex PPU.

[ASSUMPTION]
If false: snesrecomp has generic modding API + widescreen renderer that works for 2D tile games.
Impact: Decision flips if framework provides these out of box.

[ALTERNATIVE FRAMING]
Best of both worlds: **Recompilation for CPU/PPU fidelity + Native renderer for widescreen + Reimplemented sim core for moddable logic**.
But SimCity simulation IS the CPU logic. Can't split cleanly.

## Porquê? ×5
1. Why re-evaluate? → New requirements: modding, widescreen, optimization
2. Why does recompilation help modding? → Original logic exposed, patches possible
3. Why does original logic matter for mods? → Modders expect exact game behavior as base
4. Why widescreen needs recompilation? → StarFox Enhanced shows native renderer path
5. Why is StarFox renderer reusable? → It's not; different projection math

What are we optimizing that we shouldn't be?
Optimizing for "theoretical modding/widescreen" vs "shipping working game now"

### Implementor Phase

## Como se fosse uma criança
A recompilação dá "modding de verdade" e "widescreen nativo" MAS:
- Licença ainda bloqueia comercial
- Renderer widescreen do StarFox não serve pro SimCity (2D vs 3D)
- Modding em código gerado é frágil
- Custo de migração: jogar fora 15 tickets ou manter híbrido complexo

**Proposta híbrida pragmática:**
1. **Manter reimplementação do simulation core** (já funciona, testável, moddável em C++)
2. **Adotar recompilação SÓ para PPU/input/timers** se/quando precisar de cycle-accuracy
3. **Escrever renderer nativo widescreen próprio** (inspirado no StarFox Enhanced, mas para tilemap isométrico)
4. **Modding API própria** no simulation core (Lua? JSON config? C++ plugin?)

## Como se fosse um especialista

[ADDRESSING FAILURE MODE 1 - License]
**Decisão condicional:** Se projeto aceita "non-commercial only" → recompilação viável. Se comercial é requisito → veto mantido. **Precisa clarificar goal do projeto.**

[ADDRESSING FAILURE MODE 2 - Widescreen renderer]
StarFox Enhanced renderer = 3D polygon projection with adjustable FOV. SimCity = 2D isometric tilemap (diamond grid). **Zero code reuse.** Native widescreen renderer for SimCity = new code either way. Recompilation doesn't buy this.

[ADDRESSING FAILURE MODE 3 - Modding]
snesrecomp modding = config.ini toggles (GodMode, crosshair color). Not a mod API. **Reimplementation C++ core is MORE moddable** — clean APIs, deterministic, testable. Modders prefer documented C++/Lua over patching generated C.

[ADDRESSING FAILURE MODE 4 - Migration]
**Hybrid approach:** Keep `sim::City` (reimplemented, moddable, deterministic). Use recompilation only for: CPU instruction accuracy, PPU timing, APU. But SimCity uses none of the hard PPU features. **Migration ROI negative.**

[ADDRESSING ALTERNATIVE FRAMING]
**Best architecture for stated goals:**
- **Simulation**: Reimplemented C++ (moddable, deterministic, testable) ✅ already done
- **Assets**: Runtime ROM extraction ✅ already done
- **Renderer**: Native widescreen tilemap renderer (new ticket, not recompilation-dependent)
- **Modding**: Expose sim core via Lua/C++ plugin API (new ticket)
- **Optimization**: Profile first — sim core is not bottleneck

[SOLUTION PROPOSAL]
1. **Close T027 with decision: stay reimplementation** — new requirements don't change outcome
2. **Create tickets for actual needs:**
   - T028: Native widescreen renderer for isometric tilemap
   - T029: Modding API (Lua bindings for sim::City)
   - T030: Performance profiling + optimization
3. **Re-evaluate recompilation ONLY if:** cycle-accurate PPU needed (HDMA, raster effects) OR license changes

## Porquê? ×5
1. Why not recompilation for modding? → C++ core more moddable than generated C patches
2. Why not recompilation for widescreen? → StarFox renderer doesn't transfer; new renderer needed either way
3. Why not recompilation for optimization? → Sim core not bottleneck; recompilation adds emulation overhead
4. Why is license still decisive? → PolyForm Noncommercial unchanged; commercial path blocked
5. Why is this the first principle? → Engineering serves product goals; product goals achieved better by focused tickets

## Reconnaissance Summary
- Previous T026 analysis stands; new requirements analyzed
- snesrecomp widescreen = StarFox Enhanced (3D polygon), not applicable to SimCity 2D isometric
- snesrecomp modding = config.ini toggles, not general API
- Current reimplementation: 15 tickets, deterministic, testable, asset-authentic
- License unchanged: PolyForm Noncommercial

## Technical Approach
**No code changes.** Decision documented. New feature tickets proposed for actual requirements.

## Affected Files
| File | Operation | Description |
|------|-----------|-------------|
| aes/tickets/T027-plan.md | create | This decision document |
| aes/tickets/T027-learn.md | create | Learning capture (post-review) |
| aes/tickets/T028-widescreen-renderer.md | propose | New ticket: native widescreen isometric renderer |
| aes/tickets/T029-modding-api.md | propose | New ticket: Lua/C++ modding API for sim core |
| aes/tickets/T030-perf-optimization.md | propose | New ticket: profiling + optimization |

## Verification Criteria
- [ ] Two-agent protocol completed with new requirements
- [ ] Decision recorded with updated justification
- [ ] New feature tickets proposed for widescreen/modding/optimization
- [ ] License condition explicitly stated

## Estimation
- Complexity: low (analysis only)
- Risk: medium (architectural)
- Blocking dependencies: no