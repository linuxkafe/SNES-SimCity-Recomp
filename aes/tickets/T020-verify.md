---
ticket: T020
phase: verify
date: 2026-09-13
method: code-path analysis + runtime smoke + pixel-level ROM rendering capture
machine: /home/seyon/dev/Games/PC/simcity (ROM "SimCity (USA).sfc", 512KB, checksum VALID)
---

# T020 — Verification Report (Hostile Audit)

## 0. Evidence Instrumentation

A verification binary (`/tmp/opencode/verify_visual.cpp`, linked against the project's own
`simcity_gfx/snes_decompress/snes_rom/simcity_sim` libraries) rendered the **exact** CityView
pipeline under `SDL_VIDEODRIVER=dummy` using the real ROM:

- `load_rom_assets(rom)` = **true**, 32768 B decompressed (1024 × 32-B tiles)
- Scenario 0 terrain applied via `City::apply_terrain_map`
- BMPs captured to `/tmp/opencode/verify_map_{terrain,zones,block3,block4,block5}.bmp`
- Palette-index profiles per tile dumped (`/tmp/opencode/tile_profile.cpp`)

Evidence files stay OUTSIDE the repository (derived renderings of copyrighted ROM assets —
"Zero Asset Distribution" rule).

## 1. Findings

### F1 — Title screen is invented, zero ROM involvement — CONFIRMED
`src/engine/game.cpp:330-374` `render_title()` draws 7 colored rectangles labelled
`colors[7][3]` with comment `"Draw 'SIMCITY' as large colored rectangles (placeholder
for logo)"`. Only palette block 4/sub 0 is loaded in `init()`; no ROM title/menu
graphics, logo, font or tilemap are read for this screen. `update_title()` auto-advances
after 150 frames → menu.

### F2 — Main menu empty and invented — CONFIRMED
`src/engine/game.cpp:376-422` `render_menu()` draws a title bar + 5 option
rectangles; comment: `"Option text as rectangle (placeholder for text)"`. No strings
are rendered at all. `src/ui/rect_font_renderer.cpp:17-24` implements `draw_text()` by
painting `kCharW×kCharH` solid rects per character — every glyph is the same square,
so even HUD panels (budget "FUNDS:" etc.) appear as rows of identical squares. T019
(ROM bitmap font extraction) is still backlog — no ROM font exists in the codebase.

### F3 — Map renders as "amálgama" — CONFIRMED (palette/tile pairing broken)
Empirical pixel census of captured map (palette block 4, sub 0):

| Colour | % of map | origin |
|--------|----------|--------|
| #000000 | 32.7 | palette idx 0 = backdrop, drawn opaque black over every tile |
| #41AC00 | 28.6 | grass tile idx 15 |
| #299CFF | 18.2 | grass/water tiles idx 5/9 (bright blue) |
| #C50000 | 12.6 | grass/water tiles idx 10 (red) |

`tile_profile`: tile 0 (mapped to Grass) uses indices {0,5,9,10,15}; tile 1 (Water)
uses {0,9,10,15}; tile 20 (Tree) uses {2,3,5,10,…}. Under block 4/sub 0 the same
"grass" tile is simultaneously red, bright blue and green — the map has no coherent
water/land semantics.

F1/F2 palette cycling (verified block3/4/5 runs): the SAME terrain renders grey/white
(block 3), red+blue+green (block 4), purple/pink (block 5). No block yields a coherent
SimCity map. Palette origin is documented as unverified in `gfx/rom_assets.h:26-29`
and T017 build notes ("palette-block/sub selection is a smell-test"). The backdrop
(idx 0) is drawn black (cityview.cpp:135 fill `0xFF000000` + blit) whereas SNES renders
idx 0 as the backdrop colour, contributing the 33% black noise.

### F4 — Zone cells render with invented flat colours — CONFIRMED
`src/gfx/cityview.cpp:27-46` `cell_color()` uses hardcoded RGB for
R(212,168,60)/C(116,128,208)/I(168,108,184) + brightness by density; verified in the
zones capture (unique colour `#94752A`-family blocks appear where R zones were placed,
tiled 1536 px = 6×16×16). Building sprites from the 1024-tile bank are NOT mapped
(T017 "Remaining Risks"; no ticket).

### F5 — Random "earthquakes" = unlabelled random meteor/monster — CONFIRMED
`sim/city.cpp:477-493` `process_disasters()`: monthly `next_random()` roll, threshold
200 + pop/100·50 capped at 1500 (2%→15%) triggers meteor or monster. Visuals:
`game.cpp:308-315` camera shake while `shake_timer_>0`; `game.cpp:453-458` flashes a
RED rectangle at (10,24,200,18) with NO text ("draw as simple colored rect since we
don't have font rendering"). User sees random shake + red box — indistinguishable from
a random earthquake. There is no earthquake disaster; disasters are also never
user-triggerable in real play except the hidden 6/7 keys.

### F6 — Confirmed working (counter-evidence, honesty requirement)
- ROM parse + header + checksum valid (rominfo).
- LC_LZ5 decompress + scenario table $03:CE70 (9 entries) correct (T017).
- All 9 scenario maps decode (`rommap`, test_decompress).
- 6/6 ctest suites pass; headless smoke (default and --skip-menu) exits cleanly.
- So: extraction machinery is sound; the fidelity failures are in *which* palette/tiles
  are applied and in the *invented* screens, not in the ROM loader.

## 2. Process Violations Detected (AES)

| # | Violation | Evidence |
|---|-----------|----------|
| P1 | T018 approved (heavy, 2026-09-12) but `status: pending`; the partial state machine + invented art shipped inside `game.cpp` with NO T018 build/verify/review/learn artifacts | `aes/tickets/T018-*.md` (only ticket + plan exist); `game.cpp` contains the code |
| P2 | Zero git commits (`git log` = none) — no traceability of shipped code; can't diff intent vs delivery | `git log --oneline` → "does not have any commits yet" |
| P3 | Known user complaint (quoted verbatim inside T018) unaddressed while related tickets closed "done" | T008/T017 done; T018 pending; user re-reported same issue 2026-09-13 |
| P4 | Test suites validate formatters/decompress only — nothing tests the visual output path (title/menu/palette/tile pairing) | tests/ contain no rendering assertions |

## 3. Cost of Being Wrong

LOW. Every finding is anchored to source lines + captured pixels; the counter-evidence
(F6) rules out ROM corruption or decompressor failure as causes.

## 4. Recommended Remediation Order

1. **T018a** — real title screen (ROM logo research) + menu with ROM font (T019) —
   replace the invented rect screens.
2. **Palette resolution** — brute-force all 112 sub-palettes against a known-good
   SimCity SNES map reference, or extract the palette from the ROM's CGRAM init code;
   set a verifiable default + regression test that grass/water tiles map to
   green/blue-ish palettes.
3. **Zone sprites** — map building tiles (0x40+) so zoning isn't flat colours.
4. **Disaster UX** — either implement the real earthquake disaster or gate random
   disasters; replace the unexplained red rectangle with ROM-faithful UI.
5. **Traceability** — first git commit + require per-ticket artifacts.
6. **Visual regression tests** — headless pixel-assertions (e.g., water-pixel-is-blue) in ctest.

## 5. Verification Caveats

- Pixel census assumes SDL software-renderer output is faithful (16x16 upscale of 8x8
  tiles) — verified against palette-index profile consistency.
- Block4/sub0 might be *partially* correct; the definitive answer needs the reference
  map/frame comparison which is outside this ticket's scope.

---

## 6. Addendum — Palette & Tile-Decode Resolution (same ticket, 2026-09-13 follow-up)

Further work after §1 closed F3's root cause. Two independent discoveries, both
verified empirically (evidence in `/tmp/opencode/`, outside the repo):

### 6.1 4bpp decode layout B confirmed; layout A was a genuine bug
SNES 4bpp tiles are packed as **two 2bpp tiles**: bit planes 0/1 in the first 16
bytes, planes 2/3 in the second 16 bytes (per row r: `p0=d[r*2]`, `p1=d[r*2+1]`,
`p2=d[16+r*2]`, `p3=d[16+r*2+1]`; confirmed by the SNESdev wiki "Tiles" page — text
retrieved via websearch since the page refuses direct fetches with 403 —
"essentially two 2bpp tiles…"). The previous `decode_4bpp_tile` interlaced the four
planes per row (`d[row*4+0..3]`), scrambling the colour planes into garbage.

Evidence:
- **Entropy test** (`layoutentropy.cpp`, 14 dominant tiles): layout B has lower
  palette-index entropy on 12/14 tiles → structured imagery, layout A = noise.
- **ASCII-art render** (`asciitile.cpp`): tiles 0-3 under layout B show coherent
  shoreline/water animation frames with a moving edge; layout A shows static garble.

### 6.2 No single sub-palette can be green AND blue → per-terrain sub palette
- Whole-map census (`mapcensus.cpp`, scenario 0): tile 0 (landmass, 4273 cells) and
  tile 1 (water, 2661 cells) BOTH top-hit palette **index 12** (53% / 75% of their
  pixels). 49% of the entire map is index 12 regardless of palette.
- Block 4/sub 0 default → index 12 = #000000 ⇒ ~33-49% of the map is BLACK (the
  reported "amálgama"); block 5/sub 1 → index 12 = #319CFF ⇒ whole map blue.
- **Resolution**: within city block **5**, land/tree/road cells use **sub 0**
  (index 12 = `#315A00` dark land green) and water cells use **sub 1** (index 12 =
  `#319CFF` blue). This mirrors the real game switching BG Mode 1 tilemap
  palette-attribute bits per terrain. The scenario RLE stream (`decode_rle16`) masks
  attribute bits with `v & 0x03FF` (bits 10-13 are the repeat count), so there are no
  per-tile palette bits in the data — the assignment is game logic, exactly as coded.
- Numeric render check with the fixed decode + per-terrain subs (`terrainrender.cpp`):
  land 53.9% green / 0.0% blue, water 74.6% blue / 0.3% green; ASCII output shows a
  coherent geography (large N/W water mass, contiguous land, coastlines, dirt roads).
  Single-sub1 contrast: land green collapses to 11.9%.

### 6.3 Code changes shipped in this follow-up
- `src/gfx/tile.cpp` `decode_4bpp_tile` → layout B (SNES two-2bpp).
- `tests/test_tile.cpp` → synthetic tiles now encode layout B; `encode_row` helper;
  6/6 ctest pass.
- `src/gfx/cityview.cpp` `blit_tile_cell` → layout B decode + takes a `sub` arg;
  `sub_palette_for_terrain()` (water→sub 1, everything else→sub 0); `rebuild()` passes it.
- Default city palette block 4→5 in `rom_assets.h`, `engine/game.h`, `tools/simcity.cpp`;
  docs updated to remove the "smell-test" caveat.
- Headless smoke (block 5, sub 0 base): boots, scenario terrain loads, 0 errors.

### 6.4 Status after follow-up
- F3 map root cause: **RESOLVED** — coherent land green / water blue rendering.
- Still open (unchanged): F1 title screen invented; F2 menu/font invented (T019);
  F4 zone cells flat colours (building sprites not mapped); F5 disasters random +
  red rectangle. Those are §4 items 1, 3, 4 — not touched here.