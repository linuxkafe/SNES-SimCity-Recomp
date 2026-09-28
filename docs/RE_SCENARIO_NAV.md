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
