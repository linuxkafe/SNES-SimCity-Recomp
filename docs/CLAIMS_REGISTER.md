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
historical correction, not a current measurement. ~~and no current per-stage
split exists for this binary (the Deck cannot build the project at all)~~
— **RETRACTED 2026-10-02:** the Deck compiles this project natively, so a current
per-stage split is obtainable; it was simply not taken before this audit. The
Deck's own figures at `9624f0e` are guest 4.502 / upload-present 1.007 /
deadline-wait 11.275 ms per frame.
> ⚠ **ENVIRONMENT-FIDELITY CAVEAT — every Deck number in this repository.**
> The Steam Deck's rootfs is **damaged in a way pacman does not report**: 503 of
> 504 glibc headers under `/usr/include` are absent from disk while `base-devel`
> reports installed, and `echo '#include <stdio.h>' | gcc -E -` fails with
> `No such file or directory`. There is no sudo and no cached glibc, so it cannot
> be repaired. The Deck build resolves libc headers from a hand-assembled prefix
> at `/home/deck/sysroot` (from `archive.archlinux.org`) with **`-idirafter`** —
> not `-isystem`, which sorts before `/usr/include` and breaks libstdc++'s
> `#include_next <stdlib.h>` — plus `SDL_UNIX_CONSOLE_BUILD=ON`,
> `OPENGL_INCLUDE_DIR`, and `OpenGL_GL_PREFERENCE=LEGACY`.
>
> **The Deck binary was compiled on the Deck, against a reconstructed header
> prefix, on a machine whose rootfs is damaged.** A performance figure measured
> under those conditions describes those conditions. Corrected 2026-10-02: earlier
> revisions of this file and of `scripts/perf-gate.sh` said the Deck *cannot*
> build the project at all and that every Deck number was a copied binary. The
> diagnosis (the missing headers) was right; the inference was not.

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

## 4. `scripts/cross-load-peer-save.sh` is a trap — **FIXED, 2026-09-30**

The script is **known impossible**: the 32 KiB battery SRAM does not carry the
city (32746 bytes of `0xFF` around `"SIM"`; identical md5 from a session at the
naming screen and from one running JAN to MAR).

**This section is out of date and so was review finding F-08, which repeated it.**
Both said the script has *"no dead-end banner, no deprecation note, and no exit
guard"* and a header that *"still argues the case for why the experiment should
work"*. Measured 2026-10-02:

```
$ sed -n '1,8p' scripts/cross-load-peer-save.sh
#!/usr/bin/env bash
# ############################################################################
# #  DEAD END - KNOWN IMPOSSIBLE. THIS SCRIPT CANNOT WORK. DO NOT RUN IT.   #
# ############################################################################
#
# The disproof, measured 2026-09-30 and recorded in docs/CLAIMS_REGISTER.md §4
```

**The banner is there, dated, at the top of the file**, with the disproof and the
reason the file was kept rather than deleted. It was fixed on 2026-09-30; this
section and `docs/review/REVIEW-2026-10-02.md` F-08 were written afterwards and
copied the stale description.

The failure is worth naming because it has now happened **twice, to the same
file**: a document described a script's state without reading the script, and a
review then filed a finding from the document. Review 2026-10-02b retracted that
finding on measurement (its R-10) — which is the only reason the error is visible
here at all. **Read the artifact.**

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

> ⚠ **RETRACTED IN FULL 2026-10-02 by §14.** The ROM→CPU mapping **is** pinned:
> HiROM `offset = bank*0x8000 + (addr & 0x7FFF)`, byte-verified on four
> independent labels. The counter-evidence below is *true and irrelevant* —
> `0x930D` is the naive offset that omits the mask. Added 2026-10-02: this
> section carried no banner of its own, so its `exclude_range 0x930D 0x9318`
> mention read as current to anyone who stopped here.

This is a finding of this review, not a retraction, and it applies to *any*
address the project has asserted from a ROM file offset.

The project's ROM→CPU address translation could not be confirmed. Two spot
checks contradict the obvious mapping:

- The bytes at **file offset `0x930D`** are
  `22 11 a0 31 06 11 84 31 26 11 af 30 05 00 b0 31 …` — a dispatch table, not
  the `STZ $00B9 / INC / LDA / BEQ / RTS` spinlock that
  `recomp/bank00.cfg:44` describes at `exclude_range 0x930D 0x9318`.
  **RETRACTED 2026-10-02 by §14 — and this is the sentence that motivated it.**
  The measurement is *true and irrelevant*: `0x930D` is the naive file offset
  that omits the HiROM `& 0x7FFF` mask, so the bytes really are something else.
  The mapping is `bank*0x8000 + (addr & 0x7FFF)` and the config really has read
  `0x130D 0x1318` since `afceeec`. A true observation carrying a false inference
  is the most expensive kind of error in this project's record, and §14 already
  says so; the marker is repeated here because **this is the bullet a reader
  quotes.**
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

> ⚠ **THIS TABLE IS A SUPERSEDED SNAPSHOT, dated 2026-10-01 at `ec4cabe`.** It is
> kept verbatim because it is the record of what was checked then. **§14 corrects
> it, and at least one row below is now false while still carrying a ✅** — the
> `exclude_range 0x930D 0x1318` row, which has read `0x130D 0x1318` since
> `afceeec`. Do not read this table as current; read §14 and
> `docs/CAUSE_CLAIMS.md`. Left unedited on purpose: a snapshot that has been
> quietly updated is no longer a snapshot, and the two states of this table are
> both evidence. *(Corrected 2026-10-02 — review finding R-09, which is what a
> 400-line self-contradiction with a tick next to the wrong row looks like from
> the outside.)*

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
| `recomp/bank00.cfg:44` contains `exclude_range 0x930D 0x9318` | ✅ T062 AC #1 satisfied — **❌ FALSE, see §14. The range has read `0x130D 0x1318` since `afceeec`; the `& 0x7FFF` mask was missing.** Fixed inline 2026-10-02, review finding R-09 |
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

## 20. The durable claim classification now lives in `docs/CAUSE_CLAIMS.md`

Phase 4 (`aes-epistemics`). The machine-readable graph is
`aes/graph/island-clock.yaml`, extended this phase from 30 to **39 nodes**, 19
edges and **6 invariants** (INV-6 added: *no node may be MEASURED on the
strength of a bounded instrument unless the bound is written into the node*).

**`aes/` is gitignored permanently, so the graph cannot be the only place a claim
lives** — it is absent from a fresh clone. `docs/CAUSE_CLAIMS.md` is the durable
copy: all 39 claims classified MEASURED / INFERRED / RETRACTED / OPEN, each with
the instrument that produced it or the reason there is none.

**Three demotions, each moved *down* rather than sideways:**

- **C-022 "the main loop does not run at all in a city"** — was live in
  `README.md:71` as *"the bank-03 tick is compiled to native C and still does not
  run"*. 515,043 interpreted steps in that bank say otherwise. **RETRACTED as
  stated**, with the narrow surviving form (C-039, *silent from f3301*) written
  beside it so the demotion does not become a claim in the opposite direction.
- **C-007 "`$0B51` is the master city tick"** — stays **INFERRED**. Phase 0 makes
  it more interesting, not more established.
- **C-008 "`INC.w $0B51` executes zero times"** — stays **OPEN**, and is **not**
  promoted to RETRACTED. A void premise establishes nothing; that is the whole
  point of the `invalidated-premise` status.

**Two things deliberately not done, and they are the discipline of this phase:**

- **No `gmif-check` target was wired.** The skill directory ships `SKILL.md` only
  — no `gmif-check.sh`, no `install-z3.sh`, no `templates/island.yaml` — and `z3`
  is not installed on either machine. Wiring a Makefile target at a script that
  does not exist is exactly the error the CI workflow carried and had removed in
  `d816afc`, whose own commit message says *"YAML parses it fine, which is why it
  looked valid."*
- **No SAT result is claimed.** `sat_run_performed: false` stands. The
  `logical_form` fields are unwritten fragments prepared for a future run, and
  the graph says so in its own metadata so the next reader does not have to
  rediscover it.

## 21. Conflicts — the durable list is `docs/CONFLICTS.md`

Phase 5 (`aes-conflict`), 2026-10-02 at `166c82b`. `make conflict-check` does
not exist here: it is `/opt/aes/Makefile:1153`, and the target body is
`@./scripts/conflict-detection.py || true` — **`|| true`, so the upstream gate
cannot fail even where it runs.** It prints `GATE: BLOCKED` and exits 0. Three of
its five classes also need `aes/shadow/access.log`, which this project has not
got, so orphan-access, causality and session-conflict detection are structurally
unmeasurable here. Everything below is hand-measured, each item with its command.

| # | severity | axis | one line |
|---|---|---|---|
| CONF-1 | HIGH | docs↔git | `RE_SCENARIO_NAV.md:145` asserts `force_lle 0x009311` in the present tense; `436b25b` removed it — **and that file is outside the guard's `SCOPE_FILES`**, so a falsified claim sits where the guard never looks |
| CONF-2 | HIGH | docs↔git | `perf-gate.sh:68-71` says the Deck cannot build this project; it can, since `9624f0e` — **and no ledger row covers it**, so the guard cannot catch it anywhere |
| CONF-3 | MEDIUM | docs↔docs | three figures for a healthy `test-rom`; **`make test-rom` measured 257**, `verify-rom-render.sh` still says 206 in its header *and in its FAIL text* |
| CONF-4 | MEDIUM | docs↔docs | this file's §9 ticks `exclude_range 0x930D 0x9318 ✅` 400 lines above §14 saying it is false |
| CONF-5 | MEDIUM | tracked↔gitignored | **eleven** tracked references into `aes/`, which D4.3 makes uncommittable by rule |
| CONF-6 | LOW | aes↔docs | seven stale `docs/*.md` references from local artefacts |
| — | verified | docs↔code | `CODE_009311` is `$03:7649` (`bank00.cfg:344`, `funcs.h:1665`) — a real trap, not a contradiction; review F-12 verified closed against the ROM bytes |

**CONF-1 and CONF-2 are the two that matter**, and they fail the same way: the
guard only guards what the ledger names, and only in the files its scope lists.
A claim that is false, unlisted, and in an unread file is invisible to every gate
in this repository. That is the gap Phase 7 closes.

`make test-rom` was run to resolve CONF-3 rather than argued about: **PASS, 257
distinct crc32, peak luma 41.751**, 800 frames presented.

## 22. Phase 7 — what was corrected, and what each correction cost

Every row names the command that now shows the corrected state. The retraction
count moved from 19 to **22** during this phase (rows R-028..R-030, the Deck
premise) and is computed by `make retraction-count`, never written by hand.

| # | was | now | verified by |
|---|---|---|---|
| README | "the bank-03 tick … still does not run" | retracted in place; the measured form (921 PCs / 515,043 steps to f3300, zero after) stated beside it | `make check-claims`, review validator R-03 |
| README | "Therefore `INC.w $0B51` executes zero times — the tick routine is never reached" | **withdrawn, not replaced**; a table of what is measured, and `$03:8026`'s membership left **OPEN** | review validator R-02 |
| README | Deck perf table `2.45` ms / `8.13` ms, "the 65816 is not the bottleneck" | all retracted with ledger references; measured Deck figures substituted; env-fidelity caveat added | `make check-claims` |
| README | no pointer to the current-position files | a table listing all six, incl. the OPEN question | review validator R-07 |
| README | "no script yet reaches a running city" | corrected; the 89% figure labelled as measured on attract, not the city | `make check-causes` |
| `RE_SCENARIO_NAV.md:145` | `force_lle 0x009311` asserted present | dated retraction marker naming `436b25b`, mechanism replaced by `exclude_range 0x130D 0x1318` | review validator R-04 |
| `scripts/perf-gate.sh` | "the Deck cannot build this project" | corrected, with the rootfs damage and the `-idirafter` prefix recorded | `make check-claims` (R-028..R-030) |
| `scripts/verify-rom-render.sh` | "206 distinct crc32 when healthy" | 257, measured, with the floor-vs-headroom caveat | review validator R-05 |
| `CLAIMS_REGISTER.md` §8, §9 | retracted sections with no banners; a ticked row its §14 calls false | banners added; the row's correction inline; the §8 bullet that a reader quotes now carries its refutation | review validator R-09 |
| `CLAIMS_REGISTER.md` §4, `REVIEW-2026-10-02.md` F-08 | "the script has no dead-end banner" | corrected: the banner exists; the *documents* were the stale part | review validator R-10 |
| `.opencode/agent/clock-hunter.md` | "`recomp/bank00.cfg` pins `force_lle 0x009311`" | retracted with the replacement named | `make check-claims` (derived scope) |
| `.opencode/agent/question-manager.md` | heading "What is established" | renamed to "What is measured", with the reason | `make check-claims` (R-009) |
| `tests/test_deterministic_replay.c` | SKIP returned 0; ctest rendered it **Passed** | returns 2, with the reason printed | review validator R-01 |
| `tests/test_deterministic_replay.c` | relative ROM path; emulator chdir'd away from it | `absolutise()` resolves every branch | review validator R-12 |

### Three defects in this phase's own new code, found by running it

Recorded because a phase that reports only its successes is a phase nobody can
trust, and because each of these was invisible to reading the code.

1. **`make perf` died silently after run 1.** Under `set -euo pipefail`, a `grep`
   that matches nothing exits 1, the command substitution inherits it, the
   assignment fails, and the gate stops with no verdict and no error. It took
   three invocations to notice. **`|| true` added, and the reason written down.**
2. **`scripts/check-cause-claims.sh` cried wolf twice before it was right.**
   `frame` matched inside "framework change", and the cue "turns out to be"
   matched "if the containment turns out to be needed". Both patterns were
   tightened, and both failures are named in the script.
3. **Its self-test initially passed for the wrong reason** — it reported its own
   *seed* as a failure of the current tree, because the seed and the live check
   shared one counter. Found because the self-test failed when it should not
   have, which is the only direction a self-test can be trusted in.

**And one that was caught before it shipped:** the guard's first self-test
**failed**, because it did not fire on the tree at `9624f0e` — it was checking
README only, while all three of its hits were in `RE_CITY_FREEZE.md`. It now
seeds three files from git history and asserts **both** directions: that it fires
there, and that it is clean here. A guard that has never been seen to fail has
not been tested, and neither has one that has never been seen to pass.

---

## 23. R-038 (T104, 2026-10-03) — a ROM decode exported into a claim about execution

**New in this session.** Measurement: `docs/measurements/2026-10-03-t104-c87x-scan-loop.md`.

C-069 reported `$03C87F` as **"not an instruction boundary"** — "the second byte
of `8D D0 F6` = `STA $F6D0`, whose instruction starts at `$03C87E`" — and,
applying C-056's rule to that fact, recorded it as **"unattributed, not as an
executed instruction"**.

**The ROM reading is correct. The conclusion about execution is wrong.**

```
$ SNESRECOMP_CYC_WATCH=03C877-03C87F   ->  [cyc] ... op=$9F / $E8 / $E0 / $D0
$03C87E: 0 occurrences in 2 676 196 [itb] lines, absent from the whole-run bank dump
$03C87F: successor $03C877 x 36339 (taken), $03C881 x 1 (not taken, f3270)
```

`$03C87F` is the loop's **back-edge branch**, `D0 F6` = `BNE $03C877`. What is
*not* an instruction boundary is `$03C87E`, and it **never executes**.

**C-069's cost figures are NOT retracted** — `$03C87C` 36 344, `$03C877` 36 344,
`$03C87B` 36 343, `$03C87F` 36 340 all reproduce to the unit, so its 86% stands.
The retraction is **one clause**, and it is recorded as one clause.

**This is the generalisation, and it is C-056's rule failing for the third time**
(`$03:D947`/`$03D94B`; R-034's "the logged next-PC does not match its length";
now this). The rule said: report a PC unattributed when the attribution matches
neither convention. It was obeyed — and obeyed *correctly*, about a fact that was
true — and still produced a false statement, because the fact was about the ROM
and the claim was about the stream:

> **A byte-boundary question about the ROM cannot be answered by, or exported
> into, a claim about execution. Where the two disagree, the fetched opcode byte
> settles it — and `SNESRECOMP_CYC_WATCH` prints it.**

**The instrument already existed and was described only by its limitation**
(`README.md` trap 1, `RE_CITY_FREEZE.md:3082` — both list it as blind to AOT and
neither mentions the `op=$%02X` field). That is **CONF-19**, and it is why this
is filed as a conflict as well as a retraction.

**Falsified in both directions, on untracked files, CONF-11 style.** Seeding the
refuted phrase into `docs/.t104seed.md` → `VIOLATION … RESULT: FAIL`; the
identical phrase in `scripts/.t104seed.sh` → `VIOLATION … RESULT: FAIL`; both
removed → `RESULT: PASS`. The `.sh` half is the **CONF-17** fix holding under a
*new* ledger row rather than under its own self-test.

**And the guard then fired on the CONF-19 paragraph written to document it**,
which is recorded in that file rather than reworded away.

Ledger as it stood after R-038: **38 rows, 30 refuted** (third consecutive +1, after R-037).


---

## 24. R-039 (T105, 2026-10-03) — my own "the game's own code" framing, retracted one commit after I wrote it

**Measurement:** `docs/measurements/2026-10-03-t105-flags-at-03c87f.md`.
**Claim raised:** **C-073**.

At `3f24098` — the commit immediately before T105 ran — `README.md` said, and I
am quoting the whole of the load-bearing part:

> *"This is the **first thing in this project that looks like a defect in the
> game's own code** rather than in our emulation. … There are **two** readings
> and **nothing here distinguishes them**: the game's own bug … or a wrong
> register state on our side … **It is NOT established which.**"*

**Refuted. It is the second reading, and more precisely than that sentence
allowed: it is not a wrong register state, it is a wrong operand fetch.**

`CPX #imm` reads a **2-byte** immediate operand while `xf=0` and advances the PC
by **3**. The word it compares against is therefore the two bytes at
`$03C87D`/`$03C87E` = **`$8DF4`**, and the loop terminates at `X == $8DF4`.

| | |
|---|---|
| `X = $00F4` (the bound the code names) | `P=$04` — **Z clear.** The compare does not fire there |
| `X = $8DF4` (the bound our decode creates) | `P=$07` — **Z and C set**, the correct result for `$8DF4` |
| `NPC` from `$03C87C` | `$03C87F` × **36 340**, one value. `E0 F4` is 2 bytes; we advance 3 |
| `NPC` from `$03C87F` | `$03C881` × **1**, `$03C877` × **36 339** — T104's 1-exit split, explained |
| N-flag census, all 36 340 rows | operand `$8DF4` predicts N set iff `X ≥ $0DF4` → **3 572** clear / **32 768** set. **Measured 3 572 / 32 768** |

**The differential is what separates "ours" from "the game's", and it is inside
the same run with the same instrument.** `$03C871 LDA #$0000` (`A9 00 00`, m=1)
reads `$0000` correctly — `A=$0000` on all 36 340 rows, where a one-byte overread
would give `$00A2`, the next byte in ROM. **`LDA #imm` right, `CPX #imm` wrong,
in one measurement.**

### The inference I pre-registered was itself wrong

T105's stated reading was: *"if [the flags are not correct], the overrun is the
game genuinely failing its own bound and we have found a game bug."*

**That does not follow, and this run is the proof.** Our emulator computes those
flags from our own operand fetch. A wrong Z is therefore evidence about **us**,
and the `LDA` control is exactly the experiment that separates the two cases.
**I registered a falsifier with two arms and the measurement took a third.**

### Why 1 599 000 checks never saw it — CONF-21

`snesrecomp/SNES_ACCURACY_BURNDOWN.md:108` records *"533 opcode variants, 0
divergences (1.599M checks) **vs interp816**"* — AOT against our own
interpreter, not against hardware. And the corpus generator states the same
wrong rule the decoder obeys (`gen_ops.py:30` *"index-immediate compares/loads
(width = X flag)"*, `:67` emitting `[op, imm, 0x00]` for `x=0`), so the planted
test ROM is wrong in the same direction and two matching mistakes read as
agreement. **Coverage was never the problem; independence is.** Not fixed here —
a fix needs an external conformance reference this repository does not have.

**Scope of the retraction: the framing only.** Nothing else in `3f24098` is
affected. C-069's cost figures, C-070/071/072, R-038, the perf figures and every
gate result in that commit stand.

**A correction to C-070's §9 while we are here:** it recorded the loop's extent
as *"X was followed to `$14FF`. Above that, nothing was watched"* and listed that
under "what this does NOT establish". T105 traces X to **`$8DF3`** over 36 340
strictly monotonic iterations. **`$14FF` was a `WLOG_ADDR` 16-bit range limit,
not a property of the loop** — an instrument ceiling presented as a bound.

Ledger: **42 rows, 34 refuted** (R-040, R-041, R-042 added by T106).
