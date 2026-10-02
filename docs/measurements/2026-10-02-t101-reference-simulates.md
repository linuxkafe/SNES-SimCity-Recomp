# Measurement — T101: does the reference build ever actually simulate? (2026-10-02)

## The answer

**Yes.** The reference build's city clock runs, indefinitely, and it was
measured to **f30 000** on the Deck from a cold SRAM with a real save written
mid-run.

| | value |
|---|---|
| city appears | **f3000**, 1900 January (`$0B53 = 076C`, `$0B55 = 01`) |
| first month roll | **f4440** (1900 February) |
| month-roll cadence | **~780 frames**, 28 rolls in 30 000 frames |
| year 1900 → 1901 | **f13080** |
| year 1901 → 1902 | **f24600** |
| at f30 000 | **1902 MAY**, `$0B51 = $0071` |
| distinct `($0B53,$0B55)` pairs | **29** |
| `$0BA5` population | **0** at every one of 452 samples |
| `$0B9D` funds | **20 000** at every one of 452 samples |
| exit | `EXIT=0`, `RESULT failed=0 frames=30000 master_clock=10720929618 insns=356512178 sram_dirty=1`, 211 s |

**The comparative premise is therefore available after all.** "The reference
simulates and we do not" is no longer an assumption — it is a measurement, and
it survives. Nothing in this document retracts the premise. What it retracts is
two sentences that were written *against* the reference on the strength of a
broken instrument, and they are the reason this investigation was framed as
though the premise might not hold.

## The two disagreeing runs were never in conflict

The framing going in was: one run gave **23 months over 33 700 frames**, the
other gave **year stays 1900, 27 ticks of `$0B51`, 9 000 frames**, and the tick
routine's own `AND #$0003` arithmetic predicts ≈6 month advances in that second
window and finds none.

**Both halves of that were artefacts, and neither was the reference's fault.**

### The arithmetic was right. The instrument could not print the answer.

Re-running the disputed configuration on the Deck reproduces the disputed run
*byte for byte*:

```
$ ../jjhead-clean "SimCity (USA).sfc" cold.srm out.srm 9000 s.script .
RESULT failed=0 frames=9000 master_clock=3216243544 insns=107365572 sram_dirty=1
```

`master_clock = 3216243544`, `insns = 107365572` — **identical to the values in
the T093 record**. Same execution, same binary behaviour, same script.

At f9000 that same run reads:

```
frame  master_clock insns   z12 zB9 zC7  d0B51 d0B53 d0B55 d0B5C d0B53lo d0BA5 d0B9D
 9000  3216243544 107365572  01  01  62   001B  076C  0007  0101  006C    0000 4E20
```

**`$0B55 = 0007` — AUGUST.** The disputed run logged `d0B55 = 076C`. That is
`$0B53`'s value, printed a second time under a month label: our own driver
clobbered that column (`c4923de`). The month had advanced six times inside the
disputed window, at **f4440, f5220, f6000, f6780, f7560, f8400** — and the log
was structurally incapable of showing it.

So the contradiction resolves in the direction of the arithmetic: **6 advances
occurred**, exactly as predicted.

### The remaining true half: f9 000 *is* too short — for the year

`$0B53` really does read `076C` at f9000, because the first **year** roll is at
**f13080**. A 9 000-frame window contains no year change. That observation was
correct; it was over-read as "the clock is stopped".

## `$0B51` decoded: it is a month counter in units of quarters

The `AND #$0003` in the tick routine is not a mystery and the counter is not a
free-running one. Across **1 344 city samples in three independent runs**:

> **`$0B51` = 4 × (months elapsed since the city was created) + (0…3)**

| run | samples | violations |
|---|---|---|
| R1 clean core, `scripts/d_city_kbd.script` | 452 | **0** |
| R2 watched core, `scripts/d_city_kbd.script` | 452 | **0** |
| R4 clean core, `route.script` | 440 | **0** |

Observed at every month roll in R1: f3000 `m=01 b51=0000`, f4440 `m=02
b51=0004`, f5220 `m=03 b51=0008`, f6000 `m=04 b51=000C`, f6780 `m=05 b51=0010`,
f7560 `m=06 b51=0014`, f8400 `m=07 b51=0018`, f9180 `m=08 b51=001C`, f9960
`m=09 b51=0020`, f10740 `m=0A b51=0024`, f11520 `m=0B b51=0028`, f12300
`m=0C b51=002C`, f13080 `y=076D m=01 b51=0030`.

`AND #$0003` therefore extracts **the quarter within the current month**, and
the 27 ticks counted in the disputed 9 000-frame window are `6 × 4 + 3`: six
whole months and three quarters. That is the ≈6 the arithmetic predicted, and it
is why the count was never going to be zero. **[MEASURED, Deck-native]** for the
relation across 1 344 samples; **[INFERRED]** that `AND #$0003` is the
instruction that performs the extraction — the arithmetic is measured, the
mapping onto that specific opcode is read off the tick routine and was not
separately instrumented.

This also names what `INC.w $0B51` at `$03:8026` *is*: the writer of the month
sub-counter. It is the third piece of evidence pointing at C-041 as the wrong
question — see §What this changes.

## The write-watch does not perturb the reference

Refuted, measured, and the instrument is demonstrably live.

| | core | script | frames | `master_clock` | timeline |
|---|---|---|---|---|---|
| R1 | **clean** | `d_city_kbd.script` | 30 000 | `10720929618` | — |
| R2 | **watched** | `d_city_kbd.script` | 30 000 | `10720929618` | **byte-identical to R1** |

```
$ cmp R1/timeline.log R2/timeline.log && echo IDENTICAL
IDENTICAL
```

`cmp` over the whole 30 000-frame log — every dumped field, every frame. The
watch was not a silent no-op: its log holds **230 rows** spanning **f0 →
f29967**, including the `$03:C77E` zeroing at **f2985** (`A = $0007`) and the
`INC` ticks at **f3857, f4009, f4262, f4402** — the T093 frames, reproduced.

The two libraries are provably different builds of the same core:

| library | md5 | size | `T093_WATCH` strings |
|---|---|---|---|
| `lib-clean.a` | `42ae19c97ccf5fcc9a6e3025048f7c55` | 32 505 912 | **0** |
| `lib-watched.a` | `681f43aa0718a03fe2ac1bc38e355974` | 32 510 936 | **3** |

## The reference has no f3259

The question the brief asks of the reference — *at what frame does its own city
state stop being written* — has no answer, because it never stops.

From the **337 WRAM dumps** of the independent `jjwin` + clean-core +
`route.script` run (`~/clock2/jjwram.f*.bin`, f100…f33700, Deck):

- **34 491 distinct WRAM addresses change** in the city window (f3800–f33700).
- Per-100-frame churn never collapses: it sits at **~60–110 bytes** from f7500
  to f33700, with the same shape in year 1900, 1901 and 1902.
- Change events for the fields that matter:

| address | change events, f3800–f33700 | final |
|---|---|---|
| `$0B51` tick | **128** | `$0080` |
| `$0B53` year | **2** | `$076E` |
| `$0B55` month | **32** | `$09` |
| **`$0DC7` accumulated tax** | **128** | `$00E0` |
| `$0B9D` funds | **0** | `$4E20` |
| `$0BA5` population | **0** | `$0000` |

**`$0DC7` is the field this repository already proved our build never
accumulates.** C-057: *"`$0DC0`–`$0DD0` takes 61 writes in 9 000 frames in 4
events, and 0 after f3259 … `$0DC7` … is never accumulated into."* The
reference writes it **128 times** in its city window. That is the cleanest
cross-build differential in the project: same field, opposite behaviour, two
machines, two cores.

## What the reference does *not* do, and is not claimed to

**Population is 0 and funds are 20 000 in every sample of every reference run**,
over 30 000 frames and over 33 700 frames, on two routes and two cores. A city
that was zoned and populated would not look like that.

So "the reference simulates" means precisely: **the tick runs, the calendar
advances, and the tick's own accumulation is performed.** It does **not** mean a
growing economy, and no claim that it does is made here. Whether a city with no
residents and nothing zoned *should* show a moving treasury is **OPEN** and is
not answered by this document.

## The machine, and every caveat that travels with the number

**Every number on this page is Deck-native** (`ssh deck@steamdeck`), none is
host-only. The reference core is `simcity-static-recomp` **1.4.1**, from the
release tarball extracted at `~/peers/jj`, built there with **gcc 15.1.1**,
`RelWithDebInfo`, against ROM `23715fc7ef700b3999384d5be20f4db5` (524 288
bytes, verified on the Deck).

Caveat, carried by every number above: **the Deck's rootfs is damaged.** 503 of
the 504 `/usr/include` paths `pacman -Ql glibc` claims are absent while the
package reports installed, and `echo '#include <stdio.h>' | gcc -E -` gives
`No such file or directory`. The builds here use the repaired prefix at
`/home/deck/sysroot` via `-idirafter /home/deck/sysroot/usr/include`.

### What was changed, and where it lives

**Nothing in the reference core was changed for this measurement.** The clean
library was produced by restoring one file to its pre-patch content:

```
$ cp ~/peers/sc_v11_bus.c.deck-orig ~/peers/jj/static-recomp/src/sc_v11_bus.c
$ grep -rl T093_WATCH_LO ~/peers/jj/static-recomp/src | wc -l
0
```

`sc_v11_bus.c.deck-orig` is the pre-T093 file retained on the Deck; `cmp`
against it is exact. The T093 bus-write hook (the only reference-core edit that
has ever existed here) exists solely to *produce* R2 as the control, lives only
in the throwaway clone at `~/peers/jj`, and is **never in this repository**. No
peer source is vendored, copied or published. The peer declares no licence;
this is private study only.

Changed **here**, in this repository: `study/peer-linux/jjhead.c` and
`study/peer-linux/jjwin.c` — ours, not the peer's. See `c4923de`.

### `exit:` discipline

All three runs were foregrounded through `ssh` and returned **`EXIT=0`** with
`RESULT failed=0 frames=N` in the log. No run was `timeout`-truncated. An
earlier *backgrounded* attempt on the Deck died silently at f1440 and f2160 with
no error and no exit status; **those runs are discarded and nothing here rests
on them.** The foreground/background difference is **OPEN** and is not this
document's business — but it is the reason the numbers above were taken the slow
way.

## What this changes, and what it does not

**It does not answer C-006.** Why *our* city does not simulate is still OPEN, and
this document makes the question narrower rather than answering it. What is now
measured on both sides:

| | reference | ours |
|---|---|---|
| city loads | yes, f3000 | yes, f3259 |
| month advances | **28 times in 30 000 frames** | **0** |
| year advances | 1900→1902 | **stays 1900** |
| `$0DC7` accumulated | **128 writes** | **0 after f3259** (C-057) |
| city-state block last written | **never stops** | **f3259, then nothing** (C-058) |

**`make clock`'s criterion is now justified on its own terms rather than
inherited.** The gate requires ≥2 distinct date images after the city frame.
The reference produces **29** in 30 000 frames, measured, on the machine this
project ships to. The criterion is not a guess about what a SimCity ought to do;
it is a floor set below a measured behaviour of the reference implementation. It
stays exactly as it is.

**It does not rescue C-007.** `$0B51` is still **INFERRED** to be the master
city tick in *our* build. What changes is that its reference-side behaviour is
now *decoded* rather than merely observed, and the decode says a rising `$0B51`
is four times a rising month count — so "rising is not simulating" is refuted,
and the story it was used to kill (*"make `$03:8026` run and the clock
advances"*) is **no longer refuted by it**.

## Retractions this forces

| row | refuted |
|---|---|
| **R-035** | "the reference build also stops ticking productively" — `README.md`, and the same shape in the f3271 note. Refuted: 28 month rolls and two year rolls in 30 000 frames, Deck-native, on a clean core. |
| **R-036** | "rising is not simulating" / "`$0B51` rises 27× and the year stays 1900, therefore ticking is not simulating". Refuted as stated: 27 = `6 × 4 + 3`, six whole months, and the month *did* advance six times inside the very window the claim describes. |

## The single next measurement

**Our build, Deck-native, `CYC_WATCH`-free, counting executions of `$03:8026`
(`INC.w $0B51`) over f3000–f13080** — the window in which the reference is
*measured* to roll its month 16 times (f4440 … f12300). C-041c established a
zero over f0–f6000; the reference's first roll is at f4440, so f0–f6000 already
contains one. The next bracket must be wide enough to contain many rolls, and
f3000–f13080 contains sixteen. If the count is zero there, the tick is absent
across the whole span in which the reference demonstrably ticks, and the two
builds are separated at the instruction rather than at the symptom.