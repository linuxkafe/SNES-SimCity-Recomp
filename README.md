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
> This project has retracted **35** claims out of **43** ledger rows (computed,
> `make retraction-count` — never a hand-written number; the other 8 rows are 6
> `superseded` and 2 `invalidated-premise`, which is not a retraction). Where an
> old claim is quoted below it is labelled **RETRACTED** and is printed as
> history, not as the answer.
>
> **And this sentence was itself wrong until now, which is the sharpest available
> demonstration of the hole described underneath it.** It read *"29 claims out of
> 37 ledger rows"* for one commit past `79a4064`, while
> `scripts/check-retracted-claims.sh --count` said **30 of 38** — and
> `make check-claims` printed **`(no violations)`**. The reason is the guard's
> own stated limitation: its count pattern is **forward-only**,
> `N` + `retract…`, and this sentence puts the number *before* the word —
> *"retracted **29** claims"*. **CONF-14 fired on the most-read line in the
> repository, in the phrasings that guard documents as its known miss.** The
> number is now correct; **the hole is not closed** and nothing here should be
> read as closing it.
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

### ⚠️ This file was shipped empty once, and every gate in the project passed

**Read this before trusting any green verdict in this repository, including the
ones below.** Commit **`5cbf5fd`** ("T108: the per-present crc32 timeline")
committed **`README.md` as 0 bytes** — `git show --numstat 5cbf5fd -- README.md`
is `0  1895  README.md`: **1 895 lines deleted, nothing added.**

```
$ git cat-file -s 5cbf5fd:README.md        -> 0
$ git cat-file -s 5cbf5fd~1:README.md      -> 120899
$ git cat-file -s 3094650:README.md        -> 121482
```

**Every gate passed on the empty file.** Re-measured on this tree, 2026-10-03,
by truncating `README.md` to 0 bytes and running the guards with it tracked:

| guard | verdict on a **0-byte** `README.md` |
|---|---|
| `make check-claims` | **`RESULT: PASS`**, exit 0 |
| `make check-causes` | **`RESULT: PASS`**, exit 0 |
| `make check-cheat-gate` | **`RESULT: PASS`**, exit 0 |
| `make check-claims-self-test` | **PASS** |
| `make check-causes-self-test` | **PASS** |
| `make check-cheat-gate-self-test` | **`SELFTEST PASS: 5/5`** |
| `make retraction-count` | unchanged — it reads the ledger, not this file |

**[MEASURED, dev host `seyon`, 2026-10-03 — reproduced here, not quoted.]**
`make build` and `make test` (2/2) also pass, and they never read this file at
all. The file was restored in **`3094650`**.

**The cause was one Python expression, and it is worth writing down exactly
because it is short and because it is a mistake anyone can make:**

```python
open(p, 'w').write(open(p).read() + new_text)      # WRONG
```

Python evaluates the **arguments** before the call, left to right, and
`open(p, 'w')` is the first argument — so the file is **truncated to zero, and
the handle is created**, before `open(p).read()` on the second argument ever
runs. It then reads back **nothing** and writes `new_text` into a file that is
already empty. **The truncation is not a race and not a partial write; it is
guaranteed by the evaluation order.**

**The safe forms, and this repository now uses only these:**

```python
new = open(p).read() + new_text                   # read fully FIRST
open(p, 'w').write(new)                            # then open for write
# or, atomically:
import os, tempfile
fd, tmp = tempfile.mkstemp(dir=os.path.dirname(p))
os.write(fd, new.encode()); os.close(fd)
os.replace(tmp, p)
```

**Now, what this is and is not.** It is **not** a defect in
`check-retracted-claims.sh`, `check-cause-claims.sh` or `check-cheat-gate.sh`.
Each of those is doing exactly what it was written to do, and each of them would
have caught a retracted claim *if the claim were still there to be caught*. What
`5cbf5fd` demonstrates is **the class of failure they are all defeated by**, shown
on this project's own entry point: **`git ls-files` says a file is in scope, so
every guard reads it, and an empty file satisfies "no violations" perfectly.**
That is CONF-20's structural half (a guard that checks *existence* is not a
guard that checks *content*) arriving as a real, shipped, self-inflicted wound.

**And the hole is still open at the time of writing: there is no gate anywhere in
this repository that asserts `README.md` is non-empty, or that it parses.**

```
$ grep -rn "README" scripts/*.sh | wc -l          -> 8 matches, 3 files
$ grep -rn "README" scripts/*.sh | grep -ci size  -> 0
```

Three scripts *mention* the file — `check-cause-claims.sh:166,181`,
`check-retracted-claims.sh:178,338,451` — and **not one of them looks at its
size, its line count, or whether it has a heading.** **So a green
`make check-claims` says nothing about whether this file has any content in it.**

**That hole is now closed by [`scripts/check-entrypoints.sh`](scripts/check-entrypoints.sh)
(`make check-entrypoints`), which asserts every tracked entry-point document is
tracked, ≥ 1024 bytes, ≥ 40 lines, opens with its expected heading, and has no
unclosed code fence — and refuses to total on an empty file.** Its self-test
truncates this very file to 0 bytes, requires the guard to exit 1, and restores
it byte-for-byte: **`make check-entrypoints-self-test` → `SELFTEST PASS: 9/9`.**
Full falsification table and its limits in [`docs/CONFLICTS.md` CONF-23](docs/CONFLICTS.md).

**One of its own assertions was wrong on its first run and was deleted rather
than tuned**: a *"no unbalanced `**`"* check fired on **1963 legitimate
occurrences** in `docs/RE_CITY_FREEZE.md`, because bold spans line breaks and
`**` appears inside inline code. **A guard that cries wolf on its own corpus is
worse than the hole it closes.**

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
| distinct AOT symbols emitted into `src/gen/*.c` | **239** |

**Re-derive all four with `scripts/count-aot-symbols.sh`.** Do not re-derive them
by grepping: this table previously said **186** under the label *"distinct AOT
symbols emitted into `src/gen/*.c`"* with no derivation recorded anywhere, and
three people re-derived **186**, **187** and **559** from it. A number nobody can
reproduce is not a measurement. The script prints the method next to the numbers,
and here is what the four figures are:

| figure | what it counts |
|---|---|
| **239** | every definition symbol in `src/gen/*.c` carrying the register-state suffix `_MxX`. **This is the row's number.** It agrees, from a different file, with the `aot_eligible` count in `program_manifest.json`. |
| **186** | the subset of those 239 whose name was auto-derived from the program counter, matching `bank_NN_PCCC_MxX`. It **excludes** 52 symbols named from `recomp/*.cfg` (`City_Update_M1X1`, `MainLoop_M1X1`, `PPU_Bitpack_8EA9_M0X0`, …) and 1 PC-derived symbol without the `bank_NN_` prefix. **Quoting 186 under this row's label was a labelling error, not an off-by-one.** |
| **187** | 186 **plus** `CODE_00987B_M0X0` — the same PC-derived subset counted with a pattern admitting either prefix. **This is the off-by-one a reviewer reported, and it is fully accounted for.** |
| **559** | every distinct definition symbol including 320 named and runtime helpers that are not recompiled SNES code at all (`SPC_*`, `LC_LZ5_*`, `RLE_Decompress`, `Scenario_Decompress`, …). This is what a naive "grep all function signatures" returns. |

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

### How far the search has actually got

The search is **not** a list of guesses; it is a chain in which each link has been
measured and several have been retracted. Read this before reading any narrative
elsewhere. **Every claim below names the file that carries the full evidence**, and
the retraction ledger is the source of truth where this prose and it disagree.

**What is established [MEASURED, Deck-native]:**

- **The display path is intact, and it is HDMA.** Poking `$0B53` (year) renders
  `1952`; poking `$0B55` (month) renders `1900 MAY`, 100 px changed. `$0B51` is only
  the counter — poking it changes **0 of 151 frames, bit-identical**. So the HUD
  reads the *derived* words, and **C-006 is one bug, not two** (R-045 retracted the
  claim that no live path to the screen exists — HDMA writes VRAM, which is exactly
  why no CPU `$2119` write ever appears).
- **`$0B53`/`$0B55` are written once**, around f3259 by the new-city routine
  (`LDA #$076C / STA $0B53`, `LDA #$0001 / STA $0B55`), and **never again** — while
  HDMA keeps copying the same bytes forever. Hence 1 distinct date image.
- **There is no year transform.** `$0B53` **is** a plain binary-decimal year: the
  game's own milestone table at `$03:C4DC` reads `1900 1900 1900 1901 1901 1905`
  and `$03:C490 CMP ($C4DC),Y` compares `$0B53` against it **16-bit** (R-047
  retracted my "the HUD does not render the raw value" premise). What `1952` is
  from `$0FA0` is **NOT ESTABLISHED**.
- **Bank `$03` stops executing at f3271** and is the code that would derive the date
  words. In the **peer** it stays alive: **`STZ $0E15` at `$03:8222` executes 28 times,
  f3068–f8881** (56 bus writes, 2 per execution because `M=0` makes the store 16-bit),
  and `$03:8029` — the tick itself — spans **f3857–f8990** (54).
- **The divergence is measured, and the attribution is now settled.** The bytes at
  `$03:8222` are `9C 15 0E` = **`STZ abs`, 3 bytes**, from this repository's own
  declarative opcode table (`snesrecomp/recompiler/snes65816.py:369`,
  `(0x9C,'STZ',ABS,3)`) — T116's `64 15` was refuted (R-048) and its "56 times" too
  (R-050). **In our build that write never happens:** 0 bank-`$03` writes to `$0E15` in
  9 000 frames, tier-independently.
- **Our per-frame engine is NOT broken.** In f3400–f9000 it executes **1228 distinct
  PCs** across banks `$00`/`$01`, ~40.4M steps, at a control rate of
  **17 483 747 / 9000 = 1942.6 per frame** against a known-good 1929.9/f (**+0.66%**).
  The divergence is **entirely** that our bank `$03` never re-enters after the
  one-shot city-creation phase.
- **`$0012` is a one-shot latch, not the gate** (R-046). Written **exactly twice** in
  both builds and never cleared in either: f0 and f3271 in ours, f0 and **f2998** in
  the peer — where bank `$03` keeps running. **A flag that never returns to 0 cannot
  gate an action that must repeat.** The peer killed this hypothesis.
- **`$00:8061` `Init_Hardware` runs once and its `RTS` at `$00:80B1` runs zero
  times.** The last instruction before the stall is `$00:80AD JSL $018907` at f3340 —
  but `$018907` **completes**: it is city initialisation (a finite 1024-iteration loop
  at `$01F1E4-$01F1EA`, then HDMA setup). Control **bypasses** `$80B1`; it is not
  lost. `S` is **not restored** (`$00AF` written once, never again).
- **The stack-corruption hypothesis is dead.** `$00:86A4` is **OBSERVED** (reached at
  `IPC=00821E`, COP dispatch, 862 118 write lines) and it writes `$7E:2000-$21FF` —
  the stack page `$01xx-$02xx` is **untouched**.
- **`$00:804D` (`MainLoop`) is called by nobody.** JSR/JMP abs, JSL/JML long, and all
  four indirect forms: **zero hits in 512 KB**. It is reached by **fall-through once**
  from the boot routine at `$00:8000`. It is not a per-frame loop in either build.

**What remains OPEN, and it is narrower than it was:**

> **Why does `STZ $0E15` at `$03:8222` execute in the peer (28 times, f3068–f8881) and
> never in ours?**
>
> The peer side of this is now **measured** — T116 ran the `$0E15` watch that had never
> been run (every earlier peer watch covered only `$0010-$001F` or `$0B51-$0B52`), and
> T117 corrected its own attribution against the repository's own opcode table. So the
> observation stands and the explanation is no longer in doubt: the instruction really
> is `STZ $0E15`, absolute, 3 bytes.
>
> **What is still OPEN, and no run in this tree answers it:** what makes bank `$03`
> re-enter in the peer after the one-shot city-creation phase, given that its latch
> `$0012` fires **273 frames earlier** (peer f2998, ours f3271) — whether that timing is
> the gate or merely a correlate; why the peer logs only **28 executions across 5 814
> frames** there (roughly one per 200 frames, so most iterations must be no-store);
> whether `$03:8007` is the city-creation routine the peer takes and we do not; and why
> `S` is never restored after `$018907`.

**Where the evidence lives:** `docs/measurements/2026-10-03-t112-0012-is-a-latch-not-a-gate.md`,
`…-t113-init-hardware-stalls-at-jsl-018907.md`,
`…-t113-peer-bank03-entry.md`, `…-t114-018907-never-returns-to-80b1.md`,
`…-t115-perframe-engine-ours.md`, and `docs/RE_CITY_FREEZE.md` for the full history
including every retracted claim and its refutation.


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
- **86% of bank `$03`'s cost in f3000–f3271 is four PCs around a loop — and
    it is a zero-fill that runs away, not a scan.** `$03C877` `STA $7F6B00,X`,
    `$03C87B` `INX`, `$03C87C` `CPX #$F4` — 36 344 / 36 343 / 36 344 steps; the
    next PC down is 244. **[MEASURED, Deck-native, T104.]**
    > **RETRACTED (R-038): the "`$03C87F` is not an instruction boundary" clause
    > above is refuted, and it was refuted *as a statement about the executed
    > stream*.** This file previously read, as fact: *"`$03C87F` is the second
    > byte of `8D D0 F6` = `STA $F6D0`, whose instruction starts at `$03C87E`, so
    > it is reported unattributed, not as an executed instruction"*, and *"its
    > back-edge is outside the logged neighbourhood"*. **Both halves are false of
    > the stream.** `SNESRECOMP_CYC_WATCH` reports the opcode byte **fetched** at
    > `$03C87F` as **`$D0`** = `BNE $C877` — it is the loop's **only** branch and
    > its **real, executed back-edge**: taken **36 339** times, not taken **once**.
    > It is true of the **ROM** and false of the **executed stream**; the fetched
    > opcode byte settles which is which. **What never executes is `$03C87E`** —
    > **0 of 2 676 196** trace lines, and absent from the whole-run bank dump.
    > **C-069's four cost figures are NOT retracted** — all four reproduce to the
    > unit, so its 86% stands. **[MEASURED, Deck-native.]**

    **The loop is a zero-fill, and T104 answers all three of its open questions:**

    | question | answer | |
    |---|---|---|
    | **last execution** | **f3270**, in **f3259–f3270 only**. Bank `$03`'s own last frame is **f3271** — 16 steps, a 16-instruction epilogue ending `RTL` at `$03:D2B7` | so **C-039b is NOT falsified**: **the loop is a *precursor* of the bank's death, not its cause**, and **this ticket explains neither** |
    | **back-edge** | **`$03C87F` `BNE $03C877`**, a real executed instruction | 5 entries (1 fall-through from `$03C874`, 4 `RTI` resumes at `$0081A3`), **1 exit ever**, at f3270 → `$03C881` |
    | **what it scans** | **nothing.** `A=$0000`, one `$00` byte per iteration at **`$7F6B00 + X`** | see below |

    **It writes nothing but zeros, and it sails past its own bound.** X was
    **measured** walking `$0000 → $04FF` and then on to `$14FF`, crossing the
    f3259 → f3260 frame boundary — writing **$7F6B00–$7F7FFF**, over 1 280
    distinct addresses, of which **2 069 measured stores are at `X > $00F4`**.
    `LDX #$0000` runs **once** and `INX` is the loop's only X writer, so X climbs
    monotonically and **cannot get past `$00F4` without passing through it**. It
    did, and kept going. **[MEASURED, Deck-native, T104 — `WLOG_ADDR` + state.]**
    T105 then traced X to **`$8DF3`** over 36 340 strictly monotonic iterations;
    the `$14FF` ceiling above was the `WLOG_ADDR` 16-bit range, not the loop.

    > #### ⚠️ ⚠️ RETRACTED TWICE — R-039, then **R-040/R-041 (T106)**. There is no defect at all.
    >
    > **This block has been wrong twice and the second correction is larger than
    > the first.** Read it as history, not as the answer.
    >
    > **The first framing**, at `3f24098` — *one commit before it was measured* —
    > said: *"This is the **first thing in this project that looks like a defect
    > in the game's own code** rather than in our emulation … **It is NOT
    > established which.**"*
    >
    > **R-039 answered that with the opposite error, and that answer is itself
    > RETRACTED (R-040) — see below.** It said **"T105 measured
    > it. It is a defect in our emulation"** — `CPX #imm` reads a **2-byte**
    > operand while `xf=0`, so the word compared is the two bytes at
    > `$03C87D`/`$03C87E` = **`$8DF4`** and the loop terminates at
    > `X == $8DF4`. **[MEASURED, Deck-native]** And the measurements were all
    > correct, and they all reproduce:
    >
    > | at `$03C87C`, `P` = the flags the `BNE` reads | T105 | **T106 re-measure** |
    > |---|---|---|
    > | `NPC` from `$03C87C` | `$03C87F` × 36 340, one value | **identical** |
    > | `P=$04` (Z clear) | 3 571 | **3 571** |
    > | `P=$84` | 32 768 | **32 768** |
    > | **`P=$07` — Z and C set** | 1, at `X=$8DF4` | **1, at `X=$8DF4`, f3270** |
    > | `X` over the loop | 36 340 distinct, monotonic | **36 340 distinct, `sort -c` non-decreasing** |
    >
    > **⚠️ R-040: that attribution is REFUTED. The 3-byte reading is published
    > 65816 behaviour, and our decoder is right.**
    >
    > `LDX/LDY/CPX/CPY #imm` are **2 bytes at `x=1` and 3 bytes at `x=0`**; the
    > accumulator/ALU immediate group follows **`m`** instead. Three independent
    > published sources agree — snesdev's 65816 opcode tables (citing the WDC
    > datasheet, Eyes & Lichty, and Near's higan disassembler) and Chris Wright's
    > 65816 Programming Primer, which writes it as `CPX #const E0 2*` with
    > *"Add 1 byte if … (16-bit index registers)"*. `interp816_adrImm`
    > (`interp816.c:513`) implements exactly that, and the four `xFlag=true` call
    > sites are exactly `LDY`/`LDX`/`CPY`/`CPX #imm`.
    >
    > **The flags were never wrong.** At `X=$8DF4`, `CPX #$8DF4` sets **Z** because
    > `X` *equals the operand* and **C** because `X >= operand`. `P=$07` is
    > `N0 V0 m0 x0 D0 I1 Z1 C1` (`interp816_getFlags`, `interp816.c:403-413`) —
    > **the correct answer.** T105 read it as "a wrong Z", because it had already
    > assumed the operand was `$00F4`.
    >
    > **So `$03C87C` is `CPX #$8DF4` — the game's own 16-bit bound. The loop
    > zeroes `$7F6B00 + X` for `X = $0000…$8DF3`, i.e. `g_ram` `$6B00`–`$F8F3`,
    > 36 340 bytes, and stops exactly there**, on one exit, via an ordinary
    > `JSR $B477`. **No overrun. No skipped instruction. No memory destroyed
    > beyond what the game asked to clear.**
    >
    > **⚠️ R-041: R-039 is therefore retracted too.** It retired an ambiguous
    > framing correctly and supplied a wrong replacement. **The answer is
    > neither "ours" nor "the game's code" — there is no defect.** And R-039's
    > closing sentence, *"Three sessions framed this loop as a mystery in the
    > game's code … one defect on our side"*, is **wrong in its own terms**: the
    > mystery was ours, but it was not a defect, it was **a 1-byte reading of a
    > 2-byte operand written down as a finding.**

    #### T106 — the census that settles it, and the fix that must not be made

    **Deck-native, five runs of 14 000 frames (one per executing bank), all
    `EXIT=0` / `exit: RUN_FRAMES reached`, each carrying its own
    `COUNT_PC=0x009311` positive control: 27 019 166 / 1929.9 per frame,
    byte-identical to the known-good value in all five.** **[MEASURED,
    Deck-native]** — transcript
    [`docs/measurements/2026-10-03-t106-index-immediate-census.md`](docs/measurements/2026-10-03-t106-index-immediate-census.md),
    claims **C-074**.

    | the blast radius | |
    |---|---|
    | distinct executed interpreted PCs, all banks, 14 000 f | **6 083**, **109 533 634** steps |
    | **sites whose ROM opcode is `$A0`/`$A2`/`$C0`/`$E0`** | **245** |
    | steps at those sites | **344 060** = **0.3141%** of all interpreted steps |
    | read with a **2-byte** operand (`x=0`) | **195** sites, **287 062** steps |
    | read with a **1-byte** operand (`x=1`) | **49** sites, **55 566** steps |
    | **ambiguous** | **1** site, `$03DCCF`, 1 432 steps — **left ambiguous, not resolved by assumption** |

    **The partition was derived** from which landing address was fetched, so it
    was checked against **direct flag reads on both sides**. `P` is
    `interp816_getFlags()`, so **the x flag is bit 4**: `$03C874 op=$A2 P=$06`
    (x=0, 3 bytes), `$008D28 op=$A2 P=$64` (x=0, 3 bytes), `$01C826 op=$A0 P=$30`
    (x=1, 2 bytes), `$01C828 op=$A2 P=$30` (x=1, 2 bytes). **`$008D28` is logged
    `m=1, x=0`** — the two flags are independent, which is exactly why the two
    immediate groups need two different flags.

    **The ROM's own instruction stream agrees, four times, without using a length
    table** — each takes the ROM bytes as given and asks which reading is
    consistent with a *count*, which is the one thing a decoder cannot fabricate:

    1. `$03C877` executes **36 344** times; **`$03C876` executes 0** times in
       14 000 frames. `$C876` is the `$00` that a 1-byte reading of
       `$C874 A2 00 00` would make a **`BRK`** — a breakpoint between
       `LDX #$0000` and the `STA $7F6B00,X` that consumes that very X.
    2. **Under a 1-byte reading of `$03C87C` there is no branch instruction
       anywhere in `$03C877`–`$03C884`** — `$C87E 8D D0 F6` is `STA $F6D0`,
       `$C881 20 77 B4` is `JSR`, `$C884 22 3A 82 00` is `JSL. A region with no
       branch cannot re-enter its head, and `$C877` is re-entered **36 339**
       times.
    3. **90 of the 195** `x=0` sites have **`$00` at PC+2** — 211 859 steps,
       including the busiest site in the census (`$008D49`, 82 056 steps) and
       four 12–14k-step loop counters. **A `BRK` inside a 13 000-iteration loop
       is not a coherent encoding of anything.**
    4. `$01C824 E2 30` = **`SEP #$30`** sits immediately before `LDY #$0F` and
       `LDX #$1E`, and the logged `P` goes `$32` → `$30` across it. **The
       assembler emitted a flag setup because the width depends on it.**

    #### The pre-registered post-fix shape was run, and it never appeared

    **The prediction was "245 iterations, correct Z/C at `X=$00F4`".** Built on
    the Deck as `build-x` with the index-immediate group forced to 8-bit, 3 300
    frames, `EXIT=0`, `RUN_FRAMES reached`, positive control present:

    | | baseline | the proposed fix |
    |---|---|---|
    | banks that execute at all | `$00 $01 $02 $03 $05` | **`$00` only** |
    | distinct PCs, bank `$00` | **1 822** | **16 778** |
    | bank `$03` | 921 PCs / 515 043 steps | **0 / 0** |
    | **`$03C87x` loop** | **36 344** iterations | **0 — not 245** |
    | `COUNT_PC=0x009311`, 3 300 f | **6 626 029** (2007.9/f) | **13 631 085** (4130.6/f) |

    **The prediction was preempted, not refuted: the change is not local and
    destroys the trajectory before the loop, so the loop runs zero times.** What
    it refutes is the *premise* — that the width rule is the bug. **The fix is
    not made, and must not be.**

    > **And my first attempt at the experiment was wrong, and its own mechanism
    > check refuted my explanation of it.** Version 1 patched `if (1)`, forcing
    > **every** immediate to 8-bit *including the accumulator group* — so it was
    > not the proposed fix at all. It collapsed the run spectacularly; I then
    > predicted control would land on operand bytes of the index-immediate sites
    > and measured **0 of 16 749** newly-executed PCs doing so. **My causal
    > story was wrong, and the reason was my own mis-specification.** Version 2
    > corrected the specification and produced **identical** numbers on every
    > figure; **I did not establish why and assert no reason** — the available
    > candidate, that the accumulator group was never reached with `m=0` on the
    > trajectory v1 diverged onto, is **UNVERIFIED**.

    #### ⚠️ CONF-21 — the structural point survives; its specific claim does not (R-042)

    The submodule's accuracy report records *"533 opcode variants, 0
    divergences (1.599M checks) **vs interp816**"*. That suite compares **the
    AOT code generator against `interp816`** — two implementations of this
    project, not this project against hardware.

    **⚠️ RETRACTED (R-042): the sentence below is false.** It read: *"Line 30's
    comment **is the defect restated as a test-corpus invariant**, and line 67
    plants a **3-byte** encoding for the `x=0` case — so the test ROM is wrong in
    the same direction as the decoder and the differential reads two matching
    mistakes read as agreement."* **The 3-byte `x=0` encoding is correct**, and
    `gen_ops.py:30`'s comment *"width = X flag"* is correct. **The generator and
    the decoder agree because both are right.** CONF-21 was citing T105 as an
    instance of itself, and T105 was not one.

    ```
    snesrecomp/tests/cpu_diff/gen_ops.py:30
      # index-immediate compares/loads (width = X flag)
      IMMX_OPS = [("ldx", 0xA2), ("ldy", 0xA0), ("cpx", 0xE0), ("cpy", 0xC0)]
    snesrecomp/tests/cpu_diff/gen_ops.py:67
          emit(f"{label}_{imm:02x}_lo_x0", [op, imm, 0x00], 1, 0)   # 16-bit index
    ```

    **What survives, and is the real CONF-21:** a differential suite that
    compares our AOT against our own `interp816` **cannot see a defect common to
    both**, and 1 599 000 checks therefore prove nothing about conformance to
    the 65816. **Coverage was never the problem; independence is** — and that is
    still true, and it is *not* demonstrated by this ticket. **This repository
    still has no conformance reference in the tree**, which is why the rule had to
    be settled from published documentation plus the ROM's own instruction
    stream rather than from a gate.

    #### ⚠️ A limit on CONF-19 / R-038, found while doing this

    R-038 retracted C-069's clause *"`$03C87F` is the second byte of `8D D0 F6`,
    therefore not an instruction boundary"* on the evidence that `CYC_WATCH`
    reports the opcode **fetched** at `$03C87F` as `$D0`. **That evidence cannot
    bear the weight.** The PC fetched at is produced by **our own decoder**, so a
    decoder that mis-lands on a boundary will make `CYC_WATCH` report the byte
    there with total confidence. **A fetched opcode byte proves an instruction
    executed at that PC; it does not prove the PC was an instruction boundary in
    the game's intent.** What stands is that *of our build's stream* `$03C87F` is
    the loop's only branch and its real back-edge, taken 36 339 times. **No
    ledger row is added — R-038's conclusion about our stream is not shown to be
    wrong, only its stated reason.** Stated here because the next reader of
    CONF-19's `op=` field needs it.

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

### T107: a third-party cheat table, verified from the outside

**Deck-native, Release `build/`, headless, `scripts/cheat_probe.script`, three
runs, `EXIT=0` and `exit: RUN_FRAMES reached` on all three**, each carrying its
own `COUNT_PC=0x009311` positive control (**10 239 582** over 5 200 frames =
**1 969.2/frame**, and **9 667 063** over 4 900 frames = **1 972.9/frame**, twice,
byte-identical to each other). **[MEASURED, Deck-native]** — transcript
[`docs/measurements/2026-10-03-t107-cheat-verification.md`](docs/measurements/2026-10-03-t107-cheat-verification.md),
verdict on every row in [`docs/CHEAT_CODES.md`](docs/CHEAT_CODES.md).

> **A control compared the wrong way looks like a failing control.** The known-good
> readings are **6 626 029** / 3 300 f (**2 007.9/f**) and **27 019 166** / 14 000 f
> (**1 929.9/f**). These runs land inside that band but are *not* byte-identical to
> it, **because the frame counts differ.** Comparing per-frame is the only honest
> comparison here, and a raw total is the wrong one.

**The plaintext `7E AA BB CC` reading is CONFIRMED — and the byte order is
measured, from outside, on a detail a looser test could not see.**

```
7E AA BB CC   ->   write $CC to the 16-bit WRAM address $AABB
```

Sixteen WRAM dumps of `$0B40`–`$0C00` over a 1 000-frame window. **Exactly five
bytes differ between the first and the last dump — `$0B53`, `$0B54`, `$0BA5`,
`$0BA6`, `$0BF9` — and they are exactly the five that were poked.** The game
rewrote none of them. **[MEASURED, Deck-native.]**

| | measured | |
|---|---|---|
| f3900 / f4020 / f4030, before | `$0B53 = $76`, `$0B54 = $07` | 16-bit at `$0B53` = **`$076C` = 1 900** |
| **f4140 — after the first poke only** | **`$0B53 = $A0`**, `$0B54 = $07` | **`$07A0`** |
| f4260 onward — after both | `$0B53 = $A0`, **`$0B54 = $0F`** | **`$0FA0` = 4 000** |

**The low byte went to `$0B53`.** Under a big-endian reading the same two
published codes would have produced `$A00F` = **40 975** with an intermediate of
`$760F`. Only one of those is what the machine did. And `$0B53` read
**`$076C` = 1 900** *before* the poke — this project's own independently measured
year for this ROM (T101/T102), which is what ties the address to the **meaning**
rather than to a writable 16-bit field that happens to sit there. **[MEASURED,
Deck-native.]** A source with no access to this project's disassembly landed on
the right address twice; **that is the strongest argument for the format being
real, and it is also why the two open rows below are left open rather than
guessed.**

### The table, with a verdict on every row

| entry | verdict | what was measured |
|---|---|---|
| **`7E0B53A0` + `7E0B540F`** — "year 4000" | **VERIFIED** | `$0B53`/`$0B54` went `$076C` → **`$0FA0`** at the predicted frames and **persisted to f4899**; the game never rewrote them. The byte order is confirmed by the f4140 intermediate **`$07A0`** above. Corroborated from the inside: `$0B53` = 1 900 is our own measured year at city load. **Caveat: the picture did not change — see below, and that caveat is why this row's *label* is settled by corroboration and not by its screen effect** |
| **`7E0BA520` + `7E0BA64E`** — "population 20000" | **PARTIAL** | The **mechanics** are confirmed — `$0BA5`/`$0BA6` → **`$4E20`** = 20 000 little-endian, with a visible intermediate (`$0020` at f4380), persisted to f4899. **The label is not.** **`$0B9D` is also `$4E20` = 20 000**, and this project measures `$0B9D` as funds in **every** sample of **both** builds across hundreds of samples — so the cheat writes the value funds already holds into a field that reads 0. Either `$0BA5` is a population that legitimately starts at 0 while the treasury starts at 20 000, or the published table means `$0B9D` and mis-transcribed. **Nothing here settles it and this file does not pretend to.** What would settle it is a run in which the city renders and `$0BA5` is seen non-zero with no poke — **that is C-006** |
| **`7E0B-F9EB`** — "start with $49 000" | **UNRESOLVED** | `$0BF9` is real, writable 16-bit WRAM: it read **`$0000`** before and **`$00EB`** after, and persisted. **The arithmetic fails: 49 000 = `$BF68`**, and `0xBF98` = 49 048 does not close it either. Readings still open — a different field than the label names; an obfuscated entry that this table renders in plaintext while its others are plaintext, which would itself be inconsistent; or a wrong published entry. **Measured as far as it can be measured here, and no substitute entry is invented** |
| **`7E03-F501` … `7E03-F50A`** — "special buildings" | **UNVERIFIED**; the **"bank 3" characterisation is REFUTED** | Ten codes, values `$01`…`$0A`, all to one address — and under the format confirmed above, `AA BB` is a **16-bit WRAM address**, so this is **WRAM `$03F5`**, not anything in ROM bank 3. One 8-bit field written with ten different values is a **selector**, which is coherent. But `$03F5` read **`$00` both before and ~29 frames after** the poke, and the `poke` verb is **proven live in the same script in the same run** (five other pokes landed inside the 120-frame dump spacing). So either it is **overwritten within ~30 frames** or the write is masked — **these are not separated.** A `WLOG_ADDR="03F5:03F5"` run would separate them in one pass and **was not run.** Recorded as a gap |
| **`DD67-DFAA` / `DE67-DFAA`** | **REFUTED as characterised**; the *decoding* is **UNVERIFIED** | **The "RAM-injection codes patching instructions at `$67DF`" framing does not survive the address.** `$67DF` **cannot be a code address in this ROM**: bank `$67` is ROM, this ROM is 524 288 bytes (banks `$00`–`$3F`), and LoROM `offset = bank*0x8000 + (addr & 0x7FFF)` puts bank `$67` at file offset **`0x338000`, past the end of the file. There is nothing there to patch.** `DD`/`DE` are a 16-bit and an 8-bit **compare-and-freeze** pair on a 16-bit WRAM address under the standard published type table — *"freeze WRAM `$67DF` when it equals `$AA`"*, a different thing entirely. That type table is **general published format knowledge, cited as such, and is not evidence for anything measured here.** What `$67DF` *is*: **not established** |
| **`C28A-AD61` / `E28A-AD61`** | **UNVERIFIED**, with one measured connection | Both say *"write `$61` to WRAM `$8AAD`"*. **`$8AAD` lies inside the region our own emulator destroyed** — T105 traced the `$03C87x` zero-fill across `$6B00`–`$F8F3`, and `$8AAD` is inside it. So in *this build* there is nothing at `$8AAD` for the cheat to modify. **That is arithmetic over two measurements, offered as a connection and not as a claim about what `$8AAD` means** |

#### ⚠️ THE LEAD NOBODY ASKED FOR — the rendered date is not read live from `$0B53`

`md5` over **1 877 consecutive presents** (f0–f4890):

```
18 distinct picture states; the last one begins at frame 1459.
```

**Every cheat poke landed after f4025, and the framebuffer was pixel-identical
for 3 400+ frames spanning all seven of them — including a write of `$0FA0` into
the field this project has independently measured to be the year.** **[MEASURED,
Deck-native.]**

**This is not evidence that the cheats are wrong** — the WRAM dumps prove they
landed. It is evidence about **our build**, and it is the biggest open thread in
the project:

1. **The rendered date is not read live from `$0B53` each frame.**
2. **`make clock` reads the date off a screen crop** — `scripts/clock-gate.sh`
   crops the HUD at x 55–125, y 2–21 of the 336×224 framebuffer and hashes it. So
   **the gate and the memory are reading different things, and that relationship
   is unexamined.**
3. **Therefore no cheat in this table can be verified by its screen effect in
   this build** — which is exactly why the "population" label and the `$0BF9`
   value are left open above rather than guessed at.

**Recorded as a lead for C-006 and with no claim attached to it.** The two
readings this permits have very different consequences, and separating them is
the next measurement, not a conclusion: either the rendered date's *own source*
is frozen (in which case the crop is measuring something real and **C-006 is
unchanged**), or it is frozen for a different reason — a cached tilemap, a
dirty-flag never set, a DMA never triggered — in which case **`make clock` may be
measuring a display path rather than the simulation**, and the finding belongs to
the display.

### T108: the freeze is a **17-present event**, not a 3 000-frame diff

**Deck-native**, `build-instr`, `scripts/d_city.script`, **5 000 frames**,
`EXIT=0` and `exit: RUN_FRAMES reached after 5000 frames`, positive control
`COUNT_PC=0x009311` → **9 855 088** = **1971.0/frame**, inside the established
1929.9–2007.9 band. **Host-only: none.** Instrument: `SNESRECOMP_PRESENT_LOG`,
which writes `present,frame,alpha,crc32,luma` — **one row per present, no
pictures** — from the same code path `SNESRECOMP_SCREENSHOT_DIR` uses.
**[MEASURED, Deck-native]** —
[`docs/measurements/2026-10-03-t108-present-crc-timeline.md`](docs/measurements/2026-10-03-t108-present-crc-timeline.md).

| | |
|---|---|
| presents | **5 000**, frames **1…5000** — one present per frame, no decimation |
| distinct `crc32` values in order of first appearance | **165** (206 changes; some states recur) |
| **last picture change** | **frame 3381** |
| **presents after it, all bit-identical** | **1 619** |

**Then: a 106-frame gap, then 17 consecutive changes, f3365 → f3381, then 1 619
identical presents.** (⚠️ **T109 corrects the count: it is 16 changes over 17
frames**, because **f3379 and f3380 share `crc32 5509053c`** — f3379 → f3380
changes nothing. The 17-present *window* is right; the 17 *changes* is not.) The city does something visible for **17 consecutive
presents** and then stops, abruptly, and the run is bit-identical for 1 619
presents afterwards. **That is the shape of the freeze and it is a
frame-resolved, 17-sample event** — which is why the next measurement is
**17 pictures, not another 5 000-frame diff.**

Three structures fall out of the gap histogram, each cross-checked against
something already committed:

- **A `+96` signature: f1302, 1307, 1398, 1403, 1494, 1499, 1590, 1595, 1686,
  1691** — period **exactly 96**, five times, each event with a 5-frame visible
  tail. **That is C-055's bank-`$03` updater seen from the picture side**; C-055
  measured the **writer** at f1205, 1301, 1397, 1493, 1589, 1685. **Two
  instruments, two tiers of the same system, agreeing on the period to the frame
  and on where it stops to within 2 frames.**
- **The dense run f2637–f3259**, gaps of 1, 2, 6, 7, 8, 16. **It ends on f3259**
  — exactly the frame C-052/C-053/C-058 measure as the one and only write of the
  city-state block. **The city's creation animation and the city-state
  initialiser stop on the same frame.**
- **The +96 updater stops too.** C-055's writer stops at f1685; the picture stops
  showing it at f1691. **The picture side and the writer side stop within 6
  frames of each other, and this is the second boundary in this project where
  two independent instruments agree on where something stopped.**

**And `make clock` independently reports the date crop's last change at f3378** —
which lands **inside** the f3365–f3381 burst.

> ### ⚠️⚠️ **RETRACTED by T109: "two instruments, one boundary, within 3 frames" is WRONG.**
> It is **two different events, in two different places on the screen, two frames
> apart.** The gate's crop is **x 55–125, y 2–21**. The last change anywhere on
> screen is at **f3381** and it is at **x 165–178, y 124–136** — **outside the
> crop**, and it is **the tile cursor being drawn**. The crop's own last change is
> **f3379**, and it is **the last frame of the game's own brightness fade**
> (§T109 below). The gate's header says the crop "covers the date and nothing
> else", so the cursor is invisible to it **by construction**. **Two instruments
> reporting the last thing that happened inside their own rectangle is not two
> instruments agreeing on one boundary.** See
> [`2026-10-03-t109-seventeen-pictures.md`](docs/measurements/2026-10-03-t109-seventeen-pictures.md).

**A correction to T107's reasoning, carried here because T107's conclusion is
still live and one of its two legs was void:**

| T107's reason | status |
|---|---|
| the framebuffer is pixel-identical for 3 400+ frames | **DOES NOT SUPPORT THE CONCLUSION.** A display path reading `$0B53` *live* would also produce an unchanging picture, **because `$0B53` itself never changes** — C-068 measures 66 writes to `$0B51`–`$0B5F` in 14 000 frames and **none after f3259**. A frozen source and a frozen display are indistinguishable from the framebuffer alone |
| `$0B53`/`$0B54` read `$0FA0` continuously f4260→f4899 while the picture is unchanged | **SUPPORTS IT, decisively.** This is the poke — *a change to the source with no change to the display* |

**The conclusion is right on the second leg alone, and the first leg should not
be carried.** **[Derived, Deck-native, from two measurements already committed]**
— no new run was needed for it and none was taken.

**Named as a lead for C-006 with no claim attached.** This says *when* the picture
stops and that the stopping is abrupt. **It does not say why**, it is **not** a
cause, and C-006 stays **OPEN**.

### T109: the seventeen pictures — **a cursor**, and a fade, and one wrong number

**Deck-native**, `build-instr`, `scripts/d_city.script`, four runs, **every one
with its own `COUNT_PC=0x009311` control**; the three 3 400-frame runs read it
**byte-identically at 6 804 046** = 2 001.2/frame, inside the established
1 929.9–2 007.9 band. **Host-only: none.** **[MEASURED, Deck-native]** —
[`docs/measurements/2026-10-03-t109-seventeen-pictures.md`](docs/measurements/2026-10-03-t109-seventeen-pictures.md).

**`SCREENSHOT_DIR` over f3360–f3390, 31 presents, pairwise-diffed pixel by pixel:**

| frame | changed px | bounding box | what |
|---|---|---|---|
| f3360–f3364 | — | — | **`crc32 = 00000000`, `luma = 0.000` — the frame is entirely black** |
| f3365 | 27 017 | x 40–295, y 0–223 | **the city appears**, `luma` 0 → 3.591 |
| f3366 / f3367 | 27 017 / 53 619 | x 40–295, y 0–223 | `luma` → 7.278 → 17.476 |
| f3368 … f3378 | **52 215 each**, 11 frames | x 48–295, y 7–223 | `luma` 23.3 → 82.4, **+5.9/frame** |
| **f3379** | 52 215 | x 48–295, y 7–223 | `luma` → 88.627 — **last change inside the date crop** |
| **f3380** | **0** | — | **bit-identical to f3379.** `crc32 5509053c` at both |
| **f3381** | **120** | **x 165–178, y 124–136** | **the tile cursor is drawn** |
| f3382 → f3390 | **0** | — | identical, and identical for the remaining 1 619 presents |

> **The city's last visible act before the framebuffer goes bit-identical for
> 1 619 presents is: the guest draws its tile cursor, once, at f3381.** **Not the
> date. Not the money. Not the RCI toolbar. A cursor** — a 14×13 yellow reticle
> with black corners over the empty-terrain dither, with the tool palette reading
> `Bulldoze / Area / $ 1`. And **T108's "17 consecutive changes" is 16 changes
> over 17 frames**, because f3380 changes nothing.

**The 15-frame burst is ONE REGISTER'S BRIGHTNESS NIBBLE, measured at the PPU**
(`WLOG_ADDR="2100:213F"`, bank `$00` lines only):

| frames | `$2100` = INIDISP |
|---|---|
| **f3258 – f3362** | **`$038F` — bit 7 set: the screen is FORCED BLANK** |
| f3363 … f3378 | **`$0300` → `$030F`, one step per frame: brightness 0 → 15** |
| f3379 onward | `$030F`, never written again |

**The `luma` column tracks it frame for frame.** And **the same register is ramped
seven times in one run** — f300→f505 up, f506→f520 down, f565→f580 up,
f1132→f1146 down, f1148→f1163 up, f3244→f3258 down, **f3363→f3378 up**.
**Menus fade the same way the city does.**

> ### ⚠️ So `make clock`'s "last change" is **the end of a fade, not a date advance.**
> Hashing the gate's own crop over the 31 pictures: it changes on **every** frame
> f3365…f3379 and **never again**. **The date glyphs did not move; the light
> changed.**
>
> **This does not weaken the gate and nothing here weakens it.** The gate requires
> ≥2 distinct date images **after f3600**, and there are none — the correct
> verdict for a frozen city. **What is wrong is the number the gate prints.**

**⚠️ CONF-24: every frame number `make clock` prints is off by one.**
`clock-gate.sh` captures with `SCREENSHOT_FROM=0`; `SNESRECOMP_SCREENSHOT_DIR`
names files `present_NNNNNN.ppm` where **NNNNNN is a window index, not a frame**;
and the detector derives the frame from the filename
(`clock-gate.sh:212`) while `presents.csv` sits unread in the same directory.
**Measured directly**: a 40-frame run with `SCREENSHOT_FROM=0` gives
`present 0 → frame 1`. **So `LAST_CHANGE` is always `frame − 1`, and the
"last change f3378" quoted in three documents was f3379.** It changes no verdict
and the gate is still red; **the number should not be quoted as a frame.**

> #### ✅ CLOSED 2026-10-03 (T110) — the derivation is FIXED, not the quoting
>
> `clock-gate.sh` now **joins `presents.csv` on the present index**, and every
> frame number it prints is a true frame. Falsified in three directions,
> Deck-native: the normal path joins; **`presents.csv` removed → exit 3**; **a
> capture with no csv row → exit 3**. There is deliberately **no silent fallback**
> to parsing the filename, because a fallback is how this would look fixed while
> still being wrong. The gate's banner now states which number space it is in.
>
> `f3378` was a present index; the frame is **f3379**. The boundary itself is
> unchanged: the ramp ends at f3378 and the picture's last change is f3381.
> `make clock` reads `1 distinct date images after f3600` and is **red** before
> and after. Full record: `docs/CONFLICTS.md` CONF-24.

#### Where the rendered date comes from — **MEASURED, and it is `$0B53`/`$0B55`**

> #### ⚠️ RETRACTED (R-045), 2026-10-03 by T111. What follows was the strongest
> statement available and **it was wrong.** A poke placed inside the live window
> and compared frame by frame against a no-poke control **does** move the date:
> `$0B53` → `$0FA0` renders **`1952 JAN`**, `$0B55` → `$0005` renders
> **`1900 MAY`**, both within one frame, at f3365. T107's poke was void because
> it landed at f4260 on a screen bit-identical since f3381. **The display path is
> intact.** The measurements below survive and are the *mechanism* — the CPU never
> writes `$2119`, and VRAM changes anyway, because HDMA writes it.
> Full data: `docs/measurements/2026-10-03-t111-date-display-path.md`.

**The original negative claim, kept so the retraction is legible:**
**nothing redraws the date glyphs after the city is built.**
`WLOG_ADDR="2100:437F"` over 3 400 frames, 1 050 742 logged writes:

- **CPU writes to `$2119` (VMDATA) — the only way a CPU puts a byte in VRAM:
  2 082, and every one is to bank `$7E`, i.e. WRAM. ZERO to bank `$00`.**
- HDMA/DMA channel registers `$4300-$4306` and `$4310-$4315` (bank `$00`):
  **thousands of writes per run.** `$4304/$4305` and `$4314/$4315` exist only on
  HDMA channels.
- The steady state is **~29 PPU register writes per frame and not one VRAM data
  write.**

**So VRAM is filled by HDMA on channels 0 and 1, once, during city creation.**
There is no live path from `$0B53` to the screen for a poke to travel, which is
the real support for "the rendered date is not read live from `$0B53`" — **and it
is a different argument from the one T107 gave.**

> **T107's remaining leg does not survive either.** *"A poke of `$0FA0` into the
> year field at f4260 left the picture unchanged"* — **the picture's last change
> is f3381 and the first poke is f4025.** A screen that stopped moving before the
> poke cannot report that the poke did nothing. **The conclusion may be true;
> that evidence does not support it and this file does not carry it as support.**

**The next step is one run and it is fully specified:** HDMA channel 0's table
pointer is `$4302/$4303` and channel 1's is `$4312/$4313`, both written every
frame, so `WLOG_ADDR="4300:4315"` names the table's WRAM address; then
`SNESRECOMP_WRAM_DUMP_AT=<frame>` (**decimal** — CONF-22) reads the table, whose
entries are `header` (bits 0-6 line count, 0 = 256; bit 5 indirect; bit 7
do-not-repeat) plus `line_count+1` data bytes. **The entry covering the HUD
scanlines names the source offset. Not done here, and not guessed.**

#### Three traps this section paid for, all of them instrument-shaped

1. **The 32 768-write "VRAM upload" at f3271–f3277 is a DELAY LOOP.** ~4 822
   writes of **`$0000` to `$00:2118`** per frame for seven frames, **32 768 in
   total, `Y` counting `$8000 → $0000`**. The ROM agrees (LoROM offset
   `0x00690`): `LDY #$8000 / STX $2118 / DEY / BNE $869D`. **32 768 × 2 bytes =
   65 536 = exactly the size of VRAM** — a write census alone would have reported
   *"a full 64 KB VRAM upload on the frame bank `$03` dies"*, which is a very good
   story and completely false. **Third instance of C-054's / R-034's shape: a loop
   that writes N times is not a transfer.** The only real work there is `$00:86AD`
   clearing two 64-byte WRAM shadow blocks to `$80`.
2. **`WLOG_ADDR` filters on the 16-bit address and IGNORES THE BANK**
   (`cpu_state.c:157-159`), so a `2100:437F` range also catches the game's
   **WRAM shadow of the PPU registers at `$7E:2100-$213F`**, written every frame.
   Read carelessly, `7E:2100=0F` is a beautiful `INIDISP = $0F`; it is **a byte
   written to WRAM**. **Every register claim above is from bank `$00` lines
   only.** CONF-20's shape again: a filter that matches a string without matching
   the thing the string is about.
3. **The positive control read `0`** on the 40-frame run. **Legitimately**:
   `scripts/d_city.script` begins `wait 250`, so `$00:9311` has not been reached.
   Falsified by measuring the ramp rather than asserting it — **f40 → 0, f200 →
   67 382, f400 → 544 150, f3 300 → 6 626 029, f3 400 → 6 804 046.** **So the
   control has no known-good value below ~f250**, and the rule is *"reproduce the
   known-good value **or explain the difference**"* — not *"it must be non-zero"*.

#### Three discrepancies T108 did **not** resolve — recorded, not smoothed

1. **f1459 vs f3381 are two different routes and must not be compared.** T107's
   `md5` put the last picture change at **f1459**; T108 puts it at **f3381**.
   T107 ran `scripts/cheat_probe.script`, T108 ran `scripts/d_city.script`. Which
   route reaches the city sooner is **not measured**. *This project nearly made
   that comparison itself before reading which script each run used — CONF-13's
   lesson, learned a third time.*
2. **1 877 presents over f0–f4890 does not reconcile with 5 000 presents over
   5 000 frames.** `SCREENSHOT_DIR`/`PRESENT_LOG` is **per present**, and T108's
   run shows presents tracking frames **1:1**, so T107's run either presented
   fewer times than it simulated frames or covered a narrower range than its text
   says. **Not established, and not guessed. OPEN.**
3. **`scripts/check-retracted-claims.sh` was observed to flip once and did not
   reproduce.** One `make check-claims` printed
   `VIOLATION docs/RE_CITY_FREEZE.md:47  R-002 (refuted) asserted without a
   retraction marker`, on a line that reads *"The city does not load" … is
   RETRACTED* — i.e. **the phrase sits inside its own retraction marker**. A
   separate invocation printed section 3's heading as `… disagrees with the ledger
   (42)` where it now consistently prints `(34)`. **Eight subsequent runs — five
   with the file untracked, one staged, two earlier — all printed `census: 42
   rows = 34 refuted`, heading `(34)`, `RESULT: PASS`, no violations.** (Those are
   the numbers the guard **printed at the time** — the ledger has since grown past
   them (it holds 50 rows / 42 refuted today); **a historical transcript is not
   re-baselined**, and that is why this one still says 34.) **So it
   does not reproduce, no cause was established, and none is offered**; the
   candidate (a partial or stale read of the ledger or of a file in scope) is
   **UNVERIFIED**. **Filed rather than dismissed because the guard that produced
   it is the one whose absence let a run of retractions stand unchecked, and its
   self-test covers seeded phrases, not this.** Interim rule: **run
   `make check-claims` at least twice before believing a green, and if it ever
   fires on a line carrying its own marker, keep the output.**

#### DoD Rule 0b, and the guard that enforces it

> **A cheat must never make `make clock` pass.** The delivery gate must keep
> requiring the clock to advance **on a stock build, in a stock configuration,
> with no cheats and no pokes.** A cheat or a WRAM poke that turns the gate green
> is a **DIAGNOSTIC** — it names the flag that holds the gate. It is **not**
> evidence that the game works, **not** evidence that the emulation is faithful,
> and it must never be reported as either. `make clock` is red today and stays red.

Stated as **Rule 0b** in [`docs/DEFINITION_OF_DONE.md`](docs/DEFINITION_OF_DONE.md)
and at the top of [`docs/CHEAT_CODES.md`](docs/CHEAT_CODES.md), and enforced
mechanically by [`scripts/check-cheat-gate.sh`](scripts/check-cheat-gate.sh)
(`make check-cheat-gate`): it fails if any script under `scripts/` acquires the
ability to write guest memory on a run it drives, and if the rule is deleted from
either document. **It reads code, it does not run the game, and it is lexical — a
floor, not a proof.**

**The guard was itself wrong three times, and its self-test caught all three** —
which is the only reason it is in the tree at all:

1. **It cried wolf on this project's own delivery gate.** `clock-gate.sh` passes
   `"$PWD/scripts/d_city.script"`; the checker could not resolve the literal
   `$PWD` and reported the gate as unsafe. **A guard that cries wolf on the
   project's own corpus is worse than the hole it closes** — CONF-14's lesson,
   learned again one commit after it was written down.
2. **A leading dot hid a gate script from it.** `scripts/*.sh` does not match
   `scripts/.hidden.sh`, so a hidden script invoking a write knob passed silently.
   Fixed with `dotglob`; the case is now a permanent self-test assertion.
3. **A defect in the self-test itself**: `"$0" | grep -q …` under `set -o
   pipefail` reported FAIL while the guard was working, because `grep -q` exits
   at the first match and SIGPIPEs the producer.

`make check-cheat-gate-self-test` → **`SELFTEST PASS: 5/5`**, including a
**positive control** (a script with no write verb is *not* flagged) and both new
checks demonstrated on **untracked** files (CONF-11).

#### Two things this ticket found that are not about cheats

**Our own shipped `SIMCITY_GODMODE` pokes an address nobody has ever verified.**
`src/gen_stubs.c:70-73` writes `999,999` to `$7E:04B7`–`$04B9` under the comment
*"game uses 24-bit at `$7E:04B7`"*. **`$04B7` appears nowhere else in this
repository** — measured: `grep -rn '04B7' docs/ README.md scripts/` returns
nothing — **and it contradicts the measured address**, which is **`$0B9D`**
(`$4E20` = 20 000, every sample, both builds). Rubric **E-06** and **E-01** failing
on shipped source. **Recorded, not patched** — the fix is not "change `$04B7` to
`$0B9D`", it is to *establish* which address is money, and the evidence that would
do that requires a city that renders. **C-006 again.**

**The debug menu was NOT reached, and the blocker is one line.**
`snesrecomp/runner/src/desktop/host_main.c:3774`:

```c
uint32 inputs = human | (g_gamepad[1].axis_buttons << 12);
```

`GamepadInfo` carries `modifiers` — the **button** mask — and `axis_buttons`, the
d-pad-as-axis mask used by the mouse shim. **Line 3774 reads only `axis_buttons`
for the second pad**, so **controller 2's face buttons are dropped on the floor**;
the script language's `press` verb sets `g_pad_buttons`, which feeds **player 1**
only (`host_main.c:1690`). **So eight of the presses the published sequence needs
— A, Y, B, X, Select, Start, R, L — cannot be delivered at all.** **[MEASURED by
reading, 2026-10-03.]** **Size estimate, not done:** read `g_gamepad[1].modifiers`
into the pad-2 half of the joypad word (**~2 lines**, layout in
`snes/joypad.h`) plus a script verb targeting pad 2 (**~10 lines**) — **~15–30
lines in the submodule, plus a verification run.** **It is not a one-line change,
because the joypad word packs two pads and getting the byte order wrong produces a
controller that answers to the wrong player — a bug that looks exactly like "the
cheat did not work."** And even with it, the menu may be unreachable, because
reaching the *"See you soon!"* screen needs **END** chosen and this project's route
never gets there. **Recorded as the blocker it is, not worked around.**

#### Is PAR / Game Genie support worth implementing? — no, and not for the usual reason

Not for player convenience: **the game does not run**, so there is no city to cheat
in (`make clock` red, C-006 OPEN). Cheats for a city that does not simulate are a
UI for a feature that does not exist. What the verification *did* establish is the
size, and it is small:

| piece | cost |
|---|---|
| the whole `7E` family | **zero new code** — it is `poke`, which already exists (`host_main.c:823`) |
| arithmetic / logic (`01`/`03`/`05`/`D0`/`D1`/`D3`) | **~80–120 lines** — a small op table beside `ParseHexBytes` |
| compare-and-freeze (`DD`/`DE`) | **~40–60 lines** — a per-frame predicate against `g_ram` |
| ROM patches (`80`–`BF`) | **~150–250 lines**, plus a decision about when re-application is correct on a faulted code page — the only genuinely invasive part |
| **total** | **≈ 300–450 lines, all in the submodule, none in this repository** |

**What would change the answer:** the game simulating, plus a player asking.
Neither has happened. The probe is a **script**, not a gate; no `make` target runs
it, and `make check-cheat-gate` exists precisely to keep it that way.

### Where the record lives

This file is the entry point and is **not** the maintained record. Fifteen
entries, covering seventeen files, are:

| file | what it holds |
|---|---|
| [`docs/CAUSE_CLAIMS.md`](docs/CAUSE_CLAIMS.md) | every causal claim in this project, classified MEASURED / INFERRED / RETRACTED / OPEN, with the instrument or the reason there is none |
| [`docs/CONFLICTS.md`](docs/CONFLICTS.md) | every contradiction found between docs, code and git history, with the command that found it |
| [`docs/RE_CITY_FREEZE.md`](docs/RE_CITY_FREEZE.md) | the chronology — 44 entries, each with a state banner, and a maintained index at the top |
| [`docs/ROADMAP.md`](docs/ROADMAP.md) | **the tracked mirror of the board.** `aes/kanban.md` is not versioned, so this is what a clone gets; where they disagree, this file wins, because that is what a clone actually receives |
| [`docs/measurements/2026-10-02-deck-interp-histogram.md`](docs/measurements/2026-10-02-deck-interp-histogram.md) | the whole-run histogram this file's table reproduces, and the three instrument limits it established |
| [`docs/measurements/`](docs/measurements/) | raw measurements, the exact commands, and **what each instrument cannot see** |
| [`docs/measurements/2026-10-03-t108-present-crc-timeline.md`](docs/measurements/2026-10-03-t108-present-crc-timeline.md) | **T108** — the per-present `crc32` timeline on the `d_city` route: **206 picture changes, the last at f3381, then 1 619 bit-identical presents**, agreeing with `make clock`'s own f3378 to within 3 frames; C-055's **+96** updater reproduced on the picture side; the city's dense animation **ending on f3259**, the city-state write frame. Also **a correction to T107's reasoning** and **an OPEN observation that `check-retracted-claims.sh` flipped once** |
| [`docs/measurements/2026-10-03-t106-index-immediate-census.md`](docs/measurements/2026-10-03-t106-index-immediate-census.md) | **T106** — the `$A0/$A2/$C0/$E0` census (245 sites, 0.3141% of steps), the 65816 width rule settled from published documentation and from the ROM's own instruction stream, the experiment that refutes the proposed fix — **and the mis-specified first attempt whose own mechanism check refuted my explanation of it** |
| [`docs/CHEAT_CODES.md`](docs/CHEAT_CODES.md) | a **verified** third-party cheat table — what checked out, what did not, and why PAR/GG support is **not** worth implementing yet. Also where **DoD Rule 0b** lives: a cheat must never make `make clock` pass |
| [`docs/AES_CHAIN_RUN.md`](docs/AES_CHAIN_RUN.md) | the AES chain run by hand on 2026-10-03, leading with **three places its own inputs were wrong about this project** — and an explicit statement that no phase was run by the tool that normally runs it |
| [`docs/CLAIMS_REGISTER.md`](docs/CLAIMS_REGISTER.md) | the index of what is retracted, superseded or unverified |
| [`docs/DEFINITION_OF_DONE.md`](docs/DEFINITION_OF_DONE.md) | the standard of proof — **no acceptance criterion may be satisfied by a claim** |
| [`docs/review/RUBRIC.md`](docs/review/RUBRIC.md) | the pre-registered review rubric (hash-pinned in `RUBRIC.sha256` — **do not edit**) |
| [`docs/review/REVIEW-2026-10-02b.md`](docs/review/REVIEW-2026-10-02b.md) · [`-02c.md`](docs/review/REVIEW-2026-10-02c.md) | the last two full reviews — **REJECT, 3 BLOCKERs**, all since closed; and the review of the C-041 work |
| [`docs/review/VALIDATOR_DEFECTS.md`](docs/review/VALIDATOR_DEFECTS.md) | the T101 review validator's **three self-defects, found by seeded violation**, two fixed and one re-verified — and why none of it reaches a clone |

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

**Its validator has three known self-defects and was attacked on 2026-10-03
with seeded violations.** Two were real and are **fixed**; the third was already
fixed and was **re-verified**. Because `aes/` is gitignored, **none of that
reaches a clone** — the tracked account is
[`docs/review/VALIDATOR_DEFECTS.md`](docs/review/VALIDATOR_DEFECTS.md), and the
short version is: one check used to **pass when its subject had been deleted**
(vacuous — renaming the string it grepped for turned it green), and one used to
**fail when the record was corrected** (it pinned the retraction count at `28`,
so T102's legitimate R-037 broke it). **Both are fixed and both re-falsified;
both fixes are local-only, which is the same state as the review itself.**

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
| `make clock` | **the city actually simulates** | **FAIL — `1 distinct date images after f3600`**, `$0B53 = 076C` → year 1900. Re-measured 2026-10-03: `scripts/clock-gate.sh` exits **1**, `make clock` exits **2** (make's code for a failed recipe) | **2** via make, **1** via the script |
| `make check-claims` | no retracted claim asserted without a marker | PASS | 0 |
| `make check-causes` | every causal assertion carries provenance | PASS | 0 |
| `make check-causes-self-test` | the guard still fires on the tree it was written for | PASS | 0 |
| `make check-claims-self-test` | the ledger guard has been seen to fail | PASS — both seeded violations confirmed detected | 0 |
| `make check-cheat-gate` | no gate script can turn a WRAM write into a clock result (Rule 0b) | PASS | 0 |
| `make check-entrypoints` | the tracked entry-point documents are tracked, non-empty, and open with the expected heading (CONF-23) | **PASS — 12 of 12** | 0 |
| `make check-entrypoints-self-test` | it has been seen to fail, on the real tree, on this file | **PASS — `SELFTEST PASS: 9/9`**, including `README.md` truncated to 0 bytes → exit 1, then restored byte-for-byte | 0 |
| `make review-check` | the 2026-10-02 review's BLOCKERs are closed | **PASS — 17 confirmed, 0 refuted**; 3 ROM-dependent checks skipped (no `--rom`) | 0 |
| `make review-check-c041` | the C-041 review's claims reproduce | **PASS (bounded) — 26 confirmed, 0 refuted**; it refuses to total, and rubric **E-04 stays UNVERIFIED** | 0 |
| `make clock-self-test` | the clock detector still sees a live screen | PASS — 16 distinct date images over 1 200 frames, last change f1163 | 0 |
| `make retraction-count` | the retraction count, computed | **50 rows = 42 refuted + 6 superseded + 2 invalidated-premise** | 0 |

**⚠️ Read the two PASS rows at the top of that table with the preamble above in
hand.** `make check-claims` and `make check-causes` **both passed on a 0-byte
`README.md`** when that was measured on 2026-10-03. What they prove is that no
*present* claim is retracted or unprovenanced — **not** that the document
carrying those claims exists, has content, or parses. **CONF-23; the second half
is now covered by `make check-entrypoints`, the first half never will be.**

**CORRECTION, measured 2026-10-03: this file said "`make clock` exits 1, not
2", and that is wrong about `make`.** The gate script is right; the wrapper is
not. All three, measured:

| command | exit | why |
|---|---|---|
| `scripts/clock-gate.sh --frames 6000` | **1** | the FAIL verdict |
| **`make clock`** | **2** | **GNU Make 4.3 maps any failed recipe to exit 2** |
| `scripts/clock-gate.sh --help` | 0 | usage |
| `scripts/clock-gate.sh --nonsense` | 2 | unknown flag |

So **`scripts/clock-gate.sh` has no exit-2 path of its own**: exit 1 is "a city
is loaded and its date did not advance", exit 1 is *also* "no city was loaded at
all" but with its own distinct message, and exit 2 is only the usage path. **The
2 that `make clock` returns is make's, not the gate's**, and a CI job reading
`make clock`'s status as the gate's verdict is reading make's opinion.

**Red is red; this file records all four codes so nobody has to guess which
failure they are looking at.** The old sentence was wrong in the direction that
matters most here — it invited a reader to treat a *make* failure as evidence
about the *gate*. Found by running the gate instead of quoting it, which is the
`aes-project-manager` skill's own first rule: **do not answer a question a command
can answer.**

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

**`make check-claims` does not check the number, and that hole is open (CONF-14).
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
**And CONF-14 fired here again, on this file, one commit after being written
down:** line 22 read *"retracted **29** claims out of **37** ledger rows"* while
`--count` said **30 of 38**, and the gate printed `(no violations)` — the count
pattern is forward-only and that sentence puts the number *after* the word.
**CONF-14 is not closed and nothing in this file should be read as closing it.**

> **The phrase guard has a second, independent hole (CONF-20), also OPEN.**
> **And a third, which was not a hole in either guard but in what both of them
> were being asked for (CONF-23): neither asserted that the files it reads are
> non-empty.** `5cbf5fd` shipped this file at **0 bytes** and `check-claims`,
> `check-causes` and `check-cheat-gate` all printed `RESULT: PASS` on it —
> re-measured, see the preamble above. **`git ls-files` says a file is in scope;
> an empty file satisfies "no violations" perfectly.** **Closed by
> `make check-entrypoints`** — but note what it closes and what it does not: it
> proves the documents **exist and are non-empty**, and it still proves nothing
> about whether what they say is **true**.
`docs/CAUSE_CLAIMS.md` asserted R-038's refuted `$03C87F` clause as present-tense
fact — a faithful **paraphrase**, not a quotation — and **both** `make
check-claims` and `make check-causes` printed `RESULT: PASS`. The ledger's
`phrase` for R-038 is the narrow literal `03C87F is the second byte of`, and a
lexical guard cannot see a paraphrase; and D3.2 checks only that the ledger's
`where` column is **non-empty and resolves to a real file**, which says nothing
about whether it is **complete**. Falsified in both directions on **untracked**
files: the exact phrase seeded unmarked → `VIOLATION`, exit 2; the paraphrase
seeded unmarked → `(no violations)`, exit 0. **The instance is fixed; the class
is open.** A lexical retraction guard proves a retracted *string* is not
restated — not that a retracted *claim* is not.

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
| Steam Deck (Zen 2), **compiled on the Deck** — *environment-fidelity caveat above* — **measured 2026-10-03** | Release | 56.86 median (5 runs) | **7.619** | **1.538** |
| i5-8500T, built in place | Release | 54.50 median (5 runs) | 6.62–10.82 | — |
| i5-8500T, cross-built binary | Debug (`-O0`) | 42.4 | 9.52 | 5.90 |

**Three claims about this table were retracted, and the reasons are instructive.**

- The Deck row's `2.45` ms was **never re-measured** and is ledger **R-012/R-021**.
  The old row also claimed `upload-present` **8.13 ms**, measured on the Deck at
  **1.007 ms** — the opposite side of the guest by a factor of four and a half.
- **"The emulated 65816 is not the bottleneck"** is **RETRACTED** (ledger R-023,
  R-025). It rested on `guest` being 2.45 ms against a large `upload-present`.

> #### ⚠️ The Deck row above has now been measured **five** times with **five**
> different answers, and the register does not say so. This is the most
> important caveat on this page.
>
> | when | `guest` | `upload-present` | `deadline-wait` | source |
> |---|---|---|---|---|
> | 2026-10-02, at `9624f0e` | 4.502 | 1.007 | 11.275 | superseded; cited in R-012/R-021's replacement |
> | taken at `ec4cabe`, **never re-measured** | **4.511** | **6.540** | **5.916** | `docs/CLAIMS_REGISTER.md` §3 — the register's own re-measurement of the same stage. **This is the row that produced "the frame is oversubscribed"** (4.511+6.540+5.916 = **16.97 ms > 16.67 ms**), and **that sum is itself retracted** — it added *work* to *sleep*. **See the correction below** |
> | **2026-10-03, T104** | **7.619** | **1.538** | **6.930** | `docs/measurements/2026-10-03-t104-c87x-scan-loop.md` §7 — Deck, **solo**, 600 presents in 10.552694 s, median **56.86 fps**, spread **0.1%**. raster-capture 1.390 |
> | **2026-10-03, T105** | **4.572** | **1.006** | **11.131** | `make perf`, Deck, **solo**, load average 0.13 before, 600 presents in **10.548473 s**, median **56.88 fps**, spread **0.0%**. raster-capture **0.785** |
> | **2026-10-03, T108 — five readings from one `make perf` invocation** | **4.480 / 4.506 / 4.524 / 4.484 / 4.499** — **range 4.480–4.524** | — | — | Deck, **SOLO**, **load average 0.26** recorded before the run, `PERF: PASS — median 56.88 fps over 5 runs, spread 0.0% (limit 10%)`. This is the run every commit since has quoted as its baseline, and it is the **fifth** `guest` reading, not a fourth |
>
> **All five stand. None is retracted by any other, and the reason is the reason
> for five rows rather than one:** they are **different builds and different
> instrument configurations**, not five readings of one thing. Collapsing them to
> a single number would be exactly the error this ledger exists to prevent, and
> the honest statement is that **the variance is itself the finding**.
>
> **What five rows make visible that four did not.** The newest row (`guest`
> ≈4.50) reproduces the **oldest** one (`9624f0e`: 4.502 / 1.007 / 11.275) to
> within **1.6% on guest and 0.1% on upload-present**, across two days and two
> code trees. **T104's row is the outlier on all three work stages
> simultaneously** — guest ×1.69, upload-present ×1.53, raster-capture ×1.77 —
> **while fps is within 0.04%** (56.86 vs 56.88). Two *different* solo load
> averages (0.13 and 0.26) and two different builds produced the same fps and
> the same ≈4.50 guest.
>
> **That simultaneity is the observation, and no cause is offered for it.** All
> three work stages moving together while the frame rate does not move is the
> signature of the *machine* being slower rather than the *code* being slower —
> but that is an **[INFERRED]** reading of a pattern, not a measurement, and this
> file does not convert it into one. **OPEN**: what distinguishes the two
> configurations. Candidate explanations that are *not* excluded include CPU
> frequency/thermal state and whether `build/` on the Deck had been rebuilt
> since its last content change; **neither is measured and neither is asserted.**
>
> `guest` across the five Deck rows: **4.502, 4.511, 4.499, 4.572, 7.619.**
> **Four cluster at ≈4.50** — spread **1.6%** across 4.480–4.524 — **and T104's
> 7.619 stands alone, ×1.69 off the cluster.** An earlier revision of this file
> said *"four rows"* and listed `4.502, 4.511, 4.572, 7.619`; the fifth reading,
> **4.499**, was measured and **not carried**. It does not change the shape — it
> tightens the cluster to four points at ≈4.50 and leaves T104 as the single
> outlier. `upload-present`: **1.007, 6.540, 1.006, 1.538** — spread **6.5×**.
> **A 6.5× spread on a host stage is not understood, and this file does not
> pretend otherwise.**
>
> **What each supports:**
>
> - **`guest` >> `upload-present` on the Deck** — 4.572 vs 1.006 (**4.55x**) on
>   T105's row, 7.619 vs 1.538 (**4.95x**) on T104's, 4.502 vs 1.007
>   (**4.47x**) on the `9624f0e` row. **Three rows, one direction**, and it is
>   the **opposite** to the dev-host claim `perf-gate.sh`'s own header still
>   carries (*"on this hardware the host's present path costs more than the
>   emulated 65816"*). **The ratio is the most stable quantity in this table;
>   the absolute values are not.**
> - **The Deck frame is not oversubscribed, on every row.** T105: 4.572 + 1.006 +
>   0.785 raster-capture = **6.36 ms of work against a 16.67 ms budget**, with
>   `deadline-wait` 11.131 ms filling the remainder. T104: **10.55 ms**. **Same
>   verdict, a factor of 1.7 apart.** `fps` is 600 / 10.548473 = **56.88** (both
>   terms given so it can be recomputed).
> - **The `ec4cabe` row supports neither, and its 16.97 ms figure is retracted**
>   — `docs/RE_CITY_FREEZE.md:3191` records the retraction in the author's own
>   words: *"Eu estava errado sobre a folga … somei trabalho com a espera"*. It
>   added **work** to the **wait** and called the sum oversubscription.
>   `deadline-wait` is slack spent, not work added to the budget. So
>   `docs/ROADMAP.md`'s *"16.97 ms against a 16.67 ms budget, so the frame is
>   oversubscribed on the Deck"* rests on that sum and **is wrong for the same
>   reason.**
> - **No figure here supports "pacing dominates, not the CPU."** On T104's row
>   `deadline-wait` is 6.930 ms and `guest` alone is 7.619 ms, so the guest
>   *exceeds* the wait. **That claim is removed, not restated.**
>
> **No older figure is retracted by this.** Three measurements of the same stage
> with three answers is C-048's lesson (instrumentation moves the histogram)
> landing on host stages instead of guest PCs: these are different builds and
> instrument configurations, and picking one and calling it *the* number would be
> the error the ledger exists to prevent. **The variance is the finding.**
>
> `guest` is the emulated 65816; `upload-present` is the host's SDL present;
> `deadline-wait` is sleep. **Keep them apart — they are different costs with
> different owners**, and conflating them has already produced a wrong conclusion
> in this project twice.
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
```

### Step 1 (mandatory): regenerate the AOT code from your own ROM

**`src/gen/` is derived from copyrighted ROM data and is never committed**, so a
fresh clone has no generated C and **`make build` will fail**:

```
CMake Error at snesrecomp/runner/runner.cmake:890 (message):
  /path/to/src/gen is empty -- run `bash tools/regen.sh` with your
  verified ROM before building.
```

That is the error, verbatim. Run:

```bash
bash tools/regen.sh "SimCity (USA).sfc"    # or: --rom /path/to/your/copy.sfc
```

It checks the ROM against `rom_identity.txt` and writes `src/gen/*.c` plus
`src/gen/program_manifest.json`. **It needs your own legally-owned copy of the
ROM**; nothing in this repository can supply one. `cmake --build build --target
regen` does the same thing if you prefer it through the build system.

### Step 2: build

```bash
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
   > **And under parallelism the control is per-*run*, never per-batch.** The Deck
   > runs **up to 3 concurrent** heavy runs (it is ~36% CPU-busy: ~10.55 ms of
   > work against a 16.67 ms budget, with `deadline-wait` 6.930 ms of it spent
   > sleeping), and concurrency makes **"prints nothing" triply ambiguous** — a
   > dead machine, a dead instrument and a real zero are the same three log lines.
   > **`COUNT_PC=0x009311` is the control and it has a known-good value: 6 626 029
   > at 3 300 frames** (byte-identical across five runs), **27 019 166 at 14 000**.
   > **Reproduce the known-good number or explain the difference; a control that
   > reads `0` means that run produced no measurement at all.** Full text in
   > `docs/DECK_RUNBOOK.md`, *Parallel runs*.
   > **`make perf` is strictly solo** — it is the only wall-clock gate and the one
   > that has flapped (the same unchanged binary: 48.38 FAIL / 51.52 PASS / 46.99
   > FAIL against a threshold of 50). Under concurrency it measures the
   > scheduler, not the emulator.
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
   > **⚠ Trap 8 has a blind spot, and the exception is the OPPOSITE
   > instruction.** `SNESRECOMP_WRAM_DUMP_AT` is parsed
   > **`strtol(a, &end, 10)`** (`host_main.c:1539`) — **base 10, pinned**.
   > `0x0CEC` parses as **`0`**, and the run then emits **one clean,
   > correctly-formatted dump of frame 0** with no warning. **For this knob,
   > writing the `0x` prefix is what breaks it** — following trap 8 faithfully
   > is the failure. **DECIMAL, always** for `WRAM_DUMP_AT`; its siblings
   > `WRAM_DUMP_LO` / `_HI` / `_FRAME` are `strtol(…, 0)` and **do** want the
   > prefix; `WLOG_ADDR`'s `lo`/`hi` are `sscanf("%x:%x")`, plain hex and never
   > octal. **Four parse conventions across five sibling knobs, two of them the
   > same `strtol` family one argument apart.** Measured 2026-10-03,
   > **CONF-22**. The transferable lesson: **a rule about a *convention* needs a
   > per-knob table, and sibling knobs are exactly where a convention gets
   > tested** — CONF-15, CONF-22 and the `WLOG_ADDR` exception are three data
   > points a table would have caught.
   >
   > #### ⚠ The per-knob table the rule above should have had from the start
   >
   > | knob | parse | what you write | measured |
   > |---|---|---|---|
   > | `SNESRECOMP_COUNT_PC` | `strtoul(e, NULL, 0)` | **`0x…`** | **CONF-15 / R-037** — a bare leading `0` is octal |
   > | `SNESRECOMP_WRAM_DUMP_LO` / `_HI` / `_FRAME` | `strtol(v, NULL, 0)` | **`0x…`** | same defect, same family |
   > | `SNESRECOMP_WRAM_DUMP_AT` | **`strtol(a, &end, 10)`** | **DECIMAL — the `0x` prefix makes it frame 0** | **CONF-22**, measured 2026-10-03 |
   > | `SNESRECOMP_WLOG_ADDR` `lo`/`hi` | `sscanf("%x:%x:%511[^\n]")` | plain hex, **never** octal | hex-16 by construction |
> | `SNESRECOMP_CYC_WATCH` `lo-hi` | `sscanf("%lx-%lx")` | plain hex, **never** octal | base is explicit |
    > | `SNESRECOMP_SCREENSHOT_FROM` / `_TO` | `strtol(v, NULL, 0)` | **plain decimal, no leading zero** | added 2026-10-03 (T108 recipe, `host_main.c:1627-1630`). `03360` is **octal 1824** and silently selects a different window — the sixth knob in this family, and the reason this table exists |
    >
    > **Five knobs, four conventions, and two of them are the same `strtol` family
    > one argument apart.** Trap 8's blanket rule is right for four rows and
    > **actively wrong for the fifth** — and it was followed faithfully when it
    > broke CONF-22. **This table is the rule; the sentence above it is the
    > approximation.** It now has six rows and the same shape of hole: a *sixth*
    > knob was found by reading source while writing a recipe, which is the only
    > way any of them have ever been found.

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

### T104 added three capabilities to that list — the instruments were already here

Every one of these existed before T104 and is documented elsewhere in this file
**by its limitation only**. CONF-19 is that shape of gap: a reader choosing an
instrument is given what it cannot do and never what it can.

| knob | what it is *also* good for — the part that was not written down |
|---|---|
| `SNESRECOMP_CYC_WATCH=LO-HI` | **it prints the opcode byte the CPU actually fetched** (`op=$%02X`).** One run settles *is this address an instruction boundary in the executed stream, or only in the ROM decode?* Documented until now only as a cycle-accounting tool that is blind to AOT |
| `SNESRECOMP_WLOG_ADDR="LO:HI:PATH"` + `WLOG_STATE=1` | per-write **register state including `X` and the writing `IPC`**, across **both** engines. This is how T104 measured that the `$03C87x` loop is a zero-fill walking `X` from `$0000` to `$14FF` |
| `SNESRECOMP_INTERP_TRACE_FRAMES=lo-hi` | as above — **but never answer a transition question from a filtered stream.** A `$03C87x`-only filter produced four phantom loop re-entries that were frame boundaries; the full stream gives 5 entries and 1 exit |

**The rule the first of those earns, and it is the third failure of one rule**
(`$03:D947`/`$03D94B`; R-034's next-PC length; **R-038**): **a byte-boundary
question about the ROM cannot be answered by, or exported into, a claim about
execution. Where the two disagree, the fetched opcode byte settles it.**

Concretely, R-038: C-069 read `$03C87F` from ROM bytes as "the second byte of
`8D D0 F6`, therefore not an instruction boundary" and recorded it as
unattributed. `CYC_WATCH` shows the byte fetched there is **`$D0`** — it is the
loop's **only** branch, taken 36 339 times. The byte that never executes is
**`$03C87E`**: 0 of 2 676 196 trace lines.

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

## Method — how a number in this repository gets to exist, and what kills it

This project has produced **47 retraction-ledger rows, 39 of them refuted**. The
retractions were not caused by bad luck or by a careless author. They were caused
by a specific, repeatable failure: **a measurement was made, and then the number
was copied forward instead of re-derived.** So the method below is not
documentation hygiene — it is the actual defence, and it is written down here so
that a future session inherits it instead of rediscovering it.

**The ledger is the source of truth for what is refuted, not this prose.**
`make retraction-count` computes it. If a figure below disagrees with the ledger,
the ledger is right and the figure is a bug.

### The four rules

**1. A claim is never verified because a document says so. Only a command exiting 0
verifies it** (DoD Rule 0). A number's provenance is a command and its output, or
it is not a number.

**2. Re-derive, do not copy forward.** Before citing any figure, find the
measurement that produced it. *Repetition across documents is not corroboration* —
this repository has been wrong in the same place three times because the wrong
number appeared in three places. A concrete instance, caught by a peer reviewer:
three independent personas reported "the README's 186 AOT symbols is stale". Two
of them had compared **186 (symbols emitted) against 239 (`aot_eligible`
routines) — two rows of the same table.** Re-derived properly the value was a
different number again, and no derivation method had ever been written down.
`scripts/count-aot-symbols.sh` now re-derives all four figures and cross-checks
them against the manifest, so the next reader does not have to.

**3. Every instrument needs a falsifier, and the falsifier needs a negative
control.** A check that has never been seen to fail is not a check. Concretely,
in this repository:

- `scripts/check-entrypoints.sh --self-test` truncates `README.md` to 0 bytes and
  requires the guard to exit 1, then restores it byte-for-byte. It exists because
  commit `5cbf5fd` shipped this file as **0 bytes** and `check-claims`,
  `check-causes`, `check-cheat-gate` and `make test` **all passed on the empty
  file** — an empty file satisfies "no violations" perfectly.
- One of that guard's own assertions was **wrong on its first run** (an
  "unbalanced `**`" check fired on **1963 legitimate occurrences** in
  `docs/RE_CITY_FREEZE.md`, because bold spans line breaks and `**` appears inside
  inline code). **It was deleted, not tuned.** A check that cries wolf on the
  project's own corpus is worse than the hole it closes.
- The clock gate's date detector was found to be a **brightness meter**: a raw-RGB
  hash with no invariance to `$2100` INIDISP, which the guest ramps over 16
  consecutive frames. A fade alone produced 16 "distinct date images". **The gate's
  own positive control was a brightness fade.** The detector now asks a relation —
  *F shows the same date as R iff F is a pixel-consistent non-decreasing
  recolouring of R* — with no palette, threshold or brightness value involved, and
  ships with both directions falsified inside `make clock-self-test`.

**4. Instrument the instrument.** Run the thing you are about to believe against a
case where you already know the answer. Twelve measurements in this project have
falsified their own path, and **every one was the instrument working correctly** —
which is the only reason these answers are worth anything. The recurring ones:

- **A clean result is the one to distrust.** Two examples: a census of `$0012`
  writers was nearly reported as a missing 16-bit store until the log was re-read
  and showed `$0013` had been written in the same frame; and a positive control
  once composed **0 changed pixels** because a row-major pixel list was sliced as
  if it were columns — it printed a confident `0`, and only the pixel count gave
  it away.
- **A tier that cannot see a tier.** `CYC_WATCH` only sees interpreted opcodes and
  is **blind to AOT**; `AOTBLK` takes a frame *window*, not a PC range, and is
  silent without `SNESRECOMP_TRACE=1`. A PC absent from an interpreted histogram
  may be executing natively. Conversely a **fetched opcode byte proves an
  instruction executed there; it does not prove the PC was an instruction
  boundary** (R-038).
- **Leading zeros are octal.** `COUNT_PC` is `strtoul(..., 0)`, so `038026` is
  parsed as `$0003` and silently measures the wrong thing — the `0x` prefix is
  mandatory. `WRAM_DUMP_AT` is `strtol(..., 10)`, so the same prefix turns a
  27-frame list into frame 0. **Five knobs, four conventions.**
- **Instrumentation moves the histogram.** Step counts are wall-clock dependent;
  distinct-PC counts are not. Compare the latter.
- **A killed run is void regardless of its counts.** `exit: SDL_QUIT` means the
  harness was signalled, not that the guest finished.
- **Instrument a static hit before believing it.** 236 raw `$0012` candidates
  narrowed to **2 real**: one was a sliding-window false positive, and seven were
  sites that `TCD` on entry and `PLD` on exit — they were reading **their own
  frame**, not the flag.
- **Separate the observation from the attribution, always.** This is the failure
  that keeps recurring, and it has a signature: *the measurement reproduces to the
  unit and the explanation is still wrong.* R-040 was one; the `$03:8222` case was
  another. A write-watch recorded `$0E15` writes attributed to `STZ $0E15` at
  `$03:8222`, and the ROM byte at that offset is **`9C`** — `STZ $sr,S`, *stack
  relative* — not `64`. **The observation stood; the attribution was refuted.** When
  you write "X executes N times", the *count* is a measurement and the *"because X
  is Y"* is a separate claim that needs its own evidence. State them as two findings
  or state neither.
- **Check the byte. It costs one command.** `python3 -c "d=open('SimCity (USA).sfc','rb').read();
  print(d[0x18222:0x18226].hex())"` would have caught the wrong opcode before a
  measurement document, a README line and a reviewer all repeated it. A hand-decode
  is not verification, and **severity is not importance** — the finding that exposed
  this was filed MINOR and outranked the round's BLOCKER.
- **A count of writes is not a count of frames.** 56 logged writes was 28 distinct
  frames, two per frame. Say which you mean.
- **A maintained index must track the body it claims authority over.** It did not:
  `docs/RE_CITY_FREEZE.md` row 43 read `f3301 / CURRENT` while the banner in the
  same file read `f3271` and recorded `f3301` as retracted. **Three reviewer personas
  found it; no measurement agent did, and neither did I** — and a measurement session
  of mine was consuming it at the time. Anything you maintain as an index needs a
  gate that compares it against its source, or it will drift silently.
- **A measurement that exists only in a transcript is not a measurement.** Three
  measurement agents in one round produced good data and **zero files**, because the
  invoking process timed out before they wrote anything. **Write the finding down
  first, analyse afterwards.** The data was real and the run still counted for
  nothing.

### Where the measurements run

Heavy, instrumented and trace work runs on the **Steam Deck** over
`ssh deck@steamdeck`. **A host result is labelled `HOST-ONLY` and closes nothing.**
`make perf` is wall-clock and is therefore **solo-only** — a run abandoned because
the load average read `0.78`, caused by the agent's own `rsync`, is the reason the
rule exists. Up to three concurrent Deck runs are permitted, each carrying its own
embedded positive control. See `docs/DECK_RUNBOOK.md`.

### Retraction hygiene

A retracted claim stays **visible where it was made**, with what it claimed, what
refuted it, and what replaced it — not deleted, and not merely mentioned in a
changelog. `scripts/retracted-claims.tsv` is the ledger;
`docs/CLAIMS_REGISTER.md` is the readable index. **When a retraction retracts a
previous retraction, both rows are kept** (R-041 retracts R-039, and both are
readable), because the shape of the mistake is itself the lesson.

### When you are about to assert a cause

Ask whether you have **measured the cause** or merely **measured something that
accompanies it**. If you have not, write "not established" and stop. That is not a
weak result — in this repository "not established" has been the correct answer more
often than any confident story has, and the two most expensive mistakes in the
project's history were both confident explanations of this exact code path.

## Development

```bash
# Regenerate from ROM -- MANDATORY before the first build of a fresh clone;
# src/gen/ is never committed. See "Building" above.
bash tools/regen.sh "SimCity (USA).sfc"

# Re-derive the "how much is native" numbers
scripts/count-aot-symbols.sh

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
