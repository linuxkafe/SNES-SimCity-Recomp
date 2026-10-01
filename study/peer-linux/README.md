# Running the peer's SimCity recomp on Linux

`Junior-Jones/SimCity-SNES-Static-Recomp` is published as a Windows project. On
Linux you do not get a game — you get a **library**. This directory is the
recipe for turning that library into something you can actually watch.

## What is here, and whose it is

| file | whose |
|---|---|
| `build-peer-linux.sh` | ours |
| `jjhead.c` | ours — a headless driver over the peer's public C API |
| `jjwin.c` | ours — a **windowed SDL2 frontend**: play it, don't just measure it |
| `bgra2png.py` | ours — converts the peer's BGRA frame dumps to PNG |

**None of the peer's source is committed here.** The script clones it at run
time. The peer repository declares no licence (`license: null`, no `LICENSE`
file), so this is private study only: do not publish, redistribute, or port it
until the author grants permission.

## Why the build is only half a build

The peer's `CMakeLists.txt` branches on the host:

```cmake
if(WIN32)
    add_subdirectory(frontend/windows)
else()
    message(STATUS "Windows frontend omitted on this host; ...")
endif()
```

On Linux the configure step tells you so out loud. You get
`libsimcity-static-recomp.a` — the portable Full Static core — and nothing that
opens a window. The Windows frontend is where the window, the audio device and
the **mouse** live.

So a frontend is required, and the peer does not ship one for Linux. `jjhead.c`
is ours: it calls the same public API the Windows launcher calls, so both drive
the identical core.

## Run it

The default is to **play** it. Build, then the window opens:

```bash
# from the repository root, so the ROM resolves
study/peer-linux/build-peer-linux.sh
```

To measure instead of play:

```bash
study/peer-linux/build-peer-linux.sh --headless
SCRIPT="$PWD/scripts/d_city.script" FRAMES=4000 study/peer-linux/build-peer-linux.sh --headless
```

Other flags: `--sram <path>` to use a specific save, `--help` for the full
rationale. Environment: `SRC` (reuse an existing clone), `ROOT`, `OUT`,
`FRAMES`, `ROM`, `SCRIPT`.

The ROM path must be **absolute** — the core chdirs to its own executable
directory before opening anything, so a relative path will not resolve. The
script checks this and says so rather than letting the core fail obscurely.

### Positional arguments are rejected on purpose

An earlier version ignored them. Running

```
build-peer-linux.sh /tmp/opencode/peers/run/jjwin "$PWD/SimCity (USA).sfc"
```

therefore built a windowed frontend and then ran the *headless* driver, which
reads exactly like "the windowed build is still headless". It now takes
`--headless`, `--sram` and `--help`, and anything else is an error with exit
code 2. A build script that silently discards what you typed is worse than one
that refuses it.

## Two gotchas that cost real time

**`-lstdc++` is mandatory at link time.** The core's public header is C but its
implementation is C++. Without it the link dies with undefined `operator
new[]` / `operator delete[]`, which reads like a missing dependency and is not
one.

**The game needs a 32 KiB SRAM file to start.** Passing `/dev/null` or a
missing path is reported as `srm_in bad` and the run does not proceed the way
you expect. A cold SRAM is just 32 KiB of zeroes.

## Playing it

`jjwin.c` is a windowed SDL2 frontend — window, keyboard, speaker. The core
hands out a finished BGRA framebuffer and takes a 12-bit button mask, so a
frontend is all that was ever missing.

```bash
/tmp/opencode/peers/run/jjwin "$PWD/SimCity (USA).sfc"
```

| key | SNES |
|---|---|
| arrows / WASD | d-pad |
| `Z` `X` `C` `V` | A B X Y |
| `Q` `E` | L R |
| `Enter` / `Shift` | Start / Select |
| `Esc` | quit |

The ROM path must be **absolute**, for the same reason as the headless driver.

### Correction: the naming screen does NOT need a mouse

An earlier version of this file said the city-naming screen was mouse-gated and
that a keyboard could not pass it. **That was wrong**, and it was wrong in a way
worth recording.

I substituted `press a` for each `mouseclick`, the hand did not move, and I
concluded the cursor was mouse-driven. I had tested exactly one direction.
Ten `press right` on the naming screen walk the hand off `SPACE` onto the
`P`/backspace key — captured and rendered, not reasoned about.

The peer's own launcher corroborates it: `grep -ri mouse frontend/` over their
entire frontend returns **nothing**. It is keyboard and XInput only. There was
never a mouse in this path.

This matters beyond the peer. Our own `scripts/d_city.script` reaches a live
city using `mouseclick`, and it does not need to. If the naming screen is
d-pad-navigable, the route we built around a mouse is solving a problem that may
not exist — and we should find out whether our mouse support is load-bearing or
just habitual.

## The honest ceiling

What is still true: with **no input at all**, the peer sits on the title screen
and `$0B55` stays `0` forever. That is correct behaviour, not a failure.

## Reading the output

- `frame.f*.bgra` — rendered frames, 256x210 BGRA. `bgra2png.py` converts them.
- `timeline.log` — per-sample frame, master clock, instruction count, and the
  WRAM fields that matter for the clock.
- `wram.f*.bin` — full 128 KiB WRAM snapshots.

Sanity numbers for a healthy run:

```
video_standard=NTSC nominal_fps=60.098814 avg_master_clocks_per_frame=357366
```

**Date encoding.** `$0B53=0 / $0B55=0` means **1900 January**. It is not
"unset". A city that is actually simulating moves month to month roughly every
1900 frames.

**`$0B51` is a red herring.** It is a free-running counter modulo 4
(`INC $0B51` … `AND #$0003` … `INC month`), so it reads 0 one frame in four by
design. "It stays 0" is not evidence of a frozen clock. Use `$0B53`/`$0B55`.

For how these fields were found, and for the full chain from the stalled
scheduler to the frozen date, see `docs/RE_CITY_FREEZE.md`.

## Measured: how the naming-screen cursor moves

Mapped by differencing consecutive rendered frames. The screen is static except
the hand, so the bounding box of what changed **is** the hand position. No
guessing, no reading the disassembly.

| press | effect |
|---|---|
| `right` | walks one row, 10 positions, `x = 56, 72, 88 … 200`, step 16, then wraps to 56 |
| `down` | moves down one row; from the QWERTY row it lands on `y = 175`, the `Z … END` row |
| `up` | moves up a row |

The `END` key sits at the right of the `Z … END` row, `x ≈ 200, y = 175`. A
reliable route to put the hand on it is:

```
press down   1        # to the Z..END row
press right  8        # walk to END
```

### What is still unknown

**Which button confirms.** `A` on `END` does not leave the screen, and `A` on a
letter does not type into the field — measured, with frames rendered either side
of the press. Tried and ruled out: `A` on a letter, `A` on `END`, and `A` on
`END` after walking back onto a letter.

The name field reads `11111▁___`, which may be five characters already typed
rather than a prompt, so the refusal may be "name already set" rather than
"confirms with a different button". Not distinguished.

This is the last step to reaching a city from the keyboard, and it is the reason
`scripts/cross-load-peer-save.sh` has not been run yet: it needs a save from a
peer session that reached a running city, and the only one we have is the
player's.
