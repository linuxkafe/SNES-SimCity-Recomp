# T060 — the city starts, and then stops

Entry into the game is solved (`RE_SCENARIO_NAV.md`). This is the next bug: a
city that starts and freezes. A savestate taken from inside the city is the
artefact, and it is reproducible.

## What the state measures

| measurement | value | reading |
|---|---|---|
| WRAM moved between frame 200 and 3800 | **1 byte** | nothing progresses |
| the same, on a still title screen | 27 bytes | 27 is the "frozen" baseline |
| the same, on the still naming screen | 27 bytes | same |
| NMI delivered | 1 per frame, 67 of 67, no gaps | not the interrupt |
| `pre-nmi` vs `post-nmi` | same `S`, same `resume` | the handler returns clean |
| slices per frame | **1** | the guest does very little |
| the picture | black with scattered tiles | **corruption**, not a paused city |

27 bytes is worth keeping in mind: three completely different still screens all
move 27 bytes in 3600 frames. A still screen and a dead clock are
indistinguishable by byte counts, which is why `clock-probe.sh` also
screenshots and judges.

## Where it is

The NMI resume PC is `$00F8E4`:

```
$00F8E4  9F 00 02 7F   STA $7F:0200
$00F8E7  60            RTS
```

That is the game's **out-of-bank call stub**: it writes the return value to
`$7F:0200` and returns. Every frame the guest calls into bank 0 through it, and
nothing in there changes the world.

The resume PC oscillating through `$0084E1-$0084FA` is **not** the bug. Those
bytes are a 16-bit jump table (`$000F, $026C, $026D, $0270, $0271, $027C,
$027D`), and slice boundaries land inside it. A jump table is not a loop.

## The hypothesis that closed

The obvious guess, given T057, was defective AOT in bank 0 holding the tick.
**It is closed.** Every AOT entry in `src/gen/bank00_v2.c` falls back to
authoritative LLE:

```c
_r = interp_tier_run_call_frame(cpu, 0x00930du, 0x00821eu, 2, NULL);
  /* exact M1X1 -> authoritative LLE */
```

So bank 0 is faithfully interpreted. **The guest is not frozen by the
recompilation. It is genuinely stopped, with a corrupted screen, and the
corruption predates the save.**

## What that leaves

The game starts, and somewhere between "started" and "froze" the state
corrupts: the screen becomes garbage and the simulation stops. Since bank 0 is
LLE and NMI is delivered, the corruption is not on the execution path — it is
in the **data**: DMA, VRAM, or a structure the game builds while creating the
city.

## Next

Bisect the transition. Save a state **the moment the city appears**, before it
freezes:

- a fresh state progresses → bisect forward to the first freeze;
- a fresh state is already frozen → the corruption is in city creation, and the
  bug is a data write, not the clock.
