---
ticket: T018
title: Title Screen, Main Menu & Game State Machine
sprint: sprint-02
priority: high
status: pending
created: 2026-09-12
---

# T018 — Title Screen, Main Menu & Game State Machine

## Context
The game currently goes straight from ROM loading to the simulation map view. The original SimCity SNES flow is:
1. Title screen with "SimCity" logo and music
2. Main menu: New City, Load City, Scenario, Practice
3. Map view with simulation

User reports: "Não parece estar a carregar qualquer asset do rom, mesmo quando é indicado o rom, apenas carrega uma amálgama sem lógica de cores e um RSI aleatório carregado no canto superior esquerdo que nada tem que ver com o jogo original."

## Acceptance Criteria
- [ ] Title screen displays with SimCity logo extracted from ROM
- [ ] Title screen music plays (SPC/ADPCM extracted from ROM)
- [ ] Main menu with options: New City, Load City, Scenario, Practice
- [ ] Game state machine: TitleScreen → MainMenu → MapView
- [ ] ESC from map returns to main menu (not quit)
- [ ] Palette/tile mapping verified correct (no "amálgama sem lógica de cores")
- [ ] Headless smoke test still passes (SDL_VIDEODRIVER=dummy timeout 3)

## Scope
**In scope:**
- Game state machine implementation
- Title screen rendering (ROM graphics extraction)
- Main menu UI rendering
- Audio system (SPC extraction + SDL_mixer playback)
- Palette selection fix for city tiles
- Integration with existing Game class

**Out of scope:**
- Save/load city state (T012)
- Full scenario selection (T011)
- Network/multiplayer (T016)

## Dependencies
- T017 (ROM asset extraction) - completed
- T003 (Game loop) - completed
- SDL_mixer for audio (new dependency)

## Rollback
Revert Game::init() to direct map initialization, remove state machine.

## Known Risks
- SPC/ADPCM extraction is complex; may need to stub audio initially
- Palette block selection for city layer may need trial/error
- Title screen graphics location in ROM unknown - need to research

## Notes
- Original SimCity SNES uses Mode 7 for title screen; we'll use standard 2D rendering
- Music: SimCity SNES uses SPC700 format; need to extract + play via SDL_mixer
- Menu navigation: keyboard (arrows/enter) and mouse
