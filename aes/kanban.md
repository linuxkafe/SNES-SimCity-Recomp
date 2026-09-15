---
project: SimCity SNES PC Port
created: 2026-09-11
current_sprint: sprint-02
current_ticket: T024
---

# SimCity SNES PC Port — Kanban

## Status
- **Project**: Static recompilation of SimCity SNES via snesrecomp
- **Approach**: 65C816 → native C (ahead-of-time), SNES hardware runtime, native widescreen renderer
- **Language**: C (generated) + Rust (analyzer) + Python + CMake
- **Legal**: Users must provide their own ROM; PolyForm Noncommercial 1.0.0 (non-commercial only)

## Backlog
| ID | Title | Priority | Status |
|----|-------|----------|--------|
| T010 | Audio extraction & playback | low | backlog |
| T011 | Full scenario system (all 5 US scenarios) | high | backlog |
| T012 | Save/load city state (binary) | high | backlog |
| T013 | Keyboard shortcuts & hotkeys | medium | backlog |
| T019 | ROM bitmap font extraction | high | backlog |
| T020 | Responsive panel layout (window resize) | medium | backlog |
| T028 | Native widescreen renderer for isometric tilemap | high | backlog |
| T029 | Modding API — Lua bindings for sim::City | high | backlog |
| T030 | Performance profiling + optimization | medium | backlog |

## In Progress
| ID | Title | Status |
|----|-------|--------|
| T031 | Migration to Static Recompilation (snesrecomp) | **Phase 1: Framework + Bootstrap** |
| T022 | Menu system — ROM font + text | **paused (superseded by T031)** |
| T023 | Building sprites — map 1024-tile bank to zones | **paused** |
| T024 | Disaster UX — earthquake or gate random disasters | **paused** |
| T025 | Visual regression tests — headless pixel assertions | **paused** |

## Done
| ID | Title | Status |
|----|-------|--------|
| T001 | Project scaffolding + ROM parser | done |
| T002 | Graphics extraction + tile renderer | done |
| T003 | Basic game loop + window | done |
| T004 | ROM map loader (scenario terrain) | done |
| T005 | City simulation core | done |
| T006 | City map rendering + edit tools | done |
| T007 | Power grid simulation | done |
| T008 | Disaster system (meteor, monster) | done |
| T009 | UI panels (budget, population, RCI graphs) | done |
| T017 | ROM asset extraction & game integration | done |
| T020 | Visual fidelity audit (user report: invented UI + garbage map + random earthquakes) | done |
| T021 | Title screen — extract ROM logo | done |

## Learning History
| Date | Ticket | Key Insight |
|------|--------|-------------|
| 2026-09-11 | T001–T006 | AES scaffolding established; deterministic sim core works; headless smoke passes |
| 2026-09-11 | T007 | 3-step flood fill (radius → BFS through lines → power neighbors of lines) is the cleanest power grid model; coal radius 1 from 3×3 center = exactly footprint (no external coverage); zone-growth tests must provide an explicit power-line-to-zone adjacency chain; top-left dedup prevents multi-counting multi-tile plants |
| 2026-09-11 | T017 | Two-bug root cause confirmed: LC_LZ5 length−1 applies to ALL modes (not just 0xE0) and `read8/read16` were re-translating file offsets as CPU addresses (double translation). With raw byte reads, 9/9 scenario maps decode and packet ends chain against the disassembly. CORRECTION to T004 note: the $03:CE70 scenario table is the real map table; the C++ read path was simply broken (empirical 0x7C184/0x7C4B4 were false positives) |
| 2026-09-12 | T008 | Deterministic LCG seeded by month_ gives free reproducibility for random-triggered features; disasters integrate cleanly at start of step_month(); manual trigger API enables instant testing; camera shake via render offset is trivial and effective; monster straight-line path acceptable for MVP |
| 2026-09-13 | T009 | FontRenderer strategy pattern (SD-META-001) enables zero-dep MVP with clean ROM font upgrade path; headless formatting tests (SD-META-002) standardize for all simulation-to-UI data flows; horizontal RCI bars (SD-DOMAIN-001) fit 60px panel better than vertical; panel layout hardcoded — responsive needed |
| 2026-09-13 | T020 | User report verified TRUE: title/menu are invented rectangles (game.cpp placeholders); terrain tiles are ROM-correct but palette block 4/sub0 (+ tile pairing) renders red/blue/black blobs (empirical pixel census); zone cells use invented flat colours; random meteor/monster + red-box flash reads as "random earthquakes". Extraction pipeline itself is sound — failures are palette selection, invented screens, and no ROM font. Process: T018 approved-but-pending yet shipped partially inside game.cpp with no build/verify artifacts; zero git commits = no traceability |
| 2026-09-13 | T020 | **Palette/tile resolution (addendum):** (1) SNES 4bpp = two 2bpp tiles (planes 0/1 bytes 0-15, planes 2/3 bytes 16-31) — the previously shipped `decode_4bpp_tile` layout-A interlacing was a real bug (SNESdev wiki text + entropy 12/14 + ASCII structure). (2) No single sub-palette can be green AND blue: tiles 0 and 1 both top-hit index 12 (49% of map). City layer block 5 solves it: land/tree/road → sub0 (idx12 #315A00 green), water → sub1 (idx12 #319CFF blue) — mirrors the game's per-terrain BG Mode 1 palette attribute bits, which the RLE stream cannot carry (bits 10-13 = repeat count). Verified numeric render: land 53.9% green/0% blue, water 74.6% blue/0.3% green. Implemented in tile.cpp/cityview.cpp; 6/6 tests + smoke pass. F3 map resolved; F1/F2/F4/F5 remain open |
| 2026-09-14 | T021 | SNES title screen uses BG Mode 1 with three layers: L3 (2bpp, 32 tiles, stars), L1 (4bpp, 384 tiles, skyline), L2 (4bpp, 512 tiles, logo). All graphics LC_LZ5 compressed at known CPU addresses; palette is raw BGR555 (128 colors = 8 sub-palettes × 16). Extraction pattern "disassembly address → LoROM translate → LC_LZ5 decompress → format decode → compose → cache" now validated 3× (scenario maps T004, city tiles T017, title screen T021). One-time CPU composite of 3 layers with index-0 transparency, then GPU stretch — zero runtime cost. |
| 2026-09-14 | T026 | Architectural decision: clean-room reimplementation chosen over snesrecomp. Key differentiator: SimCity has no coprocessor (Super FX) — recompilation's killer feature irrelevant. License veto (PolyForm Noncommercial) independent and decisive. |
| 2026-09-14 | T027 | Re-evaluation with new requirements (optimization, modding, widescreen). StarFox widescreen renderer (3D polygon FOV) doesn't transfer to SimCity (2D isometric). snesrecomp modding = config.ini toggles, not API. License unchanged. Decision stands: reimplementation + feature tickets (T028-T030). |
| 2026-09-14 | T031 | **PIVOT**: User directive — recompilation IS project purpose. Previous decisions invalidated by new premise. 5-phase migration plan with golden master validation. Current reimplementation becomes baseline for behavioral equivalence. License (PolyForm Noncommercial) explicitly accepted. Toolchain: Docker CI, Rust/Python/CMake. Rollback tag: `reimplementation-final`. |
