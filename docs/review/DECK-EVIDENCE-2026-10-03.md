# Deck Evidence — the four rubric criteria that need the Deck

**Date:** 2026-10-03 · **Machine:** Steam Deck (`ssh deck@steamdeck`), aarch64,
Zen 2 4c/8t · **Repo:** `/home/deck/simcity` at `cfc7a99` (rsync of the host
tree; `README.md` md5 `3203805b8d61a9ea5d787a6ee63a8034` identical on both
machines) · **ROM:** `/home/deck/rom/SimCity (USA).sfc`, md5
`23715fc7ef700b3999384d5be20f4db5`, 524 288 bytes.

## Why this file exists

The AES chain's phase 6 (`aes-peer-review`, `fc5a512`) returned **REVIEW
INVALID**, and one of its four cited grounds was that **4 of 23 criteria could
not be evaluated** because they require `make perf`, `make clock`, or Deck
execution.

**That was caused by an instruction I gave, not by the project.** The brief for
that phase said, verbatim: *"do NOT run Deck measurements... do NOT run `make
clock` or `make perf`."* I turned a context optimisation into a constraint, and
it landed on the one phase that needed the hardware.

**This file closes that gap by measurement.** The four criteria are evaluated
below **on the Deck**, so the remaining grounds for INVALID are now only the
ones that are genuinely about reviewer independence.

G-04 exists precisely to stop what happened: *"Heavy work runs on the Deck,
not the dev host — measurement reports name the machine they ran on."* The
phase-6 run satisfied G-04 only vacuously, by having nothing heavy to run.

## The criteria

### G-01 — `make test`, `make test-rom`, `make perf` pass

**`make test` — PASS, 2/2**, 1.49 s.

> **The first run FAILED, and the failure is the guard working.** With no
> `SIMCITY_ROM` exported, `test_deterministic_replay` printed `SKIP: no ROM
> found` and **ctest reported it as a failure** — exit 2, `make test` Error 8.
> That is review finding R-01 behaving as designed: the test used to `return 0`
> on a skip, `CMakeLists.txt` sets no `SKIP_RETURN_CODE`, and ctest rendered the
> skip as *Passed*. **A gate that reports green on work it did not do is worse
> than one that is red**, because the green is what the next session starts
> from.
>
> **Note the environment asymmetry, because it will bite again:**
> `make test` reads **`SIMCITY_ROM`**; `make test-rom` and `make perf` read
> **`ROM=`**. All three were run with the ROM supplied. This is CONF-22's shape
> — *five knobs, four conventions* — recurring in the gate environment.

**`make test-rom` — PASS**: 800 presents, **257 distinct crc32**, peak luma
41.751. *("the emulated picture moves")*

**`make perf` — PASS, SOLO.** Load average **0.39 / 0.25 / 0.15** after a 45 s
rest; the first attempt was abandoned because load read **0.78** — caused by my
own rsync and build, which is precisely why the gate is solo-only.

| run | fps | guest ms/frame |
|---|---|---|
| 1 | 56.88 | 4.525 |
| 2 | 56.88 | 4.528 |
| 3 | 56.88 | 4.509 |
| 4 | 56.88 | 4.514 |
| 5 | 56.88 | 4.538 |

**median 56.88 fps, spread 0.0%** (limit 10%), threshold 50. The gate prints its
own limit honestly: this is a gross-regression floor and a "does it still run"
check, **not** a headroom figure, because on a vsync-capped build the fps number
cannot move. The headroom figure is `guest` ms/frame against the 16.67 budget.

### G-02 — `make clock` fails, and its message does not assert an unproven cause

**PASS — measured red on the Deck.**

```
CLOCK: FAIL - the date did not advance in a live city.
  need    : >= 2 distinct date images after f3600
  run 1/1 ... 1 distinct date images after f3600 (last change f3378 of 6000)
```

`scripts/clock-gate.sh` exits **1**; `make clock` exits **2** (GNU Make's code
for a failed recipe — read the script's, not make's).

The message asserts a **symptom**, not a cause: "the date did not advance in a
live city." It does not name `$03:8026`, a missing NMI, a decoder, or the
game. E-03 is satisfied.

The gate's own calibration paragraph is the strongest thing in it: the floor of
**2 distinct date images** is CALIBRATED against a **measured** reference —
**29 of them in 30 000 frames**, Deck-native, cold SRAM, real save, city at
f3000, month rolls every ~780 frames, year turns at f13080 and f24600, 1902 MAY
at f30000. **We produce 1.** The floor sits far below the reference's measured
behaviour, so the gate is not asking for something the original game does not
do.

### G-03 — the delivery gate is red and the status line says not deliverable

**PASS.** `README.md:237` — *"⚠️ **Not deliverable.** Our date stays `1900 JAN`
forever…"*. `README.md:1383` names `make clock` as **FAIL** with both exit
codes. `README.md:1478` — *"`make clock` is the one that matters and the one
that is red."*

### G-04 — heavy work runs on the Deck, not the dev host

**PASS, and no longer vacuously.** Every number in this file was produced on the
Deck. Build: `make build` exit 0, binary relinked 11:51.

## What this file does not settle

The review at `fc5a512` cited four grounds for INVALID. This file closes the
one that was my instruction. **Three remain, and none is closed by hardware:**

1. **No independent reviewers.** The multi-perspective fallback is a
   single-model fallback. G-04 cannot fix this and no amount of Deck time will.
2. **Self-authorship.** §4 of the protocol makes the author ineligible; these
   measurements were taken by the same agent chain that wrote the code.
3. **The rubric has no executor.** `RUBRIC.md` is exempted from every guard, so
   its own criterion I-02 — whose command is broken (`grep: Unmatched ( or
   \(`) — has never been run by anything. **Do not edit the rubric**; that
   breaks pre-registration. Register a new one.

**So the candidate remains CANDIDATE.** These four criteria are now *measured
rather than skipped*, which is the difference this project has been insisting
on since the first retraction.

## One incidental confirmation

The gate printed `last change f3378`. **CONF-24 says that number is off by one**
and is really f3379 — `SCREENSHOT_FROM=0`, `present_NNNNNN.ppm` names a window
index, and `clock-gate.sh:212` derives the frame from the filename while
`presents.csv` sits unread in the same directory.

CONF-24 was raised from a 40-frame run whose machine was not recorded. **It is
now reproduced on the Deck inside the full 6000-frame gate**, and the gate
still prints an off-by-one. The defect is in the gate's reporting, not in its
verdict — the verdict is FAIL either way — **but the number should not be quoted
as a frame**, and three documents currently quote `f3378`.

## The sixth `guest` reading

Deck `guest` ms/frame across all runs now:

    4.502 / 4.511 / 4.499 / 4.572 / 7.619 / 4.525 4.528 4.509 4.514 4.538

**Five cluster at ~4.50. T104's 7.619 stands alone.** The simultaneity noted at
T104 — high `guest` on all three work stages at once while fps stayed within
0.04% — remains **OPEN with no cause offered.** The cluster is now strong enough
that treating 7.619 as noise would be a claim, and it is not made.