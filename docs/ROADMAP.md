# Roadmap — SimCity SNES PC Port

> **Rewritten 2026-09-28.** The previous version listed Sprint 01 as current and
> a backlog of T007–T016 — tickets from the abandoned reimplementation, several
> of which were archived in T038 and one (T016, multiplayer) which is still
> marked a non-goal in the same file. `aes/kanban.md` is the live tracker; this
> file is the *shape* of the work, not the queue.

---

## CURRENT NEXT PATH — reconciled 2026-10-03, after T102

> **This section is the tracked mirror of `aes/kanban.md` + `aes/decisions/D013.md`.**
> `aes/` is **gitignored permanently** (T078, DoD D4.3), so neither the board nor
> the decision record is versioned and neither exists in a fresh clone. This is
> the only place the reconciliation survives `git clone`. **If the two disagree,
> this file is what a clone actually gets, and that is the whole reason it is
> here.**

> ### The recommendation — rewritten 2026-10-03 after T106
>
> **T105 is retracted, and so is the retraction. There is no defect.**
>
> **T106 measured the thing T105 inferred, and the answer is that the `$03C87x`
> loop was always correct.** `LDX/LDY/CPX/CPY #imm` are **3 bytes at `x=0` and
> 2 at `x=1`** — published 65816 behaviour, three independent sources, and
> exactly what `interp816_adrImm` (`interp816.c:513`) implements. So `$03C87C
> E0 F4 8D` is **`CPX #$8DF4`**: the game's own 16-bit bound. The loop zeroes
> `$7F6B00 + X` for `X = $0000…$8DF3` — `g_ram` `$6B00`–`$F8F3`, **36 340
> bytes** — and **stops exactly there**, on one exit, via an ordinary
> `JSR $B477`. **No overrun, and nothing was destroyed beyond what the game asked
> to clear.** **[MEASURED, Deck-native, T106]**
>
> | | measured, Deck-native |
> |---|---|
> | run | `EXIT=0`, `exit: RUN_FRAMES reached`, `COUNT_PC=0x009311` -> **27 019 166** / 14 000 f = **1929.9/f**, byte-identical to the known-good value in all five census runs |
> | `X = $00F4` | `P=$04` — Z clear. **Correct**: `CPX #$8DF4` at `X=$00F4` must not match |
> | `X = $8DF4` | `P=$07` = **`I1 Z1 C1`** — Z **and** C set. **Correct**: `X` equals the operand |
> | `NPC` from `$03C87C` | `$03C87F` × 36 340, one value. `E0 F4 8D` is 3 bytes at `x=0`; we advance 3 — **correctly** |
> | `NPC` from `$03C87F` | `$03C881` ×**1**, `$03C877` ×**36 339** — the 1-exit split, and `$03C881: JSR $B477` is an ordinary call |
> | X over the loop | **36 340** rows at `$03C87C`, **36 340 distinct**, `sort -c` non-decreasing, `$0001` -> `$8DF4` |
> | T105's three flag counts | `$04` ×3 571, `$84` ×32 768, `$07` ×1 — **reproduce to the unit** |
>
> **The census T106 was asked to run first, and its result — the blast radius.**
> Five runs, one per executing bank, 14 000 frames each, all `EXIT=0` /
> `RUN_FRAMES reached`, each with its own positive control. **245** distinct
> executed interpreted PCs carry a ROM opcode in `{$A0,$A2,$C0,$E0}`, taking
> **344 060 steps = 0.3141%** of 109 533 634. **195** of them (287 062 steps) are
> read with a 2-byte operand and **49** (55 566 steps) with a 1-byte operand;
> **1** is ambiguous and is **left ambiguous**. **Both branches are exercised**,
> and the logged `x` bit predicts which — checked against direct flag reads on
> both sides (`$03C874 P=$06` and `$008D28 P=$64` are x=0 and take 3 bytes;
> `$01C826 P=$30` and `$01C828 P=$30` are x=1 and take 2). **`$008D28` is logged
> `m=1, x=0`**: the two flags are independent, which is why the two immediate
> groups need two different flags. **[MEASURED, Deck-native, C-074.]**
>
> **The pre-registered post-fix shape was run and did not appear.** Forcing the
> index-immediate group to 8 bytes, built on the Deck as `build-x`, 3 300
> frames: **only bank `$00` executes** (16 778 distinct PCs against 1 822),
> banks `$01`/`$02`/`$03`/`$05` execute **nothing**, the `$03C87x` loop executes
> **0 times — not 245** — and the positive control moves from **6 626 029** to
> **13 631 085**. **The prediction was preempted, not refuted: the change is not
> local.** **The fix is not made and must not be.** My first attempt at the
> experiment patched the accumulator group too, so it was not the proposed fix at
> all; its own mechanism check refuted my explanation of it, and the correction
> is in `README.md` and in the measurement file. Recorded, not hidden.
>
> **CONF-21 is partly refuted, and its specific claim was false.** It held that
> `gen_ops.py:30`'s *"width = X flag"* and `:67`'s 3-byte `x=0` encoding were
> *the defect restated as a test-corpus invariant*. **They are correct**, and the
> generator and the decoder agree **because both are right** — CONF-21 was citing
> T105 as an instance of itself and T105 was not one. **What survives, and is the
> real CONF-21:** a differential suite comparing our AOT against our own
> `interp816` cannot see a defect common to both, so 1 599 000 checks prove
> nothing about 65816 conformance. **This repository still has no conformance
> reference in the tree.**
>
> **Three retractions: R-040** (C-073's attribution), **R-041** (R-039 itself —
> the defect is in neither), **R-042** (CONF-21's specific claim).
>
> **What this does NOT settle: C-006.** It removed a false lead from the search,
> and it answered *"what the ~36 KB of zeros destroyed"* — they are the game's
> own bounded clear at city creation — but **no cause for the city not
> simulating is asserted anywhere.** `make clock` stays red. **The next
> measurement is not a width rule: it is where the rendered date comes from**
> (the rendered-date lead below, from T107).


### The reconciled queue

| ticket | state |
|---|---|
| **T108** — when exactly does the picture stop | **CLOSED — the freeze is a 17-present event, and it agrees with `make clock`.** Deck-native, `build-instr`, `scripts/d_city.script`, **5 000 frames**, `EXIT=0` / `RUN_FRAMES reached`, own `COUNT_PC=0x009311` control **9 855 088** = 1971.0/f. `SNESRECOMP_PRESENT_LOG` (per present, no pictures): **5 000 presents = 5 000 frames**, **206 picture changes** (165 distinct `crc32`), **last change f3381**, then **1 619 bit-identical presents**. Shape: 106-frame gap → **17 consecutive changes f3365→f3381** → silence. `make clock`'s date crop independently reports **f3378**, inside the burst. C-055's **+96** updater reproduced on the picture side (period exactly 96, five times; writer stops f1685, picture f1691). The dense run f2637–f3259 **ends on f3259**, the one city-state write. **Also a correction**: T107's static-picture reason for "the date is not read live from `$0B53`" is **void** — `$0B53` itself never changes (C-068); the poke reason stands alone. **Two OPEN discrepancies recorded, not smoothed** (f1459 vs f3381 are **different routes**; 1 877 presents does not reconcile with 5 000). **Not a cause** | measurement `2026-10-03-t108-present-crc-timeline.md` |
| **T100** — does `$03:8026` run in ours past f3857 | **CLOSED `1b099ce`, and its measurement is now RETRACTED (R-037)** — `COUNT_PC=038026` was watching PC `$000003` (base-0 parse, CONF-15). The *conclusion* is re-measured over 14 000 frames by T102 and holds; the *measurement* is void. **Superseded in scope by T102** |
| **T102** — does `$03:8026` run across the reference's first year rollover | **CLOSED — NO, and the previous answer was void.** Deck-native, 4 runs × 14 000 frames, `EXIT=0` / `exit: RUN_FRAMES reached` on all four. `$03:8026` = **0 executions**, exhaustive over both tiers (`$038026` is inside an `lle_only` node; **0** `aot_eligible` nodes cover it). Bank `$03` = **0 PCs / 0 steps in f3272–f13080** and **18 AOT entries** in the window, all at f3259/f3270. City state block: **0 writes after f3259 in 14 000 frames**. **Forced retraction R-037** and **CONF-15**. **C-046c closed.** C-041, C-008, C-052, C-057, C-058 stand |
| **T105** — read the processor status at `$03C87F` | **CLOSED, AND RETRACTED (R-040, then R-041) — its measurements were right and its conclusion was wrong.** Deck-native, one run, `EXIT=0`/`RUN_FRAMES reached`, `COUNT_PC=0x009311` → **6 626 029**. **What it measured is correct and reproduces to the unit** (`NPC=$03C87F` ×36 340; `P=$04` ×3 571 / `$84` ×32 768 / `$07` ×1 at `X=$8DF4`; 36 340 distinct monotonic `X`). **What it inferred — "the defect is OURS" — is refuted:** the 3-byte operand at `x=0` is published 65816 behaviour and `interp816` implements it correctly. **R-039 was itself retracted by R-041**: the answer is neither ours nor the game's, because **there is no defect**. **CONF-21's specific claim about `gen_ops.py` is refuted (R-042)** — its structural point stands | measurement `2026-10-03-t105-flags-at-03c87f.md`; superseded by `2026-10-03-t106-index-immediate-census.md` |
| **T107** — does a third-party cheat table check out | **CLOSED — the plaintext format is CONFIRMED from the outside, and two of its six rows are now better known than before.** Deck-native, Release `build/`, 3 runs, `EXIT=0` / `RUN_FRAMES reached`, each with its own `COUNT_PC=0x009311` control (10 239 582 / 5 200 f = 1 969.2/f; 9 667 063 / 4 900 f twice). `7E AA BB CC` = *"write `$CC` to `$AABB`"*, **byte order measured**: 16 WRAM dumps over 1 000 frames, **exactly five bytes changed and they are exactly the five poked**; `$0B53`/`$0B54` `$076C` → **`$0FA0`** with the f4140 intermediate **`$07A0`** (big-endian would have given `$A00F` = 40 975 with `$760F`). Verdicts: **"year 4000" VERIFIED** · **"population 20000" PARTIAL** (mechanics yes, label no — `$0B9D` is also `$4E20`) · **"49000" UNRESOLVED** (`$0BF9` real and writable; 49 000 = `$BF68` ≠ `$00EB`) · **`DD/DE 67DF` REFUTED as characterised** (bank `$67` → offset `0x338000` in a 524 288-byte ROM; nothing to patch) · **`7E03-F5xx` UNVERIFIED** and "bank 3" **REFUTED** (it is WRAM `$03F5`). **CONF-22** new. **DoD Rule 0b** + `scripts/check-cheat-gate.sh` new — the guard was wrong three times and its self-test caught all three. **No ledger row added**: nothing here retracted a claim | measurement `2026-10-03-t107-cheat-verification.md`; `docs/CHEAT_CODES.md` |
| **T106** — census `$A0/$A2/$C0/$E0` under `xf=0`, then fix the width rule | **CLOSED — the census is done and **the fix must not be made**: the rule is correct and the loop was always correctly terminated.** Deck-native, **11 runs**, all `EXIT=0` / `RUN_FRAMES reached`, every one carrying its own `COUNT_PC=0x009311` control (**27 019 166** / 14 000 f = **1929.9/f** in all five census runs, byte-identical to known-good; **6 626 029** / 3 300 f = 2007.9/f baseline). **Blast radius: 245 executed sites, 344 060 steps = 0.3141% of 109 533 634; 195 read with a 2-byte operand (`x=0`), 49 with a 1-byte one (`x=1`), 1 ambiguous and left so.** Both branches exercised and the logged `x` bit predicts which, checked against **direct flag reads on both sides**. **The 3-byte reading is published 65816 behaviour** (three independent sources), and `interp816` implements it correctly. **ROM-internal corroboration without any length table: 4 witnesses**, including `$03C876` executed **0** times against `$03C877`'s **36 344**, and **90 of the 195** `x=0` sites have a **`BRK`** at PC+2. **T105's three flag counts reproduce to the unit.** **The pre-registered 245-iteration post-fix shape did NOT appear — the loop ran 0 times**, because the change is not local: banks 01/02/03/05 stop executing entirely and the control moves 6 626 029 → 13 631 085. **R-040, R-041, R-042.** **C-074** new; **C-072** and **C-073** retracted | measurement `2026-10-03-t106-index-immediate-census.md` |
| **T104** — frame-resolve the `$03C87x` loop | **CLOSED — the loop runs f3259–f3270, its back-edge is `$03C87F` (`BNE $C877`), it is a zero-fill that runs away, and **C-039b is NOT falsified**.** Deck-native, 9 runs, all `EXIT=0` / `RUN_FRAMES reached`, all carrying the `COUNT_PC=0x009311` positive control (6 626 029 in five of them, byte-identical). **F3 did not fire.** `CYC_WATCH` supplied the **fetched opcode bytes** and refuted C-069's `$03C87F` clause → **R-038**; C-069's four cost figures reproduce exactly. C-070, C-071, C-072 new | measurement `2026-10-03-t104-c87x-scan-loop.md` |
| **T101** — does the reference simulate at all | **CLOSED `9069182` — YES, and decisively.** Deck-native, clean core, cold SRAM, real save: city f3000, **28 month rolls**, year turns f13080/f24600, **1902 MAY at f30 000**, **29 distinct date images**. Reproduced on a second route. The two "disagreeing" runs were one execution read through a broken column of **our own** driver (`c4923de`) — `$0B55` printed `$0B53`; the month had advanced six times inside the disputed window. The write-watch is **inert** (C-063). **The comparative premise is available and it holds.** R-035, R-036 retracted |
| **T086** — why does bank `$03` go silent | **RESCOPED, not closed.** *Where* is answered (f3271) and the mechanism is measured: `$03:D2AA` sets `$0012 = 1` one instruction before bank `$03`'s final `RTL`, and `$00:804D` is never executed again. The *why* is OPEN and is no longer the delivery question |
| T087 — is `$03:8026` among the executing bank-`$03` PCs | **CLOSED** `4ba14c7`, caveated `940de2a` (C-041b); superseded in scope by T100 |
| T092 — "the Deck cannot build this project" | **CLOSED** — false; both tiers link, `deck-trace-build.sh` refuses a mute build |
| T093 — what advances `$0B51` in the peer | **CLOSED** `940de2a` — it is `$03:8026`, 27 executions, first f3857. **Its "and the answer did not help" rider is RETRACTED (R-036):** the 27 ticks are `6 × 4 + 3` — six whole months (C-060). The answer helps; the premise it was read against was broken |
| T094 — `check-causes` fires on its own header | **CLOSED** |
| T089 — README asserts refuted claims | **CLOSED** `02bc55f` |
| T095, T096, T097, T098, T099 | OPEN, not on the delivery path |
| T069 — peer repo has no licence | **BLOCKED ON OWNER** |

**Numbers this reconciliation corrected:** bank-`$03` boundary f3301 → **f3271** ·
retraction count 22 → **36 of 44 rows** (R-035, R-036 by T101 `9069182`; R-037 by T102; **R-038 by T104; R-039 by T105; R-040/R-041 by T106 — which retract R-039 in turn; R-042 by T106; R-043 by T109; R-044 by T110 — the reference's headless renders are tile data, not a game screen** ) · bank-`$03` range `$03C63D`–`$03E57E` →
**interpreted** `$03C63D`–`$03E57E` and **AOT** `$03B477`–`$03C463` (ledger
R-033) · AOT total 1 430 539 → **1 430 540**, bank `$03`'s 18 = **15 in f3270 +
3 in f3259** · **R-039** by T105 · reference month rolls **28 in 30 000 frames**
(Deck-native) · `$0B51` decoded as **4 × (months elapsed) + quarter**, 1 344 samples,
zero violations.

### What is still OPEN and is not going to be closed by any of the above

- **Why the city does not simulate** (C-006). No cause is asserted anywhere.
  **Narrowed by T102 and re-posed as a *start* question:** bank `$03` executes
  nothing whatever in f3272–f13080, and the city-state block is not written once
  in the 10 741 frames after f3259. What *starts* the simulation is now the
  question; what advances it is downstream of a bank that never runs again.
- ~~**What the `$03C87x` scan loop scans, what branches back, and what
  terminates it at f3271** (C-069)~~ → **ANSWERED by T104 (C-070/071/072).** It
  scans nothing: it is a zero-fill of `$7F6B00+X` with `A=0`. Its back-edge is
  `$03C87F` = `BNE $C877`. It runs f3259–f3270 and **stops one frame before the
  bank does.**
- ~~**Why the zero-fill does not stop at `X == $00F4`, and why `$03C87E` never
  executes**~~ → **ANSWERED by T105 (C-073).** Both are one defect on our side:
  `CPX #imm` reads a 2-byte operand while `xf=0` and advances the PC by 3, so the
  bound it enforces is `$8DF4` and `$03C87E` is stepped over. **The third
  appearance of the same rule** — *a byte-boundary question cannot be answered
  from the ROM decode* — except here it is answered from **`NPC`**, the PC the
  instruction actually advanced to, which is a third thing again and settles it
  without any decode table at all.
- ~~**What the ~36 KB of zeros destroyed, and how often `$A0/$A2/$C0/$E0` execute under `xf=0`**~~ → **ANSWERED by T106.** **The `$A0/$A2/$C0/$E0` census: 245 sites, 0.3141% of interpreted steps, 195 of them read with a 2-byte operand and 49 with a 1-byte one — the rule is correct and the decode is not to be changed.** And the ~36 KB was never an overrun: `$03C87C` is **`CPX #$8DF4`**, the game's own 16-bit bound, so the loop zeroes `$6B00`–`$F8F3` and **stops exactly there**. **The zeros are the game's own bounded clear at city creation.** Whether that clear is what the game wants is **not measured and not claimed**
- **⚠ Where the *rendered* date comes from — a lead T107 raised and nobody
  asked for, and it is now the largest unexamined thing in the project.**
  `md5` over **1 877 presents** gives **18 distinct picture states, the last
  beginning at f1459**; every cheat poke landed **after f4025**; and the
  framebuffer was **pixel-identical for 3 400+ frames** — *including a write of
  `$0FA0` into the field measured to be the year*. So the rendered date is **not
  read live from `$0B53`**, while `make clock` **reads the date off a screen
  crop** (`clock-gate.sh`, x 55–125 / y 2–21 of the 336×224 framebuffer).
  **The gate and the memory are reading different things and that relationship is
  unexamined.** Two readings with very different consequences remain open and are
  **not** separated: the date's own source is frozen (the crop measures something
  real, **C-006 unchanged**), or it is frozen for another reason — a cached
  tilemap, a dirty-flag never set, a DMA never triggered (then **`make clock` is
  measuring a display path rather than the simulation**). **Recorded as a lead for
  C-006 with no claim attached.**
  > **T108 sharpened it into a 17-present question and did not answer it.** The
  > last picture change is **f3381** and the last change **inside the gate's own
  > crop** is **f3378** — which T108 read as *two instruments, one boundary, 3
  > frames apart*. **⚠️ THAT IS RETRACTED (R-043, T109): they are two different
  > events in two different rectangles.** The crop is x 55-125 / y 2-21; the
  > last change anywhere on screen is at **x 165-178, y 124-136** — outside the crop
  > — and it is **the tile cursor being drawn at f3381**. The crop's own last
  > change is f3379 and is **the last frame of the game's own brightness fade**. **The next measurement is 17 pictures, not a diff** — what
  > is the city's last visible act — and then the trace of the date's own source.
  > **One of T107's two reasons is void and is withdrawn**: a *static* framebuffer
  > does not show that the display does not read `$0B53` live, **because
  > `$0B53` itself never changes** (C-068). The poke reason stands alone.
- ~~**Whether `$03:C87F` is a misattributed PC or a real second entry
  point**~~ → **RESOLVED by T104: neither. It is the loop's back-edge branch,
  `BNE $C877`, and it is a real executed instruction** (fetched opcode `$D0`).
  Its count matching the other three to within 4 was never the evidence of a
  misalignment — it is what a tight loop looks like. **R-038 retracts the
  misattribution reading.** ~~What remains open is *why `$03C87E` is skipped*,
  which is T105.~~ **ANSWERED by T105: the PC advance.** `CPX #imm` reads a
  2-byte operand and advances 3, so `$03C87E` is stepped over — measured as
  `NPC=$03C87F` on all 36 340 iterations, not inferred from a decode table
  (C-073).
- ~~**Whether *any* route, in either project, reaches a simulating city.**~~
  **RESOLVED (2026-10-02, T101 `9069182`) — the question was an artefact.** The
  two peer runs never disagreed about the peer: the 9 000-frame run is
  reproducible byte for byte (`master_clock=3216243544 insns=107365572`), and at
  f9000 it reads `$0B55 = 07` (AUGUST). It printed `076C` because
  `study/peer-linux/jjhead.c` clobbered the month column — **our instrument, not
  the peer**. The month advanced six times in that window; `AND #$0003` extracts
  the quarter, so 27 ticks are `6 × 4 + 3`. **The premise "the reference simulates
  and we do not" IS available**, and the reference is now the strongest reference
  we have: 29 date images in 30 000 frames, Deck-native.

---

## Where the project is

**The migration to static recompilation is done.** T031 closed it. The shipped
binary runs the game's original 65C816 logic through snesrecomp, and 187
functions are compiled ahead of time to native C. What is *not* done is the
game being playable end to end, and the reason is precise and narrow.

### Working

| Area | Evidence |
|---|---|
| Recompiled 65816 drives the PPU | `test_deterministic_replay`, `ctest` 1/1 |
| Title screen, city view, HUD, tool palette | frame 400 = ROM title logo; frame 1200 = city |
| Runtime stability | 12 000 frames headless, no watchdog, no `TRAP_BADPB` |
| Game-native graphics and palettes | decoded from the user's ROM at runtime |
| AOT of declared functions | 187 functions; behaviourally identical to 0 AOT, byte for byte |
| Mouse (P2), resolution presets, save states, rewind, turbo, config bar | T041–T054 |
| Headless capture gate | `make test-rom` — proven to fail a black build |

### Broken — the one thing that matters

**The game hangs as soon as the city loads.** It is not a slow simulation and
not a frozen clock.

- The last picture change in a 12,000-frame run is **frame 3382**. After that
  the screen is bit-identical for 8,600 frames.
- **Input does nothing** — 1,700 frames of deliberate presses and clicks after
  the city is live produce no change at all.
- CPU state sampled 800 frames apart is byte-identical except for `A`, which is
  the frame counter. `S=1FE4` never moves, so no subroutine is entered. The only
  interpreter PC that executes is `$0092E3`.

That is a livelock in the **VBlank wait loop** at `$9311` — the same address
`recomp/bank00.cfg:32` already pins with `force_lle`, and the same family as
T050. The `$0406` counter still ticks at +1/frame because the NMI path runs;
the main loop just never gets past the wait.

**What has been eliminated**, each by measurement rather than argument: game
flow (solved, `scripts/d_city.script`), headless mode (a real display and real
audio on the Deck produce byte-identical WRAM *and* screenshot), cross-machine
divergence, frame pacing, a pause/speed gate (forcing `$0408=0` arms `$040A`
and its consumer then runs every frame without producing a month), and the
"slow-moving bytes" as the date field (`$2510-$251F` is periodic on a ~10,000
frame cycle — animation phase).

**The next step is narrow and mechanical**: what is `$9311` waiting for, and why
does it never arrive? That is a disassembly of the loop and of whatever sets its
flag, plus the AOT/LLE boundary — the fork documents that AOT code never
advances the PPU beam while the interpreter advances it per opcode, which turns
a VBlank wait into a hang rather than into a wrong picture.

Three weeks of WRAM diffing produced the idle counters at `$007C`/`$01B3`/
`$01D5` because a livelocked guest has no slow-moving game state to find. Do not
diff again; disassemble.

## Next

| # | Item | Why |
|---|------|-----|
| 1 | Disassemble the `$9311` VBlank wait and find what sets its flag | The game livelocks there. Not a diff question — the guest has no game state to diff |
| 2 | Gate the clock, not the motion | `make test-rom` proves the picture moves; a frozen city moves 4×/1000 frames, so the gate would pass. The gate must ask whether the date advances |
| 3 | T025 content assertions | Prove the screen is the *right* screen, not just a moving one |
| 4 | Config bar auto-hide (F1) | Requested; the bar covers 21 of 224 rows |
| 5 | snesrecomp: "writes a hardware register → `lle_only`" | T057's root cause, defence in depth. Class size measured at 1 function, already pinned |

## Discovered, not yet ticketed

Found while measuring T061. Recorded here rather than fixed, because each is
either a different owner or a change that should not ride along on a
performance ticket.

| Item | Why it is not fixed here |
|---|---|
| **`<exe> <rom> --script X` silently ignores `--script`** | A real bug, in the snesrecomp fork (`host_main.c:2717`, `2759`): flags are consumed from the front, so `--script` must precede the ROM. It fails *silently* — exit 0, no warning, and the run is indistinguishable from one without a script. Worth noting that this invalidated a first round of scripted evidence in T061, caught only because the output was byte-identical to the unscripted run. |
| **`upload-present` costs more than the whole guest** | **RETRACTED as stated (2026-10-02).** The 2.45 ms Deck guest figure was never re-verified; the register's own re-measurement of the same stage was **4.511 ms**, i.e. 27% of the budget rather than 15%. At 4.511 vs 6.540 the ratio is 1.45x, not 3.3x, and the three stages sum to 16.97 ms against a 16.67 ms budget, so the frame is oversubscribed on the Deck. **"The 65816 is not the bottleneck" is therefore UNPROVEN and is not asserted.** No current per-stage split exists: the Deck cannot build this project (SteamOS, no glibc headers), so every Deck figure comes from a copied binary. Re-measure before using this row to justify any work. |
| **`interp816_opcode_hist_dump()` has no caller** | The per-opcode nanosecond histogram is already being collected at `interp816.c:288` and thrown away. Wiring it up would separate raw interpreter dispatch cost from the bridge's PPU-beam catch-up cost, which the current measurement cannot do. Cheap, and it belongs with a performance investigation rather than inside one. |
| **99 `lle_only` functions are ~89% of guest-execution time** | The single largest lever on emulation cost, and the reason "native" needed qualifying. The reasons are 97 `structural_poison` / 41 `empty_decode` / 8 `unproven_callee_exit` — an analysis limitation in the recompiler, not hardware. Raising the AOT fraction is a recompiler project, not a game project. |
| ~~**No script reaches a running city**~~ **RESOLVED (2026-09-30) — false.** `scripts/d_city.script` reaches a live city headlessly; `mouseclick right` does reach the guest, `press left` wraps the key list, four `press down` land on `ENT`. Deterministic across runs. The performance figures above are therefore still unmeasured *in a city*, but the excuse is gone: the route exists and the next person can just run it. |

## Non-Goals (Explicitly Out of Scope)

- ROM hacks and fan translations.
- 3D rendering or a modern-graphics overhaul.
- Mobile ports.
- **Multiplayer.** T016 appears in older revisions of this file as a backlog
  item; it is not planned. Local hotseat is not a goal.

## Archive

T010, T012, T013, T019, T020, T022, T024, T028, T029 and T038-touched tickets
referenced the deleted pre-recompilation `src/gfx` architecture and were archived
in T038. T011, T030 and T033 were re-scoped onto the recompiled game. T026, T027,
T031, T032, T035, T036, T037, T039, T040–T044, T046, T047, T049, T050, T053–T057
are closed. See `aes/kanban.md`.
