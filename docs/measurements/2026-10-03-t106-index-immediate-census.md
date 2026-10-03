# Measurement — T106: the `$A0/$A2/$C0/$E0` census, and the width rule is not a defect

**The question T106 was written to ask.** Correcting the index-immediate width
rule moves the `$03C87x` loop's terminal value from `$8DF4` (36 340 iterations)
to `$00F4` (245) — or leaves it unbounded. So: census the opcodes under `xf=0`
first, bound the blast radius, then fix.

**The answer: do not fix it. The width rule is correct, the loop was always
correctly terminated, and T105's central claim is retracted.** Deck-native
throughout (`ssh deck@steamdeck`), `build-instr`, headless,
`scripts/d_city.script`, Release. **Host-only: none.** ROM md5
`23715fc7ef700b3999384d5be20f4db5`, 524 288 bytes, verified on the Deck.

**Deliverable:** C-074 (the census, the blast radius). Retractions **R-040**,
**R-041**, **R-042**.

---

## 1. Falsifiers and controls, before the results

| # | stated before running | outcome |
|---|---|---|
| **F1** | a run not ending `exit: RUN_FRAMES reached` is void | **did not fire.** 11 runs, all `EXIT=0`, all `RUN_FRAMES reached` |
| **F2** | every run carries its own `COUNT_PC=0x009311` positive control | **satisfied on all 11.** The 14 000-frame runs read **27 019 166** = **1929.9/frame**, byte-identical to the known-good 14 000-frame value, in all five census runs and in the 14 000-frame flag run. The 3 300-frame baseline reads **6 626 029** = **2007.9/frame**, byte-identical to T104's and T105's known-good 3 300-frame value |
| **F3** | `COUNT_PC` needs the `0x` prefix; `WRAM_DUMP_AT` is decimal | **not exercised** — no `WRAM_DUMP_AT` in this ticket. `0x` used everywhere, CONF-15 |
| **F4** | the census enumerates rather than compares, so a parse bug cannot corrupt it | **held.** `INTERP_DUMP_BANK` prints every distinct PC in a bank; the opcode filter is applied afterwards, offline, against the ROM file. C-041's method |
| **F5** | `CYC_WATCH` is blind to AOT, so the flag reads cover the interpreter tier only | **held and stated.** The width rule is a decode question, which is entirely inside the interpreter tier, so the blind spot does not apply to *this* question — unlike C-039c's, which it does |

---

## 2. The census — the blast radius

Five runs, one per executing bank, **14 000 frames each**, `EXIT=0` and
`RUN_FRAMES reached` on all five, each with its own positive control.

| | |
|---|---|
| distinct executed interpreted PCs, all banks | **6 083** |
| interpreted steps at those PCs | **109 533 634** |
| **sites whose ROM opcode is `$A0`/`$A2`/`$C0`/`$E0`** | **245** |
| steps at those 245 sites | **344 060** = **0.3141%** of all interpreted steps |

Per-bank coverage, identical in all five runs and reproducing the known figures
to the unit: `$00` 1822 PCs · `$01` 1842 · `$02` 542 · **`$03` 921** · `$05` 956.

### The partition, and how it was derived

For each of the 245 sites, the executed stream either landed on **PC+2** or on
**PC+3**, and the other address was never fetched. That is a *derived* partition,
so it was checked against a **direct flag read** on both sides. `P` is
`interp816_getFlags()` (`interp816.c:403-413`): `N V m x D I Z C`, so **the x flag
is bit 4 (`$10`)**.

| | sites | steps | share |
|---|---|---|---|
| stream advanced **3** bytes — the `x=0` reading | **195** | **287 062** | 83.4% |
| stream advanced **2** bytes — the `x=1` reading | **49** | **55 566** | 16.1% |
| **ambiguous** — both or neither landing fetched | **1** | **1 432** | 0.4% |

**Both branches are exercised.** A rule whose width never varied would not need
a flag, so the census alone already says the flag is doing something.

The one ambiguous site is **reported as ambiguous, not resolved by assumption**:
`$03DCCF` (`A2 00 5A B9`), 1 432 steps, sitting in what reads as a byte table
(`$03DCCD A0 00 | A2 00 | 5A | B9 5C`) — both `$03DCD1` and `$03DCD2` are fetched
somewhere in the run.

### The direct flag read, both sides — the positive control for the partition

| site | `op` | `P` | m | **x** | stream advanced |
|---|---|---|---|---|---|
| `$03C874` | `$A2` `LDX #$0000` | `$06` | 0 | **0** | **3 bytes** → `$03C877` |
| `$008D28` | `$A2` `LDX #$0004` | `$64` | 1 | **0** | **3 bytes** → `$008D2B` |
| `$008D49` | `$E0` `CPX #$0010` | `$67` | 1 | **0** | **3 bytes** → `$008D4C` |
| `$01C826` | `$A0` `LDY #$0F` | `$30` | 1 | **1** | **2 bytes** → `$01C828` |
| `$01C828` | `$A2` `LDX #$1E` | `$30` | 1 | **1** | **2 bytes** → `$01C82A` |

**`$008D28` is logged `m=1, x=0`** — 13 676 steps — and takes the 3-byte form.
The two flags are independent, which is exactly why the two immediate groups
need two different flags, and this run shows both states at once. The logged `x`
bit predicts which form the stream took, from a flag read rather than from the
landing address. **That is the control for the inference the partition rests on.**

---

## 3. The width rule is published 65816 behaviour, and the decoder is right

`interp816_adrImm` (`interp816.c:513`) implements:

```c
if((xFlag && cpu->xf) || (!xFlag && cpu->mf)) {   // 1 operand byte
} else {                                          // 2 operand bytes
```

with the four `xFlag=true` call sites being exactly `LDY`/`LDX`/`CPY`/`CPX #imm`
(`interp816.c:2029, 2041, 2228, 2419`) and the `xFlag=false` sites the
accumulator/ALU group.

**Published 65816 behaviour, three independent sources, and they agree:**

| instruction | opcode | bytes | extra |
|---|---|---|---|
| `LDY #const` | `$A0` | **2 / 3** | **+1 if x=0** |
| `LDX #const` | `$A2` | **2 / 3** | **+1 if x=0** |
| `CPY #const` | `$C0` | **2 / 3** | **+1 if x=0** |
| `CPX #const` | `$E0` | **2 / 3** | **+1 if x=0** |
| `LDA #const` | `$A9` | **2 / 3** | +1 if **m**=0 |
| `ADC/AND/BIT/CMP/EOR/ORA/SBC #const` | `$69/$29/$89/$C9/$49/$09/$E9` | **2 / 3** | +1 if **m**=0 |

Sources: `undisbeliever.net/snesdev/65816-opcodes.html`, which cites the WDC
W65C816S datasheet, *Programming the 65816* (Eyes & Lichty), *All About Your
64*, and Near's higan `wdc65816` disassembler; and Chris Wright's 65816
Programming Primer (`snesdev/docs/65816.txt`), which writes it as
`CPX #const  E0  2*  2 | 1` with the footnote *"Add 1 byte if … (16-bit index
registers)"* and whose prose says *"when in 16 bit index register mode (x=0) that
data/memory will be 16 bits wide"*.

**`LDX/LDY/CPX/CPY #imm` are 3 bytes at `x=0`.** The decoder does that. There is
no defect. **This is the external conformance reference CONF-21 says this
repository does not have** — it does not have one *in the tree*, and for this
question one was not needed from outside either, because the rule is documented.

---

## 4. The ROM's own instruction stream says the same thing — four witnesses, none of which uses a length table

The runbook forbids using our own 65816 length table as evidence. None of the
following does. Each takes the **ROM bytes** as given and asks which reading is
consistent with a **count**, which is the one thing a decoder cannot fabricate.

**(a) The `$03C87x` loop itself.** ROM, `$03:C871`–`$03:C884`:

```
$C871: A9 00 00     LDA #$0000
$C874: A2 00 00     LDX #$0000
$C877: 9F 00 6B 7F  STA $7F6B00,X
$C87B: E8           INX
$C87C: E0 F4 8D     CPX #$8DF4        <- the 3-byte reading
$C87F: D0 F6        BNE $C877
$C881: 20 77 B4     JSR $B477
$C884: 22 3A 82 00  JSL $00823A
```

- **`$03C877` executes 36 344 times; `$03C876` executes 0 times in 14 000
  frames.** `$C876` is the `$00` that a 2-byte reading of `$C874 A2 00 00` would
  make a **`BRK`** — a breakpoint between `LDX #$0000` and the `STA $7F6B00,X`
  that consumes that very X. And `$C877` is the store that consumes X; the
  16-bit `#$0000` is what makes it well-defined.
- **Under a 2-byte reading of `$C87C`, there is no branch instruction anywhere in
  `$C877`–`$C884`.** `$C87E 8D D0 F6` would be `STA $F6D0` (no branch), `$C881
  20 77 B4` is `JSR`, `$C884 22 3A 82 00` is `JSL`. A region with no branch
  cannot re-enter its head, and `$C877` is re-entered **36 339** times.

**(b) `$008D49`.** ROM `E0 10 00 D0 …`: `CPX #$0010` then `$D0` = `BNE`. Measured:
**393 rows at `$008D49`** and **393 rows at `$008D4C`** with the byte fetched
there being `$D0`. A 2-byte reading puts a `BRK` at `$008D4B`, between them.

**(c) Ninety sites, not two.** **90 of the 195** `x=0` sites have **`$00`** at
PC+2 — 211 859 steps, including the busiest site in the whole census
(`$008D49`, 82 056 steps) and four loop counters in banks `$00`/`$01`/`$02` at
13 676 / 13 750 / 12 001 / 12 000 steps. **A `BRK` inside a 13 000-iteration
loop is not a coherent encoding of anything.**

**(d) The assembler knew.** `$01C824 E2 30` = `SEP #$30`, immediately before
`$01C826 A0 0F` (`LDY`) and `$01C828 A2 1E` (`LDX`), and the logged `P` goes
`$32` → `$30` across it. That is a flag-setup instruction emitted *because* the
width of the two instructions after it depends on the flag it sets. The
programmer's own intent is the width rule.

---

## 5. The experiment: building the proposed "fix" and measuring it

The predicted post-fix shape — **245 iterations, correct Z/C at `X=$00F4`** — was
pre-registered and is falsifiable in one Deck run. **It was run. It never
appeared, and the reason is the finding.**

### v1 — mis-specified, and my own mechanism check refuted my explanation

The first patch forced the 1-byte form for **every** immediate
(`if (1) { … }`), which also hit the accumulator group. Result: catastrophic —
only bank `$00` executed, 16 778 distinct PCs, and the positive control read
**13 631 085** against a same-session baseline of **6 626 029**.

**Then the mechanism check refuted my explanation of it.** I predicted control
would land on operand bytes of the index-immediate sites. Measured: **0 of the
16 749 newly-executed PCs** were the PC+2 address of any three-byte site. My
causal story was wrong, and the reason was that **v1 was not the proposed fix** —
it conflated the index group with the accumulator group. **Recorded, not hidden;
this is the project's own failure mode and it is recorded as it happened.**

### v2 — the actual proposed fix, correctly specified

`if (xFlag || cpu->mf) { … }` — the index-immediate group forced to 8-bit, the
accumulator group left on its existing `m` behaviour. Built on the Deck as
`build-x`, 3 300 frames, `EXIT=0`, `RUN_FRAMES reached`, positive control
present.

| | baseline (unmodified decoder) | **v2 (the proposed fix)** |
|---|---|---|
| banks that execute at all | `$00 $01 $02 $03 $05` | **`$00` only** |
| distinct PCs, bank `$00` | **1 822** | **16 778** |
| bank `$03` | 921 PCs / 515 043 steps | **0 PCs / 0 steps** |
| `$03C87x` loop | **36 344** iterations | **0 — not 245** |
| `[cyc]` rows in `$03C874`–`$03C888` | 145 364 | **0** |
| `COUNT_PC=0x009311`, 3 300 f | **6 626 029** (2007.9/f) | **13 631 085** (4130.6/f) |

**The prediction was not refuted; it was preempted.** The change is **not local**:
it destroys the trajectory long before it reaches the loop, so the loop executes
**zero** times and the predicted 245-iteration shape is never on screen. The
inference the prediction rested on — *the width rule is the bug* — is what the
experiment refutes.

**An honest loose end.** v1 and v2 produced **identical** numbers on every
figure, though they differ on the accumulator group. **I did not establish why**
and do not assert a reason. The available candidate — that the accumulator group
was never reached with `m=0` on the trajectory v1 diverged onto — is
**UNVERIFIED**.

---

## 6. T105's measurement reproduces exactly, and the loop was always correct

The same unmodified build, `CYC_WATCH=03C874-03C888`, 3 300 frames, control
**6 626 029** = 2007.9/frame.

| at `$03C87C` | measured now | T105 reported |
|---|---|---|
| rows | **36 340** | 36 340 |
| distinct `X` values | **36 340**, `sort -c` non-decreasing | 36 340 distinct, strictly monotonic |
| `X` range | `$0001` → `$8DF4` | `$0000` → `$8DF3` at `$03C877` |
| `P=$84` | **32 768** | 32 768 |
| `P=$04` | **3 571** | 3 571 |
| **`P=$07`** | **1** | 1 |

**All three flag counts reproduce to the unit.** The single `P=$07` row, verbatim:

```
[cyc] f=3270 pc=$03C87C op=$E0 P=$07 A=0000 X=8DF4 … NPC=03C87F
[cyc] f=3270 pc=$03C87F op=$D0 P=$07 A=0000 X=8DF4 … NPC=03C881
[cyc] f=3270 pc=$03C881 op=$20 P=$07 A=0000 X=8DF4 … NPC=03B477
```

`P=$07` = `N0 V0 m0 x0 D0 I1 Z1 C1` (`interp816.c:403-413`) — **Z and C set**,
which is exactly what `CPX` must produce for `X == $8DF4`. T105's reading was
right. And the exit is `$03C881: JSR $B477` → `NPC=$03B477`: an ordinary call, not
an anomaly.

### Therefore

**`$03C87C` is `CPX #$8DF4`. The game's own 16-bit bound. The loop zeroes
`$7F6B00` + `X` for `X = $0000…$8DF3` — `g_ram` `$6B00`–`$F8F3`, 36 340 bytes —
and stops exactly there.** 36 340 stores for 36 340 iterations, no overrun, no
skipped instruction, no destroyed memory beyond what the game asked to clear.

**"What the ~36 KB of zeros destroyed" is answered: they are the game's own
bounded clear at city creation.** Whether that clear is itself what the game
wants is **not measured and not claimed**.

---

## 7. What this retracts

| | |
|---|---|
| **R-040** | **C-073 is RETRACTED as stated.** "*`CPX #imm` consumes a 2-byte immediate operand while `xf=0` and advances the PC by 3 … This is a defect in OUR emulation, not in the game's code*" — **there is no defect.** The measurement was correct; the inference from it was not. The `NPC` and `P` figures stand; the attribution does not |
| **R-041** | **R-039 is RETRACTED.** It retracted the "defect in the game's own code" framing on the ground that the defect was ours. **The answer is neither: there is no defect.** R-039 correctly retired an ambiguous framing and incorrectly supplied a replacement |
| **R-042** | **C-072's "zero-fill that runs away" is REFUTED.** It did not sail past its bound; the bound is `$8DF4` and it is honoured exactly. T105's own measurement of 36 340 strictly monotonic iterations was already the refutation and was read the wrong way |
| — | **CONF-21's specific claim is REFUTED.** It says `gen_ops.py:30` *"is the defect restated as a test-corpus invariant"* and `:67` *"plants a 3-byte encoding for the `x=0` case — so the test ROM is wrong in the same direction as the decoder."* **The 3-byte x=0 encoding is correct**, and the comment *"width = X flag"* is correct. The generator and the decoder agree **because both are right** |
| — | **CONF-21's structural point survives, weakened.** A differential suite comparing the AOT generator against our own `interp816` still cannot see a defect common to both, and 1 599 000 checks still prove nothing about conformance. **But this ticket is not an instance of it**, and CONF-21 must not keep citing it as one |

**Ledger: 42 rows, 34 refuted** (`make retraction-count`) - **and this number is not a
hand-written figure; it is what the command prints, which is why it appears
here as a value and not as a count anyone typed.**

---

## 8. What is NOT established here

- **Not a cause for C-006.** C-006 stays **OPEN**. This ticket removed a false
  lead from the search; it did not find the cause.
- **Not that the game's clear of `$6B00`–`$F8F3` is correct**, only that it is
  the game's own bounded instruction and is not an overrun.
- **Not a claim about `$03C87E`/`$03C87F` from R-038.** See §9.
- **Not a general audit of the interpreter against 65816 documentation.** One
  width rule was checked, because one width rule was in question. **The census
  shows 245 sites and 0.314% of steps depend on it; it says nothing about any
  other opcode.** CONF-21's real content — *this repository has no conformance
  reference* — is **untouched and still true**.
- **The single ambiguous site** `$03DCCF` is left ambiguous.

---

## 9. A limit on CONF-19 / R-038 that this ticket exposed, stated as a limit and not as a retraction

R-038 retracted C-069's clause *"`$03C87F` is the second byte of `8D D0 F6`,
therefore not an instruction boundary"*, on the evidence that `CYC_WATCH` reports
the opcode **fetched** at `$03C87F` as `$D0`.

**That evidence cannot bear the weight, and the reason is C-073's own mechanism.**
The PC the CPU fetches at is **produced by our own decoder**. If the decoder
mis-lands on an instruction boundary, `CYC_WATCH` reports the opcode byte there
with total confidence — the byte really was fetched, at a PC that was reached.
**A fetched opcode byte proves an instruction was executed at that PC; it does
not prove the PC was an instruction boundary in the game's intent.**

What R-038 got right, and what stands: of our build's executed stream, `$03C87F`
**is** the loop's only branch and its real back-edge, taken 36 339 times. What
does not stand is the *reason* — the fetched `$D0` does not refute a ROM-decode
reading, and under the corrected understanding of §4 `$03C87E` is the second
operand byte of a `CPX #imm` and `$03C87F` is that instruction's successor,
which is `$D0 = BNE`. **The two readings happen to agree about `$D0`, and that
coincidence is not evidence.**

**No ledger row is added, because R-038's conclusion about our stream is not
shown to be wrong — only its stated reason.** Flagged as a limit on how
`CYC_WATCH`'s `op=` field may be used, and the next reader of CONF-19 needs it.

---

## 10. Related

- **R-039 / R-040 / R-041** — the retraction chain, all three recorded.
- **CONF-21 / R-042** — specific claim refuted, structural point survives.
- **CONF-19** — the `op=` field's limit, §9.
- **C-074** — the census and the blast radius. New.
- **C-072** — the "runs away" framing, refuted.
- **C-006** — **OPEN and untouched.**
- **T107** — the rendered-date lead, untouched by this ticket.
