# Requirements — SimCity SNES PC Port

## Functional Requirements

### FR-1: ROM Ingestion
- [ ] Parse LoROM SimCity (USA) header (title, cart type, ROM/SRAM size, country, checksum).
- [ ] Validate checksum (SimCity self-referential convention: sum of all bytes & 0xFFFF == stored checksum).
- [ ] Expose LoROM→file offset translation for arbitrary CPU addresses.

### FR-2: Graphics Extraction
- [ ] Decode 4bpp planar SNES tiles from ROM VRAM.
- [ ] Convert BGR555 palettes to RGBA8888.
- [ ] Render tile grid with zoom/pan; auto-exit for headless smoke test.

### FR-3: Simulation Core
- [ ] 120×100 tile grid matching SNES map dimensions.
- [ ] Terrain types: grass, water, forest, road.
- [ ] Zone types: residential, commercial, industrial.
- [ ] Residential density 0..5 per tile; growth governed by job capacity (commercial + industrial).
- [ ] Monthly budget: income = population × tax_rate × k; upkeep per tile type; funds clamped at zero.
- [ ] RCI demand indicators (Low/Medium/High) derived from population vs jobs/capacity.
- [ ] Deterministic: same operation sequence → identical state.

### FR-4: Game Loop & Window
- [ ] Fixed-timestep loop at 50 fps (SNES rate).
- [ ] SDL2 window with title "SimCity"; ESC to quit.
- [ ] Frame counter printed on clean exit.

### FR-5: Map Rendering & Editing
- [ ] Render full 120×100 map into streaming texture (16 px/tile).
- [ ] Camera pan with arrows/WASD; clamp to map bounds.
- [ ] Tools: bulldoze (1), road (2), residential (3), commercial (4), industrial (5).
- [ ] Mouse click+drag paints selected tool; rebuild texture on dirty flag.
- [ ] HUD: top bar with R/C/I demand bars + pause indicator.

### FR-5.1: Controls & Debug Keys
- **Core controls**: arrows/WASD pan; 1-5 select tool; space pause; ESC back to menu.
- **Debug hotkeys (PC port only — do NOT exist on the original SNES menu)**:
  - `F1` — cycle sub-palette (0-7) of the city palette block; prints `Palette: block=.. sub=..` to stderr.
  - `F2` — cycle palette block (0-13); prints the same line.
  - `6`/`7` — trigger Meteor/Monster disaster for testing.
  - These exist purely for live palette/tile inspection and latch onto `Game::Config`
    (`palette_block`/`palette_sub`), so a tuned value can be saved to a launch flag.

### FR-6: ROM Map Loader (T004/T017)
- [x] Locate and decompress scenario map packets using Nintendo LC_LZ5 packet format.
- [x] Decode second-stage 16-bit RLE (repeat count in bits 10–13, tile ID in low 10 bits).
- [x] Seed initial terrain from scenario 0 map (headless smoke prints `Loaded scenario terrain from ROM`).
- [x] Fallback to generated terrain if decode fails.

### FR-8: ROM Asset Rendering (T017)
- [x] Extract Layer1 city simulation tiles ($07E584–$08C4DB, 1024 × 4bpp 8x8 tiles) at runtime.
- [x] Extract BG palettes ($058000–$058E00, 14 × 0x100 raw BGR555) at runtime.
- [x] CityView renders terrain cells from ROM tiles + palette (2× upscale), flat-colour fallback otherwise.
- [x] Flat tint retained for zone cells (R/C/I) driven by sim state.

### FR-7: Audio (Future)
- [ ] Extract SPC/ADPCM samples from ROM.
- [ ] Playback via SDL_mixer or equivalent.

## Non-Functional Requirements

| ID | Requirement |
|----|-------------|
| NFR-1 | Zero copyrighted assets in repo; all extracted at runtime from user ROM. |
| NFR-2 | All simulation code headless-testable (no SDL linkage). |
| NFR-3 | Deterministic simulation: no RNG, no platform-dependent behaviour. |
| NFR-4 | CMake + C++17; SDL2 2.30+ as only external dependency. |
| NFR-5 | All unit/integration tests pass via `ctest` before ticket close. |
| NFR-6 | AES-compliant lifecycle for every non-trivial task. |