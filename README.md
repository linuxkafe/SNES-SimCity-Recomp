# SNES-SimCity-Recomp

Partial static recompilation of SimCity (SNES) for PC using [snesrecomp](https://github.com/RetroPortingToolKit/snesrecomp).

An unofficial, non-commercial project that recompiles the Super Nintendo game
**SimCity** (Nintendo, 1991) into C++17 for PC. All graphics, palettes, map data
and audio are read at runtime from a copy of the original ROM that **you**
supply; the ROM and any ripped assets are never included. Built on the
**snesrecomp** framework.

### How much of it is actually native

The honest answer, because "native recompilation" on its own oversells this:

- **204 of the game's 303 routines** are recompiled ahead of time into C++17.
- The remaining **99 run in the bundled 65816 interpreter**. They hold only 605
  of 9,813 static instructions (6.2%), but they are spin/wait loops that execute
  about **1,427 opcodes per frame**, and a sampled CPU profile puts them at
  **~7% of total process CPU — roughly 89% of the time actually spent executing
  guest code**.
- So by function count this is two-thirds native; by time spent running the
  game, the interpreter is the larger half. **Neither figure is a measurement of
  gameplay**: every workload measured so far is attract mode and menus, because
  no script yet reaches a running city.

Measured with a `CLOCK_PROCESS_CPUTIME_ID` sampler validated against `addr2line`
on the Debug build (agreement within 0.15 percentage points), with the profiled
run's WRAM hash identical to the unprofiled one to show the sampler did not
perturb the guest. Note the caveat the snesrecomp fork itself documents: AOT
code never advances the PPU beam while the interpreter advances it every
opcode, so a *time* share is not automatically a *correctness* claim.

## About the Game

**SimCity** is an open-ended city-building simulation created by **Will Wright**
and published by **Maxis** in 1989. You play as the mayor of an empty plot of
land: you zone residential, commercial and industrial areas, lay roads, power
and water, set taxes and a budget, and watch the simulation grow — or go broke.
Disasters (fires, floods, earthquakes, and a certain giant monster) keep you on
your toes.

The **Super Nintendo Entertainment System (SNES)** version was developed by
**Nintendo EAD** under license from Maxis and published by Nintendo in 1991
(JP April 26, NA August 23; EU September 24, 1992). It is widely regarded as
the best console adaptation of the original: seasons recolour the map, Nintendo
cameos appear (a Mario statue, a Bowser rampage), and extra scenario missions
were added. This port reproduces that SNES release.

## Status

✅ **Runtime stable**: 10,000+ frames headless without watchdog timeout  
✅ **APU sync fixed**: No more startup timeout  
✅ **Watchdog fixed**: VBlank wait loop at $00927C forced to interpreter  
✅ **NMI handler stabilized**: Forced to interpreter at $0080B2  
✅ **Deterministic replay**: Bit-identical state traces verified  
✅ **Holds 60 fps**: 60.06 fps peak on an i5-8500T, 60.1 fps on a Steam Deck  

⚠️ **The city does not simulate.** Entry into the game works; once a city loads
the date stays `1900 JAN` and the population stays 0. This is the one thing
between this build and a playable game, and it is tracked in
`docs/RE_CITY_FREEZE.md`. Every picture-based check in this repo passes while it
is broken — a frozen city moves about four times per 1000 frames.

### Performance

`make perf` measures it and fails on a regression. Numbers, same ROM, 600
frames:

| Machine | Build | fps | `guest` ms/frame | `upload-present` ms/frame |
|---|---|---|---|---|
| Steam Deck (Zen 2) | Release | 60.1 | **2.45** | 8.13 |
| i5-8500T | Release | 59.5 | 4.97 | 7.84 |
| i5-8500T | Debug (`-O0`) | 42.4 | 9.52 | 5.90 |

Two things worth reading off that table. The emulated 65816 is **not** the
bottleneck — on the Deck it uses about 15% of the 16.67 ms frame budget, while
the host's SDL present path uses more than the guest. And the build type is not
cosmetic: `-O0` cost 24% of the frame rate on the same machine and the same
ROM, which is why `make build` ships Release.

The per-stage split comes from `SNESRECOMP_HOST_PROFILE=1`, which writes
`video profile: stage=...` lines into `last_run_report.json`. `guest` is the
emulated CPU; `upload-present` is the host's present path. They are different
costs with different owners.

Note that a healthy build is vsync-capped at 60 fps and so has **no headroom
visible in the fps figure at all** — faster hardware would not move it. `guest`
ms/frame is the number that shows headroom.

## Building

`build/` is **not** committed — it holds a full SDL3 build and no binary belongs
in the repository. Build from source first:

```bash
make build
```

Then run with your own `SimCity (USA).sfc`, **by absolute path**: the host chdirs
to the executable directory, so a bare relative filename resolves there, not in
your shell's working directory.

```bash
./build/SimCitySNESRecomp "$PWD/SimCity (USA).sfc"
```

## Requirements

- SimCity (USA).sfc ROM (user provided, not included)
- Linux/macOS/Windows with SDL3
- CMake 3.20+, C++17 compiler

## Building

```bash
git clone --recurse-submodules https://github.com/linuxkafe/SNES-SimCity-Recomp
cd SNES-SimCity-Recomp
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
```

`make build` does the same thing and is the supported path. Two notes that
matter if you configure by hand:

- **`libxtst-dev` is not required.** A clean build directory otherwise stops at
  `Couldn't find dependency package for XTEST`, because SDL3 enables XTEST
  (its synthetic-input extension) by default and nothing here uses it. Pass
  `-DSDL_X11_XTEST=OFF`; `make build` already does.
- **`make build` is Release and `make debug` is `-O0`.** The two were proven
  byte-identical on the guest before the default changed — same 128 KB WRAM
  image and same presented-crc32 column across attract, menu and naming at
  3000 frames, with the cartridge SRAM pinned cold on both sides. The
  equivalence is proven for those paths, not for all time and all input, so
  keep `test_deterministic_replay` in the loop if you switch back and forth.

Note: the `snesrecomp` submodule is pinned to a small fork
(`linuxkafe/snesrecomp`) with the SimCity host runtime additions
(debug/watchdog globals, recomp stack depth).

## Running

```bash
# Place your SimCity (USA).sfc in the project root
./build/SimCitySNESRecomp "$PWD/SimCity (USA).sfc"

# Resolution presets — pin the window to a fixed display size
SNESRECOMP_RESOLUTION=720p ./build/SimCitySNESRecomp "$PWD/SimCity (USA).sfc"   # 1280x720
SNESRECOMP_RESOLUTION=800p ./build/SimCitySNESRecomp "$PWD/SimCity (USA).sfc"   # 1280x800
SNESRECOMP_RESOLUTION=1080p ./build/SimCitySNESRecomp "$PWD/SimCity (USA).sfc"  # 1920x1080
SNESRECOMP_RESOLUTION=2560x1440 ./build/SimCitySNESRecomp "$PWD/SimCity (USA).sfc"  # raw WxH also works

# Debug flags
SIMCITY_DEBUG_WATCHDOG=1 SIMCITY_DEBUG_APU=1 ./build/SimCitySNESRecomp "$PWD/SimCity (USA).sfc"

# Headless test
SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy ./build/SimCitySNESRecomp "$PWD/SimCity (USA).sfc"

# SNES Mouse on player 2 (opt-in; host cursor hidden, guest cursor takes over)
SNESRECOMP_MOUSE=1 ./build/SimCitySNESRecomp "$PWD/SimCity (USA).sfc"

# Input diagnostics — manual+auto read depth, auto word seen by the guest
SNESRECOMP_MOUSE=1 SNESRECOMP_PAD_PROBE=1 ./build/SimCitySNESRecomp "$PWD/SimCity (USA).sfc"
```

## Controls

All defaults below; every binding is rebindable through a `keybinds.ini`
`[KeyMap]` beside the executable (dump what a build resolved with
`SNESRECOMP_KEYMAP_DUMP=1`).

**Quick save / load** (10 slots)
- `F1`–`F10` — quick **load** slot 1–10
- `Shift+F1`–`Shift+F10` — quick **save** slot 1–10
- `F11` — save-state menu (thumbnail browser)
- `F12` — rewind filmstrip (≈60 snapshots, ~6 s of history)
- Controller: **Select + R** opens the save-state menu

**Time and speed**
- `Tab` — **turbo**: run the simulation as fast as the machine allows and
  render every 16th frame (use to fast-forward a growing city)
- `P` / `Shift+P` — pause (paused screen dimmed/not)

**Other**
- `Ctrl+R` — reset
- `Alt+Enter` — fullscreen toggle
- `Alt+W` — widescreen toggle
- `F` — display performance readout
- `Keypad +` / `Keypad −` — volume up/down

Save-state slots are stored as `saves/save1.sav` … `saves/save10.sav` in the
`saves/` directory beside the executable; battery SRAM (the in-game "SAVE" /
"LOAD" menus) uses `saves/save.srm`.

## Mod Support

The recompiled core stays **byte-deterministic**: acceleration or save/load
only changes *how many* simulated frames run or when the timeline jumps —
never the state of any single frame.

**Built-in QoL toggles** (environment variables, off by default):
- `SIMCITY_WIDESCREEN=1` — widen the isometric view (336 px frame)
- `SIMCITY_GODMODE=1` — infinite money, instant build
- `SIMCITY_DISASTER_TOGGLE=1` — disable disasters; `=2` force random ones

```bash
SIMCITY_GODMODE=1 ./build/SimCitySNESRecomp "$PWD/SimCity (USA).sfc"
```

**Mod packages (`.snesmod`)**: the framework's package loader is enabled and
stages `mods/preloaded/packages/` beside the executable; a launcher Mods page
installs and uninstalls packages. This port ships **no** packages by default
(its own mods are the `SIMCITY_*` toggles above). The package format is
documented in the framework at `snesrecomp/docs/MOD_PACKAGES.md`.

## Debug Flags

- `SIMCITY_DEBUG_WATCHDOG=1` - Watchdog handler with CPU state dump
- `SIMCITY_DEBUG_DMA=1` - DMA transfer logging with source validation
- `SIMCITY_DEBUG_APU=1` - APU sync timing logs

## Architecture

```
src/
├── main.c              # Host entry, presenter callbacks, config
├── game_rtl.c          # Frame loop, NMI/IRQ, VBlank wait
├── gen_stubs.c         # Mod hooks (GodMode, disasters)
└── gen/                # Generated by snesrecomp (gitignored)
    └── bank*_v2.c      # Recompiled 65C816 → C
recomp/
├── bank00.cfg          # Bank 0 config with force_lle
├── bank01-07.cfg       # Bank configs
└── funcs.h             # Function declarations (auto-synced)
```

## Legal & Attribution

This is an **unofficial, fan-made project**. It is **not affiliated with,
sponsored by, or endorsed by Nintendo, Electronic Arts, or Maxis.**

- **SimCity®** and the SimCity logo are registered trademarks of
  **Electronic Arts Inc.** SimCity was originally created by **Will Wright**
  and published by **Maxis** (1989); the SNES version was developed by
  **Nintendo EAD** under license from Maxis and published by **Nintendo**
  (1991). All game code, graphics, audio and other assets are © their
  respective owners (Maxis / Electronic Arts / Nintendo).
- **Super Nintendo Entertainment System**, **SNES** and **Super Famicom** are
  trademarks of **Nintendo**.
- **ROM not included** — you must legally own and supply your own
  `SimCity (USA).sfc`. No copyrighted ROM, ripped tiles, palettes or audio are
  committed to this repository.
- Published binaries contain recompiled 65C816→C code **derived from your
  ROM**; the ROM image itself is never committed or distributed.
- This project does not circumvent copy protection and is offered for
  **non-commercial** preservation and research use.
- Project license: **PolyForm Noncommercial 1.0.0** — non-commercial use only.
- snesrecomp framework: PolyForm Noncommercial 1.0.0, copyright © 2026 Matthew
  Stanley. The runner statically links MIT/ISC third-party components
  (snesrev's zelda3/smw ports, LakeSnes, ares-derived coprocessor cores);
  the required license notices ship with every distributed build in
  [`THIRD_PARTY_ATTRIBUTION.md`](THIRD_PARTY_ATTRIBUTION.md).

## Development

```bash
# Regenerate from ROM
bash tools/regen.sh "SimCity (USA).sfc"

# Run tests
cd build && ctest --output-on-failure

# Deterministic replay test
SIMCITY_DEBUG_WATCHDOG=1 SIMCITY_DEBUG_APU=1 \
  SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy \
  timeout 30 ./build/SimCitySNESRecomp --script tests/deterministic_replay.script "$PWD/SimCity (USA).sfc"
```

## Feature Status

| Feature | Status |
|---------|--------|
| Core recompilation (recompiled 65816 drives the PPU) | ✅ Done |
| Runtime stability | ✅ Done (10k+ frames) |
| Title screen | ✅ Working |
| Native widescreen (336 px, game-native renderer) | ✅ Done |
| Game-native graphics (terrain, buildings, font) | ✅ Working (verified interactively) |
| Headless capture | ✅ Fixed (T039) — `make test-rom` gates it |
| AOT compilation of declared functions | ✅ Done (187 functions, T057) |
| Config bar over the game | ✅ Done (T054) |
| SNES Mouse on player 2 (`SNESRECOMP_MOUSE=1`, bsnes-exact protocol) | ✅ Device-level done (T042, ROM-free verified) |
| Resolution presets (720p/800p/1080p, `SNESRECOMP_RESOLUTION`) | ✅ Done (T041) |
| Quick save/load (10 slots), save-state menu, rewind, turbo | ✅ Working |
| **City simulation runs (date, population, treasury advance)** | ❌ **T058 — the city view loads and then zero simulation ticks run** |
| Scenarios (all 5 US) | ⏳ T011 — confirm ENT step is the gate (see `docs/RE_SCENARIO_NAV.md` step 10) |
| Building/visual verification (headless capture) | 🔄 T033 — unblocked by T039 |

The city view renders correctly and the frame loop runs once per frame
throughout; the game state never leaves its initial values, so the date,
population and treasury never change. See `aes/tickets/T058-city-clock-does-not-advance.md`.

## License

PolyForm Noncommercial 1.0.0 - See [LICENSE](LICENSE) for details.

### Widescreen (16:9)

`Widescreen = 1` is a per-title PPU contract and does nothing here, for a
reason worth stating plainly: SimCity draws a fixed 256 columns, and the
`frame_width` of 336 is 256 plus 40 blank pixels per side. Nothing renders past
column 256, so the margins are empty by construction - a PPU-side 16:9 would be
71 blank pixels per side instead.

What works is 16:9 as a **presentation**: the authentic 256-wide picture
stretched to 16:9, filling the window edge to edge with no bars.

```ini
[Graphics]
DisplayAspect=16:9
```

The default is unchanged. The toggle key cycles 4:3 <-> 16:9.

Presentation only, and the precise claim is narrower than "identical": in 16:9
the PPU frame is 256 wide instead of 336, so the **presented framebuffer is a
different width** and will not hash equal to the 4:3 one. What is identical is
the guest's own 256 columns - cross-platform checked on two machines, where the
256 columns inside the margins hash equal to the whole 16:9 framebuffer, and the
margins are solid black. An earlier version of this file said "byte-identical
either way" and was wrong; see `docs/RE_CITY_FREEZE.md`.
