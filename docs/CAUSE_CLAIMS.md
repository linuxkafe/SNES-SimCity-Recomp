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

## Two obligations on every row, and the second one is new

**First: a state.** `MEASURED` / `INFERRED` / `OPEN` / `RETRACTED`. A claim may be
MEASURED only if it names the instrument that observed **the cause itself**, not a
correlate of it.

**Second: the machine.** Every number must name the machine it came from, because
three separate findings in this repository were caused by a number's machine label
being missing or wrong rather than the number being wrong:

- the AOT histogram was labelled host-only for a day and a half because the Deck
  was believed unable to run the instrument at all (R-032 — a false cause, not a
  false number);
- a "host-only" T093 answer turned out to have a **wrong writer in it** that only
  a second, Deck-native run could expose (R-034);
- the peer write-watch and the `$0012` gate are both Deck-native, and the
  `$0012` measurement **contradicts a host-era table** in
  `2026-10-02-deck-interp-histogram.md` that had been left standing.

`MEASURED (Deck-native)` means the number was produced on `steamdeck`.
`MEASURED (Deck + host)` means two machines agree. `HOST-ONLY` marks a number that
may **not** close any question — the AOT histogram carried that label for one
commit and it was the right call, and this row set is the reason it now carries it.

## The classification

| # | claim as asserted | state | instrument / why not | where |
|---|---|---|---|---|
| C-001 | The city loads and renders | **MEASURED** | `$0B53 = 0x076C` (1900), `$0B55 = 1`, `$0B9D = 20000`, 5 samples | DoD D2.2, `clock-gate.sh` |
| C-002 | The vblank token handshake works | **MEASURED** | `$00B9 = 0001` in 5/5 | entry (q) |
| C-003 | NMI was delivered before the guest, and the guest cleared the token | **MEASURED** | ROM bytes at `0x130D` + `$B9` before/after, `d2dbcbc` | entry (p) |
| C-004 | The guest executes every frame | **MEASURED** | `$00C7` differs in 5/5 successive samples | entry (q) |
| C-005 | The `exclude_range` mask (`& 0x7FFF`) was missing, so the spinlock never yielded to the interpreter | **MEASURED** | config diff + ROM offset arithmetic, byte-verified | entry (p) |
| **C-006** | **Why does the city not simulate?** | **OPEN** | — | T086 |
| C-007 | `$0B51` is the 16-bit master city tick | **INFERRED** — and its reference-side behaviour is now **decoded** (C-060) | deduced from the peer's trace, never measured *in our build*. **The rationale this row used to carry is RETRACTED (R-036).** It read: *"in the peer `$0B51` rises `0000→001B` over 9 000 frames — 27 ticks — while the year stays 1900, the population stays 0 and the funds stay 20000. **Ticking is not simulating**."* The month did advance six times inside that window (f4440…f8400); the log could not show it because our own driver printed `$0B53` under the label `$0B55` (`c4923de`). Population 0 and funds 20000 are still true of the reference, at every sample, and still mean that an empty city does not grow — but they do not make the tick inert | register §13, measurement `2026-10-02-t101-*` |
| **C-059** | **The reference build's city clock runs: 28 month rolls and two year rollovers in 30 000 frames, and it never stops writing city state** | **MEASURED (Deck-native)** | clean core, cold SRAM, `scripts/d_city_kbd.script`, 30 000 frames, `EXIT=0`. City f3000 (1900 JAN); month rolls every ~780 frames; 1900→1901 at f13080, 1901→1902 at f24600; f30 000 reads 1902 MAY. **29 distinct date images.** Independently reproduced on a second route (`route.script`: f3720 → 1902 APR at f30 000). Population 0 and funds 20 000 in every sample, so "simulates" means *the tick runs*, not *an economy grows* | measurement `2026-10-02-t101-reference-simulates` |
| **C-060** | **`$0B51` = 4 × (months elapsed since the city was created) + (0…3)**, so `AND #$0003` extracts the quarter within the month | **MEASURED (Deck-native)**; the mapping onto that opcode is **INFERRED** | **Two granularities, and the second is the load-bearing one. (a) 1 344 city samples across three independent runs, zero violations. (b) Re-derived from R2's write log: **113 individual tick events, every one incrementing by exactly +1, zero deviations, strictly monotonic**, and the month rolls at **exactly the 28 events** where the counter reaches a multiple of 4 — the same 28 date images the sampling found. (b) closes the limit in (a), which is one observation per 60 frames and therefore under-determines the relation Observed at every R1 roll: f3000 `m=01 b51=0000`, f4440 `m=02 b51=0004`, f5220 `m=03 b51=0008`, f6000 `m=04 b51=000C`, f12300 `m=0C b51=002C`, f13080 `y=076D m=01 b51=0030`. **This decodes the old "27 ticks in 9 000 frames" as `6 × 4 + 3`** — six whole months and three quarters — and explains where the predicted ≈6 came from | measurement `2026-10-02-t101-reference-simulates` |
| **C-061** | **The reference has no f3259: its city state is never abandoned** | **MEASURED (Deck-native)** | its own 337 WRAM dumps, f100–f33700: **34 491 distinct addresses change** in the city window (f3800–f33700); per-100-frame churn never collapses, sitting at ~60–110 bytes with the same shape in 1900, 1901 and 1902. Change events: `$0B51` 128, `$0B53` 2, `$0B55` 32, `$0DC7` 128, `$0B9D` 0, `$0BA5` 0. **This is the reference-side counterpart of C-058**, and the contrast is the finding | measurement `2026-10-02-t101-reference-simulates` |
| **C-062** | **`$0DC7` — the accumulated tax C-057 proves our build never accumulates — is written 128 times by the reference.** The cleanest cross-build differential in the project: same field, opposite behaviour, two machines, two cores | **MEASURED (Deck-native)**, both sides | reference: 128 change events over f3800–f33700, final `$00E0`. ours: 61 writes in 9 000 frames, **0 after f3259**, never accumulated into (C-057). **The first branch of the fork is not merely taken — it is now contrasted against a build that takes the other one** | measurement `2026-10-02-t101-reference-simulates`; `2026-10-02-t100-tick-past-f3857` §8 |
| **C-063** | **The peer write-watch does not perturb the reference.** Its 30 000-frame timeline is byte-identical to the unwatched core's | **MEASURED (Deck-native)** | R1 clean vs R2 watched, same script, `cmp` over the whole log → IDENTICAL; `master_clock` and `insns` equal. **The instrument was demonstrably live** — 230 watch rows spanning f0→f29967, reproducing T093's f2985 zeroing (`A=$0007`) and its f3857/f4009/f4262/f4402 ticks — so this is an inertness result and not a silent no-op. Kills "the write-watch perturbs the reference" as an explanation for the two disagreeing runs | measurement `2026-10-02-t101-reference-simulates` |
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
| **C-041c** | ~~**`INC.w $0B51` at `$03:8026` executes 0 times over f0–f6000** — a window that contains **12** of the reference build's own ticks~~ | **RETRACTED as stated** (ledger **R-037**) | **The instrument was not watching `$03:8026`.** `SNESRECOMP_COUNT_PC=038026` → `interp816.c:323` `strtoul("038026", NULL, 0)` → base 0 auto-detects the leading `0` as **octal** and stops at `8` → **3**. The run counted executions of **PC `$000003`**, in bank `$00`. **The conclusion is re-derived by other means and stands (C-066); this measurement does not.** The `0 executions over 9 000 frames` extension in `2026-10-02-t100` §8 has the same defect. CONF-15 | measurement `2026-10-03-t102-tick-across-f13080` §1 |
| **C-052** | **The date field is not rewritten with the same value — nothing writes it.** `$0B51`–`$0B5F` takes **66 byte-writes in 6 000 frames and 0 of them after f3259** | **MEASURED (Deck-native)** | `SNESRECOMP_WLOG_ADDR="0B51:0B5F"` + `WLOG_STATE=1`, a 16-bit bus write watch that funnels through `cpu_write8/16` and so sees **both** engines — the tier blindness that retracted C-039c does not apply. 66 writes in 8 events (f0 ×15, f108 ×15, f563 ×2, f581 ×2, f582 ×15, f1205–f1685 ×10, f2333 ×1, f3259 ×6); `awk '$1+0>3259' | wc -l` → **0**. This is stronger than "34 WRAM bytes change across 2 599 frames", which is a statement about a diff; this is a statement about the **writer**. **It does not say why** | measurement `2026-10-02-t100-tick-past-f3857` §3 |
| **C-053** | **`1900 / January` is *written once*, at f3259, by a hardcoded initialiser — `$03:C63F` `A9 6C 07` `LDA #$076C` / `8D 53 0B` `STA $0B53`, `$03:C646` `A9 01 00` / `8D 55 0B` `STA $0B55` — and `$03:C77E` `9C 51 0B` `STZ.w $0B51` runs in OUR build in the same frame** | **MEASURED (Deck-native)** + ROM bytes read on the host | logged `A=076C` and `A=0001` match the ROM literals exactly; logged next-PCs `$03:C642`/`$03:C648` match the two `LDA`s. So the date is **not** a default nobody overwrote. Second half: `$03:C77E` is the **same instruction and the same register state** (`A=0007 X=003C S=1FF6`) the reference build logged when it zeroed `$0B51` — **our build reaches the writer neighbourhood the reference reaches; it reaches the `STZ` and not the `INC`.** Bound: a census of writers to this one 15-byte block, f0–f6000 | measurement `2026-10-02-t100-tick-past-f3857` §4 |
| **C-054** | **R-034's block-clear sweep reproduces in OUR build, and the loop is now named: `$00:8024` `95 00` `STA dp` / `$00:8026` `E8` `INX` / `$00:8028` `88` `DEY` / `$00:802A` `D0 FA` `BNE`** | **MEASURED (Deck-native)** | f0 event: 15 writes to `$0B51`–`$0B5F`, all `=00`, `IPC=$008023`, `X` = the **target address**, `Y` = the **descending counter** — what a block clear looks like from outside. Same idiom at `$00:94CA` `9F 00 02 7F / E8 / 88` for the f582 event. **Independent corroboration of R-034 on our side, from a different instrument on a different machine** | measurement `2026-10-02-t100-tick-past-f3857` §6 |
| **C-057** | **The tick *routine* does not run either — asked by effects, not by one PC.** `$0DC0`–`$0DD0` takes **61 writes in 9 000 frames in 4 events, and 0 after f3259**; `$0DC7` (accumulated tax, the field the routine adds to two instructions after the increment) is written only by a sweep and by the initialiser, and is **never accumulated into** | **MEASURED (Deck-native)** | `exit: RUN_FRAMES reached after 9000 frames`, `[count] pc watched: 0 executions over 9000 frames` — so §2's bound is extended f6000 → **f9000**. This resolves the fork a single-PC count leaves open: either the routine never runs, or it runs and one instruction inside it is missing. **The first branch holds.** The f3259 event decodes as one *new-city* routine — `$03:C709 LDA #$0007` → `$0DC5 = 07`, `$03:C732 STZ $0DC7`, `$03:C7B9..$03:C7E3 STZ $0DC3/$0DC9/$0DCB`, plus §4's date and `$0B51` | measurement `2026-10-02-t100-tick-past-f3857` §8 |
| **C-058** | **The city-state block is written once, at f3259, by a creation routine, and never written again** | **MEASURED (Deck-native)** · **cause OPEN** | the union of C-052 and C-057: two disjoint 15–17 byte windows, `$0B51`–`$0B5F` over 6 000 frames and `$0DC0`–`$0DD0` over 9 000, both take their last write at f3259 and **none after**. Logged `A` = `0007` and `03E8` (=1000) both appear in that frame. **This is where the evidence points and it is NOT a cause**: it names what does not write the city state, not which code would have. The f3271 `$0012` gate and the bank-`$03` death are both still measured, and **neither has been shown to be why the block is abandoned** | measurement `2026-10-02-t100-tick-past-f3857` §4, §8 |
| **C-055** | **A 96-frame periodic updater in bank `$03` runs six times and then stops: `$03:D947` `99 5C 0B` `STA $0B5C,X` + `$03:D94B` `8C 5B 0B` `STY $0B5B`** | **OPEN (cause); the boundary itself is MEASURED (Deck-native)** | writes at f1205, 1301, 1397, 1493, 1589, 1685 — **exactly +96** — with `$0B5B` counting 1→6 and each step setting the next byte of the `$0B5C`–`$0B5F` run to `01`; then nothing until one lone write at f2333 (`A=4F81`). **A second silent boundary, 1 586 frames before the f3271 gate. No cause is claimed and none is implied** — two boundaries are not a causal chain | measurement `2026-10-02-t100-tick-past-f3857` §5 |
| **C-056** | **Our host's `IPC` field is the *current* instruction PC; the reference build's `cpu.pc` is the *next* PC. Same field name, opposite convention, different machine** | **MEASURED (Deck + host)** | for every real instruction in the T100 census the ROM decode matches at the *current* PC (`$03D947` and `$03D94B` are the two instructions themselves; `$03:C642`/`$03:C648` are the next-PCs of the preceding `LDA`s). This is one more reason R-034's "the logged next-PC does not match its length" was a *convention* error and not a discovery. For the three sweep events the attribution matches **neither** convention and is reported as unattributed, **not** as a writer | measurement `2026-10-02-t100-tick-past-f3857` §7 |
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
| **C-046c** | ~~**"Bank 03 runs AOT nowhere" beyond f3700~~ | **MEASURED (Deck-native)** — **CLOSED by T102** | Deck-built trace tier `build-tr`, `SNESRECOMP_AOTBLK=3000-13080`, 14 000 frames, `EXIT=0`, `exit: RUN_FRAMES reached` (466.579 s). **10 090 136** AOT block entries in the window; per bank `$01` 9 687 074, `$00` 403 044, **`$03` 18**. The 18 are **3 at f3259** (`$03C42A`, `$03C430`, `$03C463`) and **15 at f3270** (`$03B477`, `$03B491`, `$03B49B`, `$03B4A0`, `$03B4A9`) — **C-046b's list, to the entry**. **Zero bank-`$03` AOT entries in f3271–f13080**, on the far side of the reference's first year rollover | measurement `2026-10-03-t102-tick-across-f13080` §5 |
| **C-064** | **`SNESRECOMP_COUNT_PC` parses its argument with base 0, so a bare leading-zero hex PC silently becomes a different PC.** `038026` → **3**; `009311` → **0** | **MEASURED (Deck-native + host)** | `interp816.c:323` `s_interp_pc_watch = (uint32_t)strtoul(e, NULL, 0)`. Base 0 auto-detects a leading `0` as **octal** and stops at the first non-octal digit. Falsified by **positive control**: `COUNT_PC=009311` → **0** over 300 frames, while `COUNT_PC=0x009311` → **239 617** over the same 300 frames (798.7/frame) and `0x009313` → **239 617**, agreeing to the unit as a spin body's halves should. `$00:9311` is the instrument's own documented default (`interp816.c:198`) and C-002 measures `$00C7` advancing 5/5, so a zero there is the instrument, not the world. **Not confined to one knob**: `WRAM_DUMP_HI=0B60` → 0 → `[wramdump] wrote … (0 bytes)`; `WRAM_DUMP_LO=0B40` → 0. 28 `strtol(…, 0)` sites in the tree. **The rule was already in the record** — `docs/RE_CITY_FREEZE.md:1546`, 2026-09-30: *"Qualquer resultado de `COUNT_PC` registado neste projecto sem prefixo `0x` é nulo"* — and T100 used the bare form anyway. CONF-15 | measurement `2026-10-03-t102-tick-across-f13080` §1 |
| **C-065** | **Every execution-count instrument needs a positive control on a PC known to execute, in the same build and run configuration.** A zero from a counter that has never been shown non-zero is not a measurement | **MEASURED (Deck-native)** | This rule is what found C-064. T100's declared falsifier was "does the counter print anything", and it passed — the counter printed a confident, formatted, clean `0`. The control was not in T100's falsifier list and it fired on the first smoke run. Stated as a rule because it is the generalisable half; **it is not a gate**, and nothing here should be read as claiming it is one | measurement `2026-10-03-t102-tick-across-f13080` §1.1–1.3 |
| **C-066** | **`INC.w $0B51` at `$03:8026` executes 0 times over 14 000 frames — and this zero is exhaustive over BOTH tiers for this address** | **MEASURED (Deck-native)** | `COUNT_PC=0x038026` (prefixed) + `PHASE_MS=1`, `build-instr`, `scripts/d_city.script`, 14 000 frames, `[count] pc watched: 0 executions over 14000 frames`, `exit: RUN_FRAMES reached` (439.304 s), `EXIT=0`. Reproduced in a second, independently Deck-built binary, the trace tier `build-tr` (466.579 s). **The tier gap is closed from the manifest, not argued**: exactly **one** node covers `$038026` across all banks and all m/x modes — `038000:M1X1`, `$038000`–`$03815D`, 134 instructions, disposition **`lle_only`** — and **zero** `aot_eligible` nodes cover it. A counter inside `interp816_runOpcode` is therefore exhaustive for this address. Independently reconfirmed by a **different instrument**: `$038026` appears **0 times** in the 333-PC `INTERP_DUMP_BANK=03` dump (C-041's method). This is C-041c's conclusion re-derived; **not** C-041c's measurement | measurement `2026-10-03-t102-tick-across-f13080` §2, §3, §5 |
| **C-067** | **Bank `$03` executes NOTHING AT ALL in f3272–f13080 — 0 interpreted PCs, 0 interpreted steps, 0 AOT block entries. The city is never *started*, not merely never *advanced*** | **MEASURED (Deck-native), both tiers** | Two runs of one binary with the same script. Run A: `INTERP_PROFILE_START=3000 END=13080` → `[coverage] per-bank LLE` gives **`$03` 333 PCs / 169 693 steps** over f3000–f13080. Run B: `INTERP_PROFILE_START=3272 END=13080` → `[interp_dump_bank] $03: 0 distinct PCs, 0 steps`. Subtracting, **all 333 are in f3000–f3271**. Run C (trace tier, `AOTBLK=3000-13080`): 18 bank-`$03` entries, **3 at f3259 + 15 at f3270, none after**. 9 809 frames of total bank-`$03` silence, spanning the reference's **16 month rolls** (f4440…f12300) and its **first year rollover** (f13080). **This is a boundary, not a cause**: it says where bank `$03` stops, not why. It does mean the f3271 boundary is no longer a bracket that ends before the behaviour it brackets | measurement `2026-10-03-t102-tick-across-f13080` §3.1, §4, §5 |
| **C-068** | **The city-state block is written once, at f3259, and never again — extended from f9 000 to f14 000 frames** | **MEASURED (Deck-native)** · **cause OPEN** | 16-bit bus write census (`WLOG_ADDR`+`WLOG_STATE`, both engines). `$0B51`–`$0B5F`: **66 writes in 14 000 frames**, in the 8 events C-052 listed, **none after f3259**. `$0DC0`–`$0DD0`: **61 writes in 4 events** (f0, f108, f582, f3259), **none after f3259** — C-057's count reproduces exactly and its bound extends. `$0DC7` written **4×** (f0 `00`, f108 `1A`, f582 `00`, f3259 `00`) and **never accumulated into**. **10 741 frames of silence.** Corroborated by a WRAM dump: `$0B40`–`$0DC7` is **byte-identical at f3300 and f13000** — `$0B51=0000 $0B53=076C $0B55=0001 $0B5B=0105 $0BA5=0000 $0B9D=4E20 $0DC5=0007 $0DC7=0000` | measurement `2026-10-03-t102-tick-across-f13080` §3.3, §4, §6 |
| **C-069** | **86% of bank `$03`'s interpreted cost in f3000–f3271 is four PCs, one of which is ~~not an instruction boundary~~ → **that clause is RETRACTED (R-038); the cost figures are NOT** | **MEASURED (Deck-native)** · **decode clause REFUTED by T104** | `$03C87C` 36 344 · `$03C877` 36 344 · `$03C87B` 36 343 · `$03C87F` 36 340 — **145 371 of 169 693 steps (85.7%)**; the next PC down is 244. **All four figures reproduce to the unit** (T104: R1 trace and R3 whole-run dump). **R-038 — the retraction is narrow and it is about the *stream*, not the *ROM*.** *03C87F is the second byte of `8D D0 F6`* is **true of the ROM and false of the executed stream**: `CYC_WATCH` reports the opcode byte **fetched** at `$03C87F` as **`$D0`**, and in the stream it is the loop's **only** branch and its back-edge **36 339** times (taken), not taken **once**. What is *not* an instruction boundary is **`$03C87E`**, and `$03C87E` **never executes** — **0 occurrences in 2 676 196 trace lines** and absent from the whole-run bank dump. So C-056's rule was applied faithfully to a ROM-decode fact and still produced a wrong statement about execution. `CPX #$F4` remains a *test*; the back-edge is `BNE $03C877` | measurement `2026-10-03-t104-c87x-scan-loop.md` §4 |
| **C-070** | **The `$03C87x` loop runs in f3259–f3270 only. Its last execution is **f3270**; bank `$03`'s last frame is **f3271** — so **C-039b is NOT falsified** and T104's stated falsifier F3 did not fire** | **MEASURED (Deck-native)** | `INTERP_TRACE_FRAMES=3000-3272`, **2 676 196** `[itb]` lines, `EXIT=0` + `exit: RUN_FRAMES reached` (119.516 s). Per-frame bank-`$03` interpreted steps: 90/frame f3000–f3241, **0 across f3243–f3258**, 11 842 at f3259, 12 859–12 861 f3260–f3269, 7 341 at f3270, **16 at f3271** (`$03D299`…`$03:D2B7 RTL`, nothing else), **0 from f3272**. `$03C87C` per frame: 2 361 at f3259, 3 214–3 216 f3260–f3269, **1 831 at f3270**; sum **36 344**. AOT side, `AOTBLK=3272-4200`: **861 840** entries, **0** in bank `$03`. **The loop is a *precursor* of the bank's death, not its cause, and this ticket explains neither** | measurement `2026-10-03-t104-c87x-scan-loop.md` §3 |
| **C-071** | **The back-edge is `$03C87F` (`D0 F6` = `BNE $03C877`). The loop is entered **5** times and exited **ONCE**, at f3270** | **MEASURED (Deck-native)** | Taken **36 339**, not taken **1**. **5 entries**: one fall-through from `$03C874` `LDX #$0000` at f3259 (after `$03C871` `LDA #$0000`), plus **four `RTI` resumes at `$0081A3`** (f3261, f3266, f3267, f3268) — the four interrupts taken *out of* the loop, whose departures are `$03C877/$03C87B/$03C87C → $0080B2` at 4/3/4. **1 exit ever**: f3270 → `$03C881`, and `$03C881` then runs **once in 14 000 frames**. **Nothing executes between the test and the branch**: all **36 344** successors of `$03C87C` are `[itb]` lines and **0** are `[aotblk]`, so "an AOT block clobbers the flags in between" is **measured out**, not assumed away. C-050 (`RTI` resume) observed *inside* the loop | measurement `2026-10-03-t104-c87x-scan-loop.md` §5 |
| ~~**C-072**~~ | ~~**The loop is not a scan — it is a zero-fill that runs away. `A=$0000`, one `$00` byte per iteration at `$7F6B00 + X`, and it sails past its own `CPX #$F4` bound on 2 069 measured iterations**~~ | **RETRACTED (R-041, T106)** — the **runs away** and **sails past its own bound** clauses are refuted, and both were an artefact of reading a 1-byte operand. The bound is **`CPX #$8DF4`**, the game's own 16-bit constant, and the loop honours it **exactly**: 36 340 iterations, X `$0001`->`$8DF4`, one exit, then an ordinary `JSR $B477`. **What survives:** the loop is a zero-fill of `$7F6B00 + X` with `A=0`, running f3259-f3270, over the measured extent `$6B00`-`$F8F3` | **`MEASURED (Deck-native)`** for the store extent, the iteration count and the single exit - all three measured in T104 and all three reproducing in T106; the **runs away** reading was an *inference* from a 1-byte decode of the operand and is not | measurement `2026-10-03-t104-c87x-scan-loop.md` (the extent and the count); `2026-10-03-t106-index-immediate-census.md` (the refutation) |
| **C-047** | ~~**Every AOT-side instrument in the framework is unreachable from this repository**~~ | **RETRACTED as stated** | it is reachable, on **both** machines. `-DSNESRECOMP_TRACE_BUILD=ON` adds `debug_server.c` and links pthreads; the Deck additionally needed `-idirafter` on **`CMAKE_CXX_FLAGS`** as well as `CMAKE_C_FLAGS`, which the earlier diagnosis missed (R-032, CONF-9). `scripts/deck-trace-build.sh` builds it there and asserts the `[aotblk]` count is non-zero. The surviving narrow form: unreachable **from a default build**, because `cpu_trace_block()` is a no-op without `SNESRECOMP_TRACE=1` | entry (t), measurement `2026-10-02-deck-aot-histogram` |
| **C-048** | **Instrumentation overhead changes the histogram**: 921 PCs / 515,043 steps plain, 946 / 515,337 with the trace compiled in, same binary, same script | **MEASURED** | three plain runs on two machines give 921/515,043; two traced runs give 946/515,337. Cause **OPEN** | entry (t) |
| **C-049** | **Step counts are wall-clock dependent; distinct-PC counts are not** | **MEASURED** | `$00` steps 26,856,340 vs 28,769,361 across runs of one binary (+7%), while every distinct-PC count stayed 1822/1814/542/921/956 | entry (t) |
| **C-050** | **The bank-03 task region is entered by `RTI`, not by a call** | **MEASURED (Deck)** | promoted from INFERRED, and no stack dump was needed. Transition census over f1201–f1203: bank `$03` is entered **12× from `$00:8222`** (`RTI`) and left **12× to `$00:8211`**; full cycle logged as `$03DBB3 → $008211(8B/F4/AB/AB) $008218(C2 20) $00821A(C2 10) $00821C(0A) $00821D(AA) $00821E(FC 23 82 JSR ($8223,X)) $008221(AB PLB) $008222(40 RTI) → $03DBB5`. The NMI handler `$00:930D`–`$9317` also runs through the same epilogue. The table at `$00:8223` holds `$930D` twice | measurement `2026-10-02-f3271-entry-gate` §4 |
> **⚠ DUPLICATE CLAIM ID.** `C-052` / `C-053` are each defined **twice in this table with different content** — once above (the f3259 date-write claims) and once here (the `$0012` gate claims). **Both rows are MEASURED and both are correct; the identifier is the defect.** A reader who looks up this id finds two unrelated claims. Do not cite either without also citing the row's measurement file. Renumbering would touch gitignored `aes/` cross-references, so it is **documented rather than edited** — the inconsistency is the evidence. Measured 2026-10-03.
| **C-052** | **The gate on entering bank `$03` is `$0012`, tested at `$00:804D` (`LDA $0012` / `BNE $805C` / `JSL $03D283` at `$00:8056`)** | **MEASURED (Deck)** | ROM decode of `$00:8040`–`$00:8060` plus execution counts: the loop head runs **2× in 3700 frames** and the bank-`$03` entry **1×**. It is not a per-frame loop | measurement `2026-10-02-f3271-entry-gate` §1–§2 |
> **⚠ DUPLICATE CLAIM ID.** `C-052` / `C-053` are each defined **twice in this table with different content** — once above (the f3259 date-write claims) and once here (the `$0012` gate claims). **Both rows are MEASURED and both are correct; the identifier is the defect.** A reader who looks up this id finds two unrelated claims. Do not cite either without also citing the row's measurement file. Renumbering would touch gitignored `aes/` cross-references, so it is **documented rather than edited** — the inconsistency is the evidence. Measured 2026-10-03.
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

**T101 closed the second half of that question and it closed in the opposite
direction to the way it was posed.** The reference's clock runs — C-059, 28 month
rolls and two year rollovers in 30 000 frames, Deck-native. So:

- **The comparative premise is available.** "The reference simulates and we do
  not" was, until f30 000 was measured, an assumption this investigation was
  framed against. It is now a measurement, and it holds. **Nothing here retracts
  the premise.**
- **`$03:8026` *is* what advances `$0B51` in the reference**, and C-060 says
  exactly how much: four increments per month, plus the quarter. The old worry —
  that `$0B51` rises without the city ageing — is answered, and it was raised by
  a broken column in our own driver.
- **Our failure is narrower and sharper than "we do not simulate".** It is not
  that our tick runs and the date does not follow; **the tick never runs at
  all** (C-008, C-041, C-041c), **the routine that would accumulate `$0DC7`
  never runs** (C-057), and **the city-state block is written once at f3259 and
  never again** (C-058) — while the reference does all three, indefinitely
  (C-061, C-062). The two builds are separated **at the instruction**, not at
  the symptom.
- **What remains OPEN is unchanged in kind and smaller in scope**: what causes
  bank `$03` to stop at f3271, and what closes the `$0012` gate in the same
  frame. No cause is asserted.

### T102 narrowed it further, and retracted the measurement that narrowed it

**C-041c and T100's count are RETRACTED (R-037).** `SNESRECOMP_COUNT_PC=038026`
watched **PC `$000003`**: `interp816.c:323` parses with base 0, a leading `0`
means octal, and parsing stops at `8`. The falsifier that caught it was a
**positive control** on `$00:9311`, the instrument's own documented default and
an address with three independent proofs that it executes — it read **0** with
the bare form and **239 617 over 300 frames** with the `0x` prefix (C-064,
C-065). **The trap was already written down on 2026-09-30** at
`docs/RE_CITY_FREEZE.md:1546`, in the exact words *"any `COUNT_PC` result
recorded in this project without the `0x` prefix is null"*, and T100 used the
bare form anyway. The parse is not the failure; **a rule in the record that did
not reach the next session** is.

**The conclusion survives, re-derived three ways** (C-066): the prefixed counter
over 14 000 frames; the manifest, where exactly one node covers `$038026` and it
is `lle_only` with **zero** `aot_eligible` nodes over it, so an
interpreter-tier counter is exhaustive there; and C-041's *enumeration*, which a
parse bug cannot corrupt and which finds `$038026` absent from this run's
333-PC bank-`$03` dump. **C-041 and C-008 were never exposed to this and stand.**

**And the boundary is now much bigger than a bank-`$03` gap** (C-067): bank
`$03` executes **nothing at all** in f3272–f13080 — 0 interpreted PCs, 0
interpreted steps, 0 AOT block entries — across **9 809 frames** containing the
reference's **sixteen month rolls** and its **first year rollover**. So the
evidence no longer says "the tick is absent". It says **the city is never
started**: a bank that never executes again cannot be advancing anything, and
in this window was not being started either.

**Where the cost actually is, and it is a start-shaped question** (C-069): 86%
of bank `$03`'s interpreted cost in f3000–f3271 sits in four PCs around a loop
(`STA $7F6B00,X` / `INX` / `CPX #$F4`).

> **RETRACTED (R-038) — and this paragraph carried the refuted clause unmarked
> until now.** It previously read, as fact: *"**scan loop** … that stops when
> everything else does. One of the four, `$03C87F`, is **not an instruction
> boundary** and is reported unattributed. What the loop scans, what branches
> back, and what terminates it are **unmeasured**."*
> **All four halves are refuted** — the clause, because `CYC_WATCH` reports the
> opcode byte **fetched** at `$03C87F` as **`$D0`** = `BNE $C877` and it is the
> loop's **only** branch, taken 36 339 times; and "unmeasured", because T104
> measured all three questions. What is *not* an instruction boundary is
> **`$03C87E`**, and `$03C87E` **never executes** (0 of 2 676 196 trace lines).
> **C-069's four cost figures are NOT retracted** and its 86% stands.
> See `docs/CONFLICTS.md` **CONF-20** for why both evidence gates passed it.

**T104's answers** (all Deck-native, `EXIT=0` + `exit: RUN_FRAMES reached`,
every run carrying the `COUNT_PC=0x009311` positive control):

| question | answer |
|---|---|
| last execution | **f3270**, running **f3259–f3270 only**; bank `$03`'s last frame is **f3271** (16 steps, epilogue ending `RTL` at `$03:D2B7`). **C-039b is NOT falsified — the loop is a *precursor* of the bank's death, not its cause, and T104 explains neither** |
| back-edge | **`$03C87F` `BNE $03C877`**, a real executed instruction. Taken **36 339**, not taken **1**. 5 entries, **1 exit ever** |
| what it scans | **nothing.** It is a **zero-fill**: `A=$0000`, one `$00` byte per iteration at **`$7F6B00 + X`**, X **measured** `$0000 → $04FF` and on to `$14FF`, crossing f3259→f3260 |

**It sails past its own `CPX #$F4` bound: 2 069 measured stores at `X > $00F4`.**
`LDX #$0000` runs once and `INX` is the loop's only X writer, so X climbs
monotonically and cannot jump over `$00F4`. **[MEASURED, Deck-native, C-072.]**

### ~~C-073~~ — RETRACTED as stated (R-040, T106): there is no defect

> **What this row claimed, kept as history.** *"### C-073 — `CPX #imm` reads a
> 2-byte operand and advances the PC by 3 … `$E0` (`CPX #imm`) in our
> interpreter consumes a 2-byte immediate operand while `xf=0` and advances the
> PC by 3, so the word compared against is the two bytes at
> `$03C87D`/`$03C87E` = `$8DF4`. The `$03C87x` loop therefore terminates at
> `X == $8DF4`, not `$00F4`. **This is a defect in OUR emulation, not in the
> game's code**"* (T105, `32e2bd4`).

**REFUTED by T106, and the refutation is not subtle: the 3-byte reading is
published 65816 behaviour.** `LDX/LDY/CPX/CPY #imm` are **2 bytes at `x=1` and
3 bytes at `x=0`**; the accumulator/ALU immediate group follows **`m`** instead.
Three independent published sources agree (snesdev's 65816 opcode tables, which
cite the WDC datasheet, Eyes & Lichty and Near's higan disassembler; and Chris
Wright's 65816 Programming Primer). `interp816_adrImm` implements it correctly,
and so does the differential suite's corpus generator — **so CONF-21's specific
claim that they share a mistake is itself refuted (R-042).**

**The measurements in this row were correct and all three reproduce to the unit**
(`NPC` x36 340, `P=$04` x3 571 / `$84` x32 768 / `$07` x1, 36 340 distinct
monotonic `X`). **The attribution was the error**, and it was an inference: the
row reasoned from "our decoder advanced by 3" to "our decoder is wrong", which
holds only if the decoder is the authority on its own correctness.
**[MEASURED (Deck-native), T106.]**

**What replaces it — the loop was always correct.** `$03C87C` is `CPX #$8DF4`,
the game's own 16-bit bound. The loop zeroes `$7F6B00 + X` for
`X = $0000…$8DF3` — `g_ram` `$6B00`-`$F8F3`, **36 340 bytes** — and stops exactly
there, on one exit, via an ordinary `JSR $B477`. **There is no overrun, and
"what the ~36 KB of zeros destroyed" is answered: it is the game's own bounded
clear at city creation.** Whether that clear is what the game wants is **not
measured and not claimed.**

**Blast radius, now measured rather than open: C-074.**

### C-074 — the `$A0/$A2/$C0/$E0` census: 245 sites, 0.3141% of interpreted steps, and the x flag predicts the width

| | |
|---|---|
| **Claim** | **In 14 000 frames of our build, 245 distinct executed interpreted PCs carry a ROM opcode in `{$A0,$A2,$C0,$E0}`, taking 344 060 steps = 0.3141% of 109 533 634 interpreted steps. 195 of those sites (287 062 steps) are read with a 2-byte operand and 49 (55 566 steps) with a 1-byte operand; 1 is ambiguous. Both branches are exercised, and the logged `x` bit predicts which** |
| **State** | **MEASURED (Deck-native)** — five runs, one per executing bank, all `EXIT=0` + `RUN_FRAMES reached`, each with its own `COUNT_PC=0x009311` positive control (**27 019 166** / 14 000 f = **1929.9/f**, byte-identical to the known-good value in all five) |
| **Instrument** | `INTERP_DUMP_BANK` for `$00/$01/$02/$03/$05` — an **enumeration**, so a `strtoul` parse bug cannot corrupt it (C-041's advantage). The opcode filter is applied offline against the ROM file. `CYC_WATCH` for the flag reads |
| **The partition's positive control** | the partition is *derived* from which landing address was fetched, so it was checked against **direct flag reads on both sides**: `$03C874 op=$A2 P=$06` (**x=0**, 3 bytes), `$008D28 op=$A2 P=$64` (**x=0**, 3 bytes), `$01C826 op=$A0 P=$30` (**x=1**, 2 bytes), `$01C828 op=$A2 P=$30` (**x=1**, 2 bytes). `P` is `interp816_getFlags()` (`interp816.c:403-413`), `N V m x D I Z C`, so **x is bit 4**. **`$008D28` is logged `m=1, x=0`** — the two flags are independent, which is why the two immediate groups need two different flags |
| **The one ambiguous site** | **`$03DCCF`** (`A2 00 5A B9`), 1 432 steps, sitting in what reads as a byte table (`A0 00 | A2 00 | 5A | B9 5C`). **Left ambiguous, not resolved by assumption** |
| **ROM-internal corroboration, no length table used** | **90 of the 195** x=0 sites have **`$00` at PC+2** — a 1-byte reading interposes a **`BRK`** there, 211 859 steps, including the busiest site in the census (`$008D49`, 82 056 steps) and four 12–14k-step loop counters. `$03C876` executes **0** times while `$03C877` executes **36 344**. Under a 1-byte reading of `$03C87C` there is **no branch instruction anywhere in `$03C877`-`$03C884`**, and `$03C877` is re-entered 36 339 times. `$01C824 E2 30` = `SEP #$30` sits immediately before two index-immediates — a flag setup emitted *because* the width depends on it |
| **NOT claimed** | **Not** a cause for C-006, which stays **OPEN**. **Not** an audit of any other opcode — the census bounds this width rule's reach and says nothing about the rest of the interpreter. **Not** a repair of CONF-21, whose structural point (a differential suite comparing our AOT against our own `interp816` cannot see a shared defect) is untouched and still true |
| **Measurement** | `2026-10-03-t106-index-immediate-census.md` |

### The experiment that refuted the proposed fix — and the one I mis-specified first

**The predicted post-fix shape — 245 iterations, correct Z/C at `X=$00F4` — was
pre-registered and run. It never appeared: the loop executed 0 times.** Built on
the Deck as `build-x` with the index-immediate group forced to 8-bit, 3 300
frames, `EXIT=0`, `RUN_FRAMES reached`, positive control present:

| | baseline | proposed fix |
|---|---|---|
| banks that execute at all | `$00 $01 $02 $03 $05` | **`$00` only** |
| distinct PCs, bank `$00` | **1 822** | **16 778** |
| bank `$03` | 921 PCs / 515 043 steps | **0 / 0** |
| `$03C87x` loop | **36 344** iterations | **0 — not 245** |
| `COUNT_PC=0x009311`, 3 300 f | **6 626 029** (2007.9/f) | **13 631 085** (4130.6/f) |

**The prediction was preempted, not refuted:** the change is not local and
destroys the trajectory before the loop. What it refutes is the *premise* — that
the width rule is the bug.

**And my first attempt at the experiment was wrong, and its own mechanism check
refuted my explanation of it.** Version 1 patched `if (1)`, forcing **every**
immediate to 8-bit including the accumulator group — so it was not the proposed
fix at all. It collapsed the run spectacularly; I then predicted control would
land on operand bytes and measured **0 of 16 749** newly-executed PCs doing so.
**My causal story was wrong and the reason was my own mis-specification.**
Version 2 fixed the specification and produced **identical** numbers on every
figure; **I did not establish why, and assert no reason.** The available
candidate — that the accumulator group was never reached with `m=0` on the
trajectory v1 diverged onto — is **UNVERIFIED**.

### A limit on CONF-19 / R-038, stated as a limit and not as a retraction

R-038 retracted C-069's clause *"`$03C87F` is the second byte of `8D D0 F6`,
therefore not an instruction boundary"* on the evidence that `CYC_WATCH` reports
the opcode **fetched** at `$03C87F` as `$D0`. **That evidence cannot bear the
weight.** The PC fetched at is produced by **our own decoder**; a decoder that
mis-lands on a boundary will make `CYC_WATCH` report the byte there with total
confidence. **A fetched opcode byte proves an instruction executed at that PC; it
does not prove the PC was an instruction boundary in the game's intent.**

What stands: of *our build's stream*, `$03C87F` **is** the loop's only branch and
its real back-edge, taken 36 339 times. What does not stand is the reason. **No
ledger row is added, because R-038's conclusion about our stream is not shown to
be wrong — only its stated reason.**

### R-039 — RETRACTED (R-041): and the defect is in neither

> **Retracted 2026-10-03, one commit after it was written** (`README.md` at
> `3f24098`). It read: *"This is the **first thing in this project that looks like
> a defect in the game's own code** rather than in our emulation"* … *"There are
> **two** readings and **nothing here distinguishes them** … **It is NOT
> established which.**"*

**⚠ RETRACTED by R-041 (T106). R-039 correctly retired an ambiguous framing and
incorrectly supplied a replacement.** What follows is R-039's body, kept so the
correction has its original attached — **and note that its own reasoning is what
R-041 refutes.**

**Refuted by C-073 — and C-073 is itself now refuted (R-040).** R-039 said: *"It
is the second reading, and more precisely than the sentence allowed: it is not 'a
wrong register state' but a wrong **operand fetch**."* **The operand fetch was
not wrong.** `$E0 F4 8D` is a 3-byte `CPX #$8DF4` at `x=0`, which is what the
65816 does and what our decoder implements. **The third answer is that there is
no defect at all**, which is the one reading neither R-039 nor C-073 had
available.

**The inference in the pre-registered framing was itself wrong, twice.** T105's
stated reading was *"if [the flags are not correct], the overrun is the game
genuinely failing its own bound and we have found a game bug."* R-039 answered
*"that does not follow — a wrong Z is evidence about **us**"*. **T106 shows the
question was malformed underneath both:** the Z was never wrong. At `X=$8DF4`,
`CPX` sets Z because `X` **equals the operand**, and C set because `X >= operand`.
The flags were correct for the operand the ROM supplies.

**And the sentence "Three sessions framed this loop as a mystery in the game's
code" is itself now wrong.** The mystery was ours, but not in the way it was
framed: not a defect, but a **1-byte reading of a 2-byte operand** that three
sessions had written down as the finding. **Nothing about this loop is a defect
in either direction, and "the ~36 KB written" is the game's own bounded clear.**
See C-074.

> **RETRACTED (R-039, then R-041) — this is the paragraph the retraction replaced,
> kept so the correction has its original attached.** It read: *"**This is the
> first anomaly in this project that looks like a defect in the *game's* code
> rather than in our emulation** — and **it is NOT established which it is.**
> Two readings remain open and nothing here separates them … **That is T105, and
> no cause is claimed until it is measured.**"* **T105 was measured and its first
> half was false; T106 then measured the question underneath it and found
> **neither** — see C-074, R-040 and R-041.** Measurement:
> `docs/measurements/2026-10-03-t104-c87x-scan-loop.md` (the framing),
> `docs/measurements/2026-10-03-t105-flags-at-03c87f.md` (the answer that was
> wrong), `docs/measurements/2026-10-03-t106-index-immediate-census.md` (the
> answer that holds); claims **C-070, C-071, C-072**, ~~**C-073**~~ and
> **C-074**.

### `make clock`'s criterion is justified on its own terms

The gate requires **≥2 distinct date images** after the city frame. It used to
be defensible only as "a floor somebody chose". It is now **calibrated against a
measured behaviour of the reference implementation on the machine this project
ships to**:

| | distinct date images |
|---|---|
| `make clock` requires | **≥ 2** |
| reference, 30 000 frames, Deck-native | **29** |
| our build | **1** |

The criterion is **not weakened, and must not be.** It is not inherited from an
assumption about the reference — the assumption has been replaced by a
measurement, and the measurement is comfortably above the floor. If anything the
number now argues the floor is *generous*; that is an observation, not a reason
to raise it.

**What our build is actually being asked to do**, stated plainly: reach a live
city (it does, f3259), and then keep running that city's tick indefinitely, the
way the reference does for at least 33 700 frames. The gap is not "our clock is
slower" or "our date field is written once"; it is that after f3259 the code
which performs the per-tick update does not execute in our build, and the exact
instruction is known.

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
---

## The C-006 lead, restated after T108 — a 17-present event, and where it points

**Measured 2026-10-03, Deck-native, `scripts/d_city.script`, 5 000 frames,
`EXIT=0` / `exit: RUN_FRAMES reached`, positive control `COUNT_PC=0x009311` →
9 855 088 = 1971.0/frame.** Transcript:
`docs/measurements/2026-10-03-t108-present-crc-timeline.md`.

**C-006 is not answered by this and no cause is asserted.** What it does is
replace a 3 000-frame diff with a frame-resolved boundary, which is a strictly
better-posed question.

| | measured, Deck-native |
|---|---|
| presents | 5 000, frames 1…5000, one present per frame |
| picture-state changes | **206** (165 distinct `crc32` values in order of first appearance) |
| **last change** | **f3381** |
| **identical presents after it** | **1 619** |
| the burst | a 106-frame gap, then **17 consecutive changes, f3365 → f3381**, then silence |
| `make clock`'s own date crop | last change at **f3378** — **inside** the burst |
| C-055's `+96` updater, picture side | f1302, 1307, 1398, 1403, 1494, 1499, 1590, 1595, 1686, 1691 — period **exactly 96**, five times. C-055's **writer** stops at f1685; the **picture** stops showing it at f1691 — **two instruments, within 6 frames** |
| the dense run | f2637–f3259, **ends on f3259** — the one and only write of the city-state block (C-052/C-053/C-058). **Animation and initialiser stop on the same frame** |

**Two boundaries, not one.** Bank `$03` stops executing at **f3271** (C-039b);
the city-state block stops being written at **f3259** (C-068); the *picture*
stops at **f3381** (here). **Whether those three are one boundary is NOT
measured, and this file does not assert that they are.** What is measured is
that they are **three different frames**, 12 and 110 apart.

**A correction that removes a leg from T107's conclusion.** T107 held the
rendered date is not read live from `$0B53` on two grounds. **The static-picture
ground is void**: a display path reading `$0B53` live would also produce an
unchanging picture, **because `$0B53` itself never changes** (C-068). **The poke
ground stands and is decisive**: `$0B53`/`$0B54` read `$0FA0` continuously
f4260→f4899 while the picture does not move — **a change to the source with no
change to the display**. **The conclusion survives on one leg; the other should
not be carried.**

**The next measurement, and it is the one this file now points at: 17 pictures,
not another diff.** What is the city's last visible act before the framebuffer
goes bit-identical for 1 619 presents — the date, the money, a sprite, the RCI
toolbar — and **where the rendered date text comes from**. **The second half is
the largest unexamined thing in this project**, because `make clock` reads the
date off a **screen crop** (`clock-gate.sh`, x 55–125 / y 2–21 of the 336×224
framebuffer) while this file's `$0B53` row is a **memory read**, and **the
relationship between the two has never been examined.**

**If the rendered date is not sourced from `$0B53`, then `make clock` is
measuring a display path rather than the simulation** — which does **not**
weaken the gate, it *re-scopes* what the gate proves. **That is a claim to be
measured, not asserted, and nothing in this file asserts it.**
