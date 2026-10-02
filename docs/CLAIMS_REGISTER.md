# Claims register — what the repo currently asserts, and what is known to be wrong

Created 2026-10-01 at `ec4cabe`. Purpose: `docs/RE_CITY_FREEZE.md` retracts its
own errors *in place*, which is correct practice but does not reach a reader who
arrives at a different file. This file is the index. It lists every claim in the
repo's tracked docs and scripts that is retracted, superseded, or unverified, and
says where the retraction lives.

Status vocabulary used throughout:

| Status | Meaning |
|---|---|
| **MEASURED** | an artifact exists in the repo; the number is reproducible |
| **CLAIMED** | asserted in prose; no artifact a reader can re-run |
| **RETRACTED** | asserted somewhere and known false |
| **SUPERSEDED** | was true, replaced by a later measurement |
| **UNVERIFIED** | cannot be checked cheaply; explicitly not confirmed |

---

## 1. Corrected in place, but only in `RE_CITY_FREEZE.md`

These eight are the user's own retractions. `RE_CITY_FREEZE.md` handles them well.
None of them is corrected in the file a reader is most likely to open.

| # | Retracted claim | Retraction lives in | Still asserted in |
|---|---|---|---|
| 1 | `$0B51` "stays 0" — retracted *then* re-asserted as a free-running mod-4 counter, both wrong | `RE_CITY_FREEZE.md` 2026-10-01 (g) | — (README:61 now correct) |
| 2 | `$02BF` is a pause flag (written once in the whole ROM) | `RE_CITY_FREEZE.md` 2026-09-30 | README:138 — **correctly says "not a pause flag"** ✅ |
| 3 | `$0B53=0` / `$0B55=0` means "1900 January" — `$0B53` is the absolute year, 0 means **no city** | `RE_CITY_FREEZE.md` 2026-09-30 | — |
| 4 | "The naming screen needs a mouse" — the d-pad moves the cursor, B confirms | `RE_CITY_FREEZE.md` 2026-09-30 | README:95 ✅ · ROADMAP:86 ✅ |
| 5 | `$0B12` is the sharpest lead — it is `$00` across all 337 peer dumps | `RE_CITY_FREEZE.md` 2026-10-01 (g) | README:132 — **correctly retracted** ✅ |
| 6 | `CODE_008061` / `CODE_00825F` never run — false for the keyboard route, where `$1F7D..$1F7F = 00 80 03` | `RE_CITY_FREEZE.md` 2026-10-01 | **README:88 still asserts "`CODE_008061` demonstrably does not run"** ❌ · **scripts/clock-gate.sh:273** ❌ |
| 7 | `JSL ($1F7A)` at ROM `0x76DE9` — the scanner matched the operand without checking the opcode; **zero** occurrences of `FC 7A 1F` | `RE_CITY_FREEZE.md` 2026-09-30 | — |
| 8 | "Load average 8–10" — `/proc/loadavg`'s `8/1433` is running/total; real loadavg was 2.44 | — | T061 / kanban:30 still narrates "load average 8.9 em 8 cores" as the contamination cause ⚠️ |

Items 2, 4 and 5 have reached the reader. Items 1, 3, 7 are only in
`RE_CITY_FREEZE.md`. **Items 6 and 8 are still live somewhere a reader will hit
them.**

---

## 2. Retracted causes still asserted by `make clock` itself

`scripts/clock-gate.sh` is the gate. Its failure text is the most-read prose in
the project, and it is **two generations out of date**.

`scripts/clock-gate.sh` printed, under the heading "What is established"
(the following quotes are RETRACTED 2026-10-02 - see section 2; that text
has since been removed from the script):

- "the main loop at `$00804D` branches on vblank-done token `$0012`"
- "`$0012` is written in exactly one place in the whole ROM — the tail of the
  round-robin scheduler `CODE_03D283`"
- "Measured 0 in 13 of 13 frame-boundary samples, so the per-vblank body
  `CODE_008061` never runs"
- "So the gate is `$0012`, and `$0012` waits on the scheduler loop at
  `CODE_03D287` exiting, which needs bit 7 of `$0014`."

All four lines are superseded. The current measured position is that `$0B51` is
`0000` at every sample and `INC.w $0B51` executes **zero** times, and that the
tick routine's block `$03:8000-$03:81FF` has no external direct edge. The gate's
own message asserts a specific cause that has been ruled out, and it does so in
the section labelled "established".

It also frames the symptom as "The city loaded … and then **stopped** simulating"
and predicts "controller ignored". The city never simulates; the controller is
not what is being measured. A gate that reports a wrong cause is worse than a
gate that reports none — it teaches the next reader the answer.

**Also wrong, same file family:** `scripts/cross-load-peer-save.sh:7` still says
"29 WRAM bytes move in 3600 frames"; the current figure is **53 WRAM addresses
across 30,000 frames**. Its header also still asserts the `$C9 & #$9000` gating
`INC $14` chain, which is the retracted-generation diagnosis.

---

## 3. Performance numbers that are wrong in four places

**UPDATE 2026-10-02 — the figures below are SUPERSEDED, and the last of the
four sites has now been corrected.** `README.md:179`, `docs/ROADMAP.md:83` and
`scripts/perf-gate.sh:41` have all been fixed; `aes/tickets/T061` is local and
gitignored. Additionally, `make perf` **straddles its own threshold on the dev
host** — the same binary measured **FAIL at 48.38 fps** and **PASS at 51.52
fps** against a threshold of 50, in one session. So the figures below are a
historical correction, not a current measurement, and **no current per-stage
split exists for this binary** (the Deck cannot build the project at all).
`scripts/retracted-claims.tsv` rows R-012, R-021.

`aes/tickets/T061`, `README.md:179`, `docs/ROADMAP.md:83` and
`scripts/perf-gate.sh:41` all carried (RETRACTED 2026-10-02):

> Deck `guest` **2.45 ms**/frame (~15% of the 16.67 ms budget);
> `upload-present` **8.13 ms** — more than the whole guest;
> therefore **the emulated 65816 is not the bottleneck**.

The measured Deck figures were **guest 4.511 ms**, **upload-present 6.540 ms**,
**deadline wait 5.916 ms** — themselves taken at `ec4cabe` and not re-measured.

Three consequences, in ascending order of seriousness:

1. `guest` is 27% of budget, not 15%.
2. `4.511 + 6.540 + 5.916 = 16.97 ms`, which **exceeds** the 16.67 ms budget.
   The frame is oversubscribed on the Deck. The 60.05 fps reading is
   vsync-masked and there is **no headroom at all** — which `perf-gate.sh`'s own
   comment already half-admits ("vsync-capped to 60, therefore the fps shows no
   headroom whatsoever"), but then cites `guest` ms as the number that *does*
   show headroom. It does not, in combination.
3. **"The 65816 is not the bottleneck" is UNPROVEN** (RETRACTED as stated). It
   rested on guest being 2.45 ms against a large `upload-present`. At 4.511 vs
   6.540 the ratio is
   1.45×, not 3.3×, and the total does not fit in the budget. ROADMAP:83's
   "the obvious next lever on performance" is correspondingly weaker than stated.

`make perf` **PASSing at 60.05 fps against a threshold of 50 therefore proves
nothing about headroom**, exactly like the other two green gates prove nothing
about the city.

---

## 4. `scripts/cross-load-peer-save.sh` is a trap

The script is **known impossible**: the 32 KiB battery SRAM does not carry the
city (32746 bytes of `0xFF` around `"SIM"`; identical md5 from a session at the
naming screen and from one running JAN to MAR).

The script has **no dead-end banner, no deprecation note, and no exit guard**.
It is executable, well-written, plausible, and its header still argues the case
for why the experiment *should* work. Nothing in it tells the next reader it
cannot. The commit log shows the author already knew (`0c76a37 clock: the
cross-load cannot work, and I designed it on an unchecked assumption`) — the
knowledge went into a commit message instead of the artifact.

---

## 5. `study/peer-linux/jjwin.c` — the header comment contradicts the code

The comment at `jjwin.c:69` says:

```
 *   press <button> <frames>   hold a button for N frames
```

The code does not do that. `script_mask()` holds for exactly five frames:

```c
if (g_press[i].at <= frame && frame < g_press[i].at + 5u)
```

and the parser advances with `at += n`, so **`n` is the gap to the next press**,
not the hold length. A long hold is not expressible.

This is worse than a usability gap: the documentation asserts the opposite of the
behaviour, so a reader who trusts it writes `press a 200` expecting a 200-frame
hold and gets a 5-frame tap 200 frames later. That is a different experiment,
producing a different (wrong) result, with no error.

---

## 6. The peer cannot answer the open question — measured, not suspected

`simcity_recomp_log_open` exists in the peer's public API. What it emits has now
been enumerated from the source: **ten event tags, none of which carries a PC.**

```
create  log-open  log-close  cold-reset-ready  frame
renderer-warning  route-failed  reset-failed  sram-load  destroy
```

The `frame` event carries `frame`, `cpu_instructions`, `master_clock`. The
exported surface is 37 `simcity_recomp_*` symbols; the only PC anywhere in the
public header is `current_smp_pc`, which belongs to the audio DSP, not the 65816.
**There is no PC trace, no block trace, and no execution log.**

The item was filed as "cheapest remaining test of the open question". It is now
a **measured dead end**. Our own build is the only remaining instrument.

---

## 7. The peer repository declares no licence

`/tmp/opencode/peerstudy` (`github.com/Junior-Jones/SimCity-SNES-Static-Recomp`,
HEAD `454eb0a`) has no `LICENSE`, `LICENCE`, `COPYING` or `NOTICE` file. The
only licence-adjacent file is `THIRD-PARTY-NOTICES.txt`. Its README says
Snes9x-derived S-SMP semantics "remain under their original license" — which
identifies a *third-party* obligation and says nothing about the peer's own
terms.

We hold a private clone and `study/peer-linux/` links against their public API.
**This needs an owner decision, not a README footnote.** Tracked as T069.

---

## 8. Address labels in the investigation are unverified — byte-pattern claims are safe

This is a finding of this review, not a retraction, and it applies to *any*
address the project has asserted from a ROM file offset.

The project's ROM→CPU address translation could not be confirmed. Two spot
checks contradict the obvious mapping:

- The bytes at **file offset `0x930D`** are
  `22 11 a0 31 06 11 84 31 26 11 af 30 05 00 b0 31 …` — a dispatch table, not
  the `STZ $00B9 / INC / LDA / BEQ / RTS` spinlock that
  `recomp/bank00.cfg:44` describes at `exclude_range 0x930D 0x9318`.
- The byte sequence `02 00 28 6B` — the claim for `CODE_008206` — occurs exactly
  once, at **file offset `0x20C`**, not at any offset consistent with the
  `+0x10000` relationship that *does* hold for the `$03:8026` / `$03:C77E`
  labels.

Consequences, and they are asymmetric:

- **Safe.** Every claim of the form "this byte sequence occurs *exactly once* in
  524,288 bytes" is mapping-independent and was re-verified in this session
  (§9). These are the load-bearing claims.
- **Unsafe.** Every claim of the form "the instruction at *CPU address* X is Y"
  depends on a translation that nobody has written down. Tracked as T071.
  This must be pinned before any tooling consumes these offsets, and before any
  force_lle / exclude_range / pokefor target derived from them is trusted.

---

## 9. Re-verified in this session (2026-10-01, at `ec4cabe`)

Method: direct byte search over `SimCity (USA).sfc` (524,288 bytes) and direct
reads of the working tree. No game run.

| Claim | Result |
|---|---|
| `EE 51 0B` (`INC.w $0B51`) occurs exactly once, at file offset `0x18026` | ✅ **confirmed** |
| `9C 51 0B` (`STZ.w $0B51`) occurs exactly once, at `0x1c77e`, inside a run of `STZ.w` state clears (`9c cb 0c / 9c 99 01 / 9c 97 01 / 9c 01 0b / 9c 51 0b …`) | ✅ **confirmed**, and it is genuinely an initialiser |
| one writer of `$0B51` | ⚠️ **partly wrong**: `8F 51 0B` (`STA.w`) occurs **zero** times. The single writer is `8D 51 0B` = **`STA.l`**, at `0x1c9e3`. The address and the uniqueness are right; the mnemonic form is wrong. Same class as retraction #7. |
| `FC 7A 1F` (`JSL ($1F7A)`) occurs zero times | ✅ **confirmed retracted** |
| `02 00 28 6B` occurs exactly once, at file offset `0x20C` | ✅ sequence confirmed · ❌ **address label unconfirmed**, see §8 |
| `_canonical_wait_loop` is back to `LDA abs / BNE -5` (`AD … / D0 FB`) | ✅ the `LDA dp / BEQ -3` widening **was reverted** |
| `src/game_rtl.c` contains only `interp_bridge_run_until_quiescent`; none of the 12 recent `clock:` commits touch it | ✅ **the reorder and both `interp_bridge_run_loop` attempts were reverted** |
| `interp_bridge_run_scheduler` and `interp_bridge_run_loop` exist | ✅ in `snesrecomp/runner/src/snes/interp_bridge.h` — **T062's prescription is buildable**, and is *not* the same function as the two that livelocked |
| `recomp/bank00.cfg:44` contains `exclude_range 0x930D 0x9318` | ✅ T062 AC #1 satisfied |
| peer public API carries no PC or block trace | ✅ **confirmed dead end**, §6 |
| peer repository has no licence file | ✅ **confirmed**, §7 |
| `aes/` and `.aes/` are in `.gitignore` | ⚠️ **the entire ticket queue is untracked and has never been committed** |

---

## 10. Not verified — do not read these as measured

Flagged because they are load-bearing and no reader can currently re-run them:

- All WRAM sampling figures (`$0B51=0000`, `$0DC7=0000`, `$0CE7=00`,
  `$0B53=076C`, `$0B55=0001`, "53 addresses change across 30,000 frames").
  **CLAIMED.** No probe output is committed.
- All peer reference figures (`$0B51` +1 per ~200 frames, month =
  `(($0B51 >> 2) mod 12) + 1`, `$0406` +100 per 100 frames, "102,158 addresses").
  **CLAIMED.** The peer dumps live in an untracked directory.
- The Deck measurements (4.511 / 6.540 / 5.916 ms). **CLAIMED.**
- The 32746-bytes-of-`0xFF` SRAM result. **CLAIMED.**
- Asar rebuilds to md5 `23715fc7ef700b3999384d5be20f4db5`. **CLAIMED.**

The pattern is consistent and worth naming: every *byte-level* claim in this
project has survived re-verification, and every *runtime* claim is currently
unreproducible by a reader. See T074.
---

## 11. RETRACTED 2026-10-02: "the city does not load"

**This file did not list it, and that omission is the finding.**

`docs/RE_CITY_FREEZE.md` entry (q) asserted **"A cidade não carrega"** — the
city does not load — with `$0B53 = 0`, population `0`, funds `0`. **False.**

Measured at `afceeec`, five WRAM samples (f3400, f3600, f4000, f5000, f5999)
via `scripts/d_city.script`:

| | value |
|---|---|
| `$0B53` (absolute year) | `0x076C` = **1900** |
| `$0B55` (month) | `0x0001` = **January** |
| `$0B9D` (treasury, 32-bit LE) | `20000` — the `$20000` on the HUD, exactly |
| framebuffer f3400 / f4000 | a rendered city: terrain, RCI bar, `1900 JAN`, `BullDoze Area $ 1`, cursor |

Two independent instruments agree, and one of them is the guest's own state
rather than the renderer. The register's own §1 item 3 rule ("`$0B53` is the
absolute year, 0 means no city") predicts a non-zero year for a loaded city.

**Why the wrong conclusion was reached:** the runs behind entry (q) used a
**zeroed 32,768-byte save**, which is not the state `scripts/d_city.script` was
written against. The correct label for those runs is **unknown**, not
"regression". Entry (q) contained its own refutation in its final line and drew
the conclusion anyway.

**What replaces it:** the city **loads and renders**; it does **not** simulate
(§13). Why it does not simulate is **OPEN**.

`make clock` now enforces this: it reads `$0B53` itself and refuses to report
PASS when it is zero. See `docs/DEFINITION_OF_DONE.md` D2.2.

## 12. RETRACTED 2026-10-02: `force_lle 0x009311` is not in the config

`RE_CITY_FREEZE.md` stated, in the present tense and in backticks as file
content, that "o nosso cfg **já tem** `force_lle 0x009311`". It does not.
`git log -S"force_lle 0x009311" -- recomp/bank00.cfg` shows it was **removed** by
`436b25b` (2026-09-30), *before* the text asserting its presence was written.

The actual set is `{0x008000, 0x00804D, 0x0080B2, 0x00927C, 0x009280,
0x009287, 0x00928F}`. It was removed **because** a `force_lle` pins one PC inside
a function beginning at `$930D` — so a reader who acted on that sentence would
have re-introduced a bug that was already diagnosed and closed.

Related trap: `recomp/funcs.h` defines `CODE_009311` as an **alias of
`$03:7649`**, not `$00:9311`. Grepping `009311` in `recomp/` finds the wrong
routine in the wrong bank.

## 13. MEASURED 2026-10-02: the city loads, renders, and does not simulate

This is the current position. `make clock` on the dev host and on the Deck both
print `1 distinct date images after f3600 (last change f3378 of 6000)`.

Byte-level diff of 128 KiB WRAM across a live city:

| window | bytes differing |
|---|---|
| f3400 → f3600 (200 frames) | 30 |
| f3600 → f4000 (400 frames) | 33 |
| f4000 → f5000 (1000 frames) | 32 |
| f5000 → f5999 (999 frames) | 34 |
| **f3400 → f5999 (2599 frames)** | **34** |

30 of the 34 are in the first 4 KiB; 10 are at `$2510-$251F`, the cursor's OAM
slot. `$00C7` — a per-frame counter — advances at 5/5 successive samples, so the
CPU is running every frame. **The CPU runs; the city does not.**

**OPEN, not guessed:** why. `$0B51` is `0000` at 5/5, but its role as "the
master city tick" is INFERRED from the peer's trace, not measured here. The next
measurement is a PC/block histogram over f3400-f3600.

**Superseded:** "53 WRAM addresses across 30,000 frames" (§14) and "29 WRAM
bytes in 3600 frames" are different windows and are not directly comparable.
Neither is current; use the table above.

## 14. Corrections to this register's own §8, §9 and §2

- **§8 is RETRACTED.** The ROM→CPU mapping is **pinned**: HiROM
  `offset = bank*0x8000 + (addr & 0x7FFF)`, byte-verified on four independent
  labels (`$00:930D`, `$00:80B2`, `$03:8026`, `$03:C77E`). The register's
  counter-evidence — "the bytes at file offset `0x930D` are a dispatch table" —
  is *true and irrelevant*: `0x930D` is the naive offset that omits the
  `& 0x7FFF` mask.
- **§9's row** "`recomp/bank00.cfg:44` contains `exclude_range 0x930D 0x9318` ✅"
  is **false**: it has read `0x130D 0x1318` since `afceeec`.
- **§2's quotation** of `clock-gate.sh`'s "$0012 is the gate" text is
  **retracted**: `$0012` measures `0001` in 5/5 samples and `$0014` measures
  `8000`. That text has been removed from the script.
- **`CODE_008061` / `CODE_00825F` never run` — status is OPEN, not RETRACTED.**
  §1 item 6 lists it as RETRACTED, and that status is wrong. Its
  premise (`$0012 == 0`) was refuted, which voids the inference and establishes
  nothing about whether it runs. Recording it as retracted would swap one
  unmeasured assertion for another of opposite sign — the mechanism behind the
  paired `$0B51` retraction.
- **§9's closing note** offers T078 option 1 (commit `aes/`). **That option is
  forbidden**: `aes/` is gitignored permanently, and the review rubric was moved
  out of it for exactly this reason (`4b974f5`). Durable content belongs in
  `docs/`.
- **"254 distinct crc32"** is stale; `make test-rom` printed **257** on
  2026-10-02.

## 15. The ledger, and the guard that now checks it

The retraction count lives in **`scripts/retracted-claims.tsv`** — data, not
prose, so it can be checked, diffed and appended to. This file is the index
that points at it.

`scripts/check-retracted-claims.sh` (wired as `make check-claims`) fails when a
script or doc asserts a claim the ledger records as refuted **without a
retraction marker where the claim is made**. Run it with `--self-test`; it
demonstrably fails on a seeded violation, which is the only way to know a guard
works.

**A measurement is retracted only by a second measurement. An interpretation may
be retracted by reasoning.** Never retract a measurement with an argument —
that is how the correct `$0B51` claim was replaced by a wrong one.

## 16. MEASURED 2026-10-02, entry (s): bank 03 executes, and stops at f3301

Full artifact, with the exact commands and the raw per-bank counts:
**`docs/measurements/2026-10-02-deck-interp-histogram.md`**. Machine: Steam Deck,
binary compiled natively on the Deck (Release + `-DSNESRECOMP_INTERP_PROFILE`),
headless. *Environment-fidelity caveat: the Deck's SteamOS rootfs is missing 503
of 504 glibc headers while pacman reports the package installed, so this build
resolves libc headers from a hand-assembled prefix via `-idirafter`. Every Deck
number inherits that.*

| Claim | Status | Evidence |
|---|---|---|
| "bank 03 does not execute" (as a claim about the whole run) | **REFUTED** | 921 distinct PCs, 515,043 interpreted steps over f0–f3700 |
| "bank 03 does not execute in the live city" | **MEASURED, TRUE** | 0 steps in f3381–f3700; 0 steps in f3300–f3380; independently reproduced by entry (r)'s AOT bracket for f3400–f3600 |
| "the bank-03 tick … still does not run" (`README.md`) | **REFUTED as stated** | it runs until f3300; the defensible claim is "not after f3300" |
| where bank 03 stops | **MEASURED: f3301** | 160,693 steps in f3100–f3300 (≈803/frame), then zero |
| "`$03:8026` (`INC.w $0B51`) is among the bank-03 PCs that execute" | **OPEN** | `$0B51 = 0000` at f3600 while bank 03 ran 515,043 steps — suggestive, **not** decisive: the dump prints only the top 60 PCs by host-ms |
| "bank 03 executes no AOT blocks anywhere" | **OPEN** | the histogram is blind to AOT; no whole-run AOT histogram exists |
| why bank 03 stops at f3301 | **OPEN** | not measured. No cause asserted. |

Two structural facts from the same run that are worth more than the bank-03
question, because they change how the rest of the file should be read:

- **The live-city window is confined to banks 00 and 01** — 813 + 415 distinct PCs.
  `$02` and `$05` are zero there too, not only `$03`. Entry (r) reported only the
  `$03` absence.
- **The live-city window's interpreted time is the vblank handshake.**
  `$009313`/`$009311`/`$009315` at ~610,200 steps each over 339 frames, plus the
  NMI handler `$0080B2` at 320 executions (once per frame), with `$00B9 = 01` and
  `$00C7` advancing at f3600. Consistent with the deadlock being broken; it is not
  a hang.

## 17. A third instrument trap: a header with no rows is not a negative result

`[interp_profile] N distinct PCs, top 60 by host-ms` prints **nothing** unless
`SNESRECOMP_INTERP_MS_PROF=1` is set. With it unset, every entry's `ms` is `0.0`,
`_hist_cmp` returns 0 for all pairs, the sort is stable, and the leading array
slots are unused hash-table entries (`PROFILE_HIST_CAP` = 65,536, ~6,000 used), so
the `s_interp_hist[i].n` guard fails before 60 entries. Observed directly: the
whole-run dump printed `6055 distinct PCs` and then **zero** PC lines.

Third instance of the same class after (r)'s two (`CYC_WATCH` blind to AOT;
`AOTBLK` taking a frame window, not a PC range). **The rule this earns: a
measurement section header with no rows under it is not evidence of absence, and
must not be cited as one.**

## 18. THE RETRACTION COUNT, reconciled to one number and one source (2026-10-02)

This file was found carrying **four different prose counts of its own
retractions**, none of them equal and none of them computed:

| where | what it said |
|---|---|
| `docs/RE_CITY_FREEZE.md` | "doze retractações" <!-- count-quote --> |
| `docs/DEFINITION_OF_DONE.md` | "eleven retractions", in four places <!-- count-quote --> |
| `scripts/check-retracted-claims.sh` | "eleven retractions", in its own header <!-- count-quote --> |
| `docs/CLAIMS_REGISTER.md` §1 | "These eight are the user's own retractions" <!-- count-quote --> |
| the same header | "git log greps 13 commits" <!-- count-quote --> |

All five were wrong the same way: each was a human tally of a moving target, so
each drifted the moment a row was appended, and none of them was checkable.

**The definition, fixed once, in `scripts/retracted-claims.tsv`:**

> **The retraction count is the number of ledger rows whose status is `refuted`.**

`superseded` is excluded — the ledger defines it as "the claim was true and has
been replaced by a later measurement. It is not wrong, it is old."
`invalidated-premise` is excluded — the ledger says in terms that it "IS NOT a
retraction, and a checker that treats it as one will replace an unmeasured
assertion with an unmeasured assertion of the opposite sign."

Measured at this commit:

```
$ make retraction-count
ledger rows      : 27
refuted          : 19   <- THIS is the retraction count
superseded       : 6   (true but old; not a retraction)
invalidated-prem.: 2   (evidence refuted, truth unknown; explicitly NOT a retraction)
```

**Ceiling on the number, stated rather than left to be discovered:** two `refuted`
rows retract the *same* claim in two languages — R-005/R-006 (English /
Portuguese) and R-007/R-008 likewise — so **19 is an upper bound on the number of
*distinct* retracted claims.** The ledger records no claim identity, and
inventing one to make the number smaller would be judgement dressed as data.

**All five prose counts are removed** — the only surviving mentions are the five
lines of the table above, each carrying an explicit `<!-- count-quote -->`
marker, which the checker counts and prints so the exemption is auditable with
`grep -rn count-quote` rather than hidden. What replaces them is a citation, never a
number: prose that states *no* count is always acceptable, so appending a
retraction never breaks a document and no reader has to keep a tally in sync.
`scripts/check-retracted-claims.sh` grew a section that **fails** when any prose
count disagrees with the computed figure (DoD **D3.6**), and its self-test seeds
a wrong count in both languages to prove the section can fail.

**How the section was built, because the process is the finding.** The first
version matched a number *either side* of the word "retract" and reported 13
violations per run, of which 11 were false positives: `RETRACTED 2026-10-02`
read as "2026 retractions", `436b25b` as "371 retractions", and `§8 is RETRACTED`
as a count of eight. Three further rounds of tightening were needed, and each round's
false positive is named in the script's comment. **The pattern is now
forward-only and adjacent-only, and it reports zero false positives on this
corpus** — at the cost of missing a count separated from the keyword by an
intervening word, which is stated in the script as a known limitation rather than
left to be discovered by the next reader.

## 19. The narrative shape of `RE_CITY_FREEZE.md`, fixed (2026-10-02, Phase 3)

**The instrument first, because it is the standing lesson of this phase series.**
`aes-narrative` ships only `SKILL.md`. Its target is `make narrative-analysis`,
which **does not exist in this repository** — it is a target of the AES toolkit's
own Makefile at `/opt/aes/Makefile:1144`, running `scripts/narrative-analysis.py`,
which pins `AES_ROOT` from its own location and reads the *toolkit's*
`aes/shadow/SD-*.md`, `aes/INDEX.md` and `aes/shadow/access.log`. Run here it
prints **`Risk score: 0/8 — LOW`**.

**Do not believe that about this project.** It is the toolkit's score, and three
of its five dimensions have no input here at all (no `INDEX.md`, no `access.log`,
no `/synthesis` markers). A harness that prints a green number when its inputs are
absent is the same hazard as `verify-rom-render.sh` passing on a black screen.
None of its five dimensions is even about the right failure: they measure what a
*memory system surfaces*, while this project's failure is what *prose asserts*.
The measurements below were made by hand and the method is given per item so each
can be redone.

### What was measured, before anything was changed

| dimension | measured | how |
|---|---|---|
| position of the newest finding | **98%** of the file precedes entry (q) | `awk` on header line numbers |
| entries with a state label | **0 of 43** | `grep -c '^> \*\*STATE'` → 0 before this change |
| retraction banners | present and good, **in place**, at 5 of the 5 `$0012` chain sites | read each site; §"the `$0012` diagnosis" banner sits 2 lines under the assertion |
| sites where a retracted claim is asserted with **no** marker | **0** | the guard, verified by seeding (below) |
| lines added to `RE_CITY_FREEZE.md` by this phase | +382, of which 43 are per-entry banners | `git diff --stat` |

The honest finding is that **the retraction discipline was already good** — the
file retracts in place, at the point of claim, and the previous review's "asserted
at 12 further sites, retracted at zero" described a state that `3959cb6` fixed.
The defect that remained was **discoverability**: 98% of the file predates its own
newest entry and not one of the 43 entries said what state it was in.

### What was changed

1. **An INDEX OF ENTRIES at the top**, all 43 rows, each with the entry's headline
   verbatim, an epistemic state, and a one-line reason. It states which way a
   conflict resolves: the index is right, the entry is wrong, and the entry says
   so at its own header.
2. **A per-entry `> **STATE …**` banner** under every one of the 43 headers, so the
   label is visible **at the entry** and not only in the index. A reader who
   arrives at any entry by search, anchor link or scroll now learns its state
   without going back to the top.
3. **A stated vocabulary** for the states, so `RETRACTED` here and `refuted` in the
   ledger cannot drift apart silently.

**Every state was assigned by reading that entry, not by counting retractions
against it.** Two entries are labelled `OPEN` because they raise a question that
has never been answered in either direction, and they are the only two where the
file itself had not already reached a verdict.

### What this does not fix

It does not make the 3,400-line log shorter, and it does not delete anything —
deleting a retracted claim erases the record that the error was made, which is the
one practice every retraction in this file shares a common cause with. The index
is a navigation layer over an intact history, and the log remains a log.

### The guard, re-falsified rather than trusted

`scripts/check-retracted-claims.sh` exited 0 on this tree before any of the above.
An exit code is not evidence that a guard can fail, so the `$0012` chain site from
entry (a) was reconstructed in a scratch copy **with its retraction banner
removed** — the exact shape the previous review said was unmarked:

```
$ ./scripts/check-retracted-claims.sh          # in the seeded scratch tree
  VIOLATION docs/RE_CITY_FREEZE.md:1781  R-003 (refuted) asserted without a retraction marker
  RESULT: FAIL
$ ./scripts/check-retracted-claims.sh          # in this tree
  RESULT: PASS, 0 violations
```

**One violation seeded, one raised, zero false positives.** The guard works on the
shipped text of the failure it was written for.
