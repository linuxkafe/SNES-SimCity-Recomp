# Measurement — T110: `make clock`'s date detector is a brightness meter, and the reference cannot supply the control that would have caught it

**Deck-native** (`ssh deck@steamdeck`). The 3 600-present dump
`/home/deck/tick2/base/` was captured by an earlier ticket and was **not
re-captured**; ROM `/home/deck/rom/SimCity (USA).sfc`, md5
`23715fc7ef700b3999384d5be20f4db5`. Reference-build runs are the peer's, read
and cited only — never vendored, published or ported.

## The claim under test

`scripts/clock-gate.sh:179-223` crops the HUD date and hashes **raw RGB**:

```python
X0, X1, Y0, Y1 = 55, 125, 2, 21
...
out += px[o:o + 3]                      # RAW RGB bytes
seen.append((frame, hashlib.sha256(bytes(out)).hexdigest()))
```

Zero normalisation, zero brightness invariance. T109 measured `$2100`
(INIDISP) being ramped `$0300`→`$030F`, one step per frame, at **f3363…f3378**.
A global brightness ramp changes those bytes on every one of those frames.

**If that is true, a brightness fade alone satisfies the detector, and the gate
is one moved fade away from a green verdict on a dead city.**

---

## 1. The false green, demonstrated. The brief's prediction is refuted in the *other* direction

The brief predicted the fade window would report **1** distinct state and said to
stop if it did. **It reports 16.** The vector is real and larger than predicted.
The detector half of the script was copied **verbatim** into the measurement
harness; nothing was corrected.

```
$ ssh deck@steamdeck 'cd /home/deck/tick2 && python3 step1_datehash.py base'
== input ==
  ppm files                : 3600
  present index range      : 0 .. 3599
  TRUE frame range (csv)   : 1 .. 3600
  mapping                  : frame = present + 1  (csv head/tail)
  present 0 -> frame 1 ; present 3599 -> frame 3600

== (1) CURRENT DETECTOR, whole run ==
  DISTINCT_ALL (over all 3600 presents)          = 31
  LAST_CHANGE (present index, as the gate prints) = 3379
  LAST_CHANGE (TRUE frame)                        = 3380

== (2) CURRENT DETECTOR, fade window ==
  by present index 3360-3390 : 31 frames, 16 distinct
  by TRUE frame   3360-3390 : 31 frames, 16 distinct

== (3) CURRENT DETECTOR, after the threshold ==
  after present index >= 3600 (what the gate counts today): 0 frames, 0 distinct
  after TRUE frame     >= 3600                            : 1 frames, 1 distinct
  whole-frame crc32 constant from present 3381 (TRUE frame 3382) onward
```

**The gate is red today by luck.** Its threshold is *distinct date images after
f3600*; the guest's fade happens to *end* at f3379, before it. Move the fade ten
frames later and the same detector reports the dead city as advancing.

### 1a. The decisive number: the variation is **100% brightness, 0% glyphs**

Not inferred — **reconstructed**. The PPU's brightness transform is one line
(`snesrecomp/runner/src/snes/ppu_legacy.c:134`):

```c
pixelBuffer[0] = ((b << 3) | (b >> 2)) * PPU_brightness(ppu) / 15;
```

Take the crop at full brightness (TRUE f3380) and push it back down through that
transform. If each fade frame is **byte-identical** to some brightness level of
that one crop, the glyphs did not move and the light did.

```
$ ssh deck@steamdeck 'cd /home/deck/tick2 && python3 step1c_attr.py base'
== reference crop = the FULL-brightness frame ==
  present 3379 = TRUE frame 3380, max px 255, distinct colours 8
  colour set: #000000, #311000, #392900, #4a2908, #524210, #b59473, #bdad7b, #ffdeb5

== every frame f3360-f3390: is it EXACTLY the reference at some brightness? ==
  frame  csv-luma  #colours set-vs-ref  Bfit  pxdiff exact? raw-hash
  3360   0.000     1       SUBSET      0     0      YES    4b3d931aacb0
  3365   0.000     1       SUBSET      0     0      YES    4b3d931aacb0
  3366   3.591     7       OTHER       0     520    NO     b3109794b05c
  3367   7.278     7       OTHER       0     520    NO     4e2628c01e9b
  3368   17.476    8       OTHER       3     0      YES    525a681a995c
  3378   76.425    8       OTHER       13    0      YES    3a9cc000f756
  3380   88.627    8       SAME        15    0      YES    4ff5e0de0a0d
  3390   88.560    8       SAME        15    0      YES    4ff5e0de0a0d

== DECISIVE ==
  frames reproduced BYTE-EXACTLY as a brightness level of ONE constant crop: 29
  frames needing anything else                                                          : 2
  distinct raw-RGB crop hashes over f3360-f3390                                          : 16
```

**29 of 31 frames are one picture at 15 brightness levels. Sixteen distinct
hashes. Zero glyph changes.** The colour *classes* do change — and that is the
part worth keeping, because it is why the obvious fixes do not work:

| frame | distinct colours in the crop | what happened |
|---|---|---|
| f3380 (b=15) | **8** | the reference image |
| f3368 (b=3) | **8** | same 8 classes, collapsed by the integer divide |
| f3367 (b=2) | **7** | two classes have **merged** into one value |
| f3366 (b=1) | **7** | merged further |
| f3360–f3365 (b=0) | **1** | everything is `(0,0,0)` |

So the brief's question — *"is the set of distinct colours identical and only
the luma differs, or do the colour classes actually change?"* — has the answer
**the colour classes change, by monotone collapse**, and at b=1…2 the collapse
is lossy enough that the bright tan `#b59473` and `#bdad7b` both render as the
same grey `#0e0e0e`. **A palette-index classifier cannot recover an index the
renderer has already destroyed**, and a plain luma normalisation cannot either.
That is a measurement, not an argument, and it is why the fix below is a
*relation between two frames* rather than a per-frame canonicalisation.

### 1b. Two frames are not a brightness level of anything, and the cause is NOT ESTABLISHED

f3366 and f3367 are not reproducible from f3380 at any brightness, whole-frame or
per-scanline, and they are **not** a pixel-consistent recolouring of it:

```
  TRUE frame 3366 -- ref class -> image colours (px counts):
     ref #000000 (488 px) -> #000000:398, #080808:45, #0e0e0e:45
     ref #4a2908 (348 px) -> #000000:230, #010101:41, #070707:77
     ref #b59473  (53 px) -> #0e0e0e:53
     ref #ffdeb5 (193 px) -> #110e0c:193
```

A single colour class **splitting** into three values cannot happen under a
per-channel scale of a fixed palette index. Their pixel counts (810 black at
f3366 against 488 at f3368) say 322 pixels' worth of dark-region structure is
present in one frame and absent in the other.

**Cause: not established.** It is not the INIDISP formula — that reproduces
b=3…15 exactly. It is not explained by a mid-frame register write, which was
tested per scanline and matched nothing. **This is recorded as an open loose end
and no cause is asserted.**

**Consequence, and it retracts part of the brief:** the brief pre-registered F1
as *"the fade burst f3360–f3390 yields **1** distinct state"*. **That is not
achievable by any honest detector**, because f3366/f3367 are measurably a
different picture from f3368–f3390. The achievable and decisive form is the
**13-step ramp f3368–f3390**, which is 13 brightness levels of one image and
which the corrected detector collapses to **1** (§2).

---

## 2. The corrected detector, and its falsification in both directions

**Chosen: a relation between consecutive frames, not a per-frame canonical
form.** A frame `F` shows *the same date image* as the current state's
representative `R` **iff `F` is a pixel-consistent non-decreasing recolouring of
`R`** — every colour class of `R` maps to exactly one colour in `F`, and those
colours are non-decreasing in `R`'s colour order.

Why this and not the three candidates the brief listed:

| candidate | why not, measured |
|---|---|
| classify each pixel by **palette index** | the index is not recoverable. At b=1 the render is `(0,0,0)` for several distinct entries and `#b59473`/`#bdad7b` are indistinguishable (§1a). No PPU palette dump would help: the loss happened in the renderer. |
| **normalise the crop's luma** | weakest, and §1a is the proof: normalisation cannot undo a many-to-one collapse. It would also have to be proved not to mask a real date change, which is exactly the risk. |
| **glyph mask** with a brightness-invariant threshold | the threshold moves relative to the quantisation floor as brightness falls, so a fade-in reveals structure progressively. It is a threshold with a tuning parameter, on the project's most important gate. |
| **the relation** (chosen) | no threshold, no parameter, no palette, no brightness knowledge — and it is *exact* rather than tolerant. INIDISP is a non-decreasing map by construction, so a brightness change **cannot** open a state. A glyph change splits or inverts a colour class, so it **always** does. |

A uniform (single-colour) frame is handled by the same rule with no special
case: the constant map is non-decreasing and pixel-consistent, so a blank frame
is consistent with every state and can never open one.

```
$ ssh deck@steamdeck 'cd /home/deck/tick2 && python3 falsify2.py base'
== the real city crop: TRUE frame 3380 = present 3379, INIDISP $030F ==
  crop 70x19, 8 distinct colours

== F2 control: 1900 JAN -> 1901 JAN, composed from the screen's own glyphs ==
  cell copied : x 58-65, y 11-19  (the leading "1")
  cell written: x 82-89, y 11-19  (the last "0")
  pixels changed: 45 of 1330, at frame x 82-89

== F1b  the frames the guest ACTUALLY emitted, by TRUE frame ==
  TRUE f3368-f3390: the 13-step ramp, b=3->15    23 frames   OLD  13   NEW   1
  TRUE f3360-f3390: the whole fade window        31 frames   OLD  16   NEW   3
  TRUE f3381-f3600: after the last change       220 frames   OLD   1   NEW   1
  TRUE f1-f1200: the window --self-test uses   1200 frames   OLD  16   NEW   2

== SUMMARY   (1 = same date image, 2 = different) ==
  case                                                 OLD   NEW
  f3380 crop @ brightness 15   vs  @15                 1     1
  f3380 crop @ brightness 13   vs  @15                 2     1
  f3380 crop @ brightness 11   vs  @15                 2     1
  f3380 crop @ brightness  9   vs  @15                 2     1
  f3380 crop @ brightness  7   vs  @15                 2     1
  f3380 crop @ brightness  5   vs  @15                 2     1
  f3380 crop @ brightness  4   vs  @15                 2     1
  f3380 crop @ brightness  3   vs  @15                 2     1
  1900 JAN @15  vs  1901 JAN @15                       2     2
  1900 JAN @15  vs  1901 JAN @13                       2     2
  1900 JAN @15  vs  1901 JAN @ 9                       2     2
  1900 JAN @15  vs  1901 JAN @ 5                       2     2
  1900 JAN @15  vs  1901 JAN @ 3                       2     2
  1900 JAN @15  vs  itself                             1     1
  1900 JAN @15  vs  1900 JAN @15, re-read from disk    1     1
  1900 JAN @15  vs  1901 JAN @15, then BACK to 1900    2     2
```

### The pre-registered falsifiers

| # | assertion | required | measured |
|---|---|---|---|
| **F1** | **negative control** — a brightness change must not register | RED on the old detector, GREEN on the new | **RED, old: 2 states at each of 8 brightness levels, 13 over the real ramp. GREEN, new: 1 at every level, 1 over the real ramp.** |
| **F2** | **positive control** — a real date change must register | GREEN on the new one | **GREEN: 2 states**, brightness held constant by construction (both frames are the same captured frame at b=15) |
| **F3** | the date change is still seen **with brightness differing** | GREEN, or say why not | **GREEN, and it was arrangeable: 2 states at b=13, 9, 5 and 3 against a b=15 frame.** The composition is brightness-independent because it edits *palette indices* in the composed frame before the brightness transform is applied. |

**The full 2×2, which is the claim that matters:** same date + different
brightness → **1**; different date + same brightness → **2**; different date +
different brightness → **2**; identical frame → **1**. The detector separates
content from illumination and is not merely brightness-keyed.

### How the positive control was built, stated plainly

There is **no build in this project, ours or the reference's, that renders a
date change on screen** (§3). So the positive control is **composed**: the
crop's own geometry was measured from the bright-ink column profile — text row
**y12–y18**, 8-px character cells at **x = 58 + 8i** — and the cell already
holding the leading `1` of `1900` was copied over the cell holding the last `0`,
giving `1901 JAN`. **45 of 1 330 pixels change, all inside that one cell.**

This is synthetic in the strict sense that we composed it, and **it is labelled
that way**. It is not a hand-drawn glyph and it is not a screenshot of a real
roll; it is **the game's own 8×9 bitmap for a `1`, moved to where the `1` goes.**
It is a real glyph-cell change of exactly the class the detector must catch, and
that is the whole claim being tested.

---

## 3. ⚠️ RETRACTED (R-044): the reference build's headless renders are **not a game screen**

**The brief's positive-control plan cannot be carried out as written, and the
reason is a defect in a tracked document of ours.**

`study/peer-linux/README.md` says:

> `- frame.f*.bgra` — rendered frames, 256x210 BGRA. `bgra2png.py` converts them.

Both halves are false, and the file is the recipe the project uses for every
reference measurement.

**Measured, Deck-native.** 301 consecutive frames of the reference's own
`R1` run configuration, rendered every frame across the `f5190` month roll:

```
$ /tmp/opencode/t101/jjhead-win ".../SimCity (USA).sfc" cold.srm out.srm 5400 \
      s.script . 5100 5400
EXIT=0
RESULT failed=0 frames=5400 master_clock=1929726032 insns=64497354 sram_dirty=1
captured: 301 frames

$ python3 dense2.py .
captured f5100..f5400, 301 frames
== every frame in which ANY pixel changed, and where ==
  frame   changed  bbox
  5330    89       x   0-239, y  20- 28
```

**One** change in 301 frames, and the geometry is **240×224**, not 256×210.
Converted to PNG, the frame is **tile data, not a game screen**: rows of
repeating 8×8 patterns, mean luma ~110 on every one of R1's 100 renders, never
a HUD, never a map, never text.

The month *did* roll inside that window, from the guest's own WRAM:

```
$ awk 'NR>1 && $1>=5100 && $1<=5400' timeline.log
 5160 ... d0B51 0007 d0B53 076C d0B55 0002 ...
 5220 ... d0B51 0008 d0B53 076C d0B55 0003 ...
```

> **So: in the reference build the month rolls in WRAM and the rendered frame
> does not move.** Whether the peer's PPU is misrendering or the windowed
> frontend supplies state `jjhead` does not is **NOT ESTABLISHED**, and no cause
> is claimed. What is established is that **`jjhead`'s framebuffer cannot be used
> as evidence about anything on screen.**

**Two consequences, both load-bearing:**

1. **R-044 retracts the tracked claim** that `frame.f*.bgra` are rendered frames
   of the reference and are viewable. `study/peer-linux/README.md` is corrected
   in place with the measurement.
2. **The brief's "29 distinct date images in 30 000 frames" is a WRAM figure, not
   a picture figure.** T101's own table reads *"distinct `($0B53,$0B55)` pairs |
   29"* — those are **WRAM word pairs**. No document, and not the brief, has
   measured 29 distinct *images*. **The reference's on-screen date behaviour is
   unmeasured**, because the only instrument that could measure it does not
   render.

`study/peer-linux/jjhead.c` gained an optional `[render_from render_to]`
argument (our file, not the peer's) so a window can be captured frame by frame
instead of every 300. Without it a month roll cannot be told from ordinary city
growth — a simulating reference changes thousands of pixels between any two
300-frame samples, which is why the coarse cadence showed *nothing* where the
dense capture shows the roll in WRAM and no picture change at all.

---

## 4. A second finding: the gate's own positive control was a brightness fade

`make clock-self-test` runs `clock-gate.sh --self-test --frames 1200` and passes
on *"16 distinct date images over 1 200 frames, last change f1163"*. Locating
every crop-hash change in the 3 600-frame dump:

```
$ python3 -  # crop hash change frames, present index
crop-hash change frames: [0, 301..315, 506..520, 566..580, 1132..1146,
                          1149..1163, 3244..3258, 3365..3379]
```

Six contiguous runs, at **f301–315, f506–520, f566–580, f1132–1163,
f3244–3258, f3365–3379** — which is exactly T109's census of `$2100` INIDISP
ramps (`f300→f505` up, `f506→f520` down, `f565→f580` up, `f1132→f1146` down,
`f1148→f1163` up, `f3244→f3258` down, `f3363→f3378` up).

> **The self-test's 16 "distinct date images" are 16 levels of a brightness
> fade. The gate's positive control was a demonstration of the bug it exists to
> catch.**

This is why the corrected detector reports **2** over the same window and not 1:
there is one genuine content change in the menus (at f302, when the first
informative frame replaces a blank one) and the rest was never content. **The
self-test still passes**, on a real content change instead of a fade.

---

## 5. What is NOT claimed

- **Not a cause for C-006.** C-006 stays **OPEN**. This is a description of what
  the detector can and cannot see.
- **Not that the gate's verdict moves.** It does not. `DISTINCT_AFTER` after
  f3600 is **1** under both detectors, on a screen that is bit-identical for
  220 further frames. `make clock` is red before and red after. DoD Rule 0b.
- **Not a weakening.** The corrected detector is **less** sensitive to
  illumination and **equally** sensitive to content, and the criterion
  (`CLOCK_MIN_ADVANCE`, default 2, plus the `$0B53` city-loaded check) is
  untouched. A detector that reports fewer states is only safe if the states it
  drops are the ones no date change could produce — that is §2's 2×2.
- **Not a re-measurement of `make clock`.** The gate was not run on the host.
- **Not that the reference is broken.** Its clock runs and that is measured. What
  is unmeasured is its *rendering*.

---

## 6. §2 wired into the gate, and the falsifiers shipped inside it

The correction is in `scripts/clock-gate.sh`. Both falsifiers run **on every
invocation**, because a falsification performed once by hand in a measurement
ticket is a claim, and one that runs every time somebody edits the file is a
check. **The red side is printed on every run**, so the check cannot quietly
stop being able to fail.

### `make clock-self-test` — the life half (Deck-native)

```
$ scripts/clock-gate.sh --self-test --frames 1200 --rom "/home/deck/rom/SimCity (USA).sfc"
  frames captured                        : 1200
  distinct DATE images, whole run        : 2
  (raw-RGB hashes, the old detector)     : 16
  last frame the date changed            : 301

SELF-TEST: PASS - the detector sees CONTENT where content changes.
  16 raw-RGB hashes became 2 date images on this window, and every
  one of the states it dropped was a brightness fade.
EXIT=0
```

### `make clock-glyph-self-test` — both falsifiers, on a real city (Deck-native)

```
$ scripts/clock-gate.sh --self-test-glyph --frames 4200 --rom "..."
== FALSIFIER 1 (negative control): a brightness fade is not a date ==
  source crop              : present 3367, 8 palette entries
  distinct DATE images     : 1   <- must be 1
  raw-RGB hashes (old gate): 13   <- the red side; must be > 1

== FALSIFIER 2 (positive control): a real date change registers ==
  source crop                : present 3367, 8 palette entries
  composed 1900 JAN -> 1901 JAN: 45 pixels changed, one character cell

  case                                       raw(old) date(new)
  brightness HELD CONSTANT                   2        2
  date advanced, brightness 11 vs 15         2        2
  date advanced, brightness 7 vs 15          2        2
  date advanced, brightness 3 vs 15          2        2

GLYPH CONTROL: PASS
EXIT=0
```

### The RED side of the new gate, on the old detector

The check was given the pre-T110 detector — a crop is the same date image iff
its raw RGB bytes match — with everything else unchanged:

```
  distinct DATE images     : 13   <- must be 1
  raw-RGB hashes (old gate): 13   <- the red side; must be > 1
GLYPH CONTROL: FAIL - the detector counted 13 date images across 13 brightness
levels of ONE unchanged crop. It is not invariant to INIDISP, so a fade in the
guest can satisfy the real check and this gate can go green on a dead city.

Do not raise CLOCK_MIN_ADVANCE to compensate. Fix the detector.
EXIT=1
```

**One falsifier caught the falsifier.** The first version of the invariance
check lived in the 1 200-frame self-test and **refused to certify itself**:

```
  source crop            : present 301, 3 colour classes
  distinct DATE images    : 1   <- must be 1
  raw-RGB hashes (old)    : 1   <- the red side; >1 or this check is dead
SELF-TEST: FAIL - the raw-RGB hash reported 1 image(s) across the same 13
brightness levels, so this falsifier can no longer demonstrate the false green
it exists to demonstrate.
```

Because `pick_informative` chose on **distinct RGB values**, and the busiest crop
on the menu window has three, all below 8 in every channel — so all three
reconstruct to palette entry `(0,0,0)` and the whole ramp hashes identically.
**The criterion is now the count of distinct 5-bit palette entries**, which is
the thing the brightness transform actually acts on, and the falsifiers moved to
the route that reaches a city. A check that cannot fire is not a check, and this
one said so instead of passing.

### `make clock` — RED, as it must be

```
$ scripts/clock-gate.sh --rom "/home/deck/rom/SimCity (USA).sfc"
  run 1/1 ... 1 distinct date images after f3600 (last change f3366 of 6000)
  city-loaded check: $0B53 = 076C  -> a city is present (year 1900)

== verdict ==
CLOCK: FAIL - the date did not advance in a live city.
GATE_EXIT=1
```

`scripts/clock-gate.sh` exits **1**; `make clock` exits **2** (make's code for a
failed recipe, CONF-18). **Red before this change and red after.** The verdict
does not move, and DoD Rule 0b holds: nothing here weakened a criterion,
`CLOCK_MIN_ADVANCE` is untouched, the `$0B53` city-loaded requirement is
untouched, and the "a menu screen with a still date is not a city" rule is still
in the FAIL text.

### The stale FAIL paragraph, rewritten (closing the `clock-2026-10-03b` MAJOR)

Gone: *"So the two builds are separated AT THE INSTRUCTION, not at the symptom"*
— an inference presented as fact — and *"Next measurement: count executions of
`$03:8026` over f3000-f13080"*, which **T102 already did** (14 000 frames, zero
executions, C-041, both tiers, two machines). In their place the message now
separates what is measured from what is not, and states plainly that **whether
the rendered date reads `$0B51` at all was open** — which T111 has now answered,
one commit later.
