---
ticket: T020
title: Visual Fidelity Audit — invented UI vs ROM-extracted rendering
sprint: sprint-02
priority: high
status: done
created: 2026-09-13
source: user report "abre com um ecrã de título que não é do rom... lixo visual com terramotos aleatórios"
---

# T020 — Visual Fidelity Audit

## Context
User report (2026-09-13): the app opens with a title screen not from the ROM, an
empty menu with nothing from the ROM, and the map shows visual garbage with random
"earthquakes" unlike anything in the original ROM. T018 (title screen, menu) was
planned 2026-09-12 but never implemented; however a partial state machine with
INVENTED graphics shipped inside `src/engine/game.cpp` without its own ticket.

## Empirical Findings (evidence in T020-verify.md)

| # | Claim | Verdict | Evidence |
|---|-------|---------|----------|
| F1 | Title screen is invented, not from ROM | **CONFIRMED** | `game.cpp:330-374` renders hardcoded colored rectangles labelled "placeholder for logo"; zero ROM data read |
| F2 | Main menu is empty / invented | **CONFIRMED** | `game.cpp:376-422` draws option boxes as "placeholder for text"; `rect_font_renderer.cpp:17-24` paints every char as the identical rectangle (no glyphs, no ROM font — T019 still backlog) |
| F3 | Map terrain is visual garbage ("amálgama") | **CONFIRMED** | Terrain tiles ARE ROM-extracted, but palette block 4/sub 0 is an unverified guess (`rom_assets.h:26-29`, documented risk in T017) rendering grass/water tiles as red+#299CFF blue+#41AC00 green blobs; F1/F2 block cycling produces radically different colour worlds (block3=grey, block4=red/blue/green, block5=purple) |
| F4 | Zone cells use invented flat colours | **CONFIRMED** | `cityview.cpp:39-45` `cell_color()`; no building sprites mapped (1024 tile bank used for ~6 ids only) |
| F5 | Random "earthquakes" | **CONFIRMED as meteor/monster** | `city.cpp:477-493` monthly RNG disaster roll (2%→15%); `game.cpp:308-315` camera shake; `game.cpp:453-458` red rectangle top-left flash; no earthquake disaster exists, shake+red-box is unexplained to the user |

## Confirmed Working (honest counter-evidence)
- HTTP ROM load/header parse, checksum valid; scenario table $03:CE70 (9 entries) correct.
- LC_LZ5 decompressor decodes scenario maps + 1024 city tiles (32768 B) correctly.
- Scenario terrain (all 9) decodes and chains (rommap output).
- 6/6 ctest suites pass; headless smoke exits cleanly.

## Root Causes
1. **T018 phantom work**: plan approved (tier heavy) but ticket stayed `pending`;
   a partial, unaudited state machine with hardcoded art was merged into `game.cpp`
   with no build/verify/review/learn artifacts.
2. **Palette unresolved since T017/T008**: "human render smell-test" never performed;
   shipped default renders incoherent colours.
3. **No font/logo extraction**: T019 (ROM bitmap font) still backlog → all UI text is
   placeholder rectangles.
4. **Zero git commits**: no traceability; cannot diff "what shipped vs what was planned".

## Remediation (next steps, not yet executed)
- T018a implements real title (ROM logo) + menu (ROM font/text) via state machine.
- Palette resolution: brute-force render + compare against known SimCity SNES map
  palette; alternatively extract palette from game's CGRAM init code.
- Zone building sprites: map tiles 0x40+ to buildings.
- Disaster: implement earthquake (or gate random disasters behind user trigger),
  remove unexplained red-rect/flash or make it ROM-faithful.

## Acceptance Criteria (for remediation ticket)
- [ ] Title screen renders ROM-extracted logo (no invented rectangles)
- [ ] Main menu shows ROM text/graphics for the 4 real options
- [ ] Default palette renders coherent SimCity terrain (grass green/water blue)
- [ ] No unexplained sprite flash / shake for non-earthquake events
- [ ] 6/6 ctest + headless smoke still pass