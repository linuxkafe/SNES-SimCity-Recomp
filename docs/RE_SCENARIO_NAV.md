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

---

## 2026-09-28 — the short path, and what is still blocked

Re-derived while investigating T058. **Everything below was measured on the
current build; the table above is unchanged and still describes the scenario
route, which also works.**

### The short route to the city-name screen

Six menus and a briefing are not needed to reach it. From a cold boot:

| step | input | result |
|---|---|---|
| 1 | `press start` | main menu: PRACTICE, **START NEW CITY**, SELECT SCENARIO |
| 2 | `press down` | START NEW CITY highlighted |
| 3 | `press b` | **MAP SELECT**, with the map already drawn — NEXT / OK, "000-999", "No 0.00" |
| 4 | `press b` ×12, ~80 frames apart | **"Enter name of the city"** with the on-screen keyboard |

The d-pad does not move the cursor on MAP SELECT, which is why step 4 is a
run of `b` presses rather than a direction. Each `b` on the name screen appends
the character under the cursor — eight presses produce `11111111` — so **`b`
selects a key; it does not confirm.** The cursor starts on `1` and `left`
wraps it to the end of the list, so the list is linear and walks.

### What is still blocked, precisely

**Confirming `ENT` is still not reachable from a script.** A sweep of every
button on that screen — `b start x y l r a`, one run each, measured by WRAM
diff — produced 8 changed bytes per button, which is the idle noise floor, and
`a` with the cursor on a letter key wrote nothing. The d-pad moves the cursor;
the buttons select characters. The final press, on `ENT`, does not leave the
screen.

`T042` measured that SimCity is **auto-read-only** — `SNESRECOMP_PAD_PROBE=1`
gives `p0 reads=0 maxshift=0 auto=440`, so the automatic read is its only
input path, with no serial read at all. `SNESRECOMP_JOYPAD_READ_LOG=<n>` prints
the host state, the reversed word and both served bytes for automatic reads
that happen while a button is held, which is how the translation was checked.

**Attempted and reverted:** serving the automatic read without the bit reversal
looked right by construction and was measured to be wrong — with it, no input
reaches the game at all (0 of 2200 frames diverge from an idle run, against
1946 with the original mapping). The original mapping and the two tests that
pin it are correct. See `aes/tickets/T058-city-clock-does-not-advance.md`.

### Why this is worth trying by hand

The name screen's cursor is a **hand**, and the guest cursor is driven by the
SNES Mouse on player 2 with the host pointer standing in for it
(`SNESRECOMP_SOFT_MOUSE`). A headless run has no pointer to move, so the
scripted route can only walk the cursor with d-pad pulses, and the one press
that matters never lands. **In a window, moving the mouse onto `ENT` and
clicking is the one thing a script cannot do.**

That is the next experiment, and it is a hand experiment: start the game, get
to START NEW CITY, confirm the map, and on the name screen move the pointer
onto `ENT` and click. If the city starts, the date advances and the population
grows, then the remaining work is to reproduce the click headlessly — or to
establish that this screen genuinely needs the mouse and document that.

---

## 2026-09-28 — the loop, and the mouse boundary

Re-measured with the script actually loading. See T058 for the full table.

**The game never reaches a playable city. It loops between the naming screen
and the logo screen.** `a` on the last key leaves the naming screen for the
logo; `start` or `b` on the logo returns to the naming screen. 11000 frames
with no exception, trap or error — the game chooses this path. The naming
screen itself is stable and does not time out: frames 1800 through 3000 are
byte-identical.

**On the naming screen, navigating and typing are disconnected.** `right`×13
puts the hand on `S`, so the d-pad navigates; `a` on `S` still writes `1`. What
chooses the character is the mouse. `left`×5 equals `left`×1, so the key list
wraps to the end and stops rather than returning to the start.

**The mouse is served and is the boundary.** `SNESRECOMP_MOUSE=1` logs *"SNES
Mouse enabled on port 2"* and the probe serves `p1w=0001` — present, no buttons,
position 0,0. The implementation is relative, while an SNES Mouse is absolute,
and headless has no pointer to move.

**The Steam Deck answers the geometry but not the click.** No toolchain there,
so the binary is built here and copied — the SDL3 is static and the ELF needs
only six system libraries. `DISPLAY=:1` is the 1280x800 screen; the window is
1008x672 centred, so **field(x, y) maps to screen(136 + 3x, 64 + 3y)**, which
the host's own `window created: 1008x672` confirms. `xdotool` and
`ffmpeg -f x11grab` both work. But SDL3 presents nothing visible on `:1` and
`xdotool` clicks have no measurable effect: the final frame is byte-identical
with and without them. Moving the pointer in 3 px steps does move the hand, so
the soft mouse works and the real mouse does not arrive. The Deck is also not
reproducible — the naming screen appears two `b` presses later there, which
moves the cursor and makes `left`/`a` land in the wrong place.

---

## The route that works (2026-09-28) — confirmed by hand

This one starts a city. Everything above it is either a different destination
or a dead end.

```bash
SNESRECOMP_MOUSE=1 ./build/SimCitySNESRecomp "$PWD/SimCity (USA).sfc"
```

| step | input | lands on |
|---|---|---|
| 1 | `START` | leaves the attract |
| 2 | menu → **START NEW CITY** | |
| 3 | **B** | MAP SELECT, map already drawn |
| 4 | **B** ×12, ~80 frames apart | "Enter name of the city" |
| 5 | **move the mouse**, **right-click** a letter | the letter goes in the name |
| 6 | **click `ENT`** with the mouse | **the city starts** |

**Step 5 and 6 need a pointer, and the pointer needs the soft mouse.** Step 4
and 5 are the same mechanism: the soft mouse turns pointer motion into d-pad
pulses (4 px per press, 2-frame pulse), the guest's own d-pad moves its cursor
across the keyboard grid, and the shim reads the right button as B, which the
guest consumes as "select the key under the cursor".

The real SNES Mouse on port 2 is **not** what moves this cursor. It feeds the
guest's 7-bit counters, and every scripted run that tried to use it for this
screen did nothing. This is worth stating because the two look interchangeable
and only one of them works.

### What is scripted and what is not

`--script` can do steps 1-4: the d-pad works headlessly and the naming screen
is reached deterministically. **Steps 5-6 cannot**, because a headless run has
no pointer for the shim to read, so it never pulses. That is the whole reason
T058 looked like a game-flow loop for hours: measured headlessly, `A` and `B`
left the naming screen and the game returned to the logo, which reads exactly
like the game refusing to start a city. It was refusing nothing. The button
that selects a letter was never pressed, because the thing that presses it is
a mouse.

So the honest summary of T058: **the clock did not advance because the city was
never created, and the city was never created because the last two inputs are
pointer inputs that no script can make.** Not a hang, not a PPU bug, not the
automatic read, not the widescreen margins.

### Making the pointer scriptable

Steps 5-6 above are pointer inputs, and until now no script could produce
them — which is why the route existed only as prose. The harness now has two
verbs for it:

```text
mousemove <dx> <dy> [frames]   move the pointer; the soft mouse turns it into
                               the d-pad pulses a hand would produce
mouseclick <l|r|lr> [frames]   hold a mouse button
```

`mousemove` is an **impulse, not a stream**: the delta lands on the first frame
and zero on the rest, so the shim's own threshold (4 px per press) and pulse
queue decide what the motion is worth. One `mousemove 40 0 6` is **one**
right press held four frames, not ten — measured, not assumed. To walk N grid
steps, repeat it N times.

Verified headlessly: the CRCs for `mousemove` and the equivalent `press` land
on the same naming-screen states, and `mouseclick right` reproduces the select
the right mouse button performs in the working route.

What this does not yet do is reach the city: the pointer coordinates of `ENT`
are only known from a hand, because a headless run has no pointer to read them
off. Once they are, the whole route becomes a `--script` and the clock can be
gated instead of measured by hand.

### Measuring the clock, which is a different bug

The city starts and the date stays at 1900 JAN. Those are two bugs: the entry
one is closed, the clock one is open, and the clock can only be measured with a
city already running — which needs the mouse.

`scripts/clock-probe.sh` closes that loop. Save a state from inside a live city
(press **F11**, slot 1) and it loads the state, runs 3600 frames, takes two
WRAM dumps and reports which bytes move slowly:

```
scripts/clock-probe.sh
```

Nobody has mapped the date, population or treasury in WRAM, and guessing
addresses by hand in 128 KB is hopeless. But a running city with a dead clock
has a signature: the slow state is what changes every few dozen frames rather
than every frame. `scripts/wram-diff.py` does the grouping and decodes each
candidate as a byte, a u16 and a u32, because adjacent counters merge into one
run and a 4-byte and a 2-byte field side by side arrive as a 6-byte run that a
per-width decode would never explain.

It is plumbing, not a conclusion: the run above used a state from the title
screen, so it was reading the attract animation. What it proves is that the
measurement is one command away from a real state, and that the clock can
become a gate in `make test-rom` instead of a thing a person has to watch.

### The measurement, and the thing it nearly fooled us about

`clock-probe.sh` now screenshots the state it loaded, and says plainly when the
state is not a running city.

That verdict exists because of how this first went. The first real run reported
27 changed bytes, and 27 looked like a plausible "the slow state is all that
moved". It is not plausible: a live city moves population, treasury, agents and
tiles, and would change thousands. Checking it against states that are
*definitely* not cities showed the trap — **the title screen, the naming screen
and a running city all move about 27 bytes in 3600 frames.** A still screen and
a dead clock are indistinguishable by byte counts, so a probe that only prints a
table will confidently explain a state that never contained a city.

So `wram-diff.py --city` judges, and the probe screenshots:

- **under 1000 bytes moved** → not a running city. Look at the picture.
- **thousands** → a running city; the slow runs are the tick candidates.

Verified on both sides: a state saved on the naming screen loads, runs and
screenshots as the naming screen, so the load is faithful and the probe is
measuring what it claims to.
