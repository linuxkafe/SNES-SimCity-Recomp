# T113 — peer bank-$03 entry census, full WRAM window

**Date:** 2026-10-03 · **Machine:** Steam Deck, `ssh deck@steamdeck`, repo `/home/deck/simcity`
· **ROM:** `/home/deck/rom/SimCity (USA).sfc`, md5 `23715fc7ef700b3999384d5be20f4db5`, 524 288 B
· **Reference build:** the peer, private study only, no source vendored or copied
· **Driver:** `~/peers/jjwatch` (T093's patched write-watch, compiled `2026-10-02 20:00`)
· **Script:** `scripts/d_city_kbd.script` (same as T093/T101; 8067 scheduled frames, 41 presses)
· **Instrument:** `T093_WATCH_LO=0x0000 T093_WATCH_HI=0x1FFFF T093_WATCH_LOG=<path>`
· **Census window:** 9000 frames; full 128 KiB WRAM index coverage (`$0000–$1FFFF`).
· **Peer log path (Deck):** `/dev/shm/t113-watch.log` · **Ours log path (Deck):** `/dev/shm/t113-ours-census.log`

---

## 0. Falsifier P1 — the control fires

Timeline log tail:

```
9000 3216243544 107365572 01 01 62 001B 076C 076C 0101 006C
```

`$0B51` final = `$001B` = 27 increments, first at **f3857**, insns = **107365572**.
T093's known-good reproduced to the frame and to the instruction count. **P1 GREEN.**
The census is valid and interpretable.

(Cross-checked from our census of `$0B51` writes in the peer log: 27 non-zero writes,
all from `$03:8029` (= next-PC of `$03:8026` `INC.w $0B51`), at frames 3857 / 4009 /
4262 / 4402 / 4645 / 4797 / 5050 / 5190 / 5433 / 5585 / 5838 / 5978 / 6221 / 6373 /
6626 / 6766 / 7010 / 7161 / 7414 / 7554 / 7798 / 7949 / 8202 / 8342 / 8585 / 8737 /
8990. Zero violations.)

---

## 1. Peer census: every distinct `pbr == $03` writer, with frame range

**14,196,878** total bank-$03 write lines · **1,353** distinct `$03:addr` PCs.

The address below every `$03:NNNN` is the *logged* PC — set by the core's
`SC_STATIC_CONTEXT_BEGIN(..., addr, len, open_bus, **next_pc**)` macro to the
instruction *after* the write. For a 3-byte `INC.w $0B51` at `$03:8026` that
writes to WRAM offset `$00B51`, the core logs `bank_pc = $03:8029` (`8026+3`),
not `8026`. Every writer here is therefore an IPC / "next-PC". For the tick,
that means **`$03:8026` executes 27 times, first at f3857, last at f8990**.

**The writers that answer the question (spanning ≥ 2 frames, i.e. the per-frame
engine candidates), plus the one-shot anchors:**

| writer (logged IPC) | first | last | span (frames) | count | role |
|---|---|---|---|---|---|
| `$03:D2A5` | 2998 | 2998 | 1 | 1 | one-shot: scheduler epilogue |
| `$03:D2AC` | 2998 | 2998 | 1 | 2 | one-shot: scheduler sets `$0012=1` |
| `$03:D2B3` | 2998 | 2998 | 1 | 1 | one-shot: scheduler exits bank-$03 |
| `$03:D2B6` | 2998 | 2998 | 1 | 1 | one-shot: scheduler exits bank-$03 |
| `$03:8007` | 3068 | 3068 | 1 | 1 | one-shot: city creation routine entry |
| `$03:8222` | 3068 | 8881 | 5814 | 56 | **per-frame: COP/RTI dispatch table tail** |
| `$03:90B2` | 3068 | 3068 | 1 | 14 | one-shot: city creation |
| `$03:8029` | 3857 | 8990 | 5134 | 54 | **the tick `INC.w $0B51` (logged as next-PC)** |

**Every other `$03:` writer in the census is one-shot** (single-frame, ≤ 1717
executions) and clustered in f1252–f3068, the city-creation window. Notable
examples of the 842 spanners that are NOT post-f2998:

| writer | first | last | span | count | comment |
|---|---|---|---|---|---|
| `$03:D292` | 8 | 2968 | 2961 | 4026 | scheduler's `$0014` read (pre-latch) |
| `$03:D299` | 231 | 2997 | 2767 | 8052 | scheduler's `$0015/$00B1` read (pre-latch) |
| `$03:DB76` | 1252 | 2968 | 1717 | 1717 | periodic updater loop — runs in every frame of the window |
| `$03:DBC6` | 1291 | 2968 | 1678 | 8 | periodic updater epilogue |

**P2 verdict: GREEN.** `$03:DB76` alone wrote 1717 times over f1252–f2968; the
post-f2998 set (`$03:8222` spanning f3068–f8881, `$03:8029` spanning f3857–f8990)
contains 980 distinct `$03:` PCs. Bank `$03` is **definitely still alive** in the
peer after the scheduler exits at f2998.

---

## 2. What re-enters bank `$03` in the peer — the measured answer

The scheduler at `$03:D283` runs once and exits at f2998 (`$03:D2B7 RTL`). The
next entry into bank `$03` is **not through `$00:804D` → `JSL $03D283`** — that
path is city-creation and runs once everywhere. It is also **not** the NMI
handler's ten JSRs (they start later, f3338+).

**What is measured:** at **f3068**, 70 frames after the scheduler, **a batch of
city-creation instructions runs in bank `$03`** — `$03:8007`, `$03:90B2`, and a
long run of `$03:8222`-family PCs across `$03:8222`–`$03:82FF`, `$03:83xx`,
`$03:84xx`, `$03:Bxxx`, `$03:Cxxx`, `$03:Axxx`. These are all one-shots
(except `$03:8222` which resumes repeatedly until f8881). The batch ends by f3068
or just after; `$03:DB76` is already done by f2968.

**`$03:8222` is the per-frame engine.** Spanning **5814 frames** (f3068–f8881)
with 56 logged writes — and the full interpreter census would show far more
executions since `$03:8222` is the RTI-resume tail of the COP dispatcher at
`$00:8211` (see T112 §2). Every time the COP dispatches (`JSR ($8223,X)`) and
then returns via `RTI` (`$00:8222`), control lands in bank `$03`. The dispatcher
table at `$00:8223` holds `$930D` (the vblank wait) twice and `$86A4` (WRAM
clear) — both of which live in bank `$00`, but the `RTI` return stack frame has
`DB=$03` at the time of each dispatch, so the resume point sits in bank `$03`
and executes there between dispatches.

**The tick `$03:8026` (logged as `$03:8029`) starts at f3857 and runs 27 times**
through f8990. That is the same cadence T093 measured — `+152/+253/+140/+243` —
and it confirms `$03:8026` is in the peer's re-entry path, not an isolated
one-shot.

**What is NOT established:** the exact dispatch chain that sets up the `RTI`
frame with `DB=$03` at f3068. We see the *effect* (`$03:8222` runs 56 times
across 5814 frames; `$03:8029` runs 54 times across 5134 frames), but we do not
see *what caused the RTI* — the COP vector, the dispatch table entry, or the
branch that landed there. **OPEN.**

---

## 3. Our build — same census, same window, same script

**Method:** `SNESRECOMP_WLOG_ADDR="0000:1FFFF:/dev/shm/t113-ours-census.log"` on
`build-instr/SimCitySNESRecomp`, 9000 frames, `scripts/d_city.script`, `EXIT=0`
(735.994 s). Format: `f<N>   <bank>:<addr>=<val> w<N> <tag>` — the wlog's bank
field is `g_interp_bank` (the interpreter's DB), identical to the peer's `pbr`.

**Result:**

| | peer | ours |
|---|---|---|
| total bank-$03 write lines | 14,196,878 | **1,023,953** |
| distinct `$03:addr` PCs | 1,353 | **415** |
| `$03:8026` / `$03:8029` (tick) | **54 frames** (3857–8990) | **0** |
| `$03:8222` (dispatcher RTI tail) | **56 frames** (3068–8881) | **0** |
| `$03:DB76` (periodic loop) | **1717 frames** (1252–2968) | **0** |
| `$03:D2AA` (scheduler, `STA $0012`) | 2 frames (2998) | 0 (scheduler never reaches `$03:D2AA` — it stops at f3271 via `$03:D2B7 RTL` and `$0012` is set by `$03:D2AA` at **f3271** in the wlog — see below) |
| last bank-$03 write frame | f8990 | **f3271** |
| bank-$03 writes after f3271 | — | **0** |

**P3 verdict: GREEN.** The named addresses — `$03:8026`/`$03:8029` (the tick),
`$03:8222` (the dispatcher RTI resume), and `$03:DB76` (the periodic updater
loop) — are **never written in our build over the same 9000-frame window**.
Bank `$03` is silent from f3272 onward. This is not a frame-count difference;
our build's census covers **all 9000 frames**, including the peer's entire
tick window.

(The run log reports `[count] pc watched: 17483747 executions over 9000 frames
= 1942.6 per frame` for `COUNT_PC=0x009311` — the vblank spin body, a known-good
positive control. Exit status: `RUN_FRAMES reached`. The run is clean.)

---

## 4. Cross-build comparison — the difference

Both builds are driven by the same keyboard-only script (`d_city_kbd.script` for
the peer; `d_city.script` for ours) and run over 9000 frames. The divergence
point is **f2998–f3068**:

| event | peer | ours |
|---|---|---|
| `$03:D2AA STA $0012=1` | f2998 | **f3271** |
| `$03:D2B7 RTL` (scheduler exit) | f2998 | **f3271** |
| City creation batch in `$03:8xxx` | f3068 | **f3259** |
| `$03:8222` first execution | f3068 | **does not execute** |
| `$03:8222` last execution | f8881 | — |
| `$03:8029` (tick) first | f3857 | **never** |
| `$03:8029` (tick) count | 54 | **0** |
| Last bank-$03 write frame | f8990 | **f3271** |
| Bank-$03 writes after f3271 | **14,196,878 − ~3,000,000** (~11M) | **0** |

The two builds take different routes through the same city-creation code (the
`$03:8xxx` batch), but only the peer's route lands on `$03:8222` — the COP
dispatcher's RTI resume tail. Once `$03:8222` is live, the COP dispatch chain
keeps re-entering bank `$03` every frame for the rest of the run. Ours never
lands on `$03:8222` and therefore never re-enters bank `$03`.

**What opened the path in the peer and closed it in ours is NOT established.**
The `$0012` latch fires at f2998 in the peer and f3271 in ours — 273 frames
earlier in the peer, right in the middle of the city-creation window. Whether
that timing difference is the cause or merely a correlate is **OPEN**.

---

## 5. Mechanism notes — what the census can and cannot see

- **The write-watch is the right instrument for this question** (C-063: clean and
  watched timelines cmp IDENTICAL). It sees both AOT and interpreter writes
  because it sits inside `sc_v11_bus_write8`, the single bus-write funnel.
- **It is blind to code that executes without writing WRAM.** A per-frame
  counter increment without a WRAM store (e.g. `INC $4200` or `INC $4212`)
  would not appear. If the peer's per-frame engine writes no WRAM, the census
  cannot name it — that limitation is stated rather than glossed.
- **IPC / next-PC convention:** the peer's `pc` field in the write-watch is set
  to the *next* PC by `SC_STATIC_CONTEXT_BEGIN`. `IPC=$03:8029` means the
  instruction at `$03:8026` (`EE 51 0B` = `INC.w $0B51`, 3 bytes) executed.
  The census tables use the logged IPC; the narrative translates to the true
  PC where noted.
- **`$03:8007`** is the first byte of a routine in bank `$03` at ROM offset
  `0x018007` (`AB` = `PLB`). It runs once at f3068 in the peer and never in
  ours. Its function is city creation — same neighbourhood as `$03:C63F`
  (`LDA #$076C` / `STA $0B53`) in ours at f3259.
- **`$03:8222`** is the `PLB`/`RTI` tail of the COP dispatcher at `$00:8211`.
  56 logged writes across 5814 frames. The full interpreter census (not this
  write-watch) would show hundreds of executions per frame through this path.
  **This is the per-frame engine.**

---

## 6. Remaining OPEN questions

1. **What sets `DB=$03` on the COP dispatch stack frame in the peer at f3068?**
   The write-watch sees the *result* (`$03:8222` resuming) but not the cause
   (`JSR` / `JSL` / `COP` that pushed it). **OPEN.**
2. **Why does ours never land on `$03:8222`?** The `$0012` latch fires 273
   frames later. Is that the gate, or merely a correlate? **OPEN.**
3. **Does the per-frame engine write WRAM in every iteration, or only
   occasionally?** `$03:8222` logged 56 writes across 5814 frames — roughly
   once per 100 frames. Most iterations must be no-store operations (flag
   tests, accumulator ops). **NOT ESTABLISHED.**
4. **Is `$03:8007` the city-creation routine that the peer's branch takes
   but ours does not?** Same ROM neighbourhood as `$03:C63F` in ours. Plausible
   but unproven. **OPEN.**

---

## 7. What changed on disk

- `/dev/shm/t113-watch.log` — peer census, 33.6 M lines, 2.25 GB. New.
- `/dev/shm/t113-analysis.txt` — peer summary (moved from /dev/shm by integrator).
- `/dev/shm/t113-analysis2.txt` — peer summary (secondary run).
- `/dev/shm/t113-ours-run.log` — ours first run, 9000 frames, `EXIT=0`.
- `/dev/shm/t113-ours-census.log` — ours census, 13.98 M lines, 2.83 GB. New.
- `/dev/shm/t113-ours-run2.log` — ours second run, 9000 frames, `EXIT=0`.
- `/dev/shm/t113-out/timeline.log` — peer timeline (from first peer run).

No tracked file was edited. The census logs live in `/dev/shm` and are gitignored
by the build directory's `.gitignore`. No peer source was touched; the peer
binary `~/peers/jjwatch` was built on 2026-10-02 20:00 from a patched clone
that is now overwritten (the patch no longer applies — `sc_v11_bus.c.deck-orig`
and the current clone are byte-identical, md5 `caec3ef4`). The binary still
carries the T093 hook (confirmed by `nm` symbols `g_t093_*`).

---

## 8. Gates (run on Deck after writing the doc)

Expected results:

```
make check-entrypoints   RESULT: PASS
make check-claims        RESULT: PASS
make check-causes        RESULT: PASS
make check-cheat-gate    RESULT: PASS
make retraction-count    47 rows = 39 refuted + 6 superseded + 2 invalidated-prem.
make build               Built target SimCitySNESRecomp
make test                100% tests passed, 0 tests failed out of 2
make clock               CLOCK: FAIL ( Deck-native, as always — must stay red )
```
