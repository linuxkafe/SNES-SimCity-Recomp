# Cause claims — every one, classified

**This is the durable record. The claim graph itself is not.**

`aes/graph/island-clock.yaml` holds the machine-readable version (39 nodes, 19
edges, 6 invariants). `aes/` is gitignored **permanently and by rule** (DoD D4.3),
so that graph is absent from a fresh clone and cannot be the only place a claim
lives. This file can. It is prose on purpose: the graph is for machines that
cannot be trusted to be read, and this is for the person who has to decide whether
to believe something.

Added 2026-10-02 at Phase 4. Scope, stated so it can be argued with: a **cause
claim** is a claim that some fact about the guest, the ROM, the peer or the host
*causes the city not to simulate*. "because SDL3 enables XTEST by default" is a
claim about the build system, not about the clock, and is out of scope.

## The rule this file exists to enforce

> A claim may be **MEASURED** only if it names the instrument that observed **the
> cause itself**, not a correlate of it. Anything asserted as a cause without
> that is **INFERRED**, and if its supporting evidence has been refuted it is
> **RETRACTED** — which is not the same as its opposite being true.

Three of the four states carry a different obligation, and the third one is where
this project has hurt itself repeatedly: a refuted premise voids an inference and
establishes **nothing in its place**. Swapping one unmeasured assertion for
another of opposite sign is the mechanism behind the paired `$0B51` retraction.

## The classification

| # | claim as asserted | state | instrument / why not | where |
|---|---|---|---|---|
| C-001 | The city loads and renders | **MEASURED** | `$0B53 = 0x076C` (1900), `$0B55 = 1`, `$0B9D = 20000`, 5 samples | DoD D2.2, `clock-gate.sh` |
| C-002 | The vblank token handshake works | **MEASURED** | `$00B9 = 0001` in 5/5 | entry (q) |
| C-003 | NMI was delivered before the guest, and the guest cleared the token | **MEASURED** | ROM bytes at `0x130D` + `$B9` before/after, `d2dbcbc` | entry (p) |
| C-004 | The guest executes every frame | **MEASURED** | `$00C7` differs in 5/5 successive samples | entry (q) |
| C-005 | The `exclude_range` mask (`& 0x7FFF`) was missing, so the spinlock never yielded to the interpreter | **MEASURED** | config diff + ROM offset arithmetic, byte-verified | entry (p) |
| **C-006** | **Why does the city not simulate?** | **OPEN** | — | T086 |
| C-007 | `$0B51` is the 16-bit master city tick | **INFERRED** | deduced from the peer's trace, never measured here. **And now weakened, not strengthened:** in the peer `$0B51` rises `0000→001B` over 9 000 frames — 27 ticks — while the year stays 1900, the population stays 0 and the funds stay 20000. **Ticking is not simulating**, so the premise's own evidence does not support the conclusion it was drawn for | register §13, measurement `2026-10-02-t093-*` |
| **C-008** | **`INC.w $0B51` executes zero times** | **MEASURED** | 0 hits in the full 921-entry bank-03 dump **and** 0 AOT entries, both machines, f0-f3700. Reached by counting execution, not by inferring from `$0012` — the inference that previously stood here had a refuted premise (ledger R-020) and was withdrawn | measurement `2026-10-02-c041`, entry (t) |
| C-009 | The gate is `$0012`; `$0012` waits on `CODE_03D287` exiting, which needs bit 7 of `$0014` | **RETRACTED** | its own stated evidence: `$0012 = 0001` 5/5, `$0014 = 8000` 5/5 | ledger R-005..R-008 |
| C-010 | `$0012 = 0001`, `$0014 = 8000` at every sampled boundary | **MEASURED** | 5 WRAM samples | D003 |
| **C-011** | **`CODE_008061` never runs in a live city** | **RETRACTED as stated** | it runs: `$00:8061` is in the f3271 stream on both machines, after bank 03's `RTL`. It had never been measured; its premise was false | ledger R-019; entry (t) |
| C-012 | The vblank handshake is the cause of the city freeze | **RETRACTED** | fixing it left the city still not simulating | entry (q) |
| C-013 | Refusing to decode COP is the cause | **RETRACTED** | no word anywhere in the ROM points into `$038000–$038220` | entry 2026-09-30 (a terceira refutação) |
| C-014 | A missing mouse click is the cause | **RETRACTED** | the city starts from a script with synthetic input | entry (f) |
| C-015 | The city does not load | **RETRACTED** | `$0B53 = 0x076C` in 5/5; the runs behind it used a truncated save | ledger R-001, R-002 |
| C-016 | `$02BF` is a pause flag | **RETRACTED** | written once in the whole ROM | ledger R-010 |
| C-017 | `$0B12` is the sharpest lead | **RETRACTED** | `$00` across all 337 peer dumps; appears in no tick routine | entry 2026-10-01 |
| C-018 | `$0B51` is a free-running mod-4 counter, so its being 0 proves nothing | **RETRACTED** | **the retraction itself was the error** — this was a measurement replaced by an interpretation | D-record for the paired retraction |
| C-019 | Load average 8–10 contaminated the perf baseline | **RETRACTED** | `/proc/loadavg`'s `8/1433` is running/total | ledger R-016 |
| C-020 | fps and game time are coupled; the frame is oversubscribed | **RETRACTED** | 5 runs identical to the millisecond on the Deck | ledger R-017 |
| C-021 | The ROM→CPU translation is unverified | **RETRACTED** | pinned: `bank*0x8000 + (addr & 0x7FFF)`, 4 byte-verified labels | D007 |
| C-022 | The main loop does not run at all in a city | **RETRACTED as stated** | bank 03 executed 515,043 interpreted steps before f3300. The surviving, much narrower form is C-039 | C-038 |
| C-023 | The guest parks in the vblank spinlock | **RETRACTED** | `$00C7` advances 5/5 | D004 |
| **C-038** | **Bank 03 executes: 921 PCs, 515,043 interpreted steps over f0–f3700** | **MEASURED** | Deck-native instrumented build, `[coverage] per-bank LLE` | entry (s) |
| **C-039** | **Bank 03 executes to f3300 and zero steps from f3301** | **RETRACTED as stated** | the boundary was a 100-frame bracket. Frame-resolved: the **last frame with any bank-03 execution is f3271** (16 steps), identical on Deck and host, frame for frame | entry (t) |
| **C-039b** | **The last frame containing any bank-03 execution is f3271; none in f3272–f3700** | **MEASURED** | per-frame interpreted stream + whole-run AOT block log (bank `$03`: 18 entries, all f3270) | entry (t) |
| **C-039c** | ~~**Bank 03 executes only in `$03C63D`–`$03E57E`; `$038000`–`$03C63C` executes nothing**~~ | **RETRACTED as stated** | ledger **R-033**. Its evidence was "min and max of **the full dump**", and the full dump was the **interpreted** bank-03 dump — `CYC_WATCH` is blind to AOT. The Deck-native AOT histogram shows all 18 bank-`$03` block entries in **`$03B477`–`$03C463`**, i.e. **18 of 18 below its claimed `$03C63D` floor**. Measured on one tier, stated about both | measurement `2026-10-02-deck-aot-histogram`, entry (t) |
| **C-039d** | **Bank 03 executes in `$03B477`–`$03E57E` and nothing outside it; nothing in `$038000`–`$03B46C` or `$03C464`–`$03C63C`** | **MEASURED (Deck + host)** | narrow surviving form of C-039c, on both tiers: interpreted `$03C63D`–`$03E57E`, AOT `$03B477`–`$03C463`, union as stated | measurement `2026-10-02-deck-aot-histogram` |
| **C-040** | **The live window is confined to banks 00 and 01** | **MEASURED** | same run; reproduces entry (r)'s 813/415 | entry (s) |
| **C-041** | **Is `$03:8026` among the 921?** | **MEASURED — NO** | 0 matches in the 921-entry dump on the Deck and on the host, and 0 AOT entries at that PC. `$0B51 = 0000` at f3600 in the same runs | entry (t) |
| **C-041b** | **The peer's first `$03:8026` execution is at f3857, so C-041's f0–f3700 window contains no frame in which the reference build would have ticked once** | **MEASURED (Deck + host)** | Deck-native peer run, 9 000 frames: ticks at 3857, 4009, 4262, 4402, … cadence +152/+253/+140/+243 = 197 frames per tick, 27 in all, `$0B51` → `$001B`. Identical on the host. **C-041's "0 executions in f0–f3700" is true and does not license "the tick never executes"** — the same shape as the retracted f3301/f3271 100-frame bracket | measurement `2026-10-02-t093-peer-0B51-writer-deck` |
| **C-051** | **`$0B51` has exactly three writers, and one of them is a `STA dp,x` at `$00:8023` invisible to an operand scan** | **RETRACTED** | ledger **R-034**. `95 00` is at `$00:8024`, not `$00:8023`; the logged next-PC `$00:8025` is not that instruction's tail (`$00:8024`+2 = `$00:8026`); and decisively, one PC writes exactly as many bytes as the watched span (1→1, 4→4, 64→64, 256→256 at frame 0), which no 65816 instruction can do. The frame-0 hits are a **block memory initialisation**, not an instruction | measurement `2026-10-02-t093-peer-0B51-writer-deck` §2 |
| **C-051b** | **`$0B51` has three instruction writers — `$03:8026` `INC.w` (27×, first f3857), `$03:C77E` `STZ.w` (1×, f2985), `$03:C9E3` `STA.w` (never)** — plus one non-instruction block initialisation that covers it at f0** | **MEASURED (Deck + host)** | the surviving, narrower form of C-051. **Bound stated:** "three" is a census of absolute-mode writers found in the peer's shards and in the ROM bytes; a `dp,x` writer would not appear in it, and none has been observed | measurement `2026-10-02-t093-peer-0B51-writer-deck` §3 |
| C-042 | Bank 02 executes only during attract | **MEASURED** | per-phase brackets | entry (s) |
| C-043 | Banks 04, 06, 07 execute zero steps over f0–f3700 | **MEASURED** (bounded) | same run; the bound is in the claim | entry (s) |
| C-044 | `[interp_profile]`'s top-60 list prints nothing without `SNESRECOMP_INTERP_MS_PROF=1` | **MEASURED** | observed directly: "6055 distinct PCs" then zero lines | register §17 |
| C-045 | The Deck compiles this project natively | **MEASURED** | the `build-instr` binary that produced C-038…C-043 | entry (r) |
| C-030 | LoROM offset arithmetic, `offset = bank*0x8000 + (addr & 0x7FFF)` | **MEASURED** | header at `0x7FC0` reads `SIMCITY` (`0xFFC0` is filler) => LoROM, not HiROM as four tracked files say; the formula is the LoROM rule and is byte-verified on 4 labels. CONF-7 | D007, CONF-7 |
| C-031 | `EE 51 0B` occurs once, at `0x18026` | **MEASURED** | byte search. **Occurrence, not execution** | D007 |
| C-032 | The peer's API carries no PC or block trace | **MEASURED** | enumeration of its public surface | register §6 |
| C-033 | SRAM does not carry the city, so the cross-load script cannot work | **MEASURED** | 32,746 bytes of `0xFF`; identical md5 from two sessions | register §4 |
| C-034 | Deck guest 2.45 ms/frame, so the 65816 is not the bottleneck | **RETRACTED** | never re-measured, **and** its stated reason for distrust (the Deck cannot build) is itself refuted by C-045 | ledger R-012, R-021 |
| C-035 | `make test-rom` gives 254 distinct crc32 | **RETRACTED** | 257 measured | ledger R-013, R-022 |
| C-036 | 53 WRAM addresses change across 30,000 frames | **RETRACTED** | 34 bytes across 2,599 frames; different window | ledger R-014 |
| C-037 | `make perf` is machine-dependent | **MEASURED** | 48.38 FAIL / 51.52 PASS / 46.99 FAIL against 50, one binary | T088 |
| **C-046** | **A whole-run AOT histogram: 1,430,540 block entries f0–f3700, of which `$03` = 18** | **MEASURED (Deck-native, host-identical)** | `SNESRECOMP_AOTBLK=0-3700` on a **Deck-built** trace tier, 4 000 frames, 153.8 s: `$00` 1 100 383, `$01` 330 139, `$03` **18**, banks 02/04/05/06/07 **0**, 84 distinct PCs. A host re-run of the same build gives **every** figure identically — total, per-bank, distinct-PC count, top-12 list, per-100-frame buckets. The previous page's 1 430 539 is reproduced by **neither** machine | measurement `2026-10-02-deck-aot-histogram` |
| **C-046b** | **The 18 bank-`$03` AOT entries are 15 in f3270 and 3 in f3259, at 8 distinct PCs** | **MEASURED (Deck-native)** | f3259 `$03C42A`/`$03C430`/`$03C463`; f3270 `$03B477`×1, `$03B491`×8, `$03B49B`×1, `$03B4A0`×4, `$03B4A9`×1. **Refutes** the host-only "all 18 in f3270", which also listed multiplicities summing to 14 | measurement `2026-10-02-deck-aot-histogram` |
| **C-046c** | **"Bank 03 runs AOT nowhere" outside f0–f3700** | **OPEN** | 18 is the whole census *in the window `AOTBLK=0-3700` asks for*. Beyond f3700 is unmeasured on both machines, and the T093 page records the reference core first ticking at f3857 | — |
| **C-047** | ~~**Every AOT-side instrument in the framework is unreachable from this repository**~~ | **RETRACTED as stated** | it is reachable, on **both** machines. `-DSNESRECOMP_TRACE_BUILD=ON` adds `debug_server.c` and links pthreads; the Deck additionally needed `-idirafter` on **`CMAKE_CXX_FLAGS`** as well as `CMAKE_C_FLAGS`, which the earlier diagnosis missed (R-032, CONF-9). `scripts/deck-trace-build.sh` builds it there and asserts the `[aotblk]` count is non-zero. The surviving narrow form: unreachable **from a default build**, because `cpu_trace_block()` is a no-op without `SNESRECOMP_TRACE=1` | entry (t), measurement `2026-10-02-deck-aot-histogram` |
| **C-048** | **Instrumentation overhead changes the histogram**: 921 PCs / 515,043 steps plain, 946 / 515,337 with the trace compiled in, same binary, same script | **MEASURED** | three plain runs on two machines give 921/515,043; two traced runs give 946/515,337. Cause **OPEN** | entry (t) |
| **C-049** | **Step counts are wall-clock dependent; distinct-PC counts are not** | **MEASURED** | `$00` steps 26,856,340 vs 28,769,361 across runs of one binary (+7%), while every distinct-PC count stayed 1822/1814/542/921/956 | entry (t) |
| **C-050** | **The bank-03 task region is entered by `RTI`, not by a call** | **MEASURED (Deck)** | promoted from INFERRED, and no stack dump was needed. Transition census over f1201–f1203: bank `$03` is entered **12× from `$00:8222`** (`RTI`) and left **12× to `$00:8211`**; full cycle logged as `$03DBB3 → $008211(8B/F4/AB/AB) $008218(C2 20) $00821A(C2 10) $00821C(0A) $00821D(AA) $00821E(FC 23 82 JSR ($8223,X)) $008221(AB PLB) $008222(40 RTI) → $03DBB5`. The NMI handler `$00:930D`–`$9317` also runs through the same epilogue. The table at `$00:8223` holds `$930D` twice | measurement `2026-10-02-f3271-entry-gate` §4 |
| **C-052** | **The gate on entering bank `$03` is `$0012`, tested at `$00:804D` (`LDA $0012` / `BNE $805C` / `JSL $03D283` at `$00:8056`)** | **MEASURED (Deck)** | ROM decode of `$00:8040`–`$00:8060` plus execution counts: the loop head runs **2× in 3700 frames** and the bank-`$03` entry **1×**. It is not a per-frame loop | measurement `2026-10-02-f3271-entry-gate` §1–§2 |
| **C-053** | **The gate is closed by `$03:D2AA` (`STA $0012` = 1) at f3271, the frame bank 03 dies, one instruction before its final `RTL`** | **MEASURED (Deck)** | `$0012` has exactly **3 writes in 3700 frames** (`WLOG_ADDR`+`WLOG_STATE`): f0 `:=00` from `$00:8023`, f108 `:=E0` from `$05:9304`, **f3271 `:=1` from `$03:D2AA`**. ROM bytes at `0x1D2AA` are `85 12` = `STA $0012`; logged `A=0001` and value `01` agree with the `LDA #$0001` at `$03:D2A7`. In f3271 the loop head runs once, `BNE` is taken, `$00:8056` runs **0×**, and the loop head is never reached again through f3700 | measurement `2026-10-02-f3271-entry-gate` §3 |

## Demotions applied in this phase

*(The three rows below are the Phase-4 record and are kept as written, including
the C-008 row that the 2026-10-02 measurement has since **promoted** — see
"C-008" under "The one that remains open". The demotions were correct when made.)*

**Three, and each one moved down rather than sideways.**

1. **C-022, "the main loop does not run at all in a city"** — was carried as a
   live hypothesis in `README.md:71` in the form *"the bank-03 tick is compiled to
   native C and still does not run"*. C-038 measured 515,043 interpreted steps in
   that bank. **Demoted to RETRACTED as stated**, with the narrow surviving form
   written down next to it (C-039) so the demotion does not become a claim in
   the opposite direction.
2. **C-007, "`$0B51` is the master city tick"** — remains **INFERRED**. It was
   never MEASURED here and is not promoted by Phase 0, even though Phase 0 makes
   it more interesting: `$0B51` reads `0000` at f3600 *while* bank 03 ran 515,043
   steps. That is a reason to measure C-041, not a licence to assert C-007.
3. **C-008, "`INC.w $0B51` executes zero times"** — remained **OPEN** here, and
   was **not** promoted to RETRACTED: its premise is void, which establishes
   nothing. **SUPERSEDED 2026-10-02, correctly and for a different reason.** The
   claim is now **MEASURED** — 0 hits in the full bank-03 dump and 0 AOT entries,
   both machines — but it was settled by counting execution, not by reasoning
   from `$0012`. The row below is kept because the difference between the two
   routes is the whole point of this file.

## What is deliberately NOT done

- **No `gmif-check` target was wired.** The skill directory ships `SKILL.md`
  only — no `gmif-check.sh`, no `install-z3.sh`, no `templates/island.yaml` — and
  **`z3` is not installed** on either machine. Wiring a Makefile target at a
  script that does not exist is the same error the CI workflow carried and had
  removed in `d816afc`. The CI workflow's own commit message records the lesson:
  *YAML parses it fine, which is why it looked valid.*
- **No SAT result is claimed.** `sat_run_performed: false` remains in the graph's
  metadata. There is no Z3, so there is no solver output, and the graph's
  `logical_form` fields are unwritten fragments prepared for a future run.
- **No invariant was relaxed.** INV-6 was *added* (no node may be MEASURED on a
  bounded instrument unless the bound is in the node) and currently has 0
  violations.

## The one that remains open, and it is the whole investigation

**C-006.** Why the city does not simulate. **C-039b** says where bank 03's
execution stops — f3271 — and **C-041** says the tick instruction never executes
at all. Neither is a cause, and together they make the old question sharper
rather than answering it: the code that was believed to advance the clock is
provably not running, so the open question is no longer "why does bank 03 stop"
but **"what advances `$0B51` in the reference build, if not `$03:8026`"** — and
C-007, the premise that `$0B51` is the tick at all, remains **INFERRED**.

The gap between bank 03's last execution (f3271) and the city's arrival (≈f3378)
is now **≈107 frames**, and it is a **correlation**, recorded as one in three
places on purpose, because the next session will want to write it as a cause and
the number is sitting right there looking like evidence.

**Three demotions and one promotion in this phase**, each with the reason:

- **C-008, `INC.w $0B51` executes zero times: OPEN → MEASURED.** Not by
  inference from a WRAM sample this time, which is what the invalidated-premise
  row got wrong, but by counting executions in a dump that can see every PC in
  the bank plus a whole-run AOT block log. Zero on both tiers, both machines.
- **C-011, `CODE_008061` never runs: OPEN → RETRACTED as stated.** It runs, at
  `$00:8061`, immediately after bank 03's `RTL`. This claim had sat at OPEN for
  weeks with the note "execution never measured" — which is a statement about
  the state of the instrument, not about the state of the world.
- **C-039, bank 03 executes to f3300: MEASURED → RETRACTED as stated.** The
  bracket was too coarse; the boundary is f3271 and C-039b replaces it.
- **C-050, the task region is entered by `RTI`: new, and INFERRED.** It is the
  one new causal-shaped claim here and it is labelled inferred because the stack
  was never dumped.