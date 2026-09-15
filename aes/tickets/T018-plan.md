---
ticket: T018
phase: plan
status: done
created: 2026-09-12
tier: heavy
requires:
  - aes/kanban.md
  - aes/tickets/T018-title-screen-menu.md
produces:
  - aes/tickets/T018-plan.md
blocked_by: ''
---

# T018 — Plan

## Reconnaissance Summary

**Existing codebase analysis:**
- `src/engine/game.h/cpp`: Main game loop, directly initializes `CityView` and starts simulation
- `src/engine/timestep.h/cpp`: Fixed timestep at 50fps
- `src/gfx/cityview.h/cpp`: Renders city map from ROM tiles + palette
- `src/gfx/rom_assets.h/cpp`: Extracts Layer1 city tiles (1024 × 4bpp) and BG palettes (14 blocks × 8 sub-palettes)
- `src/snes/rom.h/cpp`: LoROM parser, header validation, CPU↔file offset translation
- `src/snes/decompress.cpp`: Nintendo LC_LZ5 decompressor
- `src/snes/scenariomap.cpp`: Scenario map loading (9 scenarios)
- `src/tools/simcity.cpp`: Entry point, ROM validation, Game construction

**Current flow:** `main() → Game::init() → CityView::init() → Game::run() (simulation loop)`

**Missing:** Title screen, main menu, game state machine, audio system.

**Tests:** All 5 test suites pass (test_rom, test_tile, test_timestep, test_city, test_decompress).

## Two-Agent Analysis (Heavy Ticket)

### CRITIC PHASE

**Como se fosse uma criança:**
"O jogo pula a festa de aniversário e vai direto para o bolo. O jogador nunca vê o nome do jogo, não ouve a música, não escolhe o que quer jogar — só aparece no mapa e pronto."

**Como se fosse um especialista:**
The current architecture assumes a single game state (the simulation map). Adding title screen + menu requires a proper state machine. The hardcoded palette (block 4, sub 0) is a known risk (T017 note) — it was chosen by visual inspection and may be wrong, causing the "amálgama sem lógica de cores". Audio extraction from SNES SPC700 format is non-trivial; the ROM stores music as SPC sequences + ADPCM samples, not raw PCM.

[FAILURE MODE 1]
Mechanism: State machine implemented as giant switch in Game::update/render creates spaghetti code; each state needs its own init/update/render/shutdown lifecycle.

[FAILURE MODE 2]
Mechanism: Title screen graphics location in ROM unknown. SimCity SNES title screen likely uses Mode 7 or separate tilemap. If we can't locate/extract it, we'll need fallback (which violates "extract all assets from ROM" principle).

[FAILURE MODE 3]
Mechanism: SPC700 audio extraction requires understanding the SNES sound engine format (pointers to sequences, sample directories, ADPCM BRR format). SDL_mixer doesn't natively play SPC; need custom player or convert to OGG at runtime (heavy).

[ASSUMPTION]
Palette block 4, sub 0 is correct for city layer.
If false: terrain colors wrong (water=teal, grass=olive) — matches user's "amálgama sem lógica de cores".

[ALTERNATIVE FRAMING]
Instead of full SPC playback, extract music as OGG/MP3 at build time from user's ROM (one-time conversion) and ship only the converter. Or stub audio initially (silent title screen) and implement playback in follow-up ticket.

**Porquê? ×5**
1. Why title screen first? → Because it's the first thing user sees; current UX is broken.
2. Why state machine? → Because multiple screens need different lifecycles.
3. Why SPC extraction hard? → SNES sound engine is a separate CPU (SPC700) with its own program RAM; music data is pointers + sequences + BRR samples.
4. Why palette might be wrong? → 14 blocks × 8 sub-palettes = 112 palettes; only one is correct for city layer.
5. Why not use emulator approach? → Project mandate: native reimplementation, not emulation.

**What are we optimizing that we shouldn't be?**
Optimizing for perfect audio extraction in first pass. Should stub audio, get state machine + title screen + menu working, then add audio.

---

### IMPLEMENTOR PHASE

**Como se fosse uma criança:**
"Vamos fazer uma máquina de estados simples: cada tela sabe como nascer, viver e morrer. A tela de título mostra o logo do ROM. O menu tem botões. Quando clica 'Nova Cidade', vai pro mapa. O som a gente deixa pra depois."

**Como se fosse um especialista:**
Implement a lightweight state machine pattern with `GameState` base class and concrete states (`TitleScreenState`, `MainMenuState`, `MapViewState`). Game holds `unique_ptr<GameState>` and delegates update/render. Title screen graphics: search ROM for known SimCity logo tilemap (likely uncompressed or LC_LZ5). Palette: iterate all 112 sub-palettes, render test frame, pick visually correct one (or add CLI flag to cycle). Audio: stub with SDL_mixer music callback; implement SPC→OGG extraction as separate tool in T010.

[ADDRESSING FAILURE MODE 1]
Use state pattern: `virtual void init(); virtual void handle_event(SDL_Event); virtual void update(double dt); virtual void render(); virtual void shutdown();` — clean separation, no giant switch.

[ADDRESSING FAILURE MODE 2]
Research SimCity SNES title screen ROM location. Known: title screen at $00:8000 area (reset vector). Try to find logo tiles by searching for "SIMCITY" pattern in VRAM dumps. Fallback: render "SimCity" text with SDL_ttf (but violates zero-asset-distribution if we bundle font — use ROM font instead).

[ADDRESSING FAILURE MODE 3]
Stub audio: `AudioSystem` interface with `play_music(id)`, `stop_music()`. Title screen calls `play_music(0)`; implementation logs "TODO: play title music". Real SPC extraction in T010.

[ADDRESSING ASSUMPTION]
Add `palette_block` and `palette_sub` to Game::Config, default to 4/0. Add key binding (F1/F2) to cycle palettes at runtime for visual verification. This solves the "amálgama" issue empirically.

[ADDRESSING ALTERNATIVE FRAMING]
Accept: stub audio initially. Reject: bundling converted audio (violates zero-asset-distribution). Defer: full SPC player to T010.

[SOLUTION PROPOSAL]

**1. Game State Machine (`src/engine/gamestate.h/cpp`)**
- Abstract `GameState` with init/handle_event/update/render/shutdown
- Concrete: `TitleScreenState`, `MainMenuState`, `MapViewState`
- `Game` owns `unique_ptr<GameState> current_state_`; transitions via `change_state()`

**2. Title Screen (`src/engine/titlescreen.h/cpp`)**
- Extract logo tiles from ROM (search for known pattern or hardcode offset if documented)
- Render at center with ROM palette
- Auto-advance to MainMenu after 3s or on key press

**3. Main Menu (`src/engine/mainmenu.h/cpp`)**
- Options: "New City", "Load City", "Scenario", "Practice", "Quit"
- Keyboard (arrows/Enter) + mouse navigation
- "New City" → MapViewState with generated terrain
- "Scenario" → submenu with 9 scenarios

**4. Palette Selection Fix**
- Add `palette_block`, `palette_sub` to `Game::Config`
- Runtime cycling via F1/F2 keys
- Persist selection in config (future)

**5. Audio Stub (`src/audio/audio.h/cpp`)**
- `AudioSystem` singleton with `play_music(int)`, `stop_music()`
- SDL_mixer initialization (linked but silent)
- T010 will implement real SPC extraction

**6. Game Refactor**
- Move `CityView`, `RomAssets`, `City` to `MapViewState`
- `Game::init()` creates `TitleScreenState` as initial state
- `Game::run()` delegates to current state

## Affected Files

| File | Operation | Description |
|------|-----------|-------------|
| src/engine/gamestate.h | create | Abstract GameState base class |
| src/engine/gamestate.cpp | create | Base implementation (empty) |
| src/engine/titlescreen.h | create | Title screen state |
| src/engine/titlescreen.cpp | create | Title screen implementation |
| src/engine/mainmenu.h | create | Main menu state |
| src/engine/mainmenu.cpp | create | Main menu implementation |
| src/engine/mapview.h | create | Map view state (extract from Game) |
| src/engine/mapview.cpp | create | Map view implementation |
| src/engine/game.h | modify | Add state machine, AudioSystem, Config changes |
| src/engine/game.cpp | modify | Refactor to use state machine |
| src/audio/audio.h | create | AudioSystem interface |
| src/audio/audio.cpp | create | AudioSystem stub implementation |
| src/gfx/rom_assets.h | modify | Add palette cycling helper |
| CMakeLists.txt | modify | Add audio library, new source files |
| src/tools/simcity.cpp | modify | Pass config to Game |

## Specification

### GameState Interface
```cpp
class GameState {
public:
    virtual ~GameState() = default;
    virtual bool init(Game& game) = 0;
    virtual void handle_event(Game& game, const SDL_Event& ev) = 0;
    virtual void update(Game& game, double dt) = 0;
    virtual void render(Game& game) = 0;
    virtual void shutdown(Game& game) = 0;
};
```

### Game::Config additions
```cpp
struct Config {
    // ... existing ...
    int palette_block = 4;
    int palette_sub = 0;
    bool enable_audio = true;
};
```

### State Transitions
```
TitleScreenState --(timeout/key)--> MainMenuState
MainMenuState --("New City")------> MapViewState(generated)
MainMenuState --("Scenario N")----> MapViewState(scenario N)
MainMenuState --("Quit")----------> Game::running_ = false
MapViewState --(ESC)--------------> MainMenuState
```

## Testing Strategy

**Unit tests:**
- GameState transitions (mock states)
- MainMenu navigation logic
- Palette cycling logic
- AudioSystem stub calls

**Integration tests:**
- Full game launch → title → menu → map → ESC → menu → quit
- Scenario selection loads correct terrain
- Palette cycling changes visual output

**Headless smoke test:**
- `SDL_VIDEODRIVER=dummy timeout 3 ./build/simcity rom.sfc` exits cleanly

**Visual verification:**
- Tileview with palette cycling to find correct city palette
- Title screen displays without crashes

## Verification Criteria
- [ ] Game launches to title screen (not map)
- [ ] Title screen shows SimCity logo from ROM (or fallback text)
- [ ] Title screen auto-advances or key press → main menu
- [ ] Main menu shows 5 options, navigable by keyboard/mouse
- [ ] "New City" starts map with generated terrain
- [ ] "Scenario N" starts map with scenario N terrain
- [ ] ESC from map returns to main menu (not quit)
- [ ] Palette cycling (F1/F2) changes terrain colors visibly
- [ ] AudioSystem initializes without crash (stub)
- [ ] All existing tests pass
- [ ] No new lint errors
- [ ] Headless smoke test passes

## Estimation
- Complexity: high (> 8h) — new architecture, multiple new files, ROM research
- Risk: high — title screen ROM location unknown, audio extraction complex
- Blocking dependencies: SDL_mixer (new), SDL_ttf (optional for fallback text)

**Recommendation:** Split into sub-tickets:
- T018a: Game state machine + MapViewState extraction
- T018b: Title screen + Main menu UI
- T018c: Palette selection fix + runtime cycling
- T018d: Audio stub + SDL_mixer integration
