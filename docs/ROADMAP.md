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

### The recommendation

> **T102 is CLOSED and it changed the question. The recommendation is now
> T104: frame-resolve the `$03C87x` scan loop.**
>
> **Numbering note, because it is the kind of thing that silently becomes a
> wrong pointer.** `T103` is **already taken**: `aes/decisions/D015.md`
> (an `aes-project-manager` record, 2026-10-03) filed *"T103 — create
> T102-plan + Deck runbook + `docs/QUALITY_GATES.md`"*. That ticket was
> never materialised as a file, so a `ls aes/tickets/T103*` finds nothing and
> the number looks free. **The earlier claim keeps the number**; the scan-loop
> measurement is therefore **T104**. `aes/` is untracked so this file is the
> only place the collision is visible.

T102's answer to its own question (`$03:8026` over f3000–f13080) is **0**, and
the answer is now trustworthy in a way T100's was not — it forced a retraction,
because **T100's counter was watching PC `$000003`**: `COUNT_PC=038026` parses
with base 0, a leading `0` means octal, and `038026` → `3` (ledger **R-037**,
`docs/CONFLICTS.md` **CONF-15**). The conclusion was re-derived three ways and
holds; the measurement that produced it did not.

The three companion reads are the new content, and they are why the question
changed:

| read, f3000–f13080 unless stated | result |
|---|---|
| `$03:8026` executions, whole 14 000-frame run | **0** |
| bank `$03`, interpreted, **f3272–f13080** | **0 distinct PCs / 0 steps** |
| bank `$03`, AOT, **f3000–f13080** | **18 entries: 3 at f3259, 15 at f3270, none after** |
| `$0B51`–`$0B5F` writes, whole run | **66, none after f3259** (10 741 frames of silence) |
| `$0DC0`–`0DD0` writes, whole run | **61 in 4 events, none after f3259** |
| city state at f3300 vs f13000 | **byte-identical** over `$0B40`–`$0DC7` |

**So the evidence no longer says "the tick is absent". It says the city is
never *started*.** Bank `$03` runs **169 693 interpreted steps** in f3000–f3271
and then executes **nothing whatever** for **9 809 frames** — the span holding
the reference's sixteen month rolls and its first year rollover. A bank that
never executes again cannot be advancing anything, and in this window was not
being started either.

**86% of that cost is four PCs** (C-069): `$03C877` `STA $7F6B00,X` (36 344),
`$03C87B` `INX` (36 343), `$03C87C` `CPX #$F4` (36 344), and `$03C87F` (36 340)
— **which is not an instruction boundary** (it is the second byte of
`8D D0 F6` = `STA $F6D0`) and is therefore reported unattributed. `CPX #$F4` is
followed by `STA $F6D0`, **not a branch**: this is a **scan loop's test**, and
its back-edge is outside the logged neighbourhood. **What it scans and what ends
it at f3271 are unmeasured, and no cause is claimed.**

> **T104 — the single next measurement.** Frame-resolve the `$03C87x` scan
> loop: its last execution, its back-edge, and what it is scanning. Read it as
> **what runs bank `$03` between the city appearing and the gate closing** — a
> *start* question — not as *what advances the clock*.
>
> **Instrument:** `SNESRECOMP_INTERP_TRACE_FRAMES` on a **narrow** window around
> f3271, interleaved with `SNESRECOMP_AOTBLK` over the same window, on the Deck.
> Narrow is forced: a few frames of bank `$03` is small, and 10 000 frames of
> bank `$00` is not (the T102 AOT run wrote 457 MB for 10 080 frames).
> **No source change and no new instrument.**
>
> **Falsifiers, stated before running — and the first one already fired once
> today, so it is listed first.** F1: the trace prints nothing even with
> `INTERP_PROFILE=1` → the path is **falsified as executable**, the outcome is an
> instrument trap, and **no substitute measurement is offered**. F2: a run not
> ending `exit: RUN_FRAMES reached` is **void regardless of what it prints** —
> `SDL_QUIT` on this Deck means we signalled it. F3: **if the loop's last
> execution is after f3271, C-039b's boundary is wrong** — that is the more
> interesting result, not a failed probe. F4: every execution count carries a
> **positive control** on a PC known to execute in the same run configuration
> (trap 0 in `README.md`), or the count is uninterpreted.

**Chosen over** `$0014` bit 7 / the bank-`$03` entry graph (2 311 entries, only 1
via `$00:8056`), over `$03:DBB3`, over "make `$03:8026` run", and over
re-running the tick count — for four reasons in order of weight:

1. **"Make `$03:8026` run" is no longer the recommended first move, and the brief
   for T102 said so in advance.** The evidence now says the city is never
   started; the new-city routine runs once at f3259 and the state block is never
   touched again. Proposing another tick-side theory before establishing what
   *starts* the simulation would be the same self-defeating shape as the f3301
   bracket.
2. it is the **only** candidate that names the 86% of the cost that is actually
   being spent, and **both answers are informative** (it ends at f3271 with
   everything else, or it does not and C-039b is wrong);
3. it costs **one Deck run and zero new code**;
4. it **gates the interpretation of everything else** — if the loop is what ends
   bank `$03`, then `$0012`, the bank-`$03` death and the abandoned state block
   are one boundary rather than three coincidences.

### The reconciled queue

| ticket | state |
|---|---|
| **T100** — does `$03:8026` run in ours past f3857 | **CLOSED `1b099ce`, and its measurement is now RETRACTED (R-037)** — `COUNT_PC=038026` was watching PC `$000003` (base-0 parse, CONF-15). The *conclusion* is re-measured over 14 000 frames by T102 and holds; the *measurement* is void. **Superseded in scope by T102** |
| **T102** — does `$03:8026` run across the reference's first year rollover | **CLOSED — NO, and the previous answer was void.** Deck-native, 4 runs × 14 000 frames, `EXIT=0` / `exit: RUN_FRAMES reached` on all four. `$03:8026` = **0 executions**, exhaustive over both tiers (`$038026` is inside an `lle_only` node; **0** `aot_eligible` nodes cover it). Bank `$03` = **0 PCs / 0 steps in f3272–f13080** and **18 AOT entries** in the window, all at f3259/f3270. City state block: **0 writes after f3259 in 14 000 frames**. **Forced retraction R-037** and **CONF-15**. **C-046c closed.** C-041, C-008, C-052, C-057, C-058 stand |
| **T104** — what runs bank `$03` in f3000–f3271, and what stops it | **OPEN, and it is the single next measurement** — frame-resolve the `$03C87x` scan loop (86% of the bank's cost, one of its four hot PCs not being an instruction boundary). Falsifiers stated in `2026-10-03-t102-…` §7 |
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
retraction count 22 → **29 of 37 rows** (R-035, R-036 by T101 `9069182`; **R-037 by T102**) · bank-`$03` range `$03C63D`–`$03E57E` →
**interpreted** `$03C63D`–`$03E57E` and **AOT** `$03B477`–`$03C463` (ledger
R-033) · AOT total 1 430 539 → **1 430 540**, bank `$03`'s 18 = **15 in f3270 +
3 in f3259** · reference month rolls **28 in 30 000 frames**
(Deck-native) · `$0B51` decoded as **4 × (months elapsed) + quarter**, 1 344 samples,
zero violations.

### What is still OPEN and is not going to be closed by any of the above

- **Why the city does not simulate** (C-006). No cause is asserted anywhere.
  **Narrowed by T102 and re-posed as a *start* question:** bank `$03` executes
  nothing whatever in f3272–f13080, and the city-state block is not written once
  in the 10 741 frames after f3259. What *starts* the simulation is now the
  question; what advances it is downstream of a bank that never runs again.
- **What the `$03C87x` scan loop scans, what branches back, and what terminates
  it at f3271** (C-069). 86% of the bank's cost in f3000–f3271 sits there and one
  of the four hot PCs is not an instruction boundary. **OPEN, no cause claimed.**
- **Whether `$03:C87F` is a misattributed PC or a real second entry point.** Its
  36 340 steps match the other three to within 4, which is what a misaligned
  attribution looks like and also what a real hot instruction looks like.
  **Unresolved, and deliberately reported as unattributed per C-056's rule.**
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
