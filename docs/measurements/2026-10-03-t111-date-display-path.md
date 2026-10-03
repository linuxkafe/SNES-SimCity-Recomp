# Measurement — T111: the rendered date IS live. `$0B51` is the wrong probe, and the display path is not separately broken

**Deck-native** (`ssh deck@steamdeck`), repo `/home/deck/simcity`, ROM
`/home/deck/rom/SimCity (USA).sfc` md5 `23715fc7ef700b3999384d5be20f4db5`.
Runs on `scripts/d_city.script` (identical input schedule in all four runs; the
poke entries follow the last press and cannot move it).

## The question, and the answer

**Is C-006 one bug or two?** The brief framed it as: poke `$0B51`, and either
the date moves (one bug) or it does not (two bugs, because fixing the counter
alone will not make the gate green).

> ## **The date on screen DOES move. C-006 is ONE bug.**
> ## **But not because of `$0B51`. `$0B51` has no effect on the display at all.**
> ## **The rendered date is a live, direct function of `$0B53` (year) and `$0B55` (month).**

Measured, and the two halves are separately attributable:

| poke, inside the live window | WRAM after | rendered date | pixels changed |
|---|---|---|---|
| **none (control)** | `$0B53=076C $0B55=0001` | **`1900 JAN`** | — |
| **`$0B51`/`$0B52` → `$0100`** | `$0B51=0100` | **`1900 JAN`** | **0 — all 151 frames bit-identical** |
| **`$0B53`/`$0B54` → `$0FA0`** | `$0B53=0FA0` | **`1952 JAN`** | **36 px, x 73-88, y 12-19** |
| **`$0B55`/`$0B56` → `$0005`** | `$0B55=0005` | **`1900 MAY`** | **100 px, x 97-120, y 12-19** |

The pictures, rendered from the captured frames, HUD strip only:

```
  control      1900 JAN        <- no poke
  $0B53 poked  1952 JAN        <- year word
  $0B55 poked  1900 MAY        <- month word
```

`x 73-88` is the **year's last two digits** and `x 97-120` is the **month
name** — the character cells measured in T110 §2 (8-px cells at `x = 58 + 8i`,
text row `y12-y18`). **The poke moved exactly the cells the word names, and
nothing else on the screen.**

## Why the brief's probe could never have worked

The brief's premise was that `$0B51` "is a different variable, and it is the one
the game actually increments", so it should be the one the display follows.
**That premise is refuted.** The display never read `$0B51`:

- In the reference, `$0B51 = 4 × (months elapsed) + quarter` is the **counter**
  (T101, 113 write events, 0 violations).
- The **HUD** reads the **derived** words `$0B53` (absolute year) and `$0B55`
  (month), which something downstream computes from the counter.
- So poking the counter moves nothing on screen **even in a build that works**.
  The experiment was mis-targeted before it was run, and this is now measured
  rather than assumed.

**What this does to the plan:** the remaining work is *why `$0B53`/`$0B55` are
never updated*, and `$0B51` not incrementing is the upstream cause of that. The
mechanical thread is unchanged and still open — `$00:804D` reads `$0012` and is
never executed again after f3271, and `$03:D2AA` sets `$0012 = 1` one
instruction before bank `$03`'s final `RTL`. **What is supposed to execute
`$00:804D`, and why it stops, is NOT ESTABLISHED.**

## ⚠️ RETRACTED (R-045): "no poke can move the date glyphs"

Two tracked claims are refuted by this, and both were reasonable given what was
measurable at the time.

**T107**, on a poke of `$0FA0` into the year field at f4260:

> *"a poke of `$0FA0` into the year field at f4260 left the picture unchanged"*

used to argue that the rendered date does not come from `$0B53`. **The
conclusion is false; the observation was void.** Every one of T107's pokes
landed after f4025, and T109 measured that the picture's last change anywhere is
**f3381** — so T107 poked a screen that had been bit-identical for 640 frames and
could not have reported a change. T109 retracted the *reasoning*; this
measurement retracts the *claim it was reaching for*.

**T109 §6**:

> *"The date glyphs were uploaded during city creation and are thereafter never
> rewritten — which is exactly why no poke can move them, and it's a stronger
> statement than T107's."*

**False, and the interesting half is why.** The glyphs *do* change, within one
frame of the poke, while the screen is live:

```
  -- live4_year ($0B53 alone) --            -- live4_month ($0B55 alone) --
  65  34 px  bbox x 73-88 y 12-19            65  88 px  bbox x 97-120 y 12-19
  67  36 px  bbox x 73-88 y 12-19            67 100 px  bbox x 97-120 y 12-19
  ...  (identical for every captured frame to 150)
```

Frame 65 of the capture window is **f3365**: the city is on screen (from ~f3366),
and the picture is still changing on **every** frame until f3381. Both runs'
pictures stop changing at exactly the same captured index, 81 = **f3381**, which
independently reproduces T109's f3381 on a different build and a different route.

**The reconciliation with T109 §6 is its own finding, and it is not a
contradiction.** T109 measured that **the CPU never writes `$2119` (VMDATA) in
3 400 frames** and that **VRAM is filled by HDMA on channels 0 and 1**. Those
two facts are correct and they are the *mechanism*: the date tiles are rewritten
by **HDMA**, which is why no `$2119` write appears and why a poke still moves
them. T109 inferred from the absent `$2119` writes that nothing redraws the
glyphs. **The inference was wrong; the measurement it rested on was right.**

> **A register-write census bounds what the CPU did. It says nothing about what
> HDMA did, and "the CPU never wrote VMDATA" is not "VRAM never changed".**

## Method, and the two measurement traps this had to get past

Both are recorded because both produced a clean, confident, **wrong** answer
before they were caught, and both are the same trap this project has already
fallen into twice.

**Trap 1 — poking a frozen screen answers nothing.** The first attempt poked at
~f4220 and reported "all 501 frames bit-identical". That is exactly T107's
error. The screen had been bit-identical since f3381, so the result carried no
information about the display. **The fix is structural, not clever:** the poke
is placed inside the live window and compared **frame by frame against an
identical no-poke run**, because the brightness ramp is deterministic and
identical in both, so any per-frame difference is attributable to the poke.

**Trap 2 — the script's own arithmetic is not the frame number.** Summing the
route's `wait`/`press` counts put the poke at f3360. It landed at **f3396**, 36
frames late — *after* the freeze — and the run again answered nothing. Measured
with a WRAM-dump calibration sweep:

```
  script .t110_poke_live.script  (poke 0B51 at ESTIMATED f3360)
    f3380  $0B51=$0000  $0B53=$076C
    f3390  $0B51=$0000  $0B53=$076C
    f3400  $0B51=$0100  $0B53=$076C      <- landed here
```

The poke was then moved back 30 frames of script time and re-measured:

```
    f3360  $0B51=$0000  $0B53=$076C
    f3370  $0B51=$0100  $0B53=$076C      <- now inside the live window
    f3390  $0B51=$0100  $0B53=$076C
    f3400  $0B51=$0100  $0B53=$076C
```

**A `pokefor` with a 900-frame hold was tried first and is the wrong tool:** it
delays the script schedule by its hold, the route missed the city entirely
(`$0B53 = $0000` at f3300–f3450), and the run was void. Every frame reported
here is from a **single-frame** poke, verified by WRAM dump to be present during
the window it is claimed for.

### `COUNT_PC` was NOT available as a positive control on these runs

`SNESRECOMP_COUNT_PC` is read in `interp816.c:323` — **interpreter tier only** —
and this is an AOT build, so it prints nothing at all. Stated rather than
implied. **The control these runs carry instead is the WRAM dump.** [MEASURED,
Deck-native] It shows, for each run, that the city was loaded (`$0B53 = 076C`
in every control sample), that the poke landed, and that the poked value was
still present at the frames the claim is made for — which is precisely the
thing the first two attempts got wrong.

## Runs

**The three committed routes reproduce the result exactly**, re-run from the
tracked files after they were written:

| run | script | frames | `EXIT` | completion | screenshots |
|---|---|---|---|---|---|
| control | `scripts/d_city.script` | 3 500 | **0** | `exit: RUN_FRAMES reached after 3500 frames` | 151, f3300–f3450 |
| `$0B51` | `scripts/t110_diag_poke_0B51.script` | 3 500 | **0** | same | 151 |
| year | `scripts/t110_diag_poke_year.script` | 3 500 | **0** | same | 151 |
| month | `scripts/t110_diag_poke_month.script` | 3 500 | **0** | same | 151 |

```
  -- repro_$0B51 --            -- repro_year --              -- repro_month --
  f3360  $0B51=$0000           f3360  $0B53=$07A0            f3360  $0B55=$0005
  f3370  $0B51=$0100           f3370  $0B53=$0FA0            f3370  $0B55=$0005
  f3390  $0B51=$0100           f3390  $0B53=$0FA0            f3390  $0B55=$0005

  $0B51: (none - all 151 captured frames bit-identical to the no-poke control)
  year : 65  34 px  bbox x 73-88 y 12-19  ... 36 px from f3367 on
  month: 65  88 px  bbox x 97-120 y 12-19 ... 100 px from f3367 on
```

Two discarded attempts are recorded above rather than deleted: the f4220 poke on
a frozen screen, and the calibration run that found the 36-frame offset. Plus a
`pokefor`-with-hold attempt that delayed the route so far that no city loaded at
all (`$0B53 = $0000` throughout) and was void.

## The pokes are DIAGNOSTICS, stated here and in the scripts

> **This is a diagnostic. It is not a fix.** It exists to answer one question
> about the display path. **It must never make `make clock` pass** — DoD Rule 0b
> — and `scripts/check-cheat-gate.sh` is green and re-run after the scripts were
> added. `make clock` was run before and after this ticket and is **red both
> times**, with `$0B53 = 076C → year 1900` proving a city was loaded and
> `1 distinct date image after f3600` proving the date did not move. The gate
> script itself has no write capability and never did.

## What is NOT claimed

- **Not why `$0B53`/`$0B55` stop being updated.** That is C-006 and it is
  **OPEN**. This measurement removes one candidate explanation (a broken display
  path) and leaves the rest standing.
- **Not the year encoding.** `$0B53 = $0FA0` renders `1952`, not `4000`. The
  renderer clearly transforms the stored word before drawing it, and **how is
  NOT ESTABLISHED**. It does not matter to this question — the display tracks
  the word — but it is a real open thread and it is recorded rather than
  glossed.
- **Not that HDMA is the mechanism.** T109's HDMA finding is consistent with
  what is measured here and is **not** re-measured. "The tiles change without a
  `$2119` write and the CPU never writes `$2119`" is measured; "HDMA did it" is
  inference, and it stays labelled as such.
- **Not that `$0B51` is irrelevant.** It is the counter the reference advances
  and ours does not. It simply is not what the HUD reads.
