---
ticket: T009
phase: plan
status: done
created: 2026-09-13
tier: standard
requires:
  - aes/kanban.md
  - aes/tickets/T009-ui-panels.md
produces:
  - aes/tickets/T009-plan.md
blocked_by: ''
---

# T009 — UI Panels Plan

## Reconnaissance Summary

**Existing codebase analysis:**
- `src/sim/city.h` — `Stats` struct provides all needed data: `funds`, `income`, `upkeep`, `tax_rate`, `population`, `resident_tiles`, `commercial_tiles`, `industrial_tiles`, `road_tiles`, `jobs`, `res_demand`, `com_demand`, `ind_demand`
- `src/gfx/cityview.h/cpp` — Rendering pattern: full-map texture rebuilt on demand, per-frame `RenderCopy`. Uses `RomAssets` for tiles+palettes
- `src/engine/game.h/cpp` — Main game loop with `GameState::Playing`, `render_playing()` calls `view_->render()` then `draw_hud()`. Current HUD is minimal: top status bar (22px), tool indicator (console), disaster alert rect, RCI demand bars (3 small vertical bars)
- `src/gfx/rom_assets.h/cpp` — `RomAssets` holds extracted tiles (16x16 scaled from 8x8) and BG palettes (14 blocks × 8 sub-palettes). `load_rom_assets()` extracts from ROM
- `CMakeLists.txt` — Only SDL2 dependency. No SDL2_ttf yet.
- Tests in `tests/test_city.cpp` follow CHECK macro pattern, test deterministic simulation

**Key patterns to follow:**
- Panels should be separate classes in `src/ui/` (new directory)
- Rendering via SDL_Renderer with software renderer (existing)
- Data from `sim::City::stats()` — read-only, deterministic
- Font rendering: current code uses colored rects as text placeholders (no TTF). Need to decide: add SDL2_ttf + extract font from ROM, or use bitmap font from ROM tiles, or keep rect placeholders for MVP

## Two-Agent Analysis (standard tier — not heavy)

*Skipped — not a heavy ticket (architectural decision limited to font rendering approach)*

## Hostile Analysis

### Assumptions I Am Making

- **[KNOWN]** `sim::City::Stats` provides all data needed for budget, population, and RCI panels — justification: struct has `funds`, `income`, `upkeep`, `tax_rate`, `population`, `resident_tiles`, `commercial_tiles`, `industrial_tiles`, `road_tiles`, `jobs`, `res_demand`, `com_demand`, `ind_demand`
- **[KNOWN]** Current rendering uses software renderer (`SDL_RENDERER_SOFTWARE`) — justification: `game.cpp:44` creates software renderer
- **[INFERRED]** Font rendering is the main architectural decision — evidence: current HUD uses colored rects as text placeholders (`draw_hud()` draws rects for RCI bars, disaster alert); no TTF dependency in CMakeLists.txt; ROM has font tiles at known offsets
- **[ASSUMED]** Three separate panels (budget, population, RCI) — impact if false: could be single combined panel, but SNES original has distinct UI regions
- **[ASSUMED]** Panels overlay the map view (not separate screens) — impact if false: would need state machine changes in `Game`
- **[UNKNOWN]** Exact SNES panel layout/positioning — why outside knowledge: would need ROM disassembly or visual reference; will approximate from SNES screenshots
- **[UNKNOWN]** Whether ROM font tiles are 1bpp or 4bpp, and exact offset — why outside knowledge: need to run tileview to inspect

### What Was Not Specified (That Matters)

- Font rendering approach: SDL2_ttf + system font vs ROM bitmap font vs rect placeholders
- Panel positioning: top bar only, side panels, bottom panel, or floating windows
- Interaction: are panels clickable (buttons, sliders) or read-only display?
- Animation: do RCI bars animate smoothly or jump?
- Localization: English only or support for other languages?

### Alternatives Not Chosen

| Option | Description | Rejected Because |
|--------|-------------|------------------|
| A: SDL2_ttf + system font | Add `find_package(SDL2_ttf)`, load TTF font file | Adds external dependency; font won't match SNES look; requires distributing font or asking user |
| B: ROM bitmap font extraction | Extract font tiles from ROM like terrain tiles (1bpp or 4bpp) | Matches SNES exactly; no new deps; but requires reverse-engineering font format and layout |
| C: Rect placeholders (MVP) | Keep current approach: colored rectangles as "text" | Zero deps; works immediately; but not shippable — users can't read numbers |
| D: Dear ImGui | Integrate ImGui for immediate-mode UI | Overkill for 3 panels; adds ~20k LOC; doesn't match SNES aesthetic |

**Chosen: Hybrid B→C** — Implement panel *logic and layout* now with rect placeholders (C), but design panel API to accept a `FontRenderer` interface. Later ticket (T019) implements ROM bitmap font extraction (B) and swaps implementation.

### Risks and Side Effects

- Adding `src/ui/` directory requires CMakeLists.txt changes
- If SDL2_ttf added later, all panel rendering must be refactored
- Panel layout must not break existing `draw_hud()` RCI bars (they're minimal HUD, not full panels)
- Camera shake (`shake_x_`, `shake_y_`) should not affect UI panels (UI is screen-space, not world-space)

### Cost of Being Wrong

**Medium** — Font decision affects panel readability. If we choose C (rect placeholders) and later need B (ROM font), panel rendering code must be rewritten. Mitigation: design `FontRenderer` abstraction from start.

### Scope Boundary

**In scope:**
- `src/ui/panel.h`, `src/ui/budget_panel.cpp/h`, `src/ui/population_panel.cpp/h`, `src/ui/rci_panel.cpp/h`
- `src/ui/font_renderer.h` (abstract interface)
- `src/ui/rect_font_renderer.cpp` (rect placeholder implementation)
- CMakeLists.txt: add `simcity_ui` library, link into `simcity_engine`
- `Game::draw_hud()` refactored to use panel system
- Unit tests for panel data formatting (headless, no SDL)

**Out of scope:**
- ROM font extraction (T019)
- SDL2_ttf integration
- Interactive UI (buttons, sliders) — panels are read-only display for MVP
- Scenario selection UI (T011)
- Save/load UI (T012)

## Technical Approach

### Architecture

```
src/ui/
├── panel.h                 # Base Panel class + FontRenderer interface
├── budget_panel.h/cpp      # BudgetPanel : Panel
├── population_panel.h/cpp  # PopulationPanel : Panel
├── rci_panel.h/cpp         # RciPanel : Panel
├── rect_font_renderer.h/cpp # RectFontRenderer : FontRenderer (placeholder)
└── (future) rom_font_renderer.h/cpp # RomFontRenderer : FontRenderer
```

**FontRenderer interface:**
```cpp
class FontRenderer {
public:
    virtual ~FontRenderer() = default;
    virtual void draw_text(SDL_Renderer* r, int x, int y, const char* text, uint32_t color) = 0;
    virtual void draw_text_right(SDL_Renderer* r, int x, int y, const char* text, uint32_t color) = 0;
    virtual int text_width(const char* text) = 0;
    virtual int text_height() = 0;
};
```

**Panel base class:**
```cpp
class Panel {
public:
    virtual ~Panel() = default;
    virtual void render(SDL_Renderer* r, const sim::Stats& stats, FontRenderer* font, int win_w, int win_h) = 0;
    virtual SDL_Rect bounds() const = 0;  // screen-space rect for layout
};
```

### Panel Layout (SNES-approximate)

| Panel | Position | Size | Content |
|-------|----------|------|---------|
| Budget | Top-left, below status bar | ~200×120 | "FUNDS: $XXX,XXX" / "INCOME: +$X,XXX" / "EXPENSES: -$X,XXX" / "TAX: X%" |
| Population | Top-right | ~200×100 | "POP: X,XXX" / "R: XXX  C: XXX  I: XXX" / "JOBS: X,XXX" |
| RCI | Bottom-left | ~300×60 | Three horizontal bars: R (yellow), C (blue), I (purple) with labels |

All panels render in screen-space (camera-independent).

### Affected Files

| File | Operation | Description |
|------|-----------|-------------|
| `src/ui/panel.h` | create | Base classes: `Panel`, `FontRenderer` |
| `src/ui/rect_font_renderer.h/cpp` | create | Rect-based font renderer (MVP) |
| `src/ui/budget_panel.h/cpp` | create | Budget panel implementation |
| `src/ui/population_panel.h/cpp` | create | Population panel implementation |
| `src/ui/rci_panel.h/cpp` | create | RCI panel implementation |
| `CMakeLists.txt` | modify | Add `simcity_ui` library, link into `simcity_engine` |
| `src/engine/game.h` | modify | Add `std::unique_ptr<ui::Panel>` members, `FontRenderer*` |
| `src/engine/game.cpp` | modify | Init panels in `init()`, call panel render in `draw_hud()` |
| `tests/test_ui.cpp` | create | Unit tests for panel data formatting (headless) |

### Specification

**Panel data formatting (headless-testable):**
- `BudgetPanel::format_funds(int64_t)` → `"$20,000"`
- `BudgetPanel::format_income(int64_t)` → `"+$1,250"` or `"-$500"`
- `PopulationPanel::format_population(int)` → `"12,345"`
- `RciPanel::demand_to_width(Demand)` → pixel width for bar

**Verification criteria:**
```
This ticket is done when:
- [ ] BudgetPanel renders funds, income, expenses, tax rate
- [ ] PopulationPanel renders total pop, R/C/I breakdown, jobs
- [ ] RciPanel renders three horizontal demand bars with labels
- [ ] Panels use FontRenderer abstraction (rect implementation works)
- [ ] Panels positioned correctly, don't overlap map view awkwardly
- [ ] Camera shake does not affect panel positions
- [ ] All existing tests pass
- [ ] New test_ui.cpp passes (headless formatting tests)
- [ ] Lint passes (no warnings)
- [ ] No regressions in simcity_engine or simcity targets
```

## Testing Strategy

**Unit tests (`tests/test_ui.cpp`):**
- `test_budget_format()` — verify number formatting with commas, signs
- `test_population_format()` — verify population formatting
- `test_rci_demand_width()` — verify Low=0, Medium=1/3, High=full
- `test_panel_bounds()` — verify panels fit in 800×600 and 1280×720

**Integration tests (manual via game):**
- Run game, verify panels visible in Playing state
- Verify panels update each month (funds change, population grows)
- Verify RCI bars match `Stats::res_demand` etc.
- Test window resize (panels should reposition or clamp)

**Edge cases:**
- Negative funds (show as `-$X,XXX`)
- Very large numbers (millions)
- Zero population
- All demand levels (Low/Medium/High)

## Estimation

- **Complexity: medium** (2–8h) — 6 new files, CMake changes, Game integration, tests
- **Risk: medium** — font abstraction untested until T019; layout may need iteration
- **Blocking dependencies: no** — all code exists, just new files

---

Plan complete for T009.
Output: aes/tickets/T009-plan.md
Next step: /aes-build
