---
ticket: T017
phase: 1-2
title: ROM Asset Extraction & Game Integration — Plan
status: pending-approval
---

# T017 Plan — Hostile Analysis + Solution Proposal

## Phase 0 Recon Summary (done)
- `simcity` renders flat colours; `gfx/tile.cpp` decoder unused by CityView.
- Two confirmed bugs: (A) `length += 1` missing for all LC_LZ5 modes in
  `decompress.cpp`, (B) `read8()/read16()` called with raw **file offsets** in
  `decompress.cpp`, `scenariomap.cpp` (double translation: passed offset is
  re-translated as a CPU address).
- Validated fix in Python against real ROM (MD5 `23715fc7…`):
  - 9/9 scenario maps decode, packet ends chain vs disassembly table exactly.
  - Layer1 city tile packet at file `0x3E584` → 32768 B = 1024 tiles ok.
- Disassembly (USA) pointers: maps `$0C8F27–$0DD77C`; tiles Layer1 `$07E584–$08C4DB`;
  BG palettes `$058000–$058E00` (raw BGR555); Layer2 `$05B000` (raw, animated).

---

## Phase 1 — Hostile Analysis

### INSIGHTS CONSULTED
- SD-T004 notes (empirical offsets were false positives; table points at
  select-screen graphics — **WRONG, corrected by this ticket**: table entries are
  the real scenario maps; the C++ read path was simply broken).
- bbbradsmith LC_LZ5 spec; lytron map notes; Yoshifanatic1 AssetPointersAndFiles.asm.

### ASSUMPTIONS (with uncertainty)
- [KNOWN] ROM MD5 `23715fc7ef700b3999384d5be20f4db5` = USA exact → disassembly pointers apply. Justification: MD5 match against Yoshifanatic1's USA ROM.
- [KNOWN] LC_LZ5 length semantics: low 5 bits (or 10-bit extended) are length−1 for **all** modes. Justification: validated 9/9 map decodes + bbbradsmith original.
- [KNOWN] `SnesRom::data()` exposes raw bytes; no CPU-address translation needed for asset reads. Justification: `rom.h` line 40.
- [INFERRED] Layer1 tileset (file `0x3E584`) is the in-game terrain/zone tile bank (1024 tiles). Evidence: 32768 B raw = 1024×32B exactly; disassembly name `GFX_Layer1_CitySimulationTiles`.
- [INFERRED] City view uses one of the 14 BG palette blocks (`$058000+`) as its base palette; the exact set needs a render smell-test.
- [ASSUMED] Scenario 0 (San Francisco) is an acceptable default seed; the city sim maps low-10-bit tile IDs via `kTileToTerrain[1024]` (city.cpp lines 197–205). Impact if wrong: incorrect terrain lookup → sim treats grass where water/forest should be.

### WHAT WASN'T SPECIFIED
- Which scenario to load on startup (currently hardcoded index 0; called before sim core provides scenario selection).
- Whether zones (R/C/I) should render from ROM tiles or keep the flat-colour overlay. Plan: terrain+water+forest+park from ROM; zone overlays keep flat tint (sim has no ROM tile art for empty zones until built).

### ALTERNATIVES NOT CHOSEN
- **Option A: raw `void*` cursor decompressor** (like lytron's WRAM streaming) — rejected: portfolio only needs a byte-vector contract; complexity not justified.
- **Option B: keep `read8()` and pass CPU addresses** — rejected: the bytes we need are LO-ROM mapped assets; forcing CPU addresses at call sites duplicates the lossy translation logic and resurrects the class of bug (T004) this ticket kills.
- **Option C: add a second `SnesRom::read8_file()` API and keep `read8()` untouched** — rejected (partially adopted): `data()` already exposes raw bytes; adding per-call APIs increases surface. Decided: asset readers index `rom.data()` directly (const access, zero-copy).

### INVITE CONTRADICTION
- Could the scenario table still be the "select-screen graphics" table and the real maps live elsewhere? Falsification attempt: our decode produced geographically-sane 120×100 maps with massive water borders from those very packets, packet ends chain to `CompressedSPCBlock1` at `0x06D77C`. That is decisive.
- Could 1024 tiles at 0x3E584 be wrong bank? Falsification: `ok=True` and exact 32768 size strongly suggests a complete tile bank; margin to test visually via `tileview`-style dump before wiring CityView.

### RISKS & SIDE EFFECTS
- Changing `nintendo_decompress` callers (rommap, scenariomap, tests) — public contract already returns `bool` + output vector; **signature unchanged**, only internal byte-source changes → low blast radius.
- `parse_scenario_table` entry addresses change → `load_scenario_terrain` starts returning `true`; the fallback empirical offsets become dead code (acceptable, removed).
- CityView grows a texture-atlas build; must keep the headless smoke path (SDL dummy driver) working.

### COST OF BEING WRONG: MEDIUM
- Terrain tile mapping or wrong palette set → visual glitches only, sim core unaffected (NFR-2/3 preserved). Decompression is the verified-hard part; the visual mapping is easily corrected by inspection.

### REASONING SKELETON
- [Decomp outputs are wrong] → [LC_LZ5 needs length+1 in every mode] + [read8() re-translates file offsets] → [every ROM asset decode is corrupt] → [game loads no assets].
- [Corrected decode succeeds] → [9 maps + tileset verified] → [asset loading is now possible end-to-end].

### SCOPE BOUNDARIES
In: decompress fix + regression tests, scenario table fix, terrain seeding in `game.cpp`, raw tile/palette extraction into CityView atlas, rommap fix, docs. Out: audio (T010), UI tilemaps, sprites/animation, scenario system (T011). Rationale: this ticket restores *asset plumbing* so later tickets consume it.

---

## CRITIC PHASE (in-session, adversarial)

**[FAILURE MODE 1] Palette set mis-identification**
Mechanism: 14 BG palette blocks; picking the wrong base palette yields inverted/black terrain even with correct tiles.
Mitigation: render-smell harness in the build ticket — decode block *k*, dump a low-res PNG of a hand-built tile map via existing `Image` machinery; confirm water/grass hues match gameplay footage expectations; gate on visible check, not just "no crash".

**[FAILURE MODE 2] Tile ID ↔ sim::Terrain mapping breaks existing tests**
Mechanism: `kTileToTerrain` maps IDs 0–127; scenario maps contain shores `04–13`, park `26–27`, pollutants `28–29`, rails `70–7F` that map to Grass/Road. If I render purely by tile ID but apply_tool writes sim types, rebuilt tiles may desync from blitted tiles.
Mitigation: keep CityView keyed on `sim::TileView` (terrain+zone) exactly as today; add a `terrain→tile id` lookup table for ROM tiles, not the reverse. Renderer stays driven by sim state.

**[FAILURE MODE 3] Headless smoke regression**
Mechanism: `CityView::rebuild` adds SDL texture atlas ops that fail under dummy driver or on texture-size limits (1024 tiles → atlas fine, but per-tile scale 16px in `kTilePx`).
Mitigation: pre-build atlas at `init()`; fallback to flat colour if any asset step fails, so `make test`/smoke never hard-fails. Gate: smoke exit 0 enforced.

**[ASSUMPTION] Scenario 0 always present**
If false: `load_scenario_terrain(rom,0)` returns true for US ROM (verified) but index is only 0..8; guard table before indexing.

**[ALTERNATIVE FRAMING]**
Are we optimizing renderer fidelity we don't need yet? Answer: we're optimizing *proving that ROM assets load end-to-end*. Rendering real tiles is the only externally visible proof; flat-colour stays as fallback.

## IMPLEMENTOR PHASE

1. **decompress.cpp**: replace byte reads with `rom.data()` indexing (file offsets); move `length += 1` out of the `0xE0` branch so it applies to all modes; keep signature.
2. **scenariomap.cpp**: read lo/hi/bank via `rom.data()[base+i]`; correct table bases; remove empirical-offset dead fallback.
3. **Tests** (`test_decompress.cpp`): add real-ROM regression: each of the 9 entries decodes with `ok=true` and ≥8000 tiles; packet-end chaining; Layer1 tile packet == 1024 tiles; RLE terminator edge (`v==0xFFFF`).
4. **New `test_assets` or extend cityview**: (in verify) headless check that atlas builds and rebuild() covers one known tile to a specific palette colour.
5. **CityView**: constructor gains optional `RomAssets*` (tiles + palette); `init()` builds texture atlas (1024 tiles → 16px each); `rebuild()` blits ROM tile for terrain, flat tint for zones. Fallback = old `cell_color`.
6. **game.cpp**: when `use_rom_map`, load scenario 0 → `apply_terrain_map`; pass assets into CityView.
7. **rommap.cpp**: free the fixed `parse_scenario_table` (its output now canonical). **Docs** updated (REQUIREMENTS FR-6 marked complete; ROADMAP entry).
8. **Build/Verify**: `make build`, `make test` (all suites), headless smoke, manual `rommap`/`simcity --help`.

## Phase 2 Solution Proposal (summary)

- **Change**: (a) correct LC_LZ5 decode (raw file-offset reads + universal length+1), (b) correct scenario table parse, (c) CityView consumes ROM tiles+palette with flat-colour fallback, (d) game seeds terrain from ROM.
- **NOT change**: sim core determinism (NFR-2/3), CityView's sim-driven keying, SDL dependency set.
- **Verification**: ctest green; `rommap` prints canonical addresses; smoke prints `Loaded scenario terrain from ROM`; visible terrain tiles match SNES footage palette-wise.

---

## Decision Required (AES-heavy gate)
Scope options:
- **A) Full T017** (decompress + terrain + ROM tiles & palette + rommap + tests + docs) — recommended, matches ticket ACs.
- **B) Plumbing only** (decompress + scenario table + terrain seeding + tests), renderer upgrade deferred to separate ticket.
- **C) Plumbing + tiles, palette deferred.**

Recommendation: **A**. Confirm before Phase 3 begins.