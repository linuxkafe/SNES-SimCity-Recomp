# SNES-SimCity-Recomp

Partial static recompilation of SimCity (SNES) for PC using [snesrecomp](https://github.com/RetroPortingToolKit/snesrecomp).

An unofficial, non-commercial project that recompiles the Super Nintendo game
**SimCity** (Nintendo, 1991) into C++17 for PC. All graphics, palettes, map data
and audio are read at runtime from a copy of the original ROM that **you**
supply; the ROM and any ripped assets are never included. Built on the
**snesrecomp** framework.

> ### How to read this file
>
> Every causal sentence below carries its state — **[MEASURED]** (an instrument
> observed it), **[INFERRED]** (deduced, not observed here), **[OPEN]** (asked,
> not answered) — or is a **retraction**, marked where the claim was made. The
> classification of *every* causal claim in this project, with the instrument
> behind it, is [`docs/CAUSE_CLAIMS.md`](docs/CAUSE_CLAIMS.md); the standard of
> proof is [`docs/DEFINITION_OF_DONE.md`](docs/DEFINITION_OF_DONE.md), whose Rule
> 0 is that **no criterion may be satisfied by a claim — only by a command that
> exits 0**.
>
> This project has retracted **29** claims out of **37** ledger rows (computed,
> `make retraction-count` — never a hand-written number; the other 8 rows are 6
> `superseded` and 2 `invalidated-premise`, which is not a retraction). Where an
> old claim is quoted below it is labelled **RETRACTED** and is printed as
> history, not as the answer.
>
> **And that number has a known hole in its guard — read this before you trust a
> count in prose.** A stale `26 refuted` sat in this file's own gate table for a
> commit while `make retraction-count` said 28, because the guard that checks
> counts matches `"N retractions"` and the ledger's native phrasing is `"N
> refuted"`. The fix was written, measured against this project's own corpus,
> produced **three false positives**, and was **reverted**; the attempt is
> recorded in `scripts/check-retracted-claims.sh`'s own comment block. So
> **CONF-14 is an open hole, not a closed one**, and the interim rule is
> structural: *the retraction count has exactly one authority,
> `make retraction-count`. Do not type it.* If a document must state it, state it
> as `N rows = M refuted + …` and re-read it **after** running the command.

### How much of it is actually native

"Native recompilation" on its own oversells this, so the counts are given from
the generated manifest in the working tree
(`src/gen/program_manifest.json`; `src/gen/` is derived from your ROM and is
never committed):

| | measured |
|---|---|
| routines in the manifest | **302** |
| recompiled ahead of time (`aot_eligible`) | **239** |
| interpreter-only (`lle_only`) | **63**, holding **512 of 9 803** static instructions (**5.2%**) |
| distinct AOT symbols emitted into `src/gen/*.c` | **186** |

By function count this is roughly four-fifths native. By *time* it is not, and
the honest time figure is workload-dependent — it is stated per workload
below rather than as one number:

- **A live city executes ~7 214 interpreted opcodes per frame** (2 308 599
  steps over the 320-frame live window, banks 00+01 — measured on the Deck, see
  the instrument note below). The earlier figure of ~1 427 opcodes/frame was
  measured on **attract mode and menus** and is bounded to those.
- The bank-03 tick and the task dispatcher are **not** the whole story of what
  the interpreter runs; in a live city the interpreted time is dominated by the
  **vblank spinlock** (`$009311`/`$009313`/`$009315`, ~1 800 iterations/frame)
  plus the NMI handler once per frame.

Both counts are **[MEASURED]**, both are blind to AOT, and neither is a
measurement of *gameplay*: correctness is a separate question, and the snesrecomp
fork documents the relevant asymmetry — AOT code never advances the PPU beam
while the interpreter advances it every opcode, so a share of *time* is not
automatically a claim about behaviour.

## About the Game

**SimCity** is an open-ended city-building simulation created by **Will Wright**
and published by **Maxis** in 1989. You play as the mayor of an empty plot of
land: you zone residential, commercial and industrial areas, lay roads, power
and water, set taxes and a budget, and watch the simulation grow — or go broke.
Disasters (fires, floods, earthquakes, and a certain giant monster) keep you on
your toes.

The **Super Nintendo Entertainment System (SNES)** version was developed by
**Nintendo EAD** under license from Maxis and published by Nintendo in 1991
(JP April 26, NA August 23; EU September 24, 1992). It is widely regarded as
the best console adaptation of the original: seasons recolour the map, Nintendo
cameos appear (a Mario statue, a Bowser rampage), and extra scenario missions
were added. This port reproduces that SNES release.

## Status

**The reference build simulates. Decisively, measurably, indefinitely — and we do
not. `make clock` is red and must stay red.** That comparison used to be an
*assumption* this project was leaning on and could not check, because two
reference runs appeared to disagree about whether the reference simulates at
all. **They never did.** The disagreement was **our own instrument** printing
`$0B53` under the label `$0B55`. With that fixed, the premise is a measurement
and it holds:

> **T101, Deck-native, clean core, cold SRAM, a real save, `EXIT=0`, 211 s over
> 30 000 frames** — city at **f3000** (1900 JAN), first month roll at **f4440**
> and a cadence of **~780 frames**, **28 month rolls**, year rollovers at
> **f13080** and **f24600**, and at f30 000 **1902 MAY** with `$0B51 = $0071`.
> **29 distinct date images.** **[MEASURED, Deck-native]**
> [`docs/measurements/2026-10-02-t101-reference-simulates.md`](docs/measurements/2026-10-02-t101-reference-simulates.md)

**No comparative premise was retracted. The opposite: two sentences written
*against* the reference were, and the story they were used to kill — *"make
`$03:8026` run and the clock advances"* — is no longer refuted. It is what the
evidence points at.** (`scripts/retracted-claims.tsv` R-035, R-036.)

**What "simulates" means here, precisely: the tick runs.** Population (`$0BA5`)
is **0** and funds (`$0B9D`) are **20 000** in **every** sample of **both**
builds — 452 reference samples over 30 000 frames and 440 more over 33 700,
against ours. **No claim that an economy grows is made anywhere in this
repository, and none may be.** Whether a city with no residents and nothing zoned
*should* show a moving treasury is **OPEN** and is not answered by any run in
this tree.

The rest of this section is the evidence:

| | state |
|---|---|
| ROM boots to attract, menus, naming | **working** [MEASURED] |
| A live city loads and is presented | **working** [MEASURED] — `$0B53 = 0x076C` (year 1900), `$0B55 = 1`, `$0B9D = 20000` |
| The vblank token handshake (a former deadlock) | **fixed** [MEASURED] |
| **Reference build's clock** | **RUNS** [MEASURED, Deck-native] — 28 month rolls and 2 year rollovers in 30 000 frames, never stops writing city state |
| **Our build's clock** | **DOES NOT RUN** [MEASURED, Deck-native] — the tick instruction executes **0** times in **14 000** frames; bank `$03` executes **nothing at all** in f3272–f13080; the city-state block is written **once** at f3259 and **not once in the following 10 741 frames** |
| **Why our simulation does not advance** | **OPEN** — no cause is asserted anywhere in this repository |

![SimCity title screen](docs/screenshots/title.png)

⚠️ **Not deliverable.** Our date stays `1900 JAN` forever — the seasons never
recolour the map, the population stays 0 — while the controller does nothing.
`scripts/d_city.script` drives the game from boot into a live city headlessly
and deterministically, so this is **not** a game-flow problem, and the renderer
is proven live: poking a WRAM byte moves the presented picture on the very next
frame.

![A city at 1900 JAN, frozen](docs/screenshots/city-frozen.png)

### What was fixed: the vblank token handshake

The game's vblank wait is a **token handshake**, not a flag test. ROM bytes,
bank 00, file offset `0x130F`. The mapper is **LoROM** — the SNES header sits at
`0x7FC0` and reads `SIMCITY`, and `0xFFC0` is filler; the linear rule this project
uses, `offset = bank*0x8000 + (addr & 0x7FFF)`, *is* the LoROM rule and is
byte-verified on two independent labels (several tracked documents call this
mapping "HiROM", which is wrong — see `docs/CONFLICTS.md` CONF-7). This ROM is
exactly 524 288 bytes with no copier header:

```
$00:930F  64 B9   STZ  $B9      $00:9311  E6 C7   INC  $C7
$00:9313  A5 B9   LDA  $B9      $00:9315  F0 FA   BEQ  $9311
$00:9317  60      RTS
```

The only writer of `$B9` is the NMI handler at `$00:80B2` (`INC $B9` at
`$00:80BC`). The host delivered NMI once at the top of the frame, **before** the
guest ran: the handler set `$B9 = 1`, the guest's `STZ $B9` then cleared it, and
the spin at `$9313` waited on zero forever. **[MEASURED]** — the handshake was
structurally impossible, not merely slow.

Two defects, both fixed in `afceeec`:

1. `recomp/bank00.cfg:44` carried `exclude_range 0x930D 0x9318` — a **missing
   `& 0x7FFF` mask**, so the range was compared against unmasked addresses, `STZ $B9`
   ran compiled inside `bank_00_930D_M0X0`, and the spinlock never yielded to the
   interpreter. Corrected to `0x130D 0x1318`.
2. `src/game_rtl.c:GameRunOneFrame` delivered NMI only at the top of the frame.
   It now delivers NMI **inside the slice loop after the guest parks**, plus a
   top-of-loop point armed for the case where the guest already burned the whole
   frame.

After the fix: `$00B9 = $01` at the frame boundary (it was `$00` in every prior
sample) and `$00C7` is counting. **[MEASURED]**, 5/5 samples. This is why the
vblank wait is no longer a candidate cause: fixing it left the city still not
simulating, so "the vblank handshake causes the freeze" is **RETRACTED**
(ledger R-005…R-008 territory; `docs/CAUSE_CLAIMS.md` C-012).

### What is not fixed, and what is actually measured about it

`$0B51` is the 16-bit master city tick counter — **[INFERRED]**, and its
reference-side behaviour is now **decoded** rather than merely observed.

**The reference's clock runs.** Measured Deck-native to **f30 000** from a cold
SRAM: the city appears at **f3000** (1900 January), the month rolls every
**~780 frames** — **28 rolls** — the year turns over at **f13080** and **f24600**,
and f30 000 reads **1902 MAY**, `$0B51 = $0071`. **[MEASURED, Deck-native]**
(`docs/measurements/2026-10-02-t101-reference-simulates.md`).

> **`$0B51` = 4 × (months elapsed since the city was created) + (0…3)**

It holds across **1 344 city samples in three independent runs** with **zero**
violations — and, in the stronger form, across **113 individual tick events in
the write log, every one of them +1, zero deviations, strictly monotonic**, with
the month rolling at exactly the **28** events where the counter reaches a
multiple of 4. The sample form is one observation per 60 frames; the event form
is one observation per write, and it is the one to rely on. **[MEASURED,
Deck-native]**

So `AND #$0003` extracts **the quarter within the current month**, and the 27
ticks counted in the old 9 000-frame reference run are `6 × 4 + 3` — six whole
months and three quarters. Population (`$0BA5`) stays **0** and funds (`$0B9D`)
stay **20 000** in every reference sample, so "the reference simulates" means
*the tick runs*, not *an economy grows*.

**RETRACTED (R-035, R-036).** This section previously carried two claims that
measurement has now refuted, and both were load-bearing:

- *"the reference build also stops ticking productively"* — it does not.
- *"a counter that rises and a city that does not simulate are the same
  observation, so 'rising is not simulating' is the load-bearing fact, and every
  story of the shape 'make `$03:8026` run and the clock advances' is refuted by
  it"* — **refuted.** The month *did* advance six times inside the very 9 000-frame
  window that claim cites. It appeared not to because **our own driver** printed
  `$0B53` under the label `$0B55` (`c4923de`); the arithmetic was right and the
  instrument could not read it.

The consequence is the opposite of what the retraction was used for: **the story
of the shape "make `$03:8026` run and the clock advances" is no longer refuted.**
It is the shape the evidence now points at.

`INC.w $0B51` lives in bank 03 at ROM offset `0x18026` (`EE 51 0B`, SNES
`$03:8026`). What is measured about bank 03 in **our** build is much narrower
than that premise deserves:

#### `$03:8026`'s standing, in one place

- **In our build: it does not execute. 0 executions over 14 000 frames**,
  Deck-native, in the window where the reference rolls its month sixteen times
  and turns its year — and the zero is **exhaustive over both tiers**, because
  exactly one manifest node covers `$038026` and it is `lle_only` with **zero**
  `aot_eligible` nodes over it. **[MEASURED, Deck-native, T102]**
  **C-041 and C-008 stand.**
  > **RETRACTED (R-037): the measurement that previously established this was
  > watching the wrong address.** T100 and C-041c ran
  > `SNESRECOMP_COUNT_PC=038026`, and `interp816.c:323` parses with **base 0** —
  > a leading `0` means **octal**, so `038026` parsed as **`3`** and the counter
  > counted executions of **PC `$000003`**, in bank `$00`. Their zeros were
  > clean, formatted and meaningless. See instrument trap 9.
- **In the reference build: it is what moves the clock.** 27 executions, first at
  **f3857**, and `$0B51 = 4 × (months elapsed) + quarter` with **113 tick
  events, every one +1, zero deviations** — so the month rolls at exactly the 28
  events where the counter reaches a multiple of 4. **[MEASURED, Deck-native,
  T101 `9069182`]**
- **Therefore its standing is INFERRED-but-supported, and it is no longer
  refuted.** The sentence R-036 retracted was *"make `$03:8026` run and the
  clock advances" is refuted by the reference itself*. It is not: the reference
  reaches the city through the same neighbourhood and this is the instruction
  that ages it. **[INFERRED]** — the link between *this instruction executing*
  and *the date moving* is read off the ROM plus the reference's write log; it
  has never been observed as a controlled result in this build, because no
  change has ever made it execute. Making it execute is the obvious next thing
  to try and **nothing here establishes that it would be sufficient.**
- **What is NOT standing:** "the tick is absent, therefore the tick is the
  cause." Absence of execution is a measurement; it is not a cause. What *starts*
  the simulation is a different and still **OPEN** question (C-006).

- **Bank 03 executes.** 921 distinct PCs and 515 043 interpreted steps over
  frames 0–3700. **[MEASURED]**, on the Deck and on the dev host, to the unit.
  It is not code this build never reaches — it runs in boot, in attract and
  through the menus, and it is bank `$03` that **closes its own gate** (below).
- **Bank 03 goes silent at f3271, not f3301.** The earlier boundary came from
  100-frame brackets, which could only see that the f3100–3300 bracket was
  non-empty. A per-frame stream puts the **last frame containing any bank-03
  execution at f3271** — 16 interpreted steps — with none in f3272–f3700, and
  the frame-for-frame pattern is identical on both machines. **[MEASURED]**.
  `docs/CAUSE_CLAIMS.md` C-039 is retracted as stated; C-039b replaces it.
- **Bank 03's execution is bounded, and the bound has two tiers.** The
  interpreted PCs all lie in `$03C63D`–`$03E57E`; the AOT block entries all lie
  in `$03B477`–`$03C463`. Union: bank 03 executes in `$03B477`–`$03E57E` and
  **nothing** in `$038000`–`$03B46C` or `$03C464`–`$03C63C`. **[MEASURED, Deck +
  host, both tiers.]** The earlier, broader form — *"bank 03 executes only in
  `$03C63D`–`$03E57E`"* — is **RETRACTED** (ledger **R-033**): it was derived
  from "min and max of **the full dump**", and the full dump was the
  *interpreted* one. `CYC_WATCH` is blind to AOT, and all 18 bank-`$03` AOT
  entries lie **below** its own claimed `$03C63D` floor — the retracted sentence
  contradicted a figure printed a few lines away from it in the same document.
  Measured on one tier, stated about both: that is the failure, and it is why
  every row above names its tier.
- **`INC.w $0B51` at `$03:8026` — 0 hits in the 921-entry bank-`$03` dump, 0 AOT
   block entries, both machines, both tiers.** **[MEASURED]** as a count of
   execution, not an inference from a WRAM sample, with `$0B51 = 0000` at f3600
   in the same runs. This is **C-041**, and it was never exposed to the parse
   bug in instrument trap 9 — `SNESRECOMP_INTERP_DUMP_BANK` *enumerates* PCs
   rather than comparing against a parsed value, so a bad parse cannot corrupt
   it. Reconfirmed over the T102 window: `$038026` appears **0 times** in the
   333-PC bank-`$03` dump for f3000–f13080.
   **C-041b's objection to the original f0–f3700 window is answered.** The
   reference build's first tick is at **f3857** — 157 frames past the end of
   C-041's window — so the old window contained **no frame in which the
   reference would have executed the tick even once**. That is exactly the shape
   of the already-retracted f3301 bracket (C-039 → C-039b), one tier up.
   > **The 6 000-frame and 9 000-frame re-runs that answered it are RETRACTED
   > (R-037): `COUNT_PC=038026` was watching PC `$000003`.** The answer they are
   > credited with is re-measured over **14 000 frames** with the prefix, a
   > positive control, and the `lle_only` manifest argument — see
   > [`2026-10-03-t102-tick-across-f13080.md`](docs/measurements/2026-10-03-t102-tick-across-f13080.md).
   > The historical runs are kept at
   > [`2026-10-02-t100-tick-past-f3857.md`](docs/measurements/2026-10-02-t100-tick-past-f3857.md).
 - **Bank `$03` executes NOTHING AT ALL in f3272–f13080 — and that is the
   finding.** In f3000–f13080 it takes **333 distinct PCs / 169 693
   interpreted steps**; in **f3272–f13080 it takes 0 and 0**; and the AOT tier
   contributes **18 block entries, all at f3259 (3) or f3270 (15)**, none after.
   So **all 333 PCs are in f3000–f3271**, and bank `$03` is silent for **9 809
   frames** — the span holding the reference's **sixteen month rolls** and its
   **first year rollover**. **[MEASURED, Deck-native, both tiers; T102.]**
   **This is a boundary, not a cause** — and it is why the evidence now says the
   city is never *started* rather than never *advanced*. A bank that never
   executes again cannot be advancing anything, and in this window was not being
   started either. **C-046c is closed by the same run.**
 - **86% of bank `$03`'s cost in f3000–f3271 is four PCs around a scan loop**,
   and one of the four is not an instruction boundary. `$03C877` `STA $7F6B00,X`,
   `$03C87B` `INX`, `$03C87C` `CPX #$F4` — 36 344 / 36 343 / 36 344 steps; the
   next PC down is 244. **`$03C87F` is the second byte of `8D D0 F6` = `STA
   $F6D0`, whose instruction starts at `$03C87E`**, so it is reported
   **unattributed, not as an executed instruction**. `CPX #$F4` is followed by
   `STA $F6D0`, **not a branch**, so this is a scan loop's *test* and its
   back-edge is outside the logged neighbourhood. **What it scans and what ends
   it are unmeasured, and no cause is claimed. [OPEN]** — the next measurement
   is stated in
   [`2026-10-03-t102-tick-across-f13080.md`](docs/measurements/2026-10-03-t102-tick-across-f13080.md)
   §7 and tracked as T103 in [`docs/ROADMAP.md`](docs/ROADMAP.md).
 - **The city-state block is written once, at f3259, by a creation routine, and
   never written again.** Two disjoint windows, watched as a 16-bit bus write
   census (which sees both engines, so none of the tier blindness that retracted
   C-039c applies): `$0B51`–`$0B5F` takes **66 writes** and `$0DC0`–`$0DD0` takes
   **61**, each in 4–8 events, **none after f3259** — measured over **14 000**
   frames, so **10 741 frames of silence**, and the 66 and 61 reproduce C-052's
   and C-057's counts to the unit with their bounds extended from f6 000 / f9 000
   to **f14 000**. Corroborated independently by a WRAM dump: **`$0B40`–`$0DC7`
   is byte-identical at f3300 and f13000** — `$0B51=0000 $0B53=076C $0B55=0001
   $0BA5=0000 $0B9D=4E20 $0DC7=0000`. The date is not rewritten with the same
   value and the tick counter is not failing to change; the whole block is
   initialised and abandoned. The f3259 event decodes as one *new-city*
   routine: `LDA #$076C / STA $0B53`, `LDA #$0001 / STA $0B55`,
   `LDA #$0007 / STA $0DC5`, `STZ $0DC7`, `STZ $0DC3/$0DC9/$0DCB`, and the
   straight-line `STZ.w $0B51`. **[MEASURED, Deck-native.]**
   **`$0DC7` is written 4× and never accumulated into**, so the tick *routine*
   does not run either, not merely the increment. **This is where the evidence
   now points and it is not a cause** — it names what does not write the city
   state, not which code would have. The f3271 gate and the bank-`$03` death are
   both still measured, and **neither has been shown to be why the block is
   abandoned.**
- **`1900 / January` is *written*, once, at f3259.** `$03:C63F` `LDA #$076C` /
  `STA $0B53`, `$03:C646` `LDA #$0001` / `STA $0B55`, and then `$03:C77E`
  `STZ.w $0B51` — a straight-line 16-instruction clear of the city-state block.
  The logged `A` values match the ROM literals exactly. So the date is not a
  default nobody overwrote; the city setup writes it once and nothing ever
  writes it again. **`$03:C77E` is the same instruction and the same register
  state (`A=0007 X=003C S=1FF6`) the reference build logged when it zeroed
  `$0B51`** — our build reaches the writer neighbourhood the reference reaches.
  It reaches the `STZ` and not the `INC`. **[MEASURED, Deck-native]**
- **A second silent boundary nobody was watching: a 96-frame periodic updater
  in bank `$03` runs six times and stops.** `$03:D947` `STA $0B5C,X` +
  `$03:D94B` `STY $0B5B`, at f1205, 1301, 1397, 1493, 1589, 1685 — **exactly
  +96** — with `$0B5B` counting 1→6, then nothing. 1 586 frames before the
  f3271 gate. **OPEN, cause not claimed**: two boundaries are not a causal
  chain, and this is not offered as one. **[boundary MEASURED, Deck-native]**
- **The AOT side is banks 00 and 01 too.** Whole-run AOT block log: **1 430 540**
  entries over f0–f3700, of which bank `$03` has **18** — **15 in f3270 and 3 in
  f3259** (`$03C42A`, `$03C430`, `$03C463`). In the live window f3400–f3700:
  `$00` and `$01` only. **[MEASURED, Deck-native and host, identical to the
  entry]** — 84 distinct PCs, the same per-100-frame buckets to the digit. Banks
  02, 04, 05, 06 and 07: **zero** AOT entries on both machines.
  An earlier version of this file said *1 430 539* and *"all 18 in f3270"*, and
  its PC list **summed to 14 against its own stated 18** — self-inconsistent
  before any re-run, and not reproducible on either machine since. Both are
  **RETRACTED**; the Deck-native re-run is
  [`2026-10-02-deck-aot-histogram.md`](docs/measurements/2026-10-02-deck-aot-histogram.md).
- **The city appears at ≈f3378, i.e. ≈107 frames *after* bank 03 falls silent.**
  This is a **correlation**, recorded as a correlation on purpose, because the
  next session will want to write it as a cause and the number is sitting there
  looking like evidence. **No cause is claimed for it.**

Whole-run per-bank histogram (Deck-native Release build with
`-DSNESRECOMP_INTERP_PROFILE`, 4 000 frames, rc=0). Cells are
`distinct PCs / interpreted steps`; the brackets partition exactly, to the unit:

| window | `$00` | `$01` | `$02` | `$03` | `$05` |
|---|---|---|---|---|---|
| boot 0–200 | 478 / 1 664 752 | 0 | 0 | 10 / 10 | 428 / 932 195 |
| attract 201–1200 | 494 / 5 336 356 | 575 / 5 411 938 | 542 / 601 366 | 499 / 11 035 | 392 / 328 481 |
| menus 1201–3380 | 1753 / 17 808 456 | 1231 / 56 967 | 0 | 532 / 503 998 | 191 / 250 260 |
| city 3381–3700 | 813 / 2 046 776 | 415 / 261 823 | 0 | **0 / 0** | 0 |
| **total 0–3700** | 1822 / 26 856 340 | 1814 / 5 730 728 | 542 / 601 366 | **921 / 515 043** | 956 / 1 510 936 |

Banks 04, 06 and 07: zero steps in every window above — **[MEASURED]**, and
bounded by that window.

> **Environment-fidelity caveat on every Deck number above and in every other
> document in this tree.** The Deck's SteamOS rootfs is damaged in a way pacman
> does not report: of the 504 paths `pacman -Ql glibc` claims under
> `/usr/include`, **503 are absent from disk** while `pacman -Q base-devel` still
> reports installed, and `echo '#include <stdio.h>' | gcc -E -` fails with
> `fatal error: stdio.h: No such file or directory`. The Deck build therefore
> resolves libc headers from a hand-assembled prefix at `/home/deck/sysroot`
> (headers from `archive.archlinux.org`) via `-idirafter`. **So: the Deck binary
> was compiled on the Deck, against a reconstructed header prefix, on a machine
> whose rootfs is damaged. A figure measured under those conditions describes
> those conditions.** This caveat travels with every Deck number cited anywhere.

Raw counts and what each instrument cannot see:
[`2026-10-02-c041-bank03-pc-dump.md`](docs/measurements/2026-10-02-c041-bank03-pc-dump.md)
(interpreter, both machines) ·
[`2026-10-02-deck-aot-histogram.md`](docs/measurements/2026-10-02-deck-aot-histogram.md)
(AOT, Deck-native) ·
[`2026-10-02-f3271-entry-gate.md`](docs/measurements/2026-10-02-f3271-entry-gate.md)
(the `$0012` gate and the `RTI` return) ·
[`2026-10-02-t093-peer-0B51-writer-deck.md`](docs/measurements/2026-10-02-t093-peer-0B51-writer-deck.md)
(the reference build, Deck-native).

#### Why bank 03 stops: it closes its own gate (MEASURED, Deck-native)

This is the most important structural finding in the project and it is not a
cause. The gate is **`$0012`**, read at `$00:804D`:

```
$00:804D  A5 12     LDA $0012        <-- loop head, THE GATE (read twice in 3700 frames)
$00:804F  D0 0B     BNE $805C        <-- $0012 != 0  ->  bank 03 skipped
$00:8056  22 83 D2 03   JSL $03D283  <-- BANK $03 ENTRY (taken exactly once)

$03:D2A7  A9 01 00   LDA #$0001
$03:D2AA  85 12      STA $0012       <-- 15th of 16 instructions bank 03 runs in f3271
$03:D2B7  6B         RTL             <-- the last
```

`$0012` has exactly **three** writes in 3700 frames (`WLOG`), and the third is
the one: **f3271, from `$03:D2AA`, inside bank `$03`**, one instruction before
its final `RTL`. In f3271, in this order: bank 03 sets `$0012 = 1` → `RTL` to
bank 00 → `$00:804D` reads 1 → `BNE` → `$00:8061` instead of `$00:8056` →
`$00:804D` **is never executed again in the run**. **The gate was closed before
its only two readings were resolved, and the reader never came back.**
**[MEASURED, Deck-native]** — and *where control goes*, not *why the city does
not simulate*. **The older sentence here said the reference build also stops
ticking productively and that the same `$0012` mechanism is OPEN there. That is
RETRACTED (R-035): the reference build's clock runs to f30 000 and beyond — 28
month rolls, two year rollovers, `$0DC7` accumulated 128 times — and whether
`$0012` gates anything in the reference is still **OPEN**, but "it stops
ticking" was never measured and is now refuted.**

Also **MEASURED, Deck-native**: **bank `$03` is resumed by `RTI`, not called.**
`$00:8211`–`$00:8223` is a dispatcher that ends `PLB` / `RTI`, and its handler
table at `$00:8223` holds `$930D` — the NMI handler — twice. So IRQ and NMI run
through one entry, and control returns to bank 03 by restoring an interrupted
context. **"What calls into bank 03" is therefore the wrong question**: a
caller/callee census cannot see a return, and no stack dump was needed to see
this. It also leaves the bank-`$03` entry graph **OPEN**: `$03:D287`–`$03:D29B`
ran 2 311 times while `$00:8056` ran **once**.

One thing is **not** reconciled and is recorded rather than smoothed over: the
instruction at **`$03:DBB3`**, which the trace shows transferring control to
`$00:8211`, reads as `02 00 60` = `JMP $036000`, and `$03:6000` **never
executes**. Either the ROM read at that address, the `[itb]` PC attribution for
that one step, or an interrupt taken between two logged steps is wrong. **OPEN**,
and no claim above depends on it.


### The open question, and what it is not

**Why does the city not simulate? [OPEN].** It is the only row in
`docs/CAUSE_CLAIMS.md` with no instrument, and no cause for it is asserted
anywhere in this tree.

**The question has changed shape twice, and the second change is the finding.**

First: the code believed to advance the city clock was measured not to execute
inside f0–f3700, so "why does bank 03 stop at f3301" gave way to "bank 03 stops
at f3271, and it closed its own gate on the way out" — **where control goes**,
still not a cause.

Second, and harder: **the question "what advances `$0B51` in the reference build,
if not `$03:8026`?" is now ANSWERED and it does not help.** The answer is
`$03:8026` — 27 executions, first at f3857, measured Deck-native and reproduced
on the host. And in that same run **the date never moved.** So the framing
"our tick does not run, the reference's does, therefore that is the difference"
is **refuted by the reference build itself**: the reference reaches the city
through the keyboard route, ticks 27 times, and — **this was read wrong, see
below** — appeared to simulate nothing.

> **RETRACTED (R-036): the premise that `$03:8026` running would NOT fix the
> clock had counterevidence, and the counterevidence was our own instrument.**
> `$03:8026` running **does** fix the clock. In the reference it is what moves
> the date. The story it was used to kill is the story the evidence now supports.

Two peer runs appeared to disagree about whether the reference simulates at all.
**They never did.** Reconciled, Deck-native, in
[`2026-10-02-t101-reference-simulates.md`](docs/measurements/2026-10-02-t101-reference-simulates.md):

| peer run | script | window | `$0B51` | date | population |
|---|---|---|---|---|---|
| older WRAM trace | a live city reached by that route | f33700 | `0000 -> 0080` | `076C -> 076E`, **23 months** | 0 |
| T093 write-watch | `scripts/d_city_kbd.script`, 41 presses | f9000 | `0000 -> 001B`, 27 ticks | `$0B55 = 07` — **six month advances** | **0** |
| **T101, clean core, same script as T093** | `scripts/d_city_kbd.script` | f30000 | `0000 -> 0071` | **`076C` → `076E`, 28 month rolls, 29 date images** | 0 |

**[MEASURED, Deck-native]** The T093 row was reproducible byte for byte —
`master_clock = 3216243544`, `insns = 107365572`, identical to its own record —
so it is the **same execution**. At f9000 the true `$0B55` is `07` (AUGUST). The
log printed `076C` because **`study/peer-linux/jjhead.c` clobbered the month
column and printed `$0B53` under its label** (`c4923de`). The month advanced six
times inside the disputed window; `AND #$0003` extracts the quarter, so **27 ticks
are `6 × 4 + 3`**, which is exactly where the predicted ≈6 came from.

The write-watch is **not** the cause: the clean and watched cores produce
**byte-identical** 30 000-frame timelines under `cmp`, with the watch
demonstrably live (230 rows, f0→f29967). f9 000 *is* too short — **for the
year**, whose first rollover is at f13080. That half of the old reading was right
and was over-read.

**"The reference simulates and we do not" is therefore available as a premise,
and it holds.** It is no longer an assumption this project is leaning on.
**[MEASURED, Deck-native]** — the reference row is a measurement; the "we do not"
half is C-041c/C-058.

**And that next measurement has now been made — [`2026-10-02-t100-tick-past-f3857.md`](docs/measurements/2026-10-02-t100-tick-past-f3857.md), Deck-native.** Over 9 000 frames, a window containing **12** of the reference's own ticks, `$03:8026` executes **zero** times. So the tick genuinely does not run here — that part is now a measurement rather than a window artefact. **C-041 stands.**

> **RETRACTED (R-036), and it is the sentence this paragraph used to end with.**
> It read: *"the fix-shaped story is correspondingly weaker, not stronger:
> making it run is a necessary-looking change whose sufficiency has counterevidence
> in the reference build."* There is no such counterevidence. The reference build
> **does** execute `$03:8026` and **its date does advance** — 28 month rolls in
> 30 000 frames, verified at every one of 113 tick events. The story is
> **stronger** than this paragraph claimed, and it is the shape the evidence
> points at. **What is still true and is stated here so the retraction does not
> become a claim in the opposite direction: making the instruction execute is
> [INFERRED], not [MEASURED], to be sufficient.** Nothing in this repository has
> ever made it execute, so no controlled result exists. It is the obvious next
> thing to try; it is not a demonstrated fix.

What the same runs established instead is a mechanism the project had never seen: **the city-state block is written once, at f3259, by a creation routine, and never written again** — 66 writes in 6 000 frames on `$0B51`–`$0B5F`, 61 in 9 000 on `$0DC0`–`$0DD0`, none of either after f3259. Nothing rewrites the date with the same value; nothing writes it at all. That is a measurement of *where* the freeze lives and **still not a cause** — a write census cannot see the code that would have written.

The retracted claims, each marked where it was made:

- **"The bank-03 tick is compiled to native C and still does not run"** —
  **RETRACTED 2026-10-02 (review finding R-03).** Bank 03 executed 515 043
  interpreted steps. The surviving, much narrower form is the f3301 boundary
  above, which is a location and not a cause.
- **"Therefore `INC.w $0B51` executes zero times"** — the *inference* was
  **withdrawn, not replaced** (review finding R-02; ledger R-020,
  `invalidated-premise`). Its premise (`$0012 == 0`) is false, and a refuted
  premise voids an inference and establishes **nothing in its place** — which is
  not a claim that it does run. The *conclusion* has since been reached by a
  different route and now stands on its own: **MEASURED**, zero executions in
  f0–f3700 on both tiers on both machines (C-041, C-008) — **with C-041b's
  caveat that the window stops 157 frames short of the reference's first tick.**
- **"Bank 03 executes only in `$03C63D`–`$03E57E`; `$038000`–`$03C63C` executes
  nothing"** — **RETRACTED** (ledger **R-033**). Derived from the interpreted
  dump alone; all 18 bank-`$03` AOT entries lie in `$03B477`–`$03C463`, below
  its own claimed floor. The narrow two-tier form (C-039d) replaces it.
- **"The whole-run AOT histogram is 1 430 539 entries and bank `$03`'s 18 are all
  in f3270"** — **RETRACTED**. The Deck-native re-run gives **1 430 540** on both
  machines, **15 in f3270 + 3 in f3259**, and the original's own PC list summed
  to 14 against a stated 18. Self-inconsistent before any re-run, and
  unreproducible on either machine since.
- **"`$00:8023` = `95 00` = `STA dp,x` is a fourth writer of `$0B51`"** —
  **RETRACTED** (ledger **R-034**), on three independent grounds: `95 00` is at
  `$00:8024`, not `$00:8023`; the logged next-PC `$00:8025` matches neither a 2-
  byte instruction at `$00:8024` (whose next PC is `$00:8026`) nor anything at
  `$00:8023` (a `BRA`); and decisively **frame-0 writes == watched span exactly**
  (1→1, 4→4, 64→64, 256→256 — **no 65816 instruction writes 256 consecutive
  bytes**). The frame-0 hits are a **block memory initialisation sweep**, not an
  instruction. The methodological half — that an operand-byte census is
  structurally blind to a `dp,x` store — remains true but is **now unevidenced
  here**, and is recorded as such.
- **"The main loop does not run at all in a city"** — **RETRACTED as stated**,
  same measurement as R-03.
- **"The gate is `$0012`"** — **RETRACTED.** `$0012 = 0001` and `$0014 = 8000` at
  5/5 frame-boundary samples; its own stated evidence is what measurement refuted.
- **"The city does not load"** — **RETRACTED.** It does load; the runs behind that
  claim used a deliberately truncated `save.srm` from `scripts/clock-gate.sh`.
- **"Refusing to decode COP is the cause"** — **RETRACTED.** No word anywhere in
  the ROM points into `$038000–$038220`.
- **`CODE_008061` "never runs"** — **RETRACTED as stated.** It runs, at
  `$00:8061`, on both machines, immediately after bank 03's last `RTL`. It had
  sat at OPEN with the note "execution never measured", which is a fact about the
  instrument rather than about the world.

A 9 000-frame settle protocol on the Deck (162.5 s wall, real save
`24720bb57ff09426d588da564fea6c18`) read `1900/1` at **every** snapshot,
`$0BA5` = 0, `$0B9D` = 20 000, with 19–34 bytes changing per snapshot (0.02%).
**[MEASURED]** — a real save state does not advance the clock either.

### Where the record lives

This file is the entry point and is **not** the maintained record. Ten files
are:

| file | what it holds |
|---|---|
| [`docs/CAUSE_CLAIMS.md`](docs/CAUSE_CLAIMS.md) | every causal claim in this project, classified MEASURED / INFERRED / RETRACTED / OPEN, with the instrument or the reason there is none |
| [`docs/CONFLICTS.md`](docs/CONFLICTS.md) | every contradiction found between docs, code and git history, with the command that found it |
| [`docs/RE_CITY_FREEZE.md`](docs/RE_CITY_FREEZE.md) | the chronology — 44 entries, each with a state banner, and a maintained index at the top |
| [`docs/ROADMAP.md`](docs/ROADMAP.md) | **the tracked mirror of the board.** `aes/kanban.md` is not versioned, so this is what a clone gets; where they disagree, this file wins, because that is what a clone actually receives |
| [`docs/measurements/2026-10-02-deck-interp-histogram.md`](docs/measurements/2026-10-02-deck-interp-histogram.md) | the whole-run histogram this file's table reproduces, and the three instrument limits it established |
| [`docs/measurements/`](docs/measurements/) | raw measurements, the exact commands, and **what each instrument cannot see** |
| [`docs/CLAIMS_REGISTER.md`](docs/CLAIMS_REGISTER.md) | the index of what is retracted, superseded or unverified |
| [`docs/DEFINITION_OF_DONE.md`](docs/DEFINITION_OF_DONE.md) | the standard of proof — **no acceptance criterion may be satisfied by a claim** |
| [`docs/review/RUBRIC.md`](docs/review/RUBRIC.md) | the pre-registered review rubric (hash-pinned in `RUBRIC.sha256` — **do not edit**) |
| [`docs/review/REVIEW-2026-10-02b.md`](docs/review/REVIEW-2026-10-02b.md) · [`-02c.md`](docs/review/REVIEW-2026-10-02c.md) | the last two full reviews — **REJECT, 3 BLOCKERs**, all since closed; and the review of the C-041 work |

### The peer review of T101, and its standing

**It is not an approval, and this file does not treat it as one.** The review of
the T101 result was run in **multi-perspective fallback** mode — **one agent,
one model family, no independent reviewers available**. Its **four personas are
four stances written by the same model in the same session and are explicitly
not independent**; the review says so at the top and it is not something a
script can settle. Its own verdict is **REJECT WITH CONDITIONS** (0 BLOCKER,
1 MAJOR and 1 MINOR still open), and per that protocol the candidate stays
**CANDIDATE** until **someone who did not author the session** runs
`aes/peer-reviews/T101/validate.sh` and records the output. `aes/` is
gitignored, so that script is **not in a fresh clone** — a clone inherits the
caveat and not the validator.

**What the review did change:** two findings were closed by measurement, not by
argument — the `$0B51` law was re-derived at **event** granularity (**113 tick
events, every one +1, zero deviations**) instead of at 60-frame sample
granularity, and the driver defect was attributed to us with its commit. **What
it did not change:** nothing in this file rests on the review's judgement. The
numbers came from the Deck.

## Gates

**Every figure names the machine and the date it was measured on.** A gate result
without a machine is not a measurement — see `docs/DEFINITION_OF_DONE.md` Rule 0.

Measured on the dev host `seyon` (i5-8500T, 6 threads, Ubuntu 24.04, gcc 13.3.0,
cmake 3.28.3) on 2026-10-02, **re-run in full at `be23ec3`** — every row below
is that re-run, not a carry-over:

- `make perf` is the only row that has ever moved between trees, and it moves
  with load, not code: median **54.50 fps / spread 9.6%** at `8a7340f` (load
  average 7.55 on 6 threads), **53.68 / 5.6%** at `4ba14c7`, **53.30 / 2.9%**
  at `be23ec3` (load average 7.81 on 6 threads when the gate started). Every
  other row is byte-identical across all three trees, which is the useful part.

| command | what it proves | result | exit |
|---|---|---|---|
| `make build` | Release build | ok | 0 |
| `make test` | deterministic replay (ctest) | **2/2 passed** (`test_deterministic_replay` 1.32 s, `test_display_aspect` 0.00 s) | 0 |
| `make test-rom` | the picture moves (frames 200–800) | **PASS, 800 frames presented, 257 distinct crc32, peak luma 41.751** | 0 |
| `make perf` | gross frame-rate floor (5 × 600 frames) | **PASS, median 53.30 fps, spread 2.9%** (52.03 … 53.59) | 0 |
| `make clock` | **the city actually simulates** | **FAIL — `1 distinct date images after f3600 (last change f3378 of 6000)`**, `$0B53 = 076C` → year 1900 | **1** |
| `make check-claims` | no retracted claim asserted without a marker | PASS | 0 |
| `make check-causes` | every causal assertion carries provenance | PASS | 0 |
| `make check-causes-self-test` | the guard still fires on the tree it was written for | PASS | 0 |
| `make check-claims-self-test` | the ledger guard has been seen to fail | PASS — both seeded violations confirmed detected | 0 |
| `make review-check` | the 2026-10-02 review's BLOCKERs are closed | **PASS — 17 confirmed, 0 refuted**; 3 ROM-dependent checks skipped (no `--rom`) | 0 |
| `make review-check-c041` | the C-041 review's claims reproduce | **PASS (bounded) — 26 confirmed, 0 refuted**; it refuses to total, and rubric **E-04 stays UNVERIFIED** | 0 |
| `make clock-self-test` | the clock detector still sees a live screen | PASS — 16 distinct date images over 1 200 frames, last change f1163 | 0 |
| `make retraction-count` | the retraction count, computed | **37 rows = 29 refuted + 6 superseded + 2 invalidated-premise** | 0 |

**`make clock` exits 1, not 2**, and the distinction is load-bearing: the gate
uses exit 1 for "a city is loaded and its date did not advance" and a *different*
exit-1 verdict with its own message for "no city was loaded at all". There is no
exit-2 path in `scripts/clock-gate.sh` — exit 2 belongs to `make perf`'s
`INCONCLUSIVE` and to `clock-gate.sh --help` on an unknown flag. **Red is red;
the exact code is recorded here so nobody has to guess which failure they are
looking at.**

**`make check-causes` was red for four commits and said so nowhere.** It fired on
the bare word `date` inside "candi**date**" on line 30 of its own header, so it
could never be green while that header stood — and the commit that added it
(`8a7340f`) said in its message and in DoD D3.7 that it was green. The fix is a
word boundary, which cannot lose a real match, and it was falsified three ways
before it was committed (revert the boundary → red again; a synthetic unlabelled
causal sentence still fires; the same sentence labelled `HYPOTHESIS` does not).
T094 carries the transcript. **The lesson is not the regex**: it is that a gate
added in the same commit as the claim it guards, and never run, shipped broken
and reported green.

**`make check-claims` does not check the number, and that hole is open (CONF-14).**
It reported `(no violations)` while this very table carried a stale **26
refuted** against a ledger of 28 — because its count guard matches `"N
retractions"` and the ledger's own phrasing is `"N refuted"`. **Every count in
this file, including the one in the row above, is therefore unverified by any
gate.** The repair was written, and it produced **three false positives on this
project's own corpus** (two markdown table rows whose trailing cell is `| 0 |`,
and the guard's own header sentence), so it was **reverted**; the failed attempt
is recorded in `scripts/check-retracted-claims.sh`'s comment block where the next
person meets it. A guard that cries wolf on its own corpus is worse than the
hole it closes. **This is why the count is stated as `N rows = M refuted + …`
and re-read after running `make retraction-count`, rather than typed.**

**`make clock` is the one that matters and the one that is red.** It fails
identically on the Deck. It prints the guest's own year word as proof that a city
object exists, so it **cannot report PASS on a build that loads no city** —
verified by pointing it at a script that never reaches one, which it refuses with
a distinct "no city was loaded" verdict.

**The other gates pass while the game is a still image, and they pass for the same
reason each: they all measure before the city exists.** The first three inspect
frames 30–800 and are still on the attract screen and the menus.

**`make perf` is a floor, not a headroom figure.** It was reworked on
2026-10-02 to use the **median** of five runs plus a spread check, because
worst-of-N on a quantity whose noise is symmetric converts ordinary variance into
failures: the same unchanged binary previously flapped **48.38 FAIL / 51.52 PASS
/ 46.99 FAIL** against a threshold of 50. It still has a spread guard — over 10%
is `INCONCLUSIVE` (exit 2), not PASS — and the run measured above sat at 9.6%,
which is close enough to the guard to be worth stating: **that host had a load
average of 7.55 on 6 threads while the gate ran**, and run 5 of 5 fell to 49.66
fps. On the Deck, the five runs of the **previous** worst-of-5 method finished in
exactly 10.549 s — to the millisecond, five times — so Deck pacing is frame-locked
and that gate could not detect guest slowdown there at all.

### Performance

`make perf` measures it and fails on a regression. Numbers, same ROM, 600
frames:

| Machine | Build | fps | `guest` ms/frame | `upload-present` ms/frame |
|---|---|---|---|---|
| Steam Deck (Zen 2), **compiled on the Deck** — *environment-fidelity caveat above* | Release | 56.88 | **4.502** | **1.007** |
| i5-8500T, built in place | Release | 54.50 median (5 runs) | 6.62–10.82 | — |
| i5-8500T, cross-built binary | Debug (`-O0`) | 42.4 | 9.52 | 5.90 |

**Three claims about this table were retracted, and the reasons are instructive.**

- The Deck row's `2.45` ms was **never re-measured** and is ledger **R-012/R-021**.
  The measured Deck figure is **4.502 ms**. The old row also claimed
  `upload-present` **8.13 ms**, measured on the Deck at **1.007 ms** — the
  opposite side of the guest by a factor of four and a half.
- **"The emulated 65816 is not the bottleneck"** is **RETRACTED** (ledger R-023,
  R-025). It rested on `guest` being 2.45 ms against a large `upload-present`.
  Measured on the Deck the picture is the reverse: guest 4.502, upload-present
  1.007, **deadline-wait 11.275** — **pacing dominates, not the CPU** [MEASURED,
  Deck].
- The **"upload-present costs 6.8× the guest"** figure that replaced it was an
  artifact of `SDL_VIDEODRIVER=dummy` **on the dev host**, not a property of the
  code. Neither number survives.
- The `-O0` row (42.4 fps) is the one part that still stands, and it is why
  `make build` ships Release.

The per-stage split comes from `SNESRECOMP_HOST_PROFILE=1`, which writes
`video profile: stage=...` lines into `last_run_report.json`. `guest` is the
emulated CPU; `upload-present` is the host's present path. They are different
costs with different owners.

A healthy build is vsync-capped at 60 fps and so has **no headroom visible in the
fps figure at all** — faster hardware would not move it. `guest` ms/frame is the
number that shows headroom.

## Requirements

### Any Linux (Debian/Ubuntu, Fedora, Arch, SteamOS)

- `base-devel` equivalent — a C/C++ toolchain and `make`:
  **Debian/Ubuntu** `build-essential`; **Arch/SteamOS** `base-devel`
- **`cmake`** ≥ 3.20 (measured: 3.28.3 on Ubuntu 24.04, 4.0.3 on the Deck)
- **`git`** — the `snesrecomp` submodule is required
- **`pkgconf`** (`pkg-config` on Debian/Ubuntu) — SDL3's configure probes it

**No SDL package is needed.** `snesrecomp/runner/runner.cmake` looks for an SDL3
package, finds none, and **fetches and builds SDL 3.4.10 from source, static**
(`SNESRECOMP_SDL3_FETCH=ON`, tarball hash-pinned). Pass
`-DSNESRECOMP_SDL3_FETCH=OFF` to require an installed SDL3 instead.

**`libxtst-dev` is not needed** as long as `-DSDL_X11_XTEST=OFF` is passed, which
`make build` already does. Without it a clean build directory stops with
`Couldn't find dependency package for XTEST`, because SDL3 enables XTEST — its
synthetic-input extension — by default and nothing here uses it.

For a **windowed** build on X11, the SDL3 configure additionally resolves the
X11, Xcursor, Xrandr, XInput, XFixes, Wayland, EGL/GL, ALSA, libudev and D-Bus
development packages through `pkg-config`. All of them were present on the dev
host (measured: `x11 1.8.7`, `xext 1.3.4`, `xcursor 1.2.1`, `xrandr 1.5.2`,
`xi 1.8.1`, `xfixes 6.0.0`, `wayland-client 1.22.0`, `egl 1.5`, `gl 1.2`,
`alsa 1.2.11`, `libudev 255`, `dbus-1 1.14.10`). The headless configuration
below needs **none** of them.

### SteamOS / Arch, and the damaged-rootfs caveat

Measured on the Deck on 2026-10-02: `base-devel 1-2`, `cmake 4.0.3-1`,
`git 2.50.1-3`, `pkgconf 2.5.1-1` — and that is the whole dependency list for the
headless build. `libxtst-dev` is **not** installed and not needed.

⚠️ **The Deck's rootfs is damaged in a way pacman does not report.** `pacman -Q
base-devel` and `pacman -Ql glibc` both report success, but **503 of the 504
paths pacman claims under `/usr/include` are absent from disk**, and

```
$ echo '#include <stdio.h>' | gcc -E -
<stdin>:1:10: fatal error: stdio.h: No such file or directory
compilation terminated.
```

(`gcc -x c -E -` fails identically; only the *file* `/usr/include` directory entry
itself still exists.) There is no sudo and no cached glibc, so it cannot be
repaired there. The Deck build works around it with a hand-assembled header
prefix at `/home/deck/sysroot` and `-idirafter` — deliberately **not**
`-isystem`, which sorts *before* `/usr/include` and breaks libstdc++'s
`#include_next <stdlib.h>`. **Every Deck number in this repository inherits that
caveat.**

> ⚠️ **The prefix must be passed to BOTH compilers.** An earlier revision of this
> file attributed the Deck trace tier's failure to link to the prefix being
> "partial … not a libstdc++ one" (ledger row **R-032**, CONF-9). That is false:
> `g++ -idirafter /home/deck/sysroot/usr/include` compiles `<cstdlib>` with rc 0,
> and `stdlib.h` is **present** in the prefix. The actual defect was that
> `-DCMAKE_C_FLAGS` carried `-idirafter` and `-DCMAKE_CXX_FLAGS` was left empty,
> so every `.c` unit built and the first `.cc` unit did not. If you see a Deck
> build fail in a C++ file only, diff the two flag variables before you blame the
> rootfs. The measured recipe is `scripts/deck-trace-build.sh`.

**Both tiers build natively on the Deck, and the trace tier is not optional.**
`scripts/deck-trace-build.sh` configures the instrumented tier
(`-DSNESRECOMP_TRACE_BUILD=ON -DSNESRECOMP_INTERP_PROFILE=1 -DSNESRECOMP_TRACE=1`
on **both** `-DCMAKE_C_FLAGS` and `-DCMAKE_CXX_FLAGS`, plus the header prefix on
both) and ends in a **guard that refuses a mute build as success**: it counts the
`[aotblk]` lines the link produced and fails if that count is zero. Measured
`[aotblk] lines in f1-f50 = 9771` → `OK, trace tier links and emits on this
machine`. **[MEASURED, Deck]**

**Project policy: every heavy run and every trace/instrumented build happens on
the Deck.** A host build of an instrumented tier is labelled `HOST-ONLY` and
closes nothing. That label is not a formality — the AOT histogram carried it for
a day and a half, and a host-only T093 answer turned out to contain a wrong
writer that only a second, Deck-native run could expose (R-034).

### macOS / Windows

Not measured on either; the framework carries the paths and nothing in this
repository has been verified on them. Treat this section as untested rather than
as support.

### The ROM

`SimCity (USA).sfc`, **user-supplied and never committed**. MD5
`23715fc7ef700b3999384d5be20f4db5`, exactly **524 288 bytes**; `rom_identity.txt`
carries the digests and the build turns them into `snesrecomp_rom_identity.h`.
`git ls-files | grep -ci '\.sfc$'` must be `0`.

## Building

`build/` is **not** committed — it holds a full SDL3 build and no binary belongs
in the repository.

```bash
git clone --recurse-submodules https://github.com/linuxkafe/SNES-SimCity-Recomp
cd SNES-SimCity-Recomp
make build
```

`make build` is Release (`-DCMAKE_BUILD_TYPE=Release`) and already passes
`-DSDL_X11_XTEST=OFF`; by hand that is:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DSDL_X11_XTEST=OFF
cmake --build build --parallel
```

`make debug` is the same with `-O0`. The two were proven byte-identical on the
guest before the default changed — same 128 KB WRAM image and same presented-crc32
column across attract, menu and naming at 3 000 frames, with cartridge SRAM
pinned cold on both sides. **That equivalence is proven for those paths, not for
all time and all input**, so keep `test_deterministic_replay` in the loop if you
switch back and forth.

### Headless build (no X11, no Wayland)

For a machine with no display server, or one whose window-system headers are
missing, add:

```bash
-DSDL_X11_XTEST=OFF \      # no libxtst-dev: XTEST is unused (synthetic input)
-DSDL_X11=OFF \            # no X11 headers/libs: no window on this machine
-DSDL_WAYLAND=OFF \        # no wayland-protocols / wayland-scanner
-DSDL_UNIX_CONSOLE_BUILD=ON # console (not launcherd) SDL backend on Unix
-DOPENGL_INCLUDE_DIR=/path/to/your/reconstructed/usr/include \  # e.g. /home/deck/sysroot/usr/include
-DOpenGL_GL_PREFERENCE=LEGACY  # GL headers shipped for this box, not the newest profile
```

`SDL_UNIX_CONSOLE_BUILD=ON`, `OPENGL_INCLUDE_DIR` and `OpenGL_GL_PREFERENCE=LEGACY`
are the three the Deck needed *in addition* to the display-system switches, and
they are the ones whose reason is a property of the machine rather than of this
project. On an intact Linux box the first four are enough; the last two are for
the damaged-rootfs case above.

Note: the `snesrecomp` submodule is pinned to a small fork
(`linuxkafe/snesrecomp`) with the SimCity host runtime additions
(debug/watchdog globals, recomp stack depth).

### Running it

Then run with your own `SimCity (USA).sfc`, **by absolute path**: the host chdirs
to the executable directory, so a bare relative filename resolves there, not in
your shell's working directory.

```bash
./build/SimCitySNESRecomp "$PWD/SimCity (USA).sfc"
```

## Running

```bash
# Resolution presets — pin the window to a fixed display size
SNESRECOMP_RESOLUTION=720p ./build/SimCitySNESRecomp "$PWD/SimCity (USA).sfc"   # 1280x720
SNESRECOMP_RESOLUTION=800p ./build/SimCitySNESRecomp "$PWD/SimCity (USA).sfc"   # 1280x800
SNESRECOMP_RESOLUTION=1080p ./build/SimCitySNESRecomp "$PWD/SimCity (USA).sfc"  # 1920x1080
SNESRECOMP_RESOLUTION=2560x1440 ./build/SimCitySNESRecomp "$PWD/SimCity (USA).sfc"  # raw WxH also works

# Headless
SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy ./build/SimCitySNESRecomp "$PWD/SimCity (USA).sfc"

# SNES Mouse on player 2 (opt-in; host cursor hidden, guest cursor takes over)
SNESRECOMP_MOUSE=1 ./build/SimCitySNESRecomp "$PWD/SimCity (USA).sfc"

# Input diagnostics — manual+auto read depth, auto word seen by the guest
SNESRECOMP_MOUSE=1 SNESRECOMP_PAD_PROBE=1 ./build/SimCitySNESRecomp "$PWD/SimCity (USA).sfc"
```

## Controls

All defaults below; every binding is rebindable through a `keybinds.ini`
`[KeyMap]` beside the executable (dump what a build resolved with
`SNESRECOMP_KEYMAP_DUMP=1`).

**Quick save / load** (10 slots)
- `F1`–`F10` — quick **load** slot 1–10
- `Shift+F1`–`Shift+F10` — quick **save** slot 1–10
- `F11` — save-state menu (thumbnail browser)
- `F12` — rewind filmstrip (≈60 snapshots, ~6 s of history)
- Controller: **Select + R** opens the save-state menu

**Time and speed**
- `Tab` — **turbo**: run the simulation as fast as the machine allows and
  render every 16th frame (use to fast-forward a growing city)
- `P` / `Shift+P` — pause (paused screen dimmed/not)

**Other**
- `Ctrl+R` — reset
- `Alt+Enter` — fullscreen toggle
- `Alt+W` — widescreen toggle
- `F` — display performance readout
- `Keypad +` / `Keypad −` — volume up/down

Save-state slots are stored as `saves/save1.sav` … `saves/save10.sav` in the
`saves/` directory beside the executable; battery SRAM (the in-game "SAVE" /
"LOAD" menus) uses `saves/save.srm`.

## Mod Support

The recompiled core stays **byte-deterministic**: acceleration or save/load
only changes *how many* simulated frames run or when the timeline jumps —
never the state of any single frame.

**Built-in QoL toggles** (environment variables, off by default):
- `SIMCITY_WIDESCREEN=1` — widen the isometric view (336 px frame)
- `SIMCITY_GODMODE=1` — infinite money, instant build
- `SIMCITY_DISASTER_TOGGLE=1` — disable disasters; `=2` force random ones

```bash
SIMCITY_GODMODE=1 ./build/SimCitySNESRecomp "$PWD/SimCity (USA).sfc"
```

**Mod packages (`.snesmod`)**: the framework's package loader is enabled and
stages `mods/preloaded/packages/` beside the executable; a launcher Mods page
installs and uninstalls packages. This port ships **no** packages by default
(its own mods are the `SIMCITY_*` toggles above). The package format is
documented in the framework at `snesrecomp/docs/MOD_PACKAGES.md`.

## Debug Flags

- `SIMCITY_DEBUG_WATCHDOG=1` — watchdog handler with CPU state dump
- `SIMCITY_DEBUG_DMA=1` — DMA transfer logging with source validation
- `SIMCITY_DEBUG_APU=1` — APU sync timing logs
- `SNESRECOMP_HOST_PROFILE=1` — per-stage host cost (`guest`, `upload-present`,
  `deadline-wait`) into `last_run_report.json`

### Instruments, and nine traps in them

The facts below are the most transferable result in this repository, and every
one of them cost a wrong conclusion first. They are here so the next session
does not pay for them again.

0. **Every counter instrument needs a positive control, and the control must be a
   PC that is known to execute.** A zero from a counter nobody has ever seen
   read non-zero **for the address it was given** is not a measurement. This is
   the rule that found trap 9, and it cost a retraction. See trap 9 for the
   numbers; the rule is the transferable half.
1. **Our own driver printed `$0B53` under the label `$0B55`, and it cost this
   project its central result for a day.** `study/peer-linux/jjhead.c` clobbered
   the month column of its own WRAM dump. The consequence was not a typo: a
   correct measurement (`$0B55 = 07`, **AUGUST**, at f9000) was read as
   `$0B55 = 076C`, which is *`$0B53`*, i.e. the year. The month had advanced
   **six times** inside that very window. So a live, correctly-reproducing
   reference run was recorded as *"the reference does not simulate"*, and two
   sentences were written on the strength of it — **R-035, R-036** — one of
   which was then used to kill the correct theory (*"make `$03:8026` run and the
   clock advances" is refuted*). **The arithmetic was right; the instrument could
   not read it.** Fixed in `c4923de`.
   **The generalisable form: an instrument that cannot read the answer looks
   exactly like an instrument that measured a negative.** Re-running the disputed
   configuration reproduced it byte for byte (`master_clock=3216243544
   insns=107365572`) and printed the right number — so "reproducible" and
   "correct" are two different properties of a measurement.
   **And the same section's other lesson: the write-watch was inert.** Clean and
   watched cores produce `cmp`-IDENTICAL 30 000-frame timelines, and the watch
   log holds 230 rows f0→f29967, so an inert instrument and a perturbing one are
   also indistinguishable from the outside. An instrument is proven live by
   showing it *changing something*, never by its output existing.

2. **`CYC_WATCH` is blind to AOT.** Its hook,
   `snesrecomp/runner/src/snes/interp_bridge.c:2034`, sits inside
   `_interp_run_core` (`:1009`), the per-interpreted-opcode loop, and compares
   against `pc_before`; the AOT side lives in `cpu_trace_block`, which is a
   **no-op** unless `SNESRECOMP_TRACE=1`
   (`snesrecomp/runner/src/cpu_trace.h:1316`) — the default is off.
   Blindness test: `CYC_WATCH=1C700-1C7FF` logged 28 002
   hits, all in f3094–f3103, and **zero** in f3400–f3410, while `AOTBLK=3400-3410`
   logged 11 267 entries at overlapping PCs in exactly those frames.
   **A zero from `CYC_WATCH` proves nothing about AOT execution.**
   **The same blindness belongs to `SNESRECOMP_COUNT_PC`**, which sits in
   `interp816_runOpcode` (`snesrecomp/runner/src/snes/interp816.c:317`) — the
   interpreter opcode loop, for the same reason. A count from it is a statement
   about the **interpreter tier** unless you have separately established that the
   watched address is not compiled AOT. For `$03:8026` that has been
   established, from `src/gen/program_manifest.json`: exactly **one** node
   covers it, `$038000:M1X1` spanning `$038000`–`$03815D`, and its disposition
   is **`lle_only`** — so the interpreter counter is exhaustive over *both*
   tiers for this address. **[MEASURED, from the manifest, machine-independent]**

3. **`AOTBLK` was never mute — it takes a frame window, not a PC range.**
   `snesrecomp/runner/src/cpu_trace.c:1244` `sscanf`s `"%ld-%ld"` against
   `snes_frame_counter`; `CYC_WATCH` (`interp_bridge.c:2036`) `sscanf`s
   `"%lx-%lx"` against `pc_before`. Passing `38000-381FF` asked for frames
   38000–381FF.

4. **`SNESRECOMP_INTERP_PROFILE` was exposed by no CMake option**, which is why
   the interpreted histogram had never been run in a normal build. Configure with
   `-DCMAKE_C_FLAGS="-DSNESRECOMP_INTERP_PROFILE=1"`. Note also that its
   `[interp_profile] … top 60 by host-ms` list prints **nothing** unless
   `SNESRECOMP_INTERP_MS_PROF=1` is set: without it every entry's `ms` is `0.0`
   and the leading sort slots are unused hash-table entries.
   **A section header with no rows under it is not a negative result.**

5. **Every AOT-side instrument is unreachable from a default build.**
   `SNESRECOMP_AOTBLK="lo-hi"` over 4 000 frames logged **zero** lines and no
   warning: `cpu_trace_block()` is an empty `static inline` unless
   `SNESRECOMP_TRACE=1`, and with that define the link fails on 24 undefined
   references into `debug_server.c`, which no CMake option in this repository
   added. A knob that accepts its variable and prints nothing is worse than one
   that is absent. `-DSNESRECOMP_TRACE_BUILD=ON` (default **off**) now adds
   `debug_server.c` and links pthreads, which is what makes trap 5 avoidable.
   **And it is avoidable on the Deck too** — that was long believed otherwise on
   a false cause; see the warning in *Steam Deck* above, ledger row **R-032**, and
   CONF-9. `scripts/deck-trace-build.sh` builds it there and asserts the
   `[aotblk]` count is non-zero, because a link that produces no trace output is
   not a working instrument.

6. **On the Steam Deck, `exit: SDL_QUIT` means you signalled it — not that the
   game did.** A SIGTERM reaching the host surfaces as `SDL_QUIT event after N
   frames`, with no crash and no guest-side cause. Four runs died at 1 414, 1 425,
   1 428 and 1 430 frames, and the death time tracked the supervising `timeout`
   value to within 0.5 s in every case; a fifth, run in the foreground and killed
   by the operator's own `timeout 130`, died at 129.997 s. **Only
   `[host +T s] exit: RUN_FRAMES reached` is a clean completion.** A truncated
   trace is not a smaller trace — every dead run above still produced 250 000+
   `[aotblk]` lines and a plausible per-bank split, which is precisely how a
   partial census gets reported as a whole one. Run long Deck measurements in the
   foreground of a live ssh session with a `timeout` well clear of the expected
   duration, and log to `/dev/shm` (tmpfs, 7.2 GB free) rather than `/home`, where
   the same binary ran at ~4 300 lines/s against 154.2 s for the full 4 000 frames
   to `/dev/null`. Measured in
   `docs/measurements/2026-10-02-deck-aot-histogram.md` §4.
   **Three runs backgrounded with `setsid nohup … &` died with no error, no core
   and no exit status at all** (CONF-13) — the same failure with one fewer clue.
   *Foreground every heavy Deck run through `ssh` and read `EXIT=` before you read
   the log.*

7. **`SNESRECOMP_COUNT_PC` prints nothing unless `SNESRECOMP_PHASE_MS` is also
   set.** It counts executions of one 24-bit PC and the counter works; the only
   `fprintf` that reports it sits inside `interp_profile_dump_atexit()`, whose
   first statement is `if (!HostGetenv("PHASE_MS")) return;`
   (`snesrecomp/runner/src/desktop/host_main.c:2535`). So the knob alone produces
   a clean-looking no-op run. With `SNESRECOMP_PHASE_MS=1` it prints
   `[count] pc watched: N executions over F frames`. And
   `SNESRECOMP_COUNT_PC_FRAME`, documented at `interp816.c:196` as printing the
   count per frame, is **dead code**: `s_interp_pc_frame` is declared at `:200`
   and read nowhere in the tree. This is trap 4 and trap 5's shape again — a
   *different* knob, the same mistake. Measured in
   `docs/measurements/2026-10-02-f3271-entry-gate.md` §6.

8. **`SNESRECOMP_COUNT_PC` parses hex as octal, so a bare `038026` silently
   counts the wrong PC — and it voided a measurement this project had already
   published.** `interp816.c:323` reads the knob with `strtoul(e, NULL, 0)`:
   base 0 auto-detects a leading `0` as **octal** and stops at the first digit
   that is not an octal digit. Measured:

   ```
   COUNT_PC=009311   -> 0 executions over 300 frames
   COUNT_PC=038026   -> 0 executions over 300 frames
   COUNT_PC=0x009311 -> 239617 executions over 300 frames = 798.7 per frame
   COUNT_PC=0x009313 -> 239617 executions over 300 frames = 798.7 per frame
   COUNT_PC=0x038026 -> 0 executions over 300 frames
   ```

   `$00:9311` is `INC $C7`, the vblank spin body — the instrument's **own
   default** at `interp816.c:198`, and an address with three independent proofs
   that it runs. **`038026` → 3; the counter was watching PC `$000003` in bank
   `$00`.** T100's headline and C-041c are **RETRACTED (R-037)**. T100's
   declared falsifier was *"does the counter print anything"*, and it **passed**:
   the counter printed a confident, formatted `0`. Trap 0 is the fix.
   **The same bug, two more symptoms:** `SNESRECOMP_WRAM_DUMP_HI=0B60` → `0` →
   `[wramdump] wrote … (0 bytes)`; `SNESRECOMP_WRAM_DUMP_LO=0B40` → `0` → dumps
   all 128 KB instead of 32. **`strtol(…, 0)` appears at 28 sites** in
   `snesrecomp/runner/src` and `src/`. `SNESRECOMP_WRITE_WATCH` and
   `SNESRECOMP_WRAM_WATCH` are documented *with* a `0x` prefix and are safe only
   if you follow the documentation.
   **And the part that is worse than the bug:** this was written down on
   **2026-09-30** at `docs/RE_CITY_FREEZE.md:1546`, in the right words —
   *"Qualquer resultado de `COUNT_PC` registado neste projecto sem prefixo `0x`
   é nulo"* — and T100 used the bare form four days later. It protected nothing,
   because it sat in an append-only log at line 1546 and not in this list.
   **A rule in the record that does not reach the next session is the same
   failure as a guard whose scope excludes the file.** See `docs/CONFLICTS.md`
   CONF-15. **Interim rule: any knob documented `<hex>` or `0xADDR` gets the
   `0x` prefix, always.** The parse is **not** fixed here — it is one character
   per site in a *pinned submodule*, so a local edit would reach no clone.

9. **A gate added in the same commit as the claim it guards, and never run,
   shipped broken and reported green.** `make check-causes` fired on the bare
   word `date` inside "candi**date**" on line 30 of its own header, so it could
   never be green while that header stood — and the commit that added it
   (`8a7340f`) said in its message and in DoD D3.7 that it was green. It sat red
   for four commits. Fixes to gates here are falsified in **both directions**
   before they are committed (revert the fix → red again; a synthetic unlabelled
   causal sentence still fires; the same sentence labelled `HYPOTHESIS` does
   not), and **any new guard must be demonstrated on an untracked file**, because
   both evidence gates once read `git ls-files` = the *index*, which is CONF-11
   and let an untracked doc asserting a refuted claim pass.

Two knobs were added for this, dev-only behind `-DSNESRECOMP_INTERP_PROFILE`:
`SNESRECOMP_INTERP_DUMP_BANK=03` prints **every** distinct PC in one bank with
its step count (the top-60 list cannot answer "is this one address among
them"), and `SNESRECOMP_INTERP_TRACE_FRAMES=lo-hi` prints every interpreted PC
in a frame window, all banks, in execution order.

**One caveat about the histogram itself, measured:** instrumentation overhead
moves it. The same binary, ROM, script and window give bank 03 = 921 PCs /
515 043 steps plain and 946 PCs / 515 337 steps with the trace compiled in — even
with a window in which it prints nothing. **The PC histogram is a property of an
instrumented build, not of the game** (C-048, cause OPEN). Distinct-PC counts are
stable across machines and runs; *step* counts are not, in the spinlock-dominated
banks: `$00` measured 26 856 340 and 28 769 361 steps across two runs of one
binary (C-049).

## Architecture

```
src/
├── main.c              # Host entry, presenter callbacks, config
├── game_rtl.c          # Frame loop, NMI/IRQ, VBlank wait
├── gen_stubs.c         # Mod hooks (GodMode, disasters)
└── gen/                # Generated by snesrecomp (gitignored)
    └── bank*_v2.c      # Recompiled 65C816 → C
recomp/
├── bank00.cfg          # Bank 0 config with force_lle
├── bank01-07.cfg       # Bank configs
└── funcs.h             # Function declarations (auto-synced)
```

## Legal & Attribution

This is an **unofficial, fan-made project**. It is **not affiliated with,
sponsored by, or endorsed by Nintendo, Electronic Arts, or Maxis.**

- **SimCity®** and the SimCity logo are registered trademarks of
  **Electronic Arts Inc.** SimCity was originally created by **Will Wright**
  and published by **Maxis** (1989); the SNES version was developed by
  **Nintendo EAD** under license from Maxis and published by **Nintendo**
  (1991). All game code, graphics, audio and other assets are © their
  respective owners (Maxis / Electronic Arts / Nintendo).
- **Super Nintendo Entertainment System**, **SNES** and **Super Famicom** are
  trademarks of **Nintendo**.
- **ROM not included** — you must legally own and supply your own
  `SimCity (USA).sfc`. No copyrighted ROM, ripped tiles, palettes or audio are
  committed to this repository.
- Published binaries contain recompiled 65C816→C code **derived from your
  ROM**; the ROM image itself is never committed or distributed.
- This project does not circumvent copy protection and is offered for
  **non-commercial** preservation and research use.
- Project license: **PolyForm Noncommercial 1.0.0** — non-commercial use only.
- snesrecomp framework: PolyForm Noncommercial 1.0.0, copyright © 2026 Matthew
  Stanley. The runner statically links MIT/ISC third-party components
  (snesrev's zelda3/smw ports, LakeSnes, ares-derived coprocessor cores);
  the required license notices ship with every distributed build in
  [`THIRD_PARTY_ATTRIBUTION.md`](THIRD_PARTY_ATTRIBUTION.md).

## Peer study

`study/peer-linux/` builds the reference recomp on Linux and gives it a window.

```bash
study/peer-linux/build-peer-linux.sh                    # build, then play
study/peer-linux/build-peer-linux.sh --headless         # measure instead
study/peer-linux/build-peer-linux.sh -- --date --wram 300   # pass options through
```

The peer ships a Windows-only launcher; on Linux its CMake omits the frontend and
builds just the core, so a frontend is required. `jjwin.c` is ours and talks to
the peer's public C API. Its repository declares no licence, so this is private
study: do not publish or redistribute it. A keyboard reaches a city on its own —
the naming screen's cursor walks on the d-pad and **B** confirms from a character
key. See `study/peer-linux/README.md`.

**What comparing against it proved, and what it did not.** The peer recompiles
the same ROM. **Its clock runs, and it was measured that way to f30 000 and
beyond** — 28 month rolls, two year rollovers, **29 distinct date images**, and
a city-state block that is never abandoned. **[MEASURED, Deck-native, T101
`9069182`]** Two peer runs *appeared* to disagree about this; **they never
did.** The reconciliation is in the table under *The open question* above and it
resolved against **our own driver**, which printed `$0B53` under the label
`$0B55` (`c4923de`) — the disputed 9 000-frame run is reproducible byte for
byte and reads `$0B55 = 07`, **AUGUST**. See instrument trap 1.

The arithmetic on our side is measured (`$0B51 = 0000` at every sample from
f3150 to f30000); the *role* of `$0B51` in our build is still **[INFERRED]**
from the peer and has never been measured here — but its reference-side
behaviour is now **decoded** (`$0B51` = 4 × months elapsed + quarter, verified at
every one of 113 tick events). **Do not read the peer's working clock as a
measurement of ours.** The comparison has established that the two builds are
separated **at the instruction** (`$03:8026` executes 0 times here, 27+ times
there) and it has **not** established what starts the simulation in either
build.

## Development

```bash
# Regenerate from ROM
bash tools/regen.sh "SimCity (USA).sfc"

# Run tests
cd build && ctest --output-on-failure

# Deterministic replay test
SIMCITY_DEBUG_WATCHDOG=1 SIMCITY_DEBUG_APU=1 \
  SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy \
  timeout 30 ./build/SimCitySNESRecomp --script tests/deterministic_replay.script "$PWD/SimCity (USA).sfc"
```

## Feature Status

| Feature | Status |
|---------|--------|
| Core recompilation (recompiled 65816 drives the PPU) | ✅ Done |
| Runtime stability | ✅ Done (10k+ frames) |
| Title screen | ✅ Working |
| Native widescreen (336 px, game-native renderer) | ✅ Done |
| Game-native graphics (terrain, buildings, font) | ✅ Working (verified interactively) |
| Headless capture | ✅ Fixed (T039) — `make test-rom` gates it |
| AOT compilation of declared functions | ✅ Done (239 `aot_eligible` nodes in the current manifest) |
| Config bar over the game | ✅ Done (T054) |
| SNES Mouse on player 2 (`SNESRECOMP_MOUSE=1`, bsnes-exact protocol) | ✅ Device-level done (T042, ROM-free verified) |
| Resolution presets (720p/800p/1080p, `SNESRECOMP_RESOLUTION`) | ✅ Done (T041) |
| Quick save/load (10 slots), save-state menu, rewind, turbo | ✅ Working |
| **City simulation runs (date, population, treasury advance)** | ❌ **`make clock` is red. Measured on the Deck over 14 000 frames: the tick instruction `$03:8026` executes 0 times; bank `$03` executes 0 PCs in f3272–f13080; the city-state block is written 0 times after f3259. Cause [OPEN]** |
| Scenarios (all 5 US) | ⏳ T011 — confirm ENT step is the gate (see `docs/RE_SCENARIO_NAV.md` step 10) |
| Building/visual verification (headless capture) | 🔄 T033 — unblocked by T039 |

The city view renders correctly and the frame loop runs once per frame
throughout. In **our** build the game state never leaves its initial values, so
the date, population and treasury never change. In **the reference** build the
date *does* change — 28 month rolls in 30 000 frames — while population and
treasury are equally flat. **So "the reference simulates" means the tick runs,
not that an economy grows, and this repository makes no claim that one does.**
The cause of our freeze is **OPEN** — see
[`docs/CAUSE_CLAIMS.md`](docs/CAUSE_CLAIMS.md) node C-006. (An earlier version
of this file pointed at `aes/tickets/T058-city-clock-does-not-advance.md`. `aes/`
is gitignored **permanently and by rule** — DoD D4.3 — so that path resolves in
no fresh clone. Pointing a tracked file into an uncommittable directory is the
same rule breaking itself; the pointer now names a tracked file.)

## License

PolyForm Noncommercial 1.0.0 - See [LICENSE](LICENSE) for details.

### Widescreen (16:9)

`Widescreen = 1` is a per-title PPU contract and does nothing here, for a
reason worth stating plainly: SimCity draws a fixed 256 columns, and the
`frame_width` of 336 is 256 plus 40 blank pixels per side. Nothing renders past
column 256, so the margins are empty by construction - a PPU-side 16:9 would be
71 blank pixels per side instead.

What works is 16:9 as a **presentation**: the authentic 256-wide picture
stretched to 16:9, filling the window edge to edge with no bars.

```ini
[Graphics]
DisplayAspect=16:9
```

The default is unchanged. The toggle key cycles 4:3 <-> 16:9.

Presentation only, and the precise claim is narrower than "identical": in 16:9
the PPU frame is 256 wide instead of 336, so the **presented framebuffer is a
different width** and will not hash equal to the 4:3 one. What is identical is
the guest's own 256 columns - cross-platform checked on two machines, where the
256 columns inside the margins hash equal to the whole 16:9 framebuffer, and the
margins are solid black. An earlier version of this file said "byte-identical
either way" and was wrong; see `docs/RE_CITY_FREEZE.md`.
