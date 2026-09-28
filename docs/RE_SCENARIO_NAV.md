# Scripted input: reaching a SimCity scenario headlessly

This is the input chain that drives a cold boot into a named scenario, measured
step by step against this ROM. Every row was confirmed by a screenshot, not
inferred. It exists because re-deriving it cost several hours, and because the
two errors below are not guessable from the code.

Run it with:

```sh
SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy \
SNESRECOMP_WRAM_DUMP=/tmp/bc.bin SNESRECOMP_WRAM_DUMP_AT=3000,9000,14000 \
SNESRECOMP_SCREENSHOT=/tmp/bc.ppm SNESRECOMP_SCREENSHOT_FRAME=9000 \
  ./build/SimCitySNESRecomp --config <fast.ini> --script <script> "<rom>"
```

## The chain

| # | script | lands on |
|---|---|---|
| 1 | `wait 250` / `press start 3` | main menu: PRACTICE, START NEW CITY, SELECT SCENARIO |
| 2 | `press down 4` twice | SELECT SCENARIO highlighted |
| 3 | `press b 4` | the scenario list |
| 4 | `press up 4` | the 3x2 city grid, selection shown by a green border |
| 5 | `press right 4` / `press up 4` | **Bern Traffic 1965** highlighted |
| 6 | `press b 4` | the scenario briefing |
| 7 | `press b 5` x14 | the MAP SELECT screen |
| 8 | `press down 4` twice, then `press b 5` ten times | **"Enter name of the city"** with an on-screen keyboard |
| 9 | `press right` x10, `press down` x3 | the hand sits **exactly on `ENT`** (verified by screenshot) |
| 10 | *(BLOCKED)* | confirming `ENT` -> the running city |

## Two things that are not guessable

**`A` does not confirm a menu; `B` does.** A sweep of all eleven controller
buttons over WRAM found `B` at 94 686 changed bytes and `A` at exactly 0 - the
same as eight other buttons, which is what a clean control looks like.

**Three `down` presses from PRACTICE wrap around** to PRACTICE. Two reach
SELECT SCENARIO. This cost two runs: the third press silently undid the first
two, and the run entered a new city instead of a scenario, which looks like the
navigation failing rather than like a wrap.

**A single `b` does not leave MAP SELECT; ten do.** One press appears to do
nothing, and the screen is unchanged, so it reads as "the button is not
activated" rather than as "one press is not enough".

**The d-pad does not move the hand on MAP SELECT.** `press right` there changes
nothing at all - the hand stays on `OK`. The hand only becomes d-pad-navigable
on the name-entry keyboard, where `right` moves it along a row **and wraps**
(twelve presses returned it to the left edge).

**`loadstate` does not restore a mid-flow state, so it is not a shortcut past
this screen.** A `savestate 0` written on the naming screen restores to the
**title screen** - the load reports success and the screen is nevertheless the
attract loop. `GameReset()` is not the cause: SimCity does not define
`on_reset`, so that call does nothing game-specific. Whatever the state
directory misses, it is enough to send a mid-flow frame back to the title. This
is recorded because the natural next idea - "just save past the screen" - is
already refuted, and re-testing it costs a five-minute run.

`host_main.c` gained a `savestate N` script command to go with the existing
`loadstate N`, which had been able to restore states a script could not create.
It is worth keeping even though the shortcut failed: reaching this screen costs
~8000 frames, and the next experiments should be able to start from a nearby
point.

**Confirming `ENT` on the name keyboard is the unsolved step.** Measured and
rejected: `b` once, `a`, `b` three times, and `start` all leave the screen
unchanged. The hand ends up on `SPACE` after the first `b`, which suggests `b`
is reaching the keyboard and doing something other than confirming - possibly
inserting the default placeholder. Whatever confirms it is not in {A, B, START}.

## Why a scenario, not a new city

A new city at population 0 is **inert**: over 3400 frames of simulation only
**35 bytes** of the 131 072 change, and all of them are clocks. No population,
no buildings, no tax, no traffic - so there is nothing for a differential to
find. A scenario arrives with a developed city, which is what makes the
traffic and pollution values both large and moving.

Bern Traffic 1965 is the useful one: its own briefing says *"low average
traffic density"*, so the value is simultaneously large, varying, and shown on
the HUD - exactly the three properties the address hunt needs.

## Locating a value: differential, not search

Searching WRAM for the number the HUD displays **finds coincidences**. `$25D1`
held exactly 19999, the treasury, and `pokefor $25D1 1234` did not move the
number on screen. The working method is:

1. dump WRAM at two frames where the value has **moved**;
2. diff - the address that changed to a new, plausible value is the candidate;
3. confirm by `pokefor`ing a distinctive value and **looking at the screen**.

Step 3 is not optional. Step 2 alone produced a false positive, and a wrong
address is not a cheat that does nothing - it is a cheat that overwrites
whatever the game keeps there.

## Cost

About 40 fps with frame-delay pacing; `DisableFrameDelay = 1` does not help
much because audio pacing dominates, and `EnableAudio = 0` is what actually
buys speed. A scenario needs roughly 6000 frames to settle, so budget two to
three minutes per run and run candidates four at a time - twelve at once
starved each process and none finished.

## The VBlank handshake, both ends proven

Found while working out whether the hot `$009311` could be recompiled. It cannot,
and now there is a reason rather than a comment.

The flag the spin waits on is **`$B9`, a direct-page byte**, and the NMI handler
increments it. `INC dp $B9` occurs **exactly once in the ROM**, at `$0080BC`,
which is ten bytes inside `func NMI_Handler 0x80B2`:

```
NMI_Handler $80B2
  $80B2  78           SEI
  $80B3  E2 20        SEP #$20
  $80B5  48           PHA
  $80B6  AF B1 00 00  LDA $00B1,X
  $80BA  30 04        BIT $04
  $80BC  E6 B9        INC $B9      <- the writer
  $80BE  68           PLA
  $80BF  40           RTI          <- fast path
```

and the consumer:

```
main loop  $930F  STZ $B9      ; clear the flag
           $9311  INC $C7      ; count spin iterations
           $9313  LDA $B9
           $9315  BEQ $9311    ; wait for the next VBlank
           $9317  RTS
```

A debugger watch on `$7E:00B9` confirms the consuming side: it goes to `0x01`
once per frame.

**That one fact explains all three observations:**

- `$9311` dominating execution is the game spending the frame waiting for the
  next VBlank - not doing work there.
- `force_lle 0x009311` is correct because the NMI must be delivered *during* the
  spin, and delivery happens at AOT call boundaries; this spin **is** one of
  those boundaries. A native spin never returns.
- The watchdog at frame 2607 is `$C7` overflowing.

It also settles the shape of the fix: "wait for the next VBlank" is what Super
Metroid solves with `WaitForNMI`, named in `LLE_SCHEDULER.md` as the analogous
seam. The overlay would wait for the VBlank, deliver the pending NMI, and
return.

**How it was found, and one correction.** The debugger's write-watchpoint on
`$B9` reports `pc24=0x000000` - it does not record the writing PC for interpreted
code - so the writer came from scanning the ROM for stores to that direct-page
byte. And I earlier reported "no return address anywhere in low WRAM" as a
finding; that was my search being wrong. The watch reported `S=0x1FDF` and I had
only read `$0000-$0FFF`, so the stack was a kilobyte past the end of my window.

## Running the debugger

The TCP debug server is compiled out by default: `debug_server.h` turns every
entry point into a `static inline` no-op unless `SNESRECOMP_TRACE` is 1, so
configure with `-DSNESRECOMP_ENABLE_TRACE=ON`. On a machine without XTEST that
also needs `-DSDL_X11_XTEST=OFF` or SDL3's configure step fails.

Useful once it is up - `wram_watch_log_get` records the frame and the full
register set at each write, but **not** the writing PC:

    set_wram_watch 7E 00B9 1 0 00 1
    wram_watch_log_get b9 0 3000 16
