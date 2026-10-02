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
✅ **Holds its frame rate**: 60.06 fps peak on an i5-8500T; 56.88 fps on a Steam
   Deck, **frame-locked** (five runs identical to the millisecond, so that figure
   cannot detect guest slowdown at all). Both are gross-regression floors, not
   headroom figures — see the Performance section. 

![SimCity title screen](docs/screenshots/title.png)

⚠️ **The city loads and renders, but it does not simulate.** Not deliverable. The date stays
`1900 JAN` forever — no month ever appears across 30,000 frames, the seasons
never recolour the map, the population stays 0 — while the controller does
nothing. `scripts/d_city.script` drives the game from boot into a live city
headlessly and deterministically, so this is not a game-flow problem, and the
renderer is proven live: poking a WRAM byte moves the presented picture on the
very next frame.

![A city at 1900 JAN, frozen](docs/screenshots/city-frozen.png)

**Where it is stuck: NOT KNOWN, and here is what is actually measured.**

> **RETRACTED 2026-10-02 (review finding R-03).** This file used to say, in the
> present tense and two lines below the honest status above: *"This is not a
> compilation problem — the bank-03 tick is compiled to native C and still does
> not run."* **That is false as stated.** Measured on 2026-10-02 over frames
> 0–3700, bank 03 executes **921 distinct PCs and 515,043 interpreted steps** —
> it is not code this build never reaches. What is true is much narrower and much
> more interesting:
>
> - bank 03 runs up to **f3300** (160,693 steps in f3100–f3300) and executes
>   **zero** steps from f3301;
> - the live-city window (f3381–f3700) is confined to **banks 00 and 01**;
> - the city is on screen from ≈f3378, so bank 03 goes silent roughly **78 frames
>   before** the city appears. That is a **correlation** and no cause is claimed
>   for it anywhere in this repository.
>
> Instrument, raw counts and what the instrument cannot see:
> [`docs/measurements/2026-10-02-deck-interp-histogram.md`](docs/measurements/2026-10-02-deck-interp-histogram.md).
> Classification of every causal claim in this project:
> [`docs/CAUSE_CLAIMS.md`](docs/CAUSE_CLAIMS.md).

The chain below is the generation of hypotheses that was measured false on
2026-10-02. It is printed here as history, not as the answer, because it is what
this file asserted until then:

**RETRACTED 2026-10-02 — every row of this table is history, not a diagnosis.**
(The longer retraction is directly below the table; this line exists so that no
row inside the block is more than a few lines from the word that refutes it.)

```
$009311  the guest waits on $B9, set by the NMI handler   $B9 = 0, $C7 spinning
$03D287  the round-robin scheduler loop                    $14 does not advance
$03D2F6  AND #$9000 gates INC $14            ROM 0x01D2F6  <-- THE GATE
$03D2A3  sets $12 = 1                                       $12 = 0 in 13/13 samples
$008061  the per-vblank body                               proven by pokefor
$00825F  writes CODE_038000 into $1F7D..$1F7F               00 00 00 00 in play
$038000  the task that increments the month                does not run
$0B53/$0B55  the date                                       never leaves 0
```

**RETRACTED 2026-10-02 — the block above is a retracted chain, in full.**
Do not read it as the diagnosis. Its two
load-bearing claims are both measured false:

- `$12` **is not 0**. It reads `0001` at 5 of 5 frame-boundary samples spanning
  2,599 frames of a live city, as does `$14` — so the "THE GATE" link rests on a
  value that is not what the table says it is.
- `CODE_008061 demonstrably does not run` is **OPEN, not established.** Its
  evidence was `$12 == 0`, which is false. A false premise voids the inference
  and establishes nothing in its place — which is *not* a claim that it does run.

The one solid result here is narrower than it looks: forcing `$12 = 1` with
`pokefor 0012 01 400` does make `$1F7D..$1F7F` become `00 80 03`, so **the path
through `CODE_00825F` exists**. That shows reachability, not causation; a causal
claim needs the converse, and the converse was never measured.

Also settled, and the reason the deadlock is gone: `$009311` no longer spins
(`$B9 = 0001` at 5/5, `$C7` advancing). See `docs/RE_CITY_FREEZE.md` entry (q)
and `docs/CLAIMS_REGISTER.md` §2, §13, §14.

**The current position:** the city loads and renders, and does not simulate — 34
WRAM bytes change across 2,599 frames of a live city. **Why is not established.**
The next measurement is a PC/block histogram over f3400-f3600. See
`docs/DEFINITION_OF_DONE.md` for why "the root cause is known" is listed there
under *Not criteria*.

**The reference that works, and what comparing against it proved.**
`Junior-Jones/SimCity-SNES-Static-Recomp` recompiles the same ROM and its clock
runs — 23 months across 33,700 frames. We drive it ourselves: `study/peer-linux/`
builds its portable core on Linux and adds a windowed SDL2 frontend, because its
own launcher is Windows-only and the core has no mouse. A city is reachable from
the keyboard alone. Both cores were then traced at 100-frame intervals and
diffed, and it reduces to a single fact.

`$0B51` is the 16-bit master city tick counter. `CODE_038016` does
`INC.w $0B51`, accumulates tax into `$0DC7`, and only advances the month when
`AND.w #$0003` is zero. In the peer it climbs `00 -> 006D`. In this build it is
`0000` at every single sample from frame 3150 to 30000.

> **RETRACTED 2026-10-02 (review finding R-02).** The paragraph above concluded:
> *"Therefore `INC.w $0B51` executes zero times — the tick routine is never reached
> after the city loads."* **That conclusion is withdrawn, not replaced.** Ledger
> row **R-020** records the claim with status `invalidated-premise`: its premise
> (`$0012 == 0`) is false, which voids the inference and establishes **nothing in
> its place**. The arithmetic above is correct; the inference drawn from it is not
> evidence.
>
> What is measured, stated without inference:
>
> | | |
> |---|---|
> | `$0B51` at f3600 | `0000` |
> | `INC.w $0B51` = `EE 51 0B` at ROM offset | `0x18026`, occurring exactly once |
> | **is `$03:8026` among the 921 bank-03 PCs that execute?** | **OPEN — unmeasured** |
> | `$0DC7` at f3600 | `0000` |
> | WRAM bytes changing across 2,599 frames of a live city | **34** (not 53 across 30,000 — ledger R-014) |
>
> The histogram that counted those 921 bank-03 PCs prints only the top 60 by
> host-time, so **the run that produced the count cannot answer whether this
> particular instruction is among them.** It stays open rather than being
> resolved in the convenient direction. Ticket T087; the single cheapest
> remaining measurement.

**The open question — reopened, and the last answer was wrong.** For twelve
commits this was framed as an unbounded static question — how does control reach
`$03:8026`? — and then as a bounded one, in our own recompiler. **Both framings
are now falsified by measurement on the Deck.**

**COP refusal is not the cause.** The theory was that we refuse to decode COP
(`recompiler/snes65816.py:483`), poison every function containing one, and
suppress their outgoing demands — so the COP-dispatched subtree is invisible to
reachability, and `$03:8026` has no compiled body. The tier-2 discovery journal
says otherwise:

- `$03:8000–$03:8200` appears as a dispatch target or tier-down **zero times** in
  12,000 frames. The guest never attempts the transfer.
- The COP path is **not** broken. `$008211`/`$00821E` execute, and two of the
  eleven poisoned nodes (`$008E43`, `$008E75`) were genuinely reached *through*
  the `$8223` dispatch and interpreted to a clean exit. Suppressing outgoing
  demands does not prevent execution.
- And **no word anywhere in the ROM points into `$038000–$038220`** — zero, of
  any form. Refusing to decode COP cannot cause a transfer that is never
  generated.

**The world is stopped, not mis-dispatched.** After frame 3145 the guest produced
**zero new tier-downs across 8,855 consecutive frames**, and frames 6000, 8750
and 11500 are **byte-identical** — 0 of 75,264 pixels differ. Only 35 WRAM bytes
move between frame 6000 and 11500. That is a halted machine, not a missing
dispatch.

**The ambiguity this reopens.** No pointer exists in the ROM, yet the reference
takes `$0B51` to `$006D`. So either a caller assembles bank `$03` and address
`$8000` arithmetically, or **the reference never runs `$03:8026` at all** and
advances `$0B51` some other way. We have been assuming the second half of that
without evidence since the day we adopted the peer's clock as ground truth. The
single measurement that separates them: take a PC trace of frames 3140–3150 and
identify the last block that executes before silence.

Two corrections to what this file previously claimed, both because the
measurements were wrong rather than the reasoning:

- `$0B53` is the **absolute** year, not an offset from 1900: a live city reads
  `1900` (`0x076C`). `$0B55` is the month index, 0-based. So `$0B53 = 0` means
  **there is no city** — which is what our build reads at every sample. The date
  here is not stuck, it has never been written.
- `$0B51` was retracted twice in this file, in opposite directions. It was
  **not** a free-running counter modulo 4 reading 0 one frame in four by design;
  it is the 16-bit master city tick. The original claim that "`$0B51` stays 0, so
  the tick never runs" was right, and retracting it was the error. Do not retract
  it again without reading `CODE_038016`.

- `$0B12` was published here as the sharpest lead between a clock that runs and
  this one. **It was noise.** It is `$00` across all 337 peer dumps over 33,700
  frames and 23 months, and appears nowhere in the tick routine. One unrepeated
  report was promoted to a lead; it should have been measured twice before being
  written down.

`$02BF` is also not a pause flag — it is written once in the whole ROM. The vblank
wait loop is not a livelock (the guest leaves it every frame), and the frame
counter `$0406` is not "+1 per frame". `docs/RE_CITY_FREEZE.md` records both
retractions with the evidence that overturned them, because this project has
now published a wrong diagnosis twice and the corrections are worth more than
the claims were.

### Where the current position lives

This file is the entry point, and it is **not** the maintained record. Four files
are, and this section is the only place they are listed:

| file | what it holds |
|---|---|
| [`docs/CAUSE_CLAIMS.md`](docs/CAUSE_CLAIMS.md) | every causal claim in this project, classified MEASURED / INFERRED / RETRACTED / OPEN, with the instrument or the reason there is none |
| [`docs/CONFLICTS.md`](docs/CONFLICTS.md) | every contradiction found between docs, code and git history, with the command that found it |
| [`docs/RE_CITY_FREEZE.md`](docs/RE_CITY_FREEZE.md) | the chronology, with a 43-row index at the top and a per-entry state banner on each entry |
| [`docs/measurements/`](docs/measurements/) | raw measurements, the exact commands, and **what each instrument cannot see** |
| [`docs/CLAIMS_REGISTER.md`](docs/CLAIMS_REGISTER.md) | the index of what is retracted, superseded or unverified |
| [`docs/DEFINITION_OF_DONE.md`](docs/DEFINITION_OF_DONE.md) | the standard of proof — **no acceptance criterion may be satisfied by a claim** |

**The one question this repository has not answered:** why the city does not
simulate. It is labelled **OPEN** everywhere it appears, it is the only row in
`docs/CAUSE_CLAIMS.md` with no instrument, and no cause for it is asserted
anywhere in this tree.

## Gates

Every figure below names the machine and the date it was measured on. A gate
result without a machine is not a measurement — see
`docs/DEFINITION_OF_DONE.md` Rule 0.

| command | what it proves | dev host `seyon`, 2026-10-02 | Deck `steamdeck`, 2026-10-02 |
|---|---|---|---|
| `make test` | the core replays deterministically (30 frames) | PASS (2/2) | not run |
| `make test-rom` | the picture moves (frames 200–800) | PASS, **257** distinct crc32 | not re-run |
| `make perf` | the frame rate holds (600 frames) | **PASS 51.52 / FAIL 48.38** — see below | PASS, 56.88 fps |
| `make clock` | **the city actually simulates** (6000 frames) | **FAIL** | **FAIL** (identical) |
| `make check-claims` | no retracted claim is asserted without a marker | PASS | not run |

**`make perf` straddles its own threshold on this host.** The same binary, the
same ROM, the same commit, measured twice in one session: **FAIL at 48.38 fps**
and **PASS at 51.52 fps**, against a threshold of 50. That is host load, not a
recompiler change, and it is the clearest available demonstration that this gate
proves very little. On the Deck all five runs finished in exactly 10.549 s — to
the millisecond, five times — so Deck pacing is **frame-locked** and the gate
cannot detect guest slowdown there at all. It answers "does it still run".

`make clock` is the one that matters and the one that is red. It fails
identically on both machines: `1 distinct date images after f3600 (last change
f3378 of 6000)`. It now also prints the guest's own year word as proof that a
city object exists (`$0B53 = 0x076C` = 1900), so it **cannot report PASS on a
build that loads no city** — verified by pointing it at a script that never
reaches one, which it refuses with a distinct "no city was loaded" verdict.

The determinism gate (`make test`) is now genuinely relocatable. It used to
hardcode one developer's checkout path for both the script and the ROM, so it
could only pass there, and on a relocated clone it silently read the *original*
repository's files while running the relocated binary — reporting 2/2 passed on
a tree containing no ROM at all. It now takes the ROM from `$SIMCITY_ROM` or
finds one beside the build, and **skips with a clear message** if there is none,
rather than reaching outside the tree. `make test-rom` and `make clock` are the
gates that require a ROM and fail without one.

`make clock` is the one that matters and the one that is red. The other three
pass while the game is a still image, and they pass for the same reason each:
they all measure before the city exists. The first three inspect frames 30–800
and are still on the attract screen and the menus — they pass on the strength
of motion that stopped 2,700 frames before their own window ended.

`make clock` drives `scripts/d_city.script` into a live city and then reads the
HUD date off the screen, because that is the claim under test and no WRAM
address holding the month is known. Run `make clock-self-test` after changing
anything in it: there is no build here where the clock advances, so the only
way to know the detector still sees a live screen is to make it prove that on
a window that is alive.

### Performance

`make perf` measures it and fails on a regression. Numbers, same ROM, 600
frames:

| Machine | Build | fps | `guest` ms/frame | `upload-present` ms/frame |
|---|---|---|---|---|
| Steam Deck (Zen 2), **compiled on the Deck** | Release | 56.88 | **4.502** | **1.007** |
| i5-8500T, cross-built binary | Release | 59.5 | 4.97 | 7.84 |
| i5-8500T, cross-built binary | Debug (`-O0`) | 42.4 | 9.52 | 5.90 |

**Every figure in that table is retracted or superseded, and the reason is
instructive.**

- The Deck row's `2.45` ms was **never re-measured** and is ledger **R-012/R-021**.
  The measured Deck figure is **4.502 ms**. The old row also claimed
  `upload-present` **8.13 ms**, which on the Deck was measured at **1.007 ms** —
  the opposite side of the guest by a factor of four and a half. Ledger R-012,
  R-021.
- **"The emulated 65816 is not the bottleneck" is RETRACTED** (ledger R-023,
  R-025). It rested on `guest` being 2.45 ms against a large `upload-present`.
  Measured on the Deck the picture is the reverse: guest 4.502, upload-present
  1.007, and **deadline-wait 11.275** — pacing dominates, not the CPU.
- The "upload-present costs 6.8× the guest" figure that replaced it was an
  artifact of `SDL_VIDEODRIVER=dummy` **on the dev host**, not a property of the
  code. Neither number survives.
- The `-O0` row (42.4 fps, `-O0` costing 24% of the frame rate) is the one part
  that still stands, and it is why `make build` ships Release.

> ⚠ **ENVIRONMENT-FIDELITY CAVEAT on the Deck row.** The Deck's SteamOS rootfs is
> **damaged in a way pacman does not report**: 503 of 504 glibc headers under
> `/usr/include` are absent from disk while `base-devel` reports installed, and
> `echo '#include <stdio.h>' | gcc -E -` fails with `No such file or directory`.
> There is no sudo and no cached glibc, so it cannot be repaired. The Deck build
> resolves libc headers from a hand-assembled prefix at `/home/deck/sysroot`
> (headers from `archive.archlinux.org`) with **`-idirafter`** — deliberately
> not `-isystem`, which sorts before `/usr/include` and breaks libstdc++'s
> `#include_next <stdlib.h>`. It also needs `SDL_UNIX_CONSOLE_BUILD=ON`,
> `OPENGL_INCLUDE_DIR`, and `OpenGL_GL_PREFERENCE=LEGACY`.
>
> **So: the Deck binary was compiled on the Deck, against a reconstructed header
> prefix, on a machine whose rootfs is damaged.** A performance figure measured
> under those conditions describes those conditions. This caveat travels with
> every Deck number cited anywhere in this repository.

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

## Peer study

`study/peer-linux/` builds the reference recomp on Linux and gives it a window.

```bash
study/peer-linux/build-peer-linux.sh                    # build, then play
study/peer-linux/build-peer-linux.sh --headless         # measure instead
study/peer-linux/build-peer-linux.sh -- --date --wram 300   # pass options through
```

The peer ships a Windows-only launcher; on Linux its CMake omits the frontend and
builds just the core, so a frontend is required. `jjwin.c` is ours and talks to
the peer's public C API. Its repository declares no licence, so this is private
study: do not publish or redistribute it. A keyboard reaches a city on its own —
the naming screen's cursor walks on the d-pad and **B** confirms from a character
key. See `study/peer-linux/README.md`.

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
population and treasury never change. **The cause is OPEN** — see
[`docs/CAUSE_CLAIMS.md`](docs/CAUSE_CLAIMS.md) node C-006. (This line
previously pointed at `aes/tickets/T058-city-clock-does-not-advance.md`. `aes/`
is gitignored **permanently and by rule** — DoD D4.3 — so that path resolves in
no fresh clone. Pointing a tracked file into an uncommittable directory is the
same rule breaking itself; the pointer now names a tracked file.)

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
