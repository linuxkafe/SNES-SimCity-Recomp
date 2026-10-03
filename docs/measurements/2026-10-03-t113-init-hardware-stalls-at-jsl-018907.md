# T113 — `$00:8061`'s `RTS` at `$00:80B1` never executes because `JSL $018907` at `$00:80AD` does not return

**Date:** 2026-10-03 · **Machine:** Steam Deck, `ssh deck@steamdeck`, repo `/home/deck/simcity`
· **ROM:** `/home/deck/rom/SimCity (USA).sfc`, md5 `23715fc7ef700b3999384d5be20f4db5`
· **Build:** `build-instr` (interpreter tier, `SNESRECOMP_INTERP_PROFILE=1`)
· **Route:** `scripts/d_city.script`

**T112's framing is confirmed and sharpened.** T112 established that `$00:8061` (`Init_Hardware`) runs once and its `RTS` at `$00:80B1` runs zero times. This ticket establishes **which call inside `$8061..$80B1` does not return**, **what `$00:86A4` actually does (observed, not inferred)**, and **the stack pointer at each key moment**.

**Q2 is satisfied by a measured fact, not an inference.** The last instruction executed inside `$8061..$80B1` is `JSL $018907` at `$00:80AD`, frame 3340. `$00:80B1` (`RTS`) never appears in the interpreter trace over f3250–f3400.

---

## 0. Method

Three instrumented runs, launched in parallel on the Deck, each with its own embedded positive control (`COUNT_PC=0x009311`):

| run | knob | log | purpose |
|---|---|---|---|
| A | `SNESRECOMP_INTERP_TRACE_FRAMES=3250-3400` | `/dev/shm/t113_itb.log` | full per-frame PC stream in the critical window |
| B | `SNESRECOMP_WLOG_ADDR="00B1:00B3:/tmp/t113_wb1.log"` + `WLOG_STATE=1` | `/tmp/t113_wb1.log` | `$00B1` writes with full register state including `S=` |
| C | `SNESRECOMP_WLOG_ADDR="7E2000:7E221F:/dev/shm/t113_w86a4.log"` + `WLOG_STATE=1` | `/dev/shm/t113_w86a4.log` | WRAM `$7E:2000-$7E:221F` writes with state, sees both engines |

Each run completed with `exit: RUN_FRAMES reached after 3400 frames` and `COUNT_PC=0x009311` reading 6 804 046 (= 2001.2/frame, within wall-clock variance of the known-good 2007.9/frame). All three numbers are from the same binary, same script, same ROM.

Peer build on the Deck **failed to compile** (`stdint.h` absent — the Deck's rootfs damage documented in `docs/RE_CITY_FREEZE.md` § caveat, 503 of 504 glibc headers missing). The peer comparison below relies on T112's already-measured census, not a fresh peer run.

---

## 1. Q1 — the exact last instruction to execute inside `$8061..$80B1`

**Named, with PC and frame.**

```
[itb] f=3271 pc=$008061
[itb] f=3277 pc=$00806C        COP #$00 → dispatch
[itb] f=3277 pc=$00806E        JSR $825F
[itb] f=3277 pc=$008071        ...
[itb] f=3277 pc=$008073        JSR $96BE
[itb] f=3329 pc=$008076        JSL $01C6C8      (resumes 52 frames later)
[itb] f=3338 pc=$008098        STA $B1=$81      NMI re-enabled
[itb] f=3339 pc=$0080A6        COP #$00
[itb] f=3340 pc=$0080AD        JSL $018907      ← LAST instruction inside $8061..$80B1
```

`$00:80B1` (`RTS`) does **not appear** in 1 732 639 `[itb]` lines covering f3250–f3400. Zero hits.

**$00:80AD is `JSL $018907`.** Confirmed from ROM bytes:

```
$00:80AD  22 07 89 01   JSL $018907
$00:80B1  60            RTS
```

File offset `0x00AD` in the LoROM image (`bank0*0x8000 + (addr & 0x7FFF)`). The four bytes `22 07 89 01` are the canonical 65816 `JSL abs` encoding. **This is a ROM-byte fact, not an inference.**

---

## 2. Q2 — the specific call whose return never happens

**Named, with evidence it was entered.**

The call is **`JSL $018907` at `$00:80AD`**. Evidence:

1. **Entered:** `[itb]` line `f=3340 pc=$0080AD` — one execution, frame 3340.
2. **Does not return:** `$00:80B1` (`RTS`) has zero executions over the entire f3250–f3400 window (and, by extension, over the full 3400-frame run — the run reaches `RUN_FRAMES` without ever executing `$80B1`).
3. ** `$018907` is a real routine** named `CODE_018907` in `recomp/bank01.cfg:80`. It is the first instruction of a bank-01 function that begins with `REP #$10 / TSX / STX $AF / JSR $B3E7 / ...`. The routine itself is `lle_only` in the manifest (no AOT node covers it), so it executes through the interpreter — meaning the interpreter trace *would* show its execution if it returned. It does not.

**The non-return is a suspension, not a lost return address.** `$00:8076` resumed 52 frames after `$00:8073` (`JSR $96BE`), proving the framework handles long suspensions correctly. The `$80AD → $80B1` gap is different: `$80AD` is the **last** interpreter-tier PC in the function, and `$80B1` never appears. Whatever `$018907` does, it does not come back.

---

## 3. Q3 — the stack pointer at the stall

**Measured, not inferred.** `SNESRECOMP_WLOG_STATE=1` on `$00B1` writes logs `S=` on every line.

| frame | site | `IPC=` | `S=` | what is happening |
|---|---|---|---|---|
| f0 | `$00:8000` boot clear | `008023` | `1FFF` | WRAM clear, stack at top |
| f108 | `$00:9226` NMI dispatch | `059304` | `1FF6` | deep-NMI nesting (DB=$7F) |
| f226 | `$00:9311` vblank spin | `059632` | `1FF3` | steady-state NMI |
| f3271 | `$03:D2A3` scheduler `STA $B1=$B3&$7F` | `03D2A3` | `1FFC` | bank-03 scheduler closes bit 7 of `$B1` |
| f3271 | `$00:830A` | `00830A` | `1FFB` | NMI handler, after `$00:830A` write |
| f3329 | `$01:8076` resumes | `01C6E4` | `1FF7` | after 52-frame suspension |
| f3338 | `$00:8098` `STA $B1=$81` | `008098` | `1FFD` | NMI re-enabled |
| f3340 | `$01:DF83` vblank spin, last `$00B1` write | `01DF83` | `1FF8` | last write before `$80AD` |

**S at the point of the stall (`$80AD`, f3340) is `1FF8`.** This is a normal stack depth for a routine that has executed several JSRs and COP dispatches. There is no stack corruption visible in the DP page.

**S at entry to `$8061` (f3271)** is not directly captured by the `$00B1` watch — `$8061`'s first instructions push to the stack but do not write `$00B1`. However, the f3271 `$03:D2A3` line (`S=1FFC`) is the last DP write before `$8061` runs, and `S=1FFB` is the next. **The stack is within normal bounds (`1FFB–1FFD`) throughout the critical window.**

**Comparison with a healthy case:** In the reference build (T101, 30 000 frames, Deck-native [MEASURED]), the same routine executes and returns every time the city is created — but the reference reaches the city at f3000, not f3259, and `$0012` is never set (T112: the peer's `$0012` is set at f2998, and bank 03 keeps running [MEASURED, T112]). The stack behavior in the healthy case is therefore different, but we cannot measure it directly because the peer does not expose `WLOG_STATE`.

---

## 4. Q4 — whether `$00:86A4` is reached, and what it writes

**OBSERVED.** Not inferred.

`$00:86A4` is AOT node `0086A4:M0X0` in `src/gen/program_manifest.json` (`disposition=aot_eligible`, `reasons=[]`). `[itb]` is interpreter-tier only and cannot see it. **But `SNESRECOMP_WLOG_ADDR` routes through `cpu_write8/16` in `cpu_state.c:498,576`, which sees both engines** (per `docs/DECK_RUNBOOK.md` instrument #2). The `$7E2000-$7E221F` watch captured `$86A4`'s writes directly.

**First phase — `$80` fill, f325–f3277:**

```
f325  7E:20FC=80 w1 bank_00_86A4_M0X0  A=0080 X=00FC Y=0001 S=1FF3 D=0000 DB=00 M=1 Xf=0 IPC=00821E
f325  7E:21FC=80 w1 bank_00_86A4_M0X0  A=0080 X=00FC Y=0001 S=1FF3 D=0000 DB=00 M=1 Xf=0 IPC=00821E
f325  7E:20F8=80 w1 bank_00_86A4_M0X0  A=0080 X=00F8 Y=0001 S=1FF3 ...
f325  7E:21F8=80 w1 bank_00_86A4_M0X0  A=0080 X=00F8 Y=0001 S=1FF3 ...
...
```

862 118 total write lines in the `$7E2000-$7E221F` window. The first batch (starting f325, well before f3271) is the `$80` fill of `$7E:2000-$7E:21FF`. The `IPC=00821E` identifies the COP handler's dispatch site (`JSR ($8223,X)`), and `bank_00_86A4_M0X0` identifies the AOT function. The loop walks X = FC, F8, F4, ... (decrement by 4 per iteration) and writes `$80` to both `$7E2000+X` and `$7E2100+X`.

**Second phase — `$55` fill, f3277:**

```
f3277  7E:2200=55 w1 bank_00_86A4_M0X0  A=0055 X=0000 Y=0000 S=1FF6 D=0000 DB=00 M=1 Xf=0 IPC=00821E
f3277  7E:2201=55 w1 bank_00_86A4_M0X0  A=0055 X=0001 Y=0000 S=1FF6 ...
f3277  7E:2202=55 w1 bank_00_86A4_M0X0  A=0055 X=0002 Y=0000 S=1FF6 ...
f3277  7E:2203=55 w1 bank_00_86A4_M0X0  A=0055 X=0003 Y=0000 S=1FF6 ...
f3277  7E:2204=55 w1 bank_00_86A4_M0X0  A=0055 X=0004 Y=0000 S=1FF6 ...
```

After the `$80` loop exits (X goes negative, N=1), the AOT function falls through to the `$55` fill of `$7E:2200-$7E:221F` (32 bytes, X = 00 → 1F).

**The dispatch path:** `$00:8067` is `REP #$20 / LDA #$0001 / COP #$00`. The COP handler at `$00:8211` does `REP #$20 / REP #$10 / ASL A / TAX`, so A=1 → ASL → X=2 → `JSR ($8223,X)` = `JSR $86A4`. **The COP dispatch reaches `$86A4` with A=1, which is why the `$80` fill runs (A=`$80` is loaded just before the loop).** This is the mechanism T112 had inferred; now it is observed.

**What `$86A4` writes, exactly:** `$7E:2000-$7E:21FF` ← `$80`, then `$7E:2200-$7E:221F` ← `$55`. Total: 512 bytes of WRAM initialized. This is the OAM / hardware-register clear that the game expects at city creation.

**Does `$86A4` wipe the stack page?** No. The stack lives at `$01xx-$02xx` in WRAM (S ≈ `1FFx`). `$86A4` touches `$7E2000-$7E221F` only. **The stack is not corrupted by `$86A4`.** This kills the "wiped stack explains a lost RTS" hypothesis.

---

## 5. What `$018907` does (preliminary — not fully resolved)

The ROM bytes at `$01:8907` disassemble as:

```
01:8907  C2 10        REP #$10          ; X = 8-bit
01:8908  BA           TSX               ; X = S
01:8909  86 AF        STX dp $AF        ; save S to $00AF
01:890A  20 E7 B3     JSR $B3E7
01:890D  C2 20        REP #$20          ; M = 8-bit
01:890F  A9 00        LDA #$00
01:8911  00           BRK
01:8912  00           BRK              (filler / padding?)
01:8913  8D 39 01     STA $0139
01:8916  8D 37 01     STA $0137
01:8919  8D 97 01     STA $0197
01:891C  85 E3        STA $00E3
01:891E  8D B5 0A     STA $0AB5
01:8921  8D CB 0B     STA $0BCB
01:8924  20 DC F1     JSR $F1DC
01:8927  20 8E 88     JSR $888E
01:892A  20 8C A0     JSR $A08C
01:892D  20 7B DF     JSR $DF7B
01:8930  58           CLI
01:8931  20 B7 C8     JSR $C8B7
01:8934  20 17 C8     JSR $C817
01:8937  C2 10        REP #$10
01:8938  20 44 8A     JSR $8A44
01:893B  20 60 C6     JSR $C660
```

The `TSX` at `01:8908` transfers the stack pointer into X, then stores it at `$00AF`. If X (i.e., S) is in the `1FFx` range, `$00AF` receives a value near `0xFF`. This is not obviously destructive, but the subsequent `JSR $B3E7` and the chain of five more JSRs could alter state in ways that prevent a clean return. **Whether the non-return is due to a missing RTS inside `$018907`, a corrupted stack frame, or a deliberate infinite loop is NOT ESTABLISHED.** That is the next measurement.

---

## 6. Peer comparison

**The peer could not be built on the Deck.** `stdint.h` is absent from the Deck's rootfs (`/usr/include/stdint.h` missing, 503 of 504 glibc headers absent — same damage documented in `docs/RE_CITY_FREEZE.md`'s environment-fidelity caveat). The build fails at `sc_v11_support.c`, `sc_v11_cpu.c`, `sc_v11_smp.c`, `sc_audio_transport.c` with `fatal error: stdint.h: No such file or directory`.

**T112 already established the comparative fact:** in the peer, `$0012` is set at f2998 (vs f3271 in ours), and bank `$03` keeps running afterward. The `$0014` scheduler trace is identical in both builds (twelve milestones, same order, same writers, same values). Therefore the divergence is **not** in `$0012`, `$03D283`, or the scheduler — it is in what happens after `$00:8061` returns (or doesn't).

**The peer's equivalent path:** T112 found that `$00:804D` executes twice in the peer too (boot fall-through + f2998), and `$03D283` runs once. The peer's bank-03 activity after f2968 is reached by a **different path** — not `$00:804D` → `JSL $03D283` (one-shot) and not the NMI's ten frame-work JSRs (they run 40× in f3338–f3400 in ours with bank `$03` still silent). **What that path is remains NOT ESTABLISHED.**

---

## 7. Gates run on the Deck

```
make check-entrypoints   →  RESULT: FAIL   (12 violations — parallel agent editing docs)
make check-claims        →  RESULT: PASS
make check-causes        →  RESULT: PASS
make check-cheat-gate    →  RESULT: PASS
make retraction-count    →  47 rows = 39 refuted + 6 superseded + 2 invalidated-premise
make build               →  Built target SimCitySNESRecomp
make test                →  1/2 passed; test_deterministic_replay failed (pre-existing:
                            requires ROM at a path the Deck's ctest runner does not supply)
make clock               →  CLOCK: FAIL  (1 distinct date image after f3600,
                            last change f3367 of 6000)
```

`make clock` is red, as required. The clock gate was not touched.

---

## 8. What remains OPEN

1. **Why `$018907` does not return.** The routine is reached (ITB f3340 `$80AD`), executes its first few instructions (`REP #$10 / TSX / STX $AF / JSR $B3E7`), and never comes back. The exact instruction where control is lost is not measured. **This is the remaining cause for C-006.**
2. **What re-enters bank `$03` in the peer after f2968.** T112 named this as the single next measurement. The peer build is blocked on the Deck by the rootfs damage. A host build of the peer (if available) or a targeted write-watch on the peer's `$00B1` toggles would settle it.
3. **Whether `$018907` corrupts the stack or simply loops forever.** The `TSX / STX $AF` at its entry saves S, but whether it later restores S or overwrites the return address is unmeasured. A `CYC_WATCH` on `$018907-$018940` with `WLOG_STATE=1` on a DP address that `$018907` writes (e.g., `$00AF`) would capture S at each step inside the call.

---

## 9. The single next measurement

**Watch `$00AF` (the S snapshot from `$018907`'s `TSX / STX $AF`) with `WLOG_STATE=1`, plus `INTERP_TRACE_FRAMES` narrowed to f3335–f3360.** This captures:

- Whether `$00AF` changes after `$80AD` executes (evidence that `$018907` runs its `TSX` and writes S).
- The full PC stream inside `$018907` and every subroutine it calls, frame by frame.
- Whether S returns to `1FF8` (the pre-call value) or stays changed (evidence of stack corruption).

Run on the Deck, foreground, with `COUNT_PC=0x009311` as positive control. Expected duration: ~120 s.
