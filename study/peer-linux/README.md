# Running the peer's SimCity recomp on Linux

`Junior-Jones/SimCity-SNES-Static-Recomp` is published as a Windows project. On
Linux you do not get a game — you get a **library**. This directory is the
recipe for turning that library into something you can actually watch.

## What is here, and whose it is

| file | whose |
|---|---|
| `build-peer-linux.sh` | ours |
| `jjhead.c` | ours — a headless driver over the peer's public C API |
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

```bash
# from the repository root, so the ROM resolves
study/peer-linux/build-peer-linux.sh

# with our own route into a city
study/peer-linux/build-peer-linux.sh 2>&1 | tail -20
SCRIPT="$PWD/scripts/d_city.script" FRAMES=4000 study/peer-linux/build-peer-linux.sh
```

Useful variables: `SRC` (reuse an existing clone), `ROOT`, `OUT`, `FRAMES`,
`ROM`, `SCRIPT`.

The ROM path must be **absolute** — the peer chdirs to its own executable
directory, so a relative path will not resolve.

## Two gotchas that cost real time

**`-lstdc++` is mandatory at link time.** The core's public header is C but its
implementation is C++. Without it the link dies with undefined `operator
new[]` / `operator delete[]`, which reads like a missing dependency and is not
one.

**The game needs a 32 KiB SRAM file to start.** Passing `/dev/null` or a
missing path is reported as `srm_in bad` and the run does not proceed the way
you expect. A cold SRAM is just 32 KiB of zeroes.

## The ceiling, and it is not a broken build

Without a mouse the game stops on the city-naming screen, showing `11111_` with
the hand resting on SPACE. Measured, not assumed:

```
$0193 (town-route index):  0 -> 0
$00C5 (dispatch index):    0 -> 0
```

The naming screen's cursor is controlled by the mouse, not the d-pad. The d-pad
moves the tool cursor on the map, but not here. Substituting `press a` for each
`mouseclick` does not move the hand. The peer contains no mouse code at all —
`grep -ril mouse` over its source returns nothing.

**This is the same obstacle our own port has**, which is why it is worth having
measured it on both sides. Ours has a scriptable mouse and a stalled clock; the
peer has a working clock and no mouse. Getting past this on the peer means
running the Windows frontend under Wine/Proton with `xdotool` driving a real
pointer.

If you ran with no input and `$0B55` stayed `0` forever, that is **correct**
behaviour, not a failure. See the date encoding below.

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
