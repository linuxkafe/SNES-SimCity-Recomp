# Roadmap — SimCity SNES PC Port

## Current Sprint
**Sprint 01** — Foundation + Core Simulation + Playable Map
- T001: Project scaffolding + ROM parser ✅
- T002: Graphics extraction + tile renderer ✅
- T003: Basic game loop + window ✅
- T005: City simulation core ✅
- T006: City map rendering + edit tools ✅
- T007: Power grid simulation ✅
- T004: ROM map loader (scenario terrain) ✅
- T017: ROM asset extraction & game integration (tiles + palette render, rommap fix) ✅

## Backlog (Prioritized)

### Sprint 02 — Polish & Systems
| ID | Title | Impact | Effort | Priority | Status |
|----|-------|--------|--------|----------|--------|
| T007 | Power grid simulation | High | Medium | High | backlog |
| T008 | Disaster system (meteor, monster) | Medium | Medium | Medium | backlog |
| T009 | UI panels (budget, population, RCI graphs) | High | Medium | High | backlog |
| T010 | Audio extraction & playback | Low | Medium | Low | backlog |

### Sprint 03 — Scenario & Save
| ID | Title | Impact | Effort | Priority | Status |
|----|-------|--------|--------|----------|--------|
| T011 | Full scenario system (all 5 US scenarios) | High | High | High | backlog |
| T012 | Save/load city state (binary) | High | Medium | High | backlog |
| T013 | Keyboard shortcuts & hotkeys | Medium | Low | Medium | backlog |

### Sprint 04 — Advanced Features
| ID | Title | Impact | Effort | Priority | Status |
|----|-------|--------|--------|----------|--------|
| T014 | Time controls (pause, 1×, 2×, 3×) | High | Low | High | backlog |
| T015 | City advisers (pollution, traffic, crime) | Medium | High | Medium | backlog |
| T016 | Network/multiplayer (local hotseat) | Low | Very High | Low | backlog |

## Non-Goals (Explicitly Out of Scope)
- Emulation accuracy layer (we are a reimplementation, not an emulator).
- Support for ROM hacks / fan translations.
- 3D rendering or modern graphics overhaul.
- Mobile ports.

## Discovered During Work
| ID | Description | Origin | Added |
|----|-------------|--------|-------|
| | Duplicate formatters in serialize/ | T003-build | 2026-09-11 |