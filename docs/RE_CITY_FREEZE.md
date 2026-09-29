# T060 — the clock does not advance in a running city

Entry into the game is solved (`RE_SCENARIO_NAV.md`). This is the remaining
bug, and it is narrow: **the city runs, animations play, and the date stays at
1900 JAN.**

## A correction worth keeping

The first version of this document concluded that the city started and then
froze with a corrupted screen. **That was wrong**, and the reason it was wrong
is the most useful thing in it.

The evidence was a savestate taken from a running city: load it headlessly and
one byte of WRAM moves in 3600 frames, on a black screen with scattered tiles.
The inference drawn was that the game corrupts itself while creating the city.

But the owner reports the live game does not freeze — the animations keep
running. So the frozen state is **our savestate, not the game**, and every
conclusion drawn from it was about the wrong subject. Two things follow:

- The measurement was invalid. One byte in 3600 frames measures the state
  *load*, not the city.
- **Saving and loading a state from a running city is itself a bug**, separate
  and worth its own ticket. The garbled picture after a load says the state is
  incomplete.

The diagnostic detail gathered along the way still stands as exclusions, and
they are worth having:

- NMI is delivered every frame and `pre-nmi` / `post-nmi` return with the same
  stack pointer and the same resume PC, so the interrupt path is clean.
- Every AOT entry in `src/gen/bank00_v2.c` falls back to authoritative LLE, so
  bank 0 is faithfully interpreted and the T057 class of bug is not in play.
- The resume PC oscillating through `$0084E1-$0084FA` is a 16-bit jump table,
  not a loop. A jump table is not a bug.

## Measuring it properly

`scripts/clock-probe-live.sh` measures the **live** game: it takes two WRAM
dumps by frame number while the city is on screen, plus a screenshot, and
diffs them. No savestate, nothing to trust but the game itself.

```bash
scripts/clock-probe-live.sh                 # dumps at frames 8000 and 14000
scripts/clock-probe-live.sh "$ROM" 12000 20000
```

The screenshot matters as much as the diff: the dumps are taken by frame
number, and if the run was not in a city when they landed, the picture says so
instead of the table quietly explaining a wrong moment.

## What the readings mean

- **Thousands of bytes moving** → the simulation is running and the clock is
  specifically stuck. The slow runs are then the tick candidates.
- **Dozens of bytes moving** → not a live city; the screenshot will show which
  screen it actually was.

The 27-byte baseline is the number to keep in mind: a still title screen, a
still naming screen and a broken state load all move about that much in 3600
frames. A still screen and a dead clock are indistinguishable by byte counts
alone, which is why the probe also screenshots and judges.

## Where the month probably is

Unmeasured, so stated as a guess and nothing more: SimCity advances time from
its NMI handler, once per vblank, against a frame counter. NMI is delivered and
returns clean, so if the month does not move, the handler is running and
deciding not to — gated on a counter, a speed setting, or a paused flag the
port does not carry. That is a question about a specific value, and the live
diff is what would name it.

---

## Two savestate bugs, one fixed

Both were found by measuring rather than by reading, and both had the same
shape: something the snapshot does not carry, which is silent until you look.

### 1. Nothing carried the resume point — fixed

`g_resume_pc` is a static in `src/game_rtl.c`. It is not in the `Snes`, and it is
**not in the CPU either** — logging the restored register right after a load
shows `pc=0000`, so `snes_saveload` does not restore the program counter at all.

So a state restored into a fresh process leaves the resume point at 0, the frame
loop reads `booting = (g_resume_pc == 0)` as true, and the guest is answered
with the reset vector. Measured: resuming at `$008000` — the reset vector — with
`S=01FF` and `DP=0000`, boot-time register values, drawn over WRAM that still
held the city.

The fix puts it in the game's own chunk, which is what `state_save_extra` and
`state_load_extra` exist for. The state grows by exactly four bytes, and the
cross-check is the good part: a state saved on the naming screen now reports
`state loaded: resume PC restored as $009311`, and `$009311` is the address this
project already carries in `recomp/bank00.cfg` as
`force_lle 0x009311  # VBlank wait loop main polling address`. The resume point is
the game's vblank wait, which is where it should be.

A state written before this fix has no chunk and cannot be resumed; it falls
back to the reset vector. Those files are not recoverable, which is why the
city state has to be taken again.

### 2. The snapshot does not carry the render state — open

With the resume point fixed, a loaded state still draws the wrong picture.
Capturing the frame one frame before a save and the frame after the load, both
from the naming screen:

| | CRC |
|---|---|
| before the save | `a3bc4b3bf5` (the naming screen) |
| after the load | `90a7a5ffbd` (not it) |

So VRAM, OAM, CGRAM or the PPU registers are not in the blob, or the load resets
the PPU. This is why a loaded state looked like corruption in the first place,
and it is a separate bug from the resume point.

**It is also the good news for measuring the clock.** The simulation lives in
WRAM, WRAM *is* in the snapshot, and the clock advances in WRAM. So a loaded
city state runs the real simulation with a wrong picture, and the picture is not
what the clock is being measured from.
