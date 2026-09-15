---
ticket: T004
phase: plan
status: done
created: 2026-09-11
tier: standard
requires:
  - aes/kanban.md
  - aes/tickets/T004-rom-map-loader.md
produces:
  - aes/tickets/T004-plan.md
blocked_by: ''
---

# T004 — Plan

## Reconnaissance Summary
- Existing: `src/snes/rom.{h,cpp}` provides `SnesRom` with `translate()`, `read8/16/32()`, and header parsing.
- `sim::City` currently 128×64; must resize to 120×100.
- `Game::init()` creates `City` and `CityView`; natural injection point for ROM map seeding.
- Empirical ROM scan (see ticket) found valid packets at `$0F:C184` and `$0F:C4B4` decoding to ~12000 tiles via two-stage decompression.
- Scenario pointer table at `$03:CE70` (split low/hi/bank) — but those pointers decompressed poorly; may be for select-screen images, not full maps. Will use empirical packet locations as fallback with table as cross-reference.

## Two-Agent Analysis
*This is a standard-tier ticket; critic/implementor inline below.*

### CRITIC PHASE
```
## Como se fosse uma criança
É como tentar ler um mapa do tesouro que foi escrito em código secreto, comprimido duas vezes, e o guia de decodificação está em notas rasuradas de hackers de 30 anos atrás. Se você errar um byte, a cidade nasce errada — ou o jogo cai.

## Como se fosse um especialista
Implementar o descompressor Nintendo (formato de pacote com byte de controle) e o RLE de 16-bit (contagem de repetição nos bits 10-13). O formato é documentado por bbbradsmith (para texto) e lytron (para mapas de cenário), mas a rotina COP #08 usada em SimCity pode ter variantes. O ponteiro da tabela de cenários em $03:CE70 não produziu mapas válidos em meu teste — pode apontar para retratos, não mapas completos. O scan empírico achou pacotes válidos em $0F:C184 e $0F:C4B4. Risco: confiar no scan empírico em vez da tabela oficial.

[FAILURE MODE 1]
Mechanism: O descompressor Nintendo tem modo 0xE0 (extend) que muda o mode depois de ler byte extra. Se a ordem de leitura estiver errada (ler o byte extra antes de verificar mode==0xE0), o stream desincroniza e lixo é produzido.

[FAILURE MODE 2]
Mechanism: O RLE de 16-bit usa $FFFF como terminador, mas o stream descomprimido pode conter $FFFF como dado válido (tile ID 0x3FF + repetição 15). Se não distinguir terminador de dado, o mapa truncará prematuramente.

[FAILURE MODE 3]
Mechanism: Back-references relativas (mode 0xC0/0xE0) calculam `ref = len(d) - next_byte()`. Se `next_byte()` > `len(d)`, ref fica negativo → crash ou leitura fora do buffer. O código bbbradsmith checa `ref < 0` mas nossa implementação Python ignorou.

[ASSUMPTION]
Se os pacotes empíricos em $0F:C184/$0F:C4B4 forem os mapas de cenário corretos, então a tabela em $03:CE70 é irrelevante para o mapa jogável.
If false: Carregamos o mapa errado (tela de título, não cidade inicial). A cidade nasce com terreno de demo.

[ALTERNATIVE FRAMING]
Em vez de depender de offsets fixos descobertos empiricamente, implementar busca heurística: varre ROM procurando pacotes que decodifiquem para ~12000 tiles e validam estrutura (borda de água, interior de terra). Mais robusto, mas mais lento no boot.

## Porquê? ×5
1. Why use empirical offsets? → Because the documented pointer table failed to produce valid maps.
2. Why did the pointer table fail? → It likely points to scenario select-screen graphics (smaller, different format).
3. Why trust bbbradsmith's format? → It's the only public reverse-engineering of the Nintendo packet format used by SimCity; lytron corroborates for the second stage.
4. Why 120×100 not 128×64? → Three independent sources (lytron, Gingold, snescityeditor) confirm 120×100.
5. Why not use zlib/LZSS? → The format is proprietary Nintendo packet, not standard compression.
```

### IMPLEMENTOR PHASE
```
## Como se fosse uma criança
Vou construir uma máquina que pega o código comprimido do jogo original, descomprime camada por camada, e transforma em um mapa de terreno que nosso motor entende. Se der erro, uso o mapa gerado automaticamente como plano B.

## Como se fosse um especialista
Implementar `NintendoDecompressor` stateless: recebe `SnesRom&` + offset inicial, retorna `vector<uint8_t>` + offset final (ou error). Segue exatamente o autômato do bbbradsmith: checa 0xFF, extrai mode/length, trata 0xE0 extend PRIMEIRO, depois despacha por mode. Back-refs com bounds check. Segunda etapa: `decode_rle16()` itera LE u16, quebra em 0xFFFF, expande contagem (bits 10-13) + tile (bits 0-9). Parser de tabela em $03:CE70 lê 9 entradas (lo/hi/bank em arrays separados). Tenta cada uma; se falhar, cai para offsets empíricos conhecidos. Mapeamento tile→terreno: tabela fixa baseada em lytron/snescityeditor.

[ADDRESSING FAILURE MODE 1]
Implementar `if (mode == 0xE0) { l = next(); length = l | ((ctrl&3)<<8); length++; mode = (ctrl<<3)&0xE0; }` ANTES do switch. Testar com packet conhecido ($4A $01 $3C...).

[ADDRESSING FAILURE MODE 2]
No decode_rle16: só trata $FFFF como terminador se NÃO estiver no meio de uma repetição (ou seja, lê u16, se ==0xFFFF break). Como o formato garante que $FFFF só aparece como sentinela (lytron: "At the end of each scenario data, they write an '$FF $FF $FF'"), é seguro.

[ADDRESSING FAILURE MODE 3]
Back-ref relativo: `int ref = (int)d.size() - next_byte(); if (ref < 0) return error;`. Absoluto: `ref = next_byte() | (next_byte()<<8); if (ref >= (int)d.size()) return error;`. Adicionar bounds em todos os modos.

[ADDRESSING ASSUMPTION]
Tentar a tabela oficial PRIMEIRO. Se nenhum dos 9 ponteiros gerar ≥11000 tiles válidos, usar offsets empíricos hardcoded (0x7C184, 0x7C4B4). Logar qual fonte foi usada.

[ADDRESSING ALTERNATIVE FRAMING]
Heurística de scan custa ~500ms no boot (varre 512KB). Aceitável, mas desnecessário se offsets fixos funcionarem. Deixar como fallback documentado no código, não ativado por default.

[SOLUTION PROPOSAL]
1. `src/snes/decompress.h/cpp`: `bool nintendo_decompress(SnesRom&, size_t off, vector<uint8_t>& out, size_t& end_off)`.
2. `src/snes/scenariomap.h/cpp`: `decode_rle16()`, `parse_scenario_table()`, `load_scenario_terrain(int idx, City&)`.
3. `src/tools/rommap.cpp`: CLI `rommap <rom> [scenario_idx]` → dump tiles + ASCII preview.
4. `sim::City`: `apply_terrain_map(const uint16_t* tiles_120x100)`, resize grid to 120×100.
5. `engine/game.cpp`: em `init()`, após criar `City`, chamar `load_scenario_terrain(0, city_)`; se falhar, log warning e usar default.
6. Testes: `test_decompress.cpp` (vectors do ROM), `test_scenariomap.cpp` (RLE + table), integração em `test_city.cpp` (resize + seed).

## Porquê? ×5
1. Why stateless decompressor? → Pure function = testable, no hidden state, thread-safe.
2. Why try table first then empirical? → Defesa em profundidade: tabela é "oficial", empírico é fallback validado.
3. Why hardcode tile→terrain table? → Sem dados de paleta no RLE; tile ID semântico é estável entre versões US/EU.
4. Why resize City to 120×100? → Alinhamento com hardware original; evita scaling/offsets estranhos na renderização.
5. Why feature-flag fallback? → Segurança: se ROM for versão diferente (EU/JP), não quebra o jogo.
```

## Hostile Analysis

### ASSUMPTIONS I AM MAKING
- **[KNOWN]** LoROM mapping formula: `file_off = bank * 0x8000 + (addr & 0x7FFF)`. Justification: used by `SnesRom::translate()`, verified with header vectors.
- **[INFERRED]** Nintendo packet format for SimCity scenario maps = bbbradsmith's format. Evidence chain: bbbradsmith reverse-engineered from $0090A6 (text decompressor); lytron says COP #08 routine is shared with title screen; both games use same compression family.
- **[ASSUMED]** Scenario 0 (first table entry) = beginner/starting city map. Impact if false: loads wrong scenario (e.g., San Francisco instead of blank), but still a valid terrain map.
- **[ASSUMED]** Tile IDs semantic mapping (0=empty, 1-3=water, 14-25=forest, 30-5F=road) is stable across US/EU ROMs. Impact if false: EU ROM seeds wrong terrain; fallback to generated.
- **[UNKNOWN]** Whether the `$03:CE70` table entries are actually compressed with the same routine. Why outside reliable knowledge: my Python decompressor hit bounds errors on all 9 entries; could be bug in my impl or different format.

### WHAT WAS NOT SPECIFIED (that matters)
- Exact tile→terrain mapping for shore tiles (0x04-0x13) — I'll map all to Grass (buildable).
- Whether scenario high bits (zone/power flags) should be preserved — OUT OF SCOPE per ticket.
- Behavior when map decode produces <12000 tiles (partial) — truncate and log.

### ALTERNATIVES NOT CHOSEN
- **Heuristic full-ROM scan for packets** — Rejected: 500ms boot penalty, unnecessary if fixed offsets work.
- **Embed decompressed map as C array** — Rejected: violates Zero Asset Distribution (would be derived asset).
- **Use zlib on raw ROM slices** — Rejected: wrong format; Nintendo packet is not DEFLATE.

### RISKS AND SIDE EFFECTS
- Resizing `City` from 128×64 to 120×100 changes memory layout; all existing tests use coordinates < 100, so should pass.
- `CityView` world size changes (120*16=1920w, 100*16=1600h); camera clamp logic already dynamic.
- If Nintendo decompressor has off-by-one in extend mode, entire map corrupts silently.

### COST OF BEING WRONG: MEDIUM
- Wrong terrain → visual glitch, not crash. Gameplay still functional (zones, sim, budget work).
- Crash in decompressor → caught by `SDL_VIDEODRIVER=dummy` smoke test; fallback triggers.

### SCOPE BOUNDARY
This plan covers: Nintendo decompressor, RLE decoder, scenario table parser, terrain seeding, grid resize, CLI debug tool. Excludes: zone/building seeding, scenario selection UI, save/load, audio.

### INVITATION FOR CONTRADICTION
What critical flaw might I be missing? The pointer table at `$03:CE70` might use a DIFFERENT compression (lytron says "the data itself gets decompressed when it gets transfered from ROM to $7E8000" — implying the table points to the FIRST-stage compressed data, which my decompressor should handle). But my test failed on all 9. Either my decompressor has a bug, or those aren't map packets. I'm proceeding with empirical offsets as primary and table as validation — if the table works, great; if not, empirical is proven.

## Technical Approach
1. **NintendoDecompressor** — pure function, no state, returns `std::expected<vector<uint8_t>, DecompressError>` (or bool+out param for C++17).
2. **RLE16Decoder** — iterates u16 LE, expands repeat counts, stops at 0xFFFF or 12000 tiles.
3. **ScenarioTable** — reads 9 entries from `$03:CE70` (split arrays), tries each; validates by tile count ≈12000.
4. **City::apply_terrain_map** — loops 120×100, maps tile ID → Terrain via constexpr array.
5. **Game integration** — call in `init()` after `City` construction; feature flag `--no-rom-map` for debugging.

## Affected Files
| File | Operation | Description |
|------|-----------|-------------|
| src/snes/decompress.h | create | Nintendo packet decompressor interface |
| src/snes/decompress.cpp | create | Implementation + error enum |
| src/snes/scenariomap.h | create | RLE decoder + scenario table parser |
| src/snes/scenariomap.cpp | create | Implementation |
| src/sim/city.h | modify | Add apply_terrain_map, resize constants to 120×100 |
| src/sim/city.cpp | modify | Implementation of apply_terrain_map |
| src/engine/game.cpp | modify | Seed terrain in init(); add --no-rom-map flag |
| src/tools/rommap.cpp | create | CLI debug tool |
| tests/test_decompress.cpp | create | Unit tests for Nintendo decompressor |
| tests/test_scenariomap.cpp | create | Unit tests for RLE + table |
| CMakeLists.txt | modify | Add new sources, link simcity_sim to engine |

## Specification
### NintendoDecompressor
```cpp
enum class DecompressError { None, Truncated, BadControl, BadRef, BufferOverflow };

bool nintendo_decompress(const SnesRom& rom, size_t offset,
                         std::vector<uint8_t>& out, size_t& end_offset);
// Returns true on success; out = decompressed bytes; end_offset = byte after 0xFF control.
```

### RLE16Decoder
```cpp
bool decode_rle16(const uint8_t* data, size_t len, uint16_t* out_tiles, size_t max_tiles);
// Returns true if terminated by 0xFFFF and tile_count <= max_tiles.
// out_tiles filled with low-10-bit tile IDs.
```

### ScenarioTable
```cpp
struct ScenarioEntry { uint32_t rom_addr; size_t file_off; };
std::vector<ScenarioEntry> parse_scenario_table(const SnesRom& rom);
// Reads $03:CE70/$03:CE79/$03:CE82 arrays (9 entries).
```

### City::apply_terrain_map
```cpp
void apply_terrain_map(const uint16_t* tiles_120x100);
// tiles_120x100: row-major 120×100, low 10 bits = tile char.
// Mapping: constexpr Terrain tile_to_terrain[1024] initialized once.
```

## Testing Strategy
- **test_decompress.cpp**: 3 cases — (1) copy mode 0x00, (2) byte repeat 0x20, (3) word repeat 0x40, (4) extend mode 0xE0, (5) back-ref 0x80/0xC0. Vectors from known ROM packets (0x7C184 first 32 bytes).
- **test_scenariomap.cpp**: (1) decode_rle16 with synthetic stream (water border + land interior), (2) parse_scenario_table returns 9 entries, (3) load_scenario_terrain(0) produces valid terrain array (water border present).
- **test_city.cpp extension**: verify City resize to 120×100, apply_terrain_map sets correct terrain counts.

## Verification Criteria
- [ ] All tests pass: `ctest` → 6/6 (4 existing + 2 new).
- [ ] Headless smoke: `SDL_VIDEODRIVER=dummy timeout 3 ./build/simcity "SimCity (USA).sfc"` → frames > 0, exit 124.
- [ ] `./build/rommap "SimCity (USA).sfc" 0` prints tile counts, ASCII preview shows water border.
- [ ] No new lint warnings (`clang-tidy` if available).
- [ ] No regressions in T001–T006 functionality.

## Estimation
- Complexity: medium (2–8h)
- Risk: medium
- Blocking dependencies: no (all deps done)