# Measurement — T093 on the Deck, and the "fourth writer" is refuted (2026-10-02)

**Two results, one of each kind.** The peer write-watch now runs
**Deck-natively** and reproduces the host answer exactly. And the host answer
contained an error that only became visible once the write-watch could be run at
all: **there is no fourth writer of `$0B51`.** The two frame-0 writes the watch
logged are a **block memory initialisation**, not an instruction.

## The machine

Deck `steamdeck`, gcc 15.1.1, cmake 4.0.3, Zen 2, 8 threads. Peer core
`simcity-static-recomp`, `RelWithDebInfo`, built from the release tarball in
`~/peers/jj` (version 1.4.1, matching the host's `454eb0a` checkout) against the
same hand-assembled glibc header prefix at `/home/deck/sysroot` via `-idirafter`
— on **both** `CMAKE_C_FLAGS` and `CMAKE_CXX_FLAGS`, which is the whole of
R-032. Driver `study/peer-linux/jjhead.c` (ours, unmodified) linked with `gcc`,
`cold.srm`, `scripts/d_city_kbd.script` (8067 scheduled frames, 41 presses),
9 000 frames, ROM `23715fc7ef700b3999384d5be20f4db5`.

**What was changed in the reference core, and where it lives.** One hook at the
top of `sc_v11_bus_write8()` in `static-recomp/src/sc_v11_bus.c` — the single
bus-write funnel — logging every write that lands on a watched WRAM *offset*.
The peer's published API has no PC trace and no WRAM write hook. The pristine
file is retained on the Deck at `~/peers/sc_v11_bus.c.deck-orig`; the edit exists
**only** in the throwaway clone at `~/peers/jj`, **never in this repository**, and
no peer source is vendored, copied or published. The peer declares no licence.
The hook is described in full in `2026-10-02-t093-peer-0B51-writer.md` §Method and
was transferred as a `.patch`, so only the diff crossed machines.

```
$ cmake --build build --target simcity-static-recomp -j8
[100%] Linking CXX static library libsimcity-static-recomp.a
[100%] Built target simcity-static-recomp

$ T093_WATCH_LO=0x0B51 T093_WATCH_HI=0x0B52 T093_WATCH_LOG=.../watch.log \
    ~/peers/jjwatch "SimCity (USA).sfc" run/cold.srm run/srm.out 9000 \
    scripts/d_city_kbd.script .../out
RESULT failed=0 frames=9000 master_clock=3216243544 insns=107365572
```

## 1. The host answer reproduces, exactly

| | host (previous page, host-only) | **Deck (this page)** |
|---|---|---|
| `$0B51` first incremented | frame **3857** | frame **3857** |
| increments in 9 000 frames | **27** | **27** |
| final value | `001B` | `001B` |
| cadence | +152/+253/+140/+243 = 197 frames per tick | **identical, every delta** |
| zeroed at | f2985, `$03:C77E` = `9C 51 0B` `STZ.w $0B51` | **f2985, same writer** |
| writers seen | 3 | 3 |

Every logged tick carries the same register state — `A=$0000 D=$1EFF X=$5DC0`,
`dbr=$03` — which is what a deterministic core at a deterministic point looks
like, and the `f2985` zeroing carries `A=$0007 D=$0000 X=$003C`. `$03:C9E3`
(`8D 51 0B` `STA.w $0B51`) again never executes.

The comparison that matters for C-041: **the peer's tick never fires inside
f0–f3700 either.** First at f3857. So C-041's window contains no frame in which
the reference build would have executed the tick once. That is unchanged, and
now measured on both machines.

## 2. There is no fourth writer. The frame-0 writes are an initialisation.

The previous page says:

> The watch, however, logged a **fourth** writer the census could not contain:
> **`$00:8023` = `95 00` = `STA dp,x`** — a *direct-page indexed* store with a
> **one-byte** operand. The watch captured it with `D = $0000`, `X = $0B51`, so
> `dp + D + X = $0B51`, twice, at frame 0, value 0.

**That is refuted, on three independent grounds.**

**(a) `95 00` is not at `$00:8023`.** LoROM, `offset = bank*0x8000 + (addr & 0x7FFF)`,
ROM md5 `23715fc7ef700b3999384d5be20f4db5`:

```
$00:8008: FF 1F 9A A9 00 48 AB A9
$00:8010: 8F 8D 00 21 A9 00 85 B1
$00:8018: 8D 00 42 A0 00 20 A2 00
$00:8020: 00 A9 00 95 00 E8 88 D0
$00:8028: FA A0 00 60 A2 00 00 9F
```

`95 00` is at **`$00:8024`**. `$00:8023` is the operand byte `00` of the `LDA #$0000`
that precedes it. The previous page's address is off by one.

**(b) The logged PC is not that instruction's tail either.** The watch logs
`cpu.pc`, which the core sets to the **next** PC. It logged `$00:8025`. A `95 00`
at `$00:8024` has length 2, so its next PC is `$00:8026`, not `$00:8025`. Whatever
produced the logged value does not begin at `$00:8024` either — a 2-byte
instruction ending at `$00:8025` would have to start at `$00:8023`, which is a
`BRA`, not a store. So the identification was never self-consistent; it was
accepted because "the log said `$00:8025`" and "`95 00` is one byte of operand"
were each individually true of *different* addresses.

**(c) The decisive one: a single PC writes exactly as many bytes as the watch
range spans.** Run on the Deck, 60 frames, watching a range and counting only
frame-0 hits:

| watched range | bytes in range | writes logged at frame 0 | distinct PCs |
|---|---|---|---|
| `$0B51`–`$0B51` | 1 | **1** | `$00:8025` |
| `$0B50`–`$0B53` | 4 | **4** | `$00:8025` |
| `$0B40`–`$0B7F` | 64 | **64** | `$00:8025` |
| `$0B00`–`$0BFF` | 256 | **256** | `$00:8025` |

**No 65816 instruction writes 256 consecutive bytes.** (The instruction this
was read as — a one-byte-operand direct-page indexed store — writes one.) The
frame-0 hits are a **block memory initialisation pass** — every byte of the
watched span written once, in order, from whatever the CPU happened to be parked
at. `$0B51` was not written by an instruction at all; it was covered by a sweep.

(The wider control, range `$0000`–`$FFFF`, logs 466 041 writes of which 3 719 are
at frame 0 across three PCs — `$00:800E`, `$00:8018`, `$00:8025`. So the
initialisation is a partial, multi-site pass, not a single full clear. Also not
an instruction. **OPEN** as to what performs it.)

## 3. What survives, and what does not

**Does not survive:** the fourth writer, its address, and its use as evidence.

**Does survive, on its own logic rather than on that observation:** the
*methodological* point that an operand-byte census is structurally blind to a
`dp,x` store, because the operand is one byte and the effective address does not
exist until `D` and `X` are known at run time. That is true as a statement about
what an operand scan can see, and it is independently supported by this
repository's earlier `JSL (abs)` mis-scan. **But it is now unevidenced here**, and
it is recorded as such: no `dp,x` writer of `$0B51` has been observed.

**So the writer census is, on the evidence available:**

| writer | bytes | instruction | executed |
|---|---|---|---|
| `$03:8026` | `EE 51 0B` | `INC.w $0B51` | **yes, 27×, first f3857** |
| `$03:C77E` | `9C 51 0B` | `STZ.w $0B51` | yes, 1× at f2985 |
| `$03:C9E3` | `8D 51 0B` | `STA.w $0B51` | no |

plus one non-instruction sweep at f0 that covers `$0B51` along with every other
byte of the span it touches. Ledger row **R-034**.

## 4. Still OPEN

- **What performs the frame-0 initialisation, and where the CPU is parked while
  it happens.** `$00:8025` is reported because that is the PC field's value; it is
  not the writer.
- **Whether `$03:8026` executes in OUR build at any frame ≥ 3857.** Nothing here
  touches it, and it is still the cheapest next measurement in the project.
- **Whether the peer can be driven to a genuinely running city at all.** Both
  builds reach "city loaded" and stop: ours at `$0B53 = 0x076C`, funds 20000,
  population 0; the peer's `$0B51` rises 27 times and the year stays 1900 with
  the population at 0. Twenty-seven tick increments bought nothing, in the run
  that was supposed to prove ticking matters.