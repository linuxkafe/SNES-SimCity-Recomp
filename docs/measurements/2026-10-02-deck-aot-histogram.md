# Measurement — the whole-run AOT histogram, re-run on the Deck (2026-10-02)

**What this closes.** The 1 430 539-entry whole-run AOT histogram in
`2026-10-02-c041-bank03-pc-dump.md` §5 was labelled **host-only** and could not
close anything, because the machine that could have produced it was believed
unable to run the instrument. That belief rested on a cause now refuted
(ledger row **R-032**, CONF-9): the Deck's header prefix was said not to satisfy
a libstdc++ translation unit. It does. `scripts/deck-trace-build.sh` builds the
trace tier on the Deck and refuses to report success unless the instrument emits.

**So this page is the Deck-native re-run. Every number below was produced on
`steamdeck`.** The one host figure appears once, in the comparison table, and is
marked.

## The machine

Deck `steamdeck`, gcc 15.1.1, cmake 4.0.3, Zen 2, 8 threads, SDL3 3.4.10 built
from source, compiling natively against a hand-assembled glibc header prefix at
`/home/deck/sysroot` via `-idirafter` (503 of the 504 `/usr/include` paths pacman
claims are absent from disk). Trace tier: `-DSNESRECOMP_TRACE_BUILD=ON` plus
`-DSNESRECOMP_INTERP_PROFILE=1 -DSNESRECOMP_TRACE=1` on **both** `-DCMAKE_C_FLAGS`
and `-DCMAKE_CXX_FLAGS`. Headless, `SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy`,
`scripts/d_city.script`, `SNESRECOMP_AOTBLK=0-3700`, `SNESRECOMP_RUN_FRAMES=4000`,
`SNESRECOMP_TRACE=1` at run time.

```
$ scripts/deck-trace-build.sh          # from a clean build-tr
deck-trace-build: [aotblk] lines in f1-f50 = 9771
deck-trace-build: OK, trace tier links and emits on this machine.

$ env SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy \
    SNESRECOMP_RUN_FRAMES=4000 SNESRECOMP_TRACE=1 SNESRECOMP_AOTBLK=0-3700 \
    build-tr/SimCitySNESRecomp --script .../d_city.script .../SimCity (USA).sfc
[host +153.849s] exit: RUN_FRAMES reached after 4000 frames
video totals: simulations=4000 presentations=4000 seconds=153.530
```

## 1. The histogram reproduces

**DECK-NATIVE, 4000 frames, 1 430 540 `[aotblk]` entries.**

| | total | `$00` | `$01` | `$03` | other banks | distinct PCs |
|---|---|---|---|---|---|---|
| **Deck (this page)** | **1 430 540** | 1 100 383 | 330 139 | **18** | **0** | 84 |
| **host, same build, re-run for cross-check** | **1 430 540** | 1 100 383 | 330 139 | **18** | **0** | 84 |
| host-only figure in the previous page | 1 430 539 | 1 100 383 | 330 138 | 18 | 0 | — |

The Deck run and the host run agree on **every** derived figure — total, per-bank,
84 distinct PCs, the identical top-12 PC list, the identical bank-`$03` frame
split and PC list, and every per-100-frame bucket to the digit. **The AOT census
is deterministic and cross-machine reproducible.** So the previous page's
1 430 539 is not a Deck/host difference: **the host does not produce it either.**
It was a wrong number, and §2 shows the same paragraph got the bank-`$03` detail
wrong as well.

**This closes one open question.** "Bank 03 runs AOT nowhere" is now **MEASURED on
the Deck** for the window f0–f3700, and confirmed: **18** entries, all the
AOT-side presence bank 03 has in the whole run. The bound is in the claim and
stays in the claim — f0–f3700 is what `AOTBLK=0-3700` asks for. Banks 02, 04, 05,
06 and 07: **zero** AOT entries on both machines.

## 2. "All 18 in f3270" is REFUTED — three of them are in f3259

The previous page states the 18 bank-`$03` entries are "**all in f3270**", at
`$03B477`, `$03B491` (×7), `$03B49B`, `$03B4A0` (×4), `$03B4A9`. The Deck run
gives every one of them, and the list does not match:

| frame | distinct PCs | entries |
|---|---|---|
| **f3259** | `$03C42A`, `$03C430`, `$03C463` | **3** |
| f3270 | `$03B477` (×1), `$03B491` (×8), `$03B49B` (×1), `$03B4A0` (×4), `$03B4A9` (×1) | **15** |
| | **8 distinct PCs** | **18** |

Three separate things are wrong with the previous sentence, and the third was
already visible without any new measurement:

1. **15 are in f3270, 3 are in f3259.** Not all 18.
2. **`$03B491` occurs 8 times, not 7.**
3. **The list omits three PCs entirely**, and as written it sums to
   1 + 7 + 1 + 4 + 1 = **14**, against a stated total of **18**. The previous
   page contradicted itself by four.

The host's raw 63 MB AOT log from the original run was not retained
(`/tmp/i1/` holds no `[aotblk]` lines), so the original cannot be re-read. What
*is* now established: **a fresh host run of the same build produces the same 3
entries at f3259 and the same 8 distinct PCs as the Deck** (§1), so this is not a
Deck/host difference and not an instrument-tier difference. What the previous
page reported is not reproducible on either machine. It was also wrong
*arithmetically* before any re-run: its own PC list sums to 14 against a stated
total of 18.

**Nothing here moves the boundary.** The last bank-`$03` activity of any kind is
still f3270–f3271, and f3259 sits inside the region bank 03 was already
interpreted in. C-039b is untouched.

## 3. The consequence nobody wrote down: C-039c is refuted by the AOT tier

C-039c, in `docs/CAUSE_CLAIMS.md`, reads:

> **Bank 03 executes only in `$03C63D`–`$03E57E`; `$038000`–`$03C63C` executes
> nothing** — MEASURED, "min and max of the full dump, both machines, both
> instrument configurations".

**All 18 bank-`$03` AOT entries lie below `$03C63D`.** The measured range is
**`$03B477` … `$03C463`**, and 18 of 18 are `< $03C63D`. The previous page listed
`$03B477` in its own body without noticing that this contradicts a claim filed as
MEASURED.

How C-039c got into that state is a question about its instrument, not about the
guest: its evidence is "min and max of **the full dump**", and the full dump was
the **interpreted** bank-03 dump. `CYC_WATCH` is blind to AOT (README trap 1), and
the AOT log was host-only at the time. So C-039c was measured on one tier and
stated as a fact about both.

**C-039c is therefore retracted as stated.** The narrow surviving form, which is
worth keeping and is now measured on both tiers:

- **interpreted:** every bank-`$03$ PC that executes lies in `$03C63D`–`$03E57E`.
- **AOT:** every bank-`$03` block entry lies in `$03B477`–`$03C463`.
- **union:** bank 03 executes in `$03B477`–`$03E57E`, and **nothing** in
  `$038000`–`$03B46C` or in `$03C464`–`$03C63C`.

Ledger row **R-033**.

## 4. An environment trap: `SDL_QUIT` on this Deck means somebody sent a signal

Recording this because it cost four runs and it would cost the next session four
more. **A Deck run that dies with `exit: SDL_QUIT event after N frames` has been
signalled. It has not crashed and it has not reached a game condition.** Three
independent runs died at 1 414, 1 425, 1 428 and 1 430 frames, and the death
time tracked the supervising `timeout` value to within 0.5 s in every case:

| run | supervising `timeout` | host timestamp at exit | frames |
|---|---|---|---|
| 1 | `60` s | `[host +59.299s]` | 1414 |
| 2 | `60` s | `[host +59.494s]` | 1428 |
| 3 | `60` s | `[host +59.596s]` | 1425 |
| 4 | `60` s | `[host +59.518s]` | 1430 |
| 5 | inner `timeout 130` | `[host +129.997s]` | 3230 |

Run 5 is the control that identifies it: it ran in the foreground of a live ssh
session and was killed by the operator's own `timeout`, and it still reported
`SDL_QUIT`. So the mapping is **SIGTERM → `SDL_QUIT`**, and a backgrounded Deck
run whose supervising ssh connection is torn down is on the receiving end of it.

The consequences that matter:

- **A truncated histogram is not a smaller histogram.** Every dead run above still
  produced 250 000+ `[aotblk]` lines and a plausible-looking per-bank split. Had
  the f3259 discovery landed in one of those files it would have been reported as
  a complete run.
- **Run long measurements in the foreground of a live ssh session**, with a
  `timeout` comfortably longer than the run, and write the log to `/dev/shm`
  (tmpfs, 7.2 GB free) rather than to `/home`: the same binary writing to
  `/dev/null` completed all 4 000 frames in 154.2 s while writing to a file on
  `/home` was I/O-bound at ~4 300 lines/s.
- **`[host +T s] exit: RUN_FRAMES reached` is the only clean completion marker.**
  `SDL_QUIT` is not one.

## 5. What this does NOT establish

- **Not that bank 03 never runs AOT.** It does not, in f0–f3700. Beyond f3700 is
  unmeasured on both machines, and the previous page's own open question
  ("does `$03:8026` execute in ours at any frame >= 3857?", from the T093 page,
  where the reference core first ticks at f3857) is untouched by this.
- **Not a cause for the f3271 boundary.** 18 entries over 4 000 frames is not an
  activity curve; it is a census.
- **The f3259 entries are unexplained, not analysed.** Three AOT block entries at
  `$03C42A`/`$03C430`/`$03C463`, 11 frames before the f3270 burst. Nothing in this
  page says what leads to them or whether they are reachable again. **OPEN.**
- **The previous page's 1 430 539 is not explained**, only refuted. It is 1 entry
  in 1 430 540 and its log is gone, so the discrepancy cannot be diagnosed. What
  can be said is that neither machine reproduces it.
- **Agreement across machines here does not license agreement on *step* counts.**
  Block entries are a census and are stable; C-049's step counts are wall-clock
  dependent and are not. Do not generalise this table.