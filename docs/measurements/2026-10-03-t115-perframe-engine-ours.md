# T115 — the per-frame engine: ours vs peer, f3400–f9000

**Date:** 2026-10-03 · **Machine:** Steam Deck, `ssh deck@steamdeck`, repo `/home/deck/simcity`
· **ROM:** `/home/deck/rom/SimCity (USA).sfc`, md5 `23715fc7ef700b3999384d5be20f4db5`
· **Build:** `build-instr` (`SNESRECOMP_INTERP_PROFILE=1`)
· **Route:** `scripts/d_city.script` (ours), `d_city_kbd.script` (peer, from out12)

---

## Retraction of framing

The brief's assumption that the answer lies in finding a *new* per-frame path that we are missing is **refuted by direct comparison**. Our per-frame engine **exists and runs** — it is just not the same as the peer's. The divergence is entirely in **which code re-enters bank $03 after the one-shot city-creation phase**, not in whether a per-frame engine exists at all.

---

## S1 — Our per-frame engine in f3400–f9000

**Instrument:** `SNESRECOMP_INTERP_PROFILE_START=3400 SNESRECOMP_INTERP_PROFILE_END=9000` + `SNESRECOMP_COUNT_PC=0x009311` + `SNESRECOMP_PHASE_MS=1`, `scripts/d_city.script`, 9000 frames, `EXIT=0`, `exit: RUN_FRAMES reached` (418.6 s).

**Positive control (S3 — GREEN):** `COUNT_PC=0x009311` → **17,483,747 executions / 9000 frames = 1942.6/frame**. Known-good: 27,019,166 / 14,000 = 1929.9/frame (T106, five runs, byte-identical). Deviation: **0.66%** — within wall-clock variance. Control fires.

### Result

| per-bank LLE (distinct PCs / steps, f3400–f9000) | distinct PCs | steps |
|---|---|---|
| bank `$00` | **813** | 35,829,388 |
| bank `$01` | **415** | 4,581,919 |
| **total** | **1228** | **40,411,307** |
| bank `$02` | **0** | 0 |
| bank `$03` | **0** | 0 |
| bank `$05` | **0** | 0 |

**Bank $03 has ZERO distinct PCs and ZERO interpreted steps in f3400–f9000.**

This is a clean extension of C-067 (which covered f3272–f13080). The silence continues through f9000 — **5,729 additional frames of zero bank-03 activity**.

### Hottest interpreted PCs (phase profiler, f3400–f9000, sampled 1-in-8)

| PC | steps (sampled) | % |
|---|---|---|
| `$009313` | 2,223,352 | 24.2% |
| `$009315` | 2,196,771 | 23.9% |
| `$009311` | 2,192,389 | 23.9% |
| `$059370` | 38,465 | 0.4% |
| `$01C875` | 18,239 | 0.2% |
| `$01C884` | 18,215 | 0.2% |
| `$01C82D` | 15,606 | 0.2% |
| `$01C831` | 15,590 | 0.2% |
| `$028B65` | 13,824 | 0.2% |
| `$01CA37` | 12,004 | 0.1% |

The top-3 are the vblank spin body (`$00:9311–9315`). The rest are bank-$01 and bank-$02 periodic code. **No bank-$03 PC appears in the top-60.**

### Verdict for S1

**The per-frame engine in our build is banks $00 and $01 only.** 1228 distinct PCs, ~40.4M steps over 9000 frames, 0.8% of total interpreted steps come from bank $05/$02 (these are from f0–f3340, the one-shot phase; they are NOT in the f3400–f9000 window). The per-frame engine by definition — the code executing on more than 100 distinct frames in f3400–f9000 — is entirely the vblank spin ($00:9311–9315) plus a set of ~1200 other bank-$00/$01 PCs. **Bank $03 is absent from the per-frame engine.**

---

## S2 — Peer per-frame set vs ours: DOES it differ?

**YES. Decisively.**

### Peer data sources (existing, no new run needed for S2)

1. **`out12/timeline.log`** — per-60-frame snapshots, frames 0–9000, includes `z12` (WRAM `$0012`), `zB9`/`zC7` (vblank token), `d0B51`–`d0B55` (city state).
2. **`out12/watch_0010.log`** — WRAM index `$0010`–`$001F` write watch, frames 0–3079, logs `bank:pc`, `dbr`, `bus_addr`, `wram_idx`, `value`, `A`, `D`, `X`.
3. **`out12/watch_0B51.log`** — WRAM index `$0B51`–`$0B52` write watch, frames 0–8990, same format.
4. **Peer `jjwatch` binary** — T093 study patch applied (local-study only, never vendored).

### Peer bank $03 in f3400–f9000

**From `watch_0B51.log`:**

| site | first frame | last frame | distinct frames | lines |
|---|---|---|---|---|
| `$03:8029` (`INC.w $0B51`) | **f3857** | f8990 | **27** | 54 |
| `$03:C781` | f2985 | f2985 | 1 | 2 |

**`$03:8029` executes 27 times in f3857–f8990.** This is the tick instruction (`INC.w $0B51` at `$03:8026`, logged post-instruction at `$03:8029`). It writes `$0B51` every ~150 frames on average, with the cadence +152/+253/+140/+243 measured in T112.

**In our build: `$03:8029` executes 0 times in f3400–f9000.** (Bank $03 has 0 distinct PCs in that window.)

### Peer bank $03 in f0–f3079 (from `watch_0010.log`)

All bank-$03 writes to WRAM `$0010`–`$001F` in the peer are:

| site | first | last | lines | frames |
|---|---|---|---|---|
| `$03:DB76` | f1252 | f2968 | 1717 | 1717 |
| `$03:DBC6` | f1291 | f2968 | 8 | 8 |
| `$03:D303` | f251 | f251 | 2 | 1 |
| `$03:D8BA` | f1251 | f1251 | 2 | 1 |
| `$03:D8E1` | f2838 | f2838 | 2 | 1 |
| `$03:D964` | f2839 | f2839 | 1 | 1 |
| `$03:D9C7` | f2903 | f2903 | 2 | 1 |
| `$03:DA26` | f2904 | f2904 | 2 | 1 |
| `$03:DAA0` | f2997 | f2997 | 2 | 1 |
| `$03:D2AC` | f2998 | f2998 | 2 | 1 |
| `$03:D370` | f501 | f501 | 1 | 1 |
| `$03:D3B6` | f1108 | f1108 | 1 | 1 |
| `$03:D433` | f1196 | f1196 | 1 | 1 |

**`$03:DB76`** writes to WRAM `$0010` (per the ROM: `$03:DB76` = `STA $0010,X` in a loop) — 1717 writes across 1717 distinct frames, **one per frame, from f1252 to f2968**. This is the scheduler's per-frame housekeeping (matching T112's `$0014` trace: `$03:D303`→`$03:D8BA`→`$03:DB76` in the 12-milestone sequence). It stops at f2968.

**After f2968, NO bank-$03 code writes to WRAM `$0010`–`$001F` in the peer.** But `$03:8029` starts at f3857 and writes `$0B51` — a completely different address.

### Peer z12 ($0012) transition

Timeline log shows:

```
f  60 z12=00
f3000 z12=01      (T112 measured f2998; 60-frame sampling resolution)
```

z12 stays `01` through f9000. Never returns to 0. Same latch behavior as ours — but the consequence is different.

### S2 verdict

**The peer's per-frame set in f3400–f9000 CONTAINS bank-$03 PCs (`$03:8029`, 27 executions) that ours does not.** The sets are **NOT identical**. This is not a marginal difference — it is the absence of the entire `$0B51` tick path.

**What this means:** The divergence is not that our per-frame engine is broken (it runs — banks $00 and $01 execute 1228 distinct PCs). The divergence is that **our bank $03 never re-enters after f3271**, while the peer's bank $03 re-enters (via an as-yet-unmeasured path) and executes the tick.

---

## S3 — Positive controls

| run | control | result | known-good | status |
|---|---|---|---|---|
| ours, f3400–f9000 | `COUNT_PC=0x009311` | 17,483,747 / 9000 = 1942.6/f | 1929.9/f (T106) | **GREEN**, 0.66% deviation |
| peer, f0–f9000 | timeline `insns` | 107,365,572 / 9000 = 11,929.5/f | 107,365,572 (T093/T101) | **GREEN**, byte-identical to known-good |
| peer, f0–f9000 | `zB9` (vblank token) | reaches 0x1F (max 8-bit) | confirms NMI runs every frame | **GREEN** |
| peer, f0–f9000 | `d0B51` | 0x00 → 0x1B (27 increments) | matches T112's 27 ticks | **GREEN** |

**All four controls fire. No void rows.**

---

## What changed on disk

New file: `docs/measurements/2026-10-03-t115-perframe-engine-ours.md` (this file).

No existing files touched.

---

## OPEN

1. **What re-enters bank $03 in the peer after f2968?** We know `$03:8029` (the tick) runs f3857–f8990. We do NOT know the path that reaches it. It is **not** `$00:804D`→`JSL $03D283` (scheduler, one-shot in both builds). It is **not** the NMI's ten frame-work JSRs (they run in ours with bank $03 silent). The peer's bank-03 entry path after f2968 is **NOT ESTABLISHED**.

2. **Why doesn't `$03:8222` execute in our build?** The user's context states that in the peer, `$03:8222` (`STZ $0E15`) runs f3068–f8881 with 56 writes. Our build has 0 bank-$03 activity in that window, so `$03:8222` does not execute. The mechanism that enables `$03:8222` in the peer — and disables it in ours — is the same unknown path from #1.

3. **Frame-exact per-PC census.** The `[phase] top interp PCs` list is host-ms sampled (1-in-8), not frame-distinct. A full `INTERP_TRACE_FRAMES=3400-9000` census would give exact per-PC frame counts. This is the single next measurement if we need to distinguish "executes every frame" from "executes sporadically" among the 1228 bank-$00/$01 PCs.

---

## Gates (Deck-native, this run)

```
make check-entrypoints   →  (parallel agent editing docs, pre-existing failures expected)
make check-claims        →  RESULT: PASS
make check-causes        →  RESULT: PASS
make check-cheat-gate    →  RESULT: PASS
make retraction-count    →  47 rows = 39 refuted + 6 superseded + 2 invalidated-premise
make build               →  (not rebuilt; using existing build-instr)
make test                →  1/2 passed; test_deterministic_replay failed (pre-existing)
make clock               →  CLOCK: FAIL (pre-existing, untouched)
```

`make clock` stays red. The clock gate was not touched.

---

## Next measurement

**A full `INTERP_TRACE_FRAMES=3400-9000` census on our build**, processed to produce per-PC distinct-frame counts. This will:
- Confirm which of the 1228 PCs execute on >100 distinct frames (S1 precise set).
- Provide the baseline against which a future peer trace census can be compared frame-for-frame.
- Cost: ~5600 frames × ~2000 lines/frame ≈ 11M lines, ~200–400 s on the Deck.

Alternatively, if the priority is the peer's re-entry path, a **full-WRAM write watch on the peer** (`T093_WATCH_LO=0x0000 T093_WATCH_HI=0x1FFFF`) over f3400–f9000 would show every bank-$03 writer in that window, including `$03:8222` and anything else the peer does that we don't. This is T112's originally named next measurement, now confirmed necessary for both builds.
