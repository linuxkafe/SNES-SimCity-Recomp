---
ticket: T031
phase: plan
status: in-progress
created: 2026-09-14
tier: heavy
requires:
  - aes/kanban.md
  - aes/tickets/T031-migration-to-recompilation.md
produces:
  - aes/tickets/T031-plan.md
blocked_by: ''
---

# T031 — Plan: Migration to Static Recompilation (Two-Agent Protocol)

## Two-Agent Analysis

### Critic Phase

## Como se fosse uma criança
O projeto inteiro vai ser reescrito. Em vez de escrever nossa própria versão do SimCity em C++, vamos pegar o código original do SNES (65C816), passar por uma máquina (snesrecomp) que traduz para C nativo, e rodar isso com um emulador de hardware SNES embutido. É como trocar o motor do carro enquanto ele está andando — arriscado, mas o resultado roda o código ORIGINAL exato.

## Como se fosse um especialista

**Current state:** 15 tickets completed, clean-room C++17 reimplementation, deterministic sim core, runtime ROM asset extraction, SDL2 renderer, 9/9 tests pass.

**Target state:** snesrecomp-based: 65C816 → C via Rust analyzer, SNES runtime (CPU/PPU/APU/DMA), recompiled SimCity logic runs natively, native widescreen renderer (StarFox Enhanced model), modding via generated C patches.

[FAILURE MODE 1 - License]
Mechanism: PolyForm Noncommercial 1.0.0 prohibits commercial use. Project accepts this (user directive: non-commercial is fine).
Mitigation: Explicit acceptance documented. No commercial path ever.

[FAILURE MODE 2 - Toolchain complexity]
Mechanism: Adds Rust toolchain, Python 3.9+ analyzer, snesrecomp framework (~50MB), custom CI, framework pin management. Build goes from `apt install build-essential cmake libsdl2-dev` to multi-language pipeline.
Mitigation: Documented in CI/CD; pinned framework commits; Docker build image for reproducibility.

[FAILURE MODE 3 - Disassembly completeness]
Mechanism: snesrecomp needs per-bank function metadata, memory maps, hardware register definitions. SimCity disassembly (Yoshifanatic1) must cover all banks used. Gaps = recompilation failures or wrong behavior.
Mitigation: Audit disassembly coverage before migration; stub unknown functions with runtime traps.

[FAILURE MODE 4 - PPU modeling gaps]
Mechanism: SimCity may use mid-frame palette swaps, HDMA, or raster effects not modeled in snesrecomp PPU. StarFox Enhanced renderer bypasses PPU for 3D; SimCity 2D tilemap needs accurate PPU.
Mitigation: snesrecomp PPU must pass SimCity visual validation (T020 pixel census). If gaps found, contribute upstream or fork.

[FAILURE MODE 5 - Migration scope]
Mechanism: 15 tickets of working reimplementation → archive. New bugs in recompiled code + runtime. Regression surface huge.
Mitigation: Incremental migration: (1) framework integrate, (2) boot ROM, (3) title screen, (4) gameplay. Each step validated against current test suite.

[ASSUMPTION]
If false: SimCity uses undocumented PPU features or SA-1/DSP coprocessor.
Impact: Recompilation fails or needs major framework work.

[ALTERNATIVE FRAMING]
Hybrid: Keep C++ sim core (deterministic, testable), recompile only PPU/input. But SimCity logic IS the CPU code — can't split cleanly. User directive: full recompilation.

## Porquê? ×5
1. Why migrate? → User directive: recompilation IS the project purpose
2. Why recompilation? → Original logic + widescreen + modding + optimization
3. Why accept license? → Project is non-commercial; authenticity > commercialization
3. Why accept toolchain? → One-time cost; enables goals unattainable otherwise
4. Why throw away 15 tickets? → They solve different problem (reimplementation); new problem = recompilation
5. Why is this the first principle? → Project purpose defined by user; engineering serves purpose

What are we optimizing that we shouldn't be?
Optimizing for "clean-room purity" when user wants "original binary authenticity + modern features"

### Implementor Phase

## Como se fosse uma criança
Vamos fazer a migração em passos pequenos, cada um testado:
1. Colocar snesrecomp como submódulo, compilar framework
2. Configurar banks do SimCity (LoROM, sem coprocessador)
3. Rodar analisador no ROM → gera C em src/gen/
4. Compilar runtime + C gerado → bootar ROM
5. Title screen via recompiled code (valida PPU/paleta)
6. Gameplay loop via recompiled code
7. Renderer nativo widescreen (adaptar StarFox Enhanced para tilemap isométrico)
8. Modding: hooks no C gerado + config.ini
9. Testes: comparar saída com reimplementação atual (golden master)

## Como se fosse um especialista

[ADDRESSING FAILURE MODE 1 - License]
**Accepted:** PolyForm Noncommercial 1.0.0 — project explicitly non-commercial. Documented in LICENSE.md and README.

[ADDRESSING FAILURE MODE 2 - Toolchain]
**Mitigation:** 
- Dockerfile for CI: `rust:1.75`, `python:3.11`, `cmake`, `ninja`, `sdl3`
- `tools/regen.sh` regenerates src/gen/ from ROM (user runs locally)
- GitHub Actions: framework pin + host-check build (no ROM)
- Local dev: `bash tools/regen.sh && cmake -B build && cmake --build build`

[ADDRESSING FAILURE MODE 3 - Disassembly]
**Plan:**
- Audit Yoshifanatic1/SimCity-SNES-Disassembly coverage
- Map: bank $00-$0F (system), $10-$1F (game logic), $20-$2F (graphics), $30-$3F (maps)
- Per-bank `recomp/*.toml` with function signatures, memory maps, hardware regs
- Unknown functions → `TRAP_UNIMPLEMENTED` in runtime

[ADDRESSING FAILURE MODE 4 - PPU]
**Validation:** 
- T020 pixel census = golden master for PPU accuracy
- StarFox Enhanced PPU model adapted: tilemap rendering + palette + scroll
- SimCity PPU simpler: Mode 1, 3 BGs, no raster effects (per disassembly)
- If HDMA found → implement in runtime

[ADDRESSING FAILURE MODE 5 - Migration]
**Incremental phases:**
```
Phase 1: Framework + bootstrap (week 1-2)
  - snesrecomp submodule, CMake integration, ROM verify
  - tools/regen.sh produces src/gen/
  - Host-check build (no ROM) passes CI

Phase 2: Boot + Title (week 3-4)
  - CPU + minimal PPU + APU stub
  - Boot to title screen via recompiled code
  - Visual diff vs T021 current output

Phase 3: Gameplay (week 5-8)
  - Full PPU, DMA, timers, input
  - Scenario load, city sim via original logic
  - Determinism test: same ROM → same state after N frames

Phase 4: Widescreen + Modding (week 9-12)
  - Native renderer (isometric tilemap, variable viewport)
  - Config.ini hooks (GodMode, disaster toggle, etc.)
  - Generated C patch points for modders

Phase 5: Parity + Cleanup (week 13-14)
  - All 9 ctest adapted + pass
  - Headless smoke passes
  - Archive old reimplementation (git tag `reimplementation-final`)
```

[ADDRESSING ALTERNATIVE FRAMING]
Hybrid rejected per user directive. Full recompilation is the purpose.

[SOLUTION PROPOSAL]
**Execute 5-phase migration** with validation gates at each phase. Current reimplementation serves as golden master for behavioral comparison. New tickets created for each phase.

## Porquê? ×5
1. Why phased? → Risk reduction; each phase validates before next
2. Why golden master? → Behavioral equivalence proof; catches regressions
3. Why Docker CI? → Reproducible toolchain; onboards contributors
4. Why per-bank config? → snesrecomp requirement; enables incremental analysis
5. Why this first principle? → User purpose = recompilation; migration is the work

## Reconnaissance Summary
- Current: 15 tickets, C++17, deterministic sim, asset extraction, SDL2, 9/9 tests
- snesrecomp: Rust analyzer, Python fallback, SNES runtime, StarFox Enhanced widescreen ref
- SimCity: LoROM, 512KB, no coprocessor, Mode 1 PPU, LC_LZ5 compression
- Disassembly: Yoshifanatic1 covers banks $00-$3F, function labels, hardware regs
- Shadow docs: SD-META-003 (extraction pattern), SD-DOMAIN-001/002 (sim/PPU)

## Technical Approach
**5-phase incremental migration** with behavioral validation against current reimplementation at each gate. No big-bang cutover.

## Affected Files (Phase 1)
| File | Operation | Description |
|------|-----------|-------------|
| .gitmodules | create | snesrecomp submodule (pinned commit) |
| CMakeLists.txt | modify | Add snesrecomp, Rust/Python detection, regen target |
| tools/regen.sh | create | Regenerates src/gen/ from ROM via snesrecomp |
| recomp/ | create | Per-bank TOML configs (memory maps, function metadata) |
| src/gen/ | create (gitignored) | Recompiler output — never committed |
| Dockerfile.ci | create | CI build image with Rust/Python/CMake |
| .github/workflows/ci.yml | modify | Framework pin, host-check, regen validation |

## Specification (Phase 1)
- `bash tools/regen.sh "SimCity (USA).sfc"` → produces `src/gen/*.c`, `src/gen/*.h`
- `cmake -B build && cmake --build build` → links runtime + generated code
- `./build/simcity` → boots via recompiled 65C816

## Testing Strategy
- **Golden master tests:** Current test suite outputs → reference for Phase 2+
- **Phase 1:** Host-check build (no ROM) in CI
- **Phase 2:** Title screen pixel diff vs T021 current
- **Phase 3:** Deterministic replay: same inputs → same state after N frames
- **Phase 4:** Widescreen visual validation; modding hook tests
- **Phase 5:** All 9 ctest adapted + pass; headless smoke

## Verification Criteria
- [ ] snesrecomp submodule pinned, framework builds
- [ ] tools/regen.sh produces valid C from SimCity ROM
- [ ] Host-check CI passes (no ROM required)
- [ ] Phase 2: Title screen boots, visual match
- [ ] Phase 3: Gameplay boots, deterministic replay
- [ ] Phase 4: Widescreen renderer works, mod hooks functional
- [ ] Phase 5: All tests pass, old reimplementation archived

## Estimation
- Complexity: high (> 8h per phase, 14 weeks total)
- Risk: high (architectural pivot, framework dependency)
- Blocking dependencies: yes — snesrecomp framework readiness, disassembly audit