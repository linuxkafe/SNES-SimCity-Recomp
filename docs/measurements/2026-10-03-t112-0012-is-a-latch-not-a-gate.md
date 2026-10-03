# T112 — `$0012` is a one-shot latch, not the re-entry gate, and the gate is `$00:8061`'s missing `RTS`

**Date:** 2026-10-03 · **Machine:** Steam Deck, `ssh deck@steamdeck`, repo `/home/deck/simcity`
· **ROM:** `/home/deck/rom/SimCity (USA).sfc`, md5 `23715fc7ef700b3999384d5be20f4db5`, 524 288 B
· **Reference build:** the peer, private study only, no source vendored or copied
· **Route:** `scripts/d_city.script` (ours), `scripts/d_city_kbd.script` (peer)

**TWO THINGS IN THE BRIEF ARE REFUTED HERE, and both were load-bearing:**

1. **"`$0012` is the re-entry gate for `$00:804D`" — REFUTED, by the peer.**
   `$0012` is written **exactly twice** in both builds and **never cleared in either**.
   In the peer it is `1` from f2998 onward and bank `$03` keeps running. A flag that
   never returns to `0` cannot gate an action that has to repeat. It is a latch.
   (Ledger **R-046**.)
2. **"`$0B53` is not the raw decimal value, so there is a year transform to find" —
   REFUTED as a premise.** `$0B53` **is** a plain binary-decimal year: the game's
   own milestone table at `$03:C4DC` holds `1900, 1900, 1900, 1901, 1901, 1905`
   and `$03:C490 CMP ($C4DC),Y` compares `$0B53` against it **16-bit**. There is
   no transform to find; §7 explains what `1952` actually is. (Ledger **R-047**.)

**AND THE ANSWER TO THE QUESTION ASKED — "what is supposed to execute `$00:804D`,
and why does it stop after f3271" — is measured, and it is not `$0012`:**

    $00:804D  CODE_00804D  "MainLoop"        executes  2 times in 3400 frames
    $03D283   the round-robin scheduler      enters/exits ONCE, f3259..f3271
    $00:8061  CODE_008061  "Init_Hardware"   executes  1 time,  and
    $00:80B1  its RTS                       executes  0 times
    $03D283   re-entered after that         0 times, forever

`$00:8061` **is** supposed to return to `$00:805F BRA $00:804D`. It does not, and
that single missing `RTS` is why bank `$03` never comes back. §5 has the frame map.

---

## 0. Method, and the ROM mapping, established rather than assumed

`$03:8026` → ROM `0x18026` and `$00:825F` → ROM `0x25F` together fix the mapping as
**LoROM with an 0x8000-byte bank stride**: `offset = (bank << 15) | (addr & 0x7FFF)`.
`0x7FC0` reads `SIMCITY ` and `$00:FF00-$FFFF` mirrors `$7F00-$7FFF`.

⚠ **These are ROM *bytes*.** Per R-038, a byte-boundary question about the ROM was
settled here two ways: with `SNESRECOMP_CYC_WATCH`'s fetched-`op=` field, and
against the interpreter's own executed-PC stream (`[itb]`). Every instruction
boundary quoted below is an address the CPU was **observed at**, not one a linear
decoder produced. The one place a linear decode could have lied and did — see §6.

## 1. `$00:804D` disassembled, with its caller, and the answer to "who calls it"

**Nobody calls it.** It is reached by **fall-through**, exactly once, out of the
one-shot boot routine at `$00:8000` (`recomp/bank00.cfg:60 force_lle 0x00804D`,
`recomp/bank00.cfg:101 name … CODE_00804D`, cfg label `MainLoop`):

```
$00:8000  18           CLC
$00:8001  FB           XCE            ; emulation off -> native
$00:8002  78           SEI
$00:8003  C2 10        REP #$10
$00:8005  E2 20        SEP #$20
$00:8007  A2 FF 1F     LDX #$1FFF
$00:800A  9A           TXS
$00:800B  A9 00        LDA #$00
$00:800D  48 / AB      PHA / PLB      ; DB = 0
$00:800F  A9 8F / 8D 00 21   LDA #$8F / STA $2100      ; INIDISP, force blank
$00:8014  A9 00 / 85 B1 / 8D 00 42   LDA #$00 / STA $B1 / STA $4200   ; NMI off
$00:801B  A0 00 20     LDY #$2000
$00:801E  A2 00 00     LDX #$0000
$00:8021  A9 00        LDA #$00
$00:8023  95 00        STA $00,X      ; clear WRAM $0000-$1FFF      <== D = 0
$00:8025  E8 / 88 / D0 FA   INX / DEY / BNE $8023
$00:8029  A0 00 60     LDY #$6000
$00:802C  A2 00 00     LDX #$0000
$00:802F  9F 00 20 7E  STA $7E2000    ; clear $7E:2000-$7E:7FFF
$00:8035  D0 F8        BNE $802F
$00:8037  C2 20        REP #$20
$00:8039  A9 00 00 / 8D 65 04  LDA #$0000 / STA $0465
$00:803F  E2 20        SEP #$20
$00:8041  EE 25 0B     INC $0B25
$00:8044  A9 81        LDA #$81
$00:8046  85 B3        STA $B3
$00:8048  85 B1        STA $B1        ; bit 7 SET -> the NMI will do real work
$00:804A  8D 00 42     STA $4200
$00:804D  A5 12        LDA $12        <== MainLoop starts here, by fall-through
```

A ROM-wide scan for every call/jump form that could reach `$00:804D` —
`JSR/JMP abs`, `JSL/JML long`, `JSR (abs,X)`, `JMP (abs)`, `JMP (abs,X)` —
returns **zero hits in 512 KB**. `$00:804D` is reachable only by falling into it
and by its own two backward branches.

### `$0012` really is WRAM `$0012`, and this time it is measured

`$00:804D`'s `LDA $12` is **direct-page relative**, so it is only `$0012` if `D = 0`.
Two independent reasons, one of them a measurement:

* **By construction.** The boot loop at `$00:8023` (bytes `95 00`, an
  X-indexed direct-page store) with `X = $0000..$07FF` clears WRAM `$0000-$1FFF`;
  that is only the boot code's intent if `D = 0`. In OUR build this one is
  **measured** to execute as an instruction, not as a block fill — the write-watch
  logs 16 separate `w1` lines with `IPC=008023` and `X` counting `$0010, $0011, …`,
  one byte per execution. (R-034 retracted the claim that this is a `$0B51` writer;
  that is a different claim about the peer and is not re-asserted here.)
  The same routine's `STA $B1`/`STA $B3` are the `$00B1`/`$00B3` that `$00:80B6
  LDA $0000B1` and `$03:D29F LDA $B3` read, so `D = 0` there too.
* **By measurement** (§2): the writer of `$0012` logs `D=0000`.

## 2. `$00:804D`, `$03D283`, `$03:D2AA`, `$00:8061` — the disassembly

```
CODE_00804D  "MainLoop"                              (force_lle; 2 executions / 3400 f)
$804D  A5 12        LDA $12                ; D = 0, so WRAM $0012
$804F  D0 0B        BNE $805C
$8051  64 B7        STZ $B7
$8053  20 65 8D     JSR $8D65
$8056  22 83 D2 03  JSL $03D283           ; the round-robin scheduler
$805A  80 F1        BRA $804D
$805C  20 61 80     JSR $8061              ; == Init_Hardware
$805F  80 EC        BRA $804D              ; re-enter the loop

CODE_03D283  the scheduler (JSL'd only from $8056; not a manifest node)
$D283  4B / AB      PHK / PLB             ; DB = $03
$D285  C2 20        REP #$20
$D287  C2 30        REP #$30
$D289  A5 14        LDA $14               ; round-robin index
$D28B  C2 10        REP #$10
$D28D  0A / AA      ASL A / TAX           ; X = index * 2
$D28F  FC 55 D2     JSR ($D255,X)         ; table of 16-bit JSL stubs at $D255
$D292  C2 20        REP #$20
$D294  A9 00 00     LDA #$0000
$D297  02 00        COP #$00
$D299  A5 14        LDA $14
$D29B  10 EA        BPL $D287             ; exits when bit 7 of $0014 is set
$D29D  E2 20        SEP #$20
$D29F  A5 B3        LDA $B3               ; $00B3
$D2A1  29 7F        AND #$7F              ; <== CLEARS BIT 7 OF $00B1
$D2A3  85 B1        STA $B1               ; $00B1
$D2A5  C2 20        REP #$20
$D2A7  A9 01 00     LDA #$0001
$D2AA  85 12        STA $12               ; <== 16-bit store: $0013=$00, $0012=$01
$D2AC  E2 20        SEP #$20
$D2AE  A9 FF / 8D 2A 0B   LDA #$FF / STA $0B2A
$D2B3  A9 00 / 48 / AB / 6B   LDA #$00 / PHA / PLB / RTL   ; bank $03's last instruction

CODE_008061  "Init_Hardware"  (lle_only: cop_at_00806C, structural_poison)
$8061  20 88 82     JSR $8288              ; $2100=$8F, $0060-$0064 palette regs
$8064  20 90 86     JSR $8690              ; VRAM clear, 32768 iterations
$8067  C2 20 / A9 01 00 / 02 00   REP #$20 / LDA #$0001 / COP #$00
$806E  20 5F 82     JSR $825F              ; installs the CODE_038000 hook at $1F7D..F
$8071  E2 20 / 20 BE 96 / 22 C8 C6 01 / 20 1C 96
$807D  C2 20 / A9 80 00 / 8D BF 02   REP #$20 / LDA #$0080 / STA $02BF
$8087  AD 95 01 / 29 08 / …  STA dp $04
$8096  A9 81 / 85 B1 / 85 B3 / 8D 00 42   LDA #$81 / STA $B1 / STA $B3 / STA $4200
$809F  C2 20 / A9 00 00 / 02 00   REP #$20 / LDA #$0000 / COP #$00   -> $930D vblank wait
$80A6  C2 20 / A9 00 00 / 02 00   REP #$20 / LDA #$0000 / COP #$00   -> $930D again
$80AD  22 07 89 01  JSL $018907
$80B1  60           RTS                    <== 0 executions. THIS IS THE BUG'S EDGE.
```

`$00:8211` is the COP handler (`recomp/bank00.cfg:16`, vector `$00:FFE4` = `$8211`):

```
$8211  58 / 8B / F4 00 00 / AB / AB   CLI / PHB / PEA $0000 / PLB / PLB   (DB = 0)
$8218  C2 20 / C2 10 / 0A / AA       REP #$20 / REP #$10 / ASL A / TAX
$821E  FC 23 82                       JSR ($8223,X)      ; dispatch, table at $8223
$8221  AB / 40                        PLB / RTI
```

`$00:8223` dispatch table: `+0 $930D` (vblank wait `STZ $B9 / INC $C7 / LDA $B9 /
BEQ / RTS`), `+2 $86A4` (WRAM clear: `$7E2000-$7E21FF = $80`, `$7E2200-$7E221F = $55`),
`+4 $8EA9`, `+6 $8E43`, `+8 $8E75`, `+$E $9479`, `+$10 $90DD`, `+$12 $8F82`, `+$14 $86C8`.

`$00:80B2` is the NMI handler (`recomp/bank00.cfg:14`, vector `$00:FFEA`):

```
$80B2  78 / E2 20 / 48 / AF B1 00 00 / 30 04   SEI / SEP #$20 / PHA /
                                              LDA $0000B1 / BMI $80C0
$80BC  E6 B9 / 68 / 40                        INC $B9 / PLA / RTI     <- short path
$80C0  68 / C2 30 / 0B / 8B / 48 / DA / 7A     PLA / REP #$30 / PHD / PHB / PHA / PHX / PHY
$80C8  A9 00 00 / 5B                          LDA #$0000 / TCD        <- D = 0
$80D0  AD 10 42 / A5 12 / D0 03 / 4C B5 81    LDA $4210 / LDA $12 / BNE $80DA / JMP $81B5
$80DA  JSR $BADA, $8C28, $8CDD, $8707, $833A, $9318, $8A14, $8C42, $8397, $8851, …
```

**`$0012` is read in exactly two places, and `$00:80D3` is the second: with `D = 0`
set by `$00:80CB TCD`, it is unambiguously WRAM `$0012`, and it gates the NMI
handler's entire frame-work path.**

## 3. Full census of WRAM `$0012`: writers and readers

**Writers — exactly two, in 3400 frames, measured** (`SNESRECOMP_WLOG_ADDR="0010:001F"`
+ `SNESRECOMP_WLOG_STATE=1`, which routes through `cpu_write8/16` in
`snesrecomp/runner/src/cpu_state.c:498,576` and therefore sees **both** engines):

| frame | site | width | value | D | DB | M | A | `IPC=` |
|---|---|---|---|---|---|---|---|---|
| 0 | `$00:8023` (`95 00`, X-indexed DP store) | w1 | `$00` | `0000` | `00` | 1 | `0000` | `008023` |
| **3271** | **`$03:D2AA STA $12`** | **w1 ×2** | `$0013=$00`, `$0012=$01` | **`0000`** | `03` | **0** | `0001` | `03D2AA` |

`D=0000` on the f3271 line is the measurement that turns "`$0012`" from an
assumption into a fact. Nothing else writes `$0010-$0012` at any frame.

⚠ **Scope of that census, stated rather than glossed:** the watch matches on the
16-bit address regardless of bank, it covers `$0010-$001F` only, and **DMA/HDMA
writes do not pass through `cpu_write8/16`** and would not appear. The static half
of the census below is what closes the rest.

**Readers — two, both direct-page with `D = 0`:**

| address | instruction | what it gates |
|---|---|---|
| `$00:804D` | `LDA $12` / `BNE $805C` | main loop: `$12 == 0` → `JSL $03D283`; `$12 != 0` → `JSR $8061` |
| `$00:80D3` | `LDA $12` / `BNE $80DA` | NMI handler: `$12 == 0` → `JMP $81B5` (short path) |

**`$0012` is a plain one-byte flag, not a wider quantity.** `REP #$30` at
`$03:D285/$D287` makes the scheduler's frame 16/16-bit, so `STA $12` at `$03:D2AA`
is the only place the pair is touched as a word; both readers are **8-bit** loads
of the low byte, and `$0013` participates in nothing the scheduler tests.

**Static cross-check, and a false positive it killed.** A raw 512 KB byte scan for
any DP-mode opcode with operand `$12` returns **236 candidates**, of which exactly
two survive a length-aware decode in banks `$00-$03`: `$00:804D` (a real `LDA dp`)
and `$03:D2AA` (a real `STA dp`). The rest are data: `$00:8C05 TSB dp $12` is
**false** — a sliding window over `$00:8BFC` reads `C5 04 / 12 C4`, and the `85`
that produced the hit is the *tail* of `12 C4` one byte earlier. `bank_03_894C`,
`_9AD7`, `_9C11`, `_9CDF`, `_9DCA`, `_A350`, `_AFB0` all show
`cpu->D + 0x0012` in `src/gen/*.c` and all of them open with `cpu->D = cpu->A`
(`TCD`) and close with `cpu->D = cpu_read16(…)` (`PLD`) — **their `$0012` is their
own stack frame, not WRAM `$0012`**. Reading that generated-C census as a `$0012`
census would have produced seven phantom writers.

## 4. The peer, same census, same instrument family — and the falsifier

Reference build, private study, no source vendored. The instrument is the existing
local-study write-watch (a patch to the throwaway clone at `~/peers/jj`, described
in `docs/measurements/2026-10-02-t093-peer-0B51-writer.md` §Method, keyed on the
**WRAM index** via the core's own `sc_wram_index()` rule, logging frame,
`bank:pc`, `dbr`, bus address, WRAM index, value, `A`, `D`, `X`).

**Pre-registered control, fired, in its own run:** watching `$0B51-$0B52`.

```
RESULT failed=0 frames=9000 master_clock=3216243544 insns=107365572 sram_dirty=1
first $0B51 increment  f3857  by $03:8029          (27 increments; zeroed f2985)
```

That is T093's known-good **to the frame and to the instruction count**
(`3216243544` / `107365572`), so the instrument and the route are proven alive and
the census below is interpretable. A census whose control is silent would have been
void and would have been reported as void.

**Peer census of WRAM `$0010-$001F`, 9000 frames — exactly two writes to `$0012`:**

| frame | site | WRAM index | value | `A` | `D` |
|---|---|---|---|---|---|
| 0 | `$00:8023` | `$00012` | `$00` | `$0000` | `$0000` |
| **2998** | **`$03:D2AA`** | `$00013` then `$00012` | `$00` then `$01` | `$0001` | `$0000` |

**⇒ THE GATE IS FALSIFIED. `$0012` is `1` from f2998 in the peer and from f3271 in
ours, and it is never cleared in either build — while the peer's bank `$03` keeps
running.** A flag that never returns to `0` cannot be the gate for something that
must happen every frame. **`$0012` is a one-shot latch, and it is not the answer.**

### The `$0014` scheduler trace is the same sequence in both builds

`$0014`/`$0015` are the scheduler's round-robin index and its bit-7 exit flag.
Ours is from `IPC=` in the write-watch; the peer's is `bank:pc` **after** the
instruction, hence the constant +2/+3 offset on the same rows.

| `$0014` | ours: frame / `IPC` | peer: frame / `pc` |
|---|---|---|
| `$01` | 223 / `$059330` | 229 / `$05:9332` |
| `$02` | 251 / `$03D301` | 251 / `$03:D303` |
| `$03` | 316 / `$02BC8C` | 317 / `$02:BC8E` |
| `$04` | 503 / `$03D36E` | 501 / `$03:D370` |
| `$05` | 1093 / `$03D3B4` | 1108 / `$03:D3B6` |
| `$06` | 1109 / `$03D431` | 1196 / `$03:D433` |
| `$07` | 1164 / `$03D8B8` | 1251 / `$03:D8BA` |
| `$08` | 2636 / `$03D8DF` | 2838 / `$03:D8E1` |
| `$09` | 2637 / `$03D962` | 2839 / `$03:D964` |
| `$15` | 2939 / `$03D9C5` | 2903 / `$03:D9C7` |
| `$16` | 2940 / `$03DA24` | 2904 / `$03:DA26` |
| `$00` (with `$0015=$80` → `$8000`) | 3270 / `$03DA9E` | 2997 / `$03:DAA0` |
| **`$0013 = $0001`** | **3271 / `$03D2AA`** | **2998 / `$03:D2AC`** |

**Twelve identical milestones, same order, same writers, same values, in both
builds — only the frame numbers drift (ours runs ≈8% slower in guest-time per
frame).** Two consequences, and the second is the important one:

* **`$03D283` runs ONCE in the peer too.** The peer's continuing bank-`$03`
  activity (`$03:8026` ticking 27 times from f3857, `$03:DB76` writing `$0010`
  1717 times over f1252–f2968) is reached by **some other path**. So
  "`$00:804D` → `JSL $03D283`" was never the per-frame engine in either build;
  it is city-creation, once.
* The divergence is **after** f3271/f2998 and **not** in `$0012`, `$0014`,
  `$03D283` or `$00:804D`, because all four are identical in both builds.

## 5. Why `$00:804D` stops: `$00:8061` runs once and never returns

`SNESRECOMP_COUNT_PC`, 3400 frames, `scripts/d_city.script`, each run carrying its
own embedded positive control (the `$0012` write-watch, which must show its two
writes including the f3271 one) and each reading `exit: RUN_FRAMES reached`:

| watched PC | executions / 3400 frames | embedded control |
|---|---|---|
| `$00:804D` (`MainLoop`) | **2** | `$0012` = 2 writes, f0 + f3271 |
| `$00:8061` (`Init_Hardware`) | **1** | `$0012` = 2 writes, f0 + f3271 |
| `$00:8076` | **1** | `$0012` = 2 writes, f0 + f3271 |
| `$00:009311` (the run's own known-good) | 6 804 046 = 2001.2/frame | — (runbook known-good 2007.9/frame at 3300; the rate is wall-clock dependent) |

The two `$00:804D` executions are the boot fall-through and f3271. The third can
never happen because `$00:8061` never gets to its `RTS`.

Frame map from `SNESRECOMP_INTERP_TRACE_FRAMES=3271-3400` — 1 461 215 `[itb]`
lines, execution-ordered, per frame:

```
f3271  $804D $805C $8061 $8064 ...            enter the loop, enter Init_Hardware
f3271  $03D2AA  STA $0012 = 1 ; $03D2A3  STA $B1 = $B3 & $7F ; $03D2B7 RTL
f3271  $804D $805C $8061 ...                  $0012 is now 1 -> BNE taken
f3272-3337   NMI: $80B2 $80BA $80BC $80BE $80BF     SHORT path only, 66 frames
f3277  $806C COP -> $8211..$8222 RTI ; $806E $825F..$8287 ; $8071 $8073 JSR $96BE
f3277  $96BE..$96D5 COP #$0008 -> $821E -> NMI $80B2..$80BF      <- last $80BC
f3329  $8076  JSL $01C6C8                     <- 52 frames later, it resumes
f3338  $807A $807D $8096 $8098 $809C $80A4    <- STA $B1 = $81. THE NMI IS RE-ENABLED.
f3339  $80AB  COP -> $930D vblank wait
f3340  $80AD  JSL $018907
        $80B1  RTS                            0 executions, f3271-f3400
f3338-3400  $80DA $80DD $80E0 $80E3 $80E6 $80E9 $80EC $80EF $80F2 $80F5  40x each,
            $8CDD (STA $420B, HDMAEN) 40x -- the NMI frame work is back
f3272-3400  interpreted PCs in bank $03:  ZERO
```

⚠ **`$00:86A4` (the WRAM clear) does not appear in that stream and is not absent.**
It is AOT node `0086A4:M0X0` in `src/gen/program_manifest.json`, and `[itb]` is
interpreter-tier only (C-039c). `$86A4`'s `M0X0` entry state matches the COP
handler's `REP #$20 / REP #$10`, so it ran as AOT. This is stated rather than
inferred because the whole point of the trace is that a missing address means
nothing until its tier is checked.

### The consequence chain, in the order it happened

1. **f3271** — `$03D283` exits, sets `$0012 = 1` **and clears bit 7 of `$00B1`**
   (`$03:D2A3 STA $B1 = $B3 & $7F`).
2. **f3271** — `$00:804D` re-reads `$0012`, branches to `$00:8061`.
3. **f3272–f3337** — with `$00B1`'s bit 7 clear, `$00:80BA BMI $80C0` is **not**
   taken, so the NMI handler does `INC $B9 / RTI` and **none of its ten frame-work
   JSRs** for 66 frames. `$00:8CDD`, the routine that writes `$420B` (HDMAEN)
   from `$00B7`, is one of them and does not run.
4. **f3277** — `$00:8061` has cleared VRAM (`$00:8690`, 32 768 iterations), WRAM
   (`$00:86A4`) and installed the `CODE_038000` hook (`$00:825F` → `$1F7D..F` =
   `00 80 03`).
5. **f3329–f3340** — `$00:8061` reaches `$00:8098 STA $B1` with `$81` and
   **re-enables the NMI's frame work**. From f3338 `$00:80C0` runs, 40 times in
   62 frames, all ten JSRs including `$00:8CDD`.
6. **and still `$00:80B1 RTS` never executes**, so `$00:804D` never runs a third
   time, `$03D283` is never re-entered, and `$03:8026 INC.w $0B51` never ticks.

**So the clock does not advance because the *only* code that can re-enter bank
`$03` never gets its return — and it is not the flag, it is the missing `RTS`.**

## 6. A hypothesis of mine that my own instrument killed before I claimed it

The f3271 write-watch line reads `M=0` (16-bit accumulator) at `$03:D2AA`, and the
store is `STA $12`. The log line's width field said **`w1`** — a single byte. The
peer logged the same instruction as **two** bus writes. My first reading was
"our emulator performed an 8-bit store where the 65816 requires a 16-bit one, and
`$0013` was never written".

**That was wrong, and the same log disproved it one query later.**
`grep '00:0013=' w12.log` returns `f3271  00:0013=00 w1 … IPC=03D2AA`. The store
**was** 16-bit: `interp816_sta` (`snesrecomp/runner/src/snes/interp816.c:850`)
branches on `cpu->mf`, `interp816_writeWord` (`:483`) falls through to two
`cpu_write8` calls when the bridge has no `write_word` handler, and the write-watch
prints one `w1` line per byte. Two lines, not one. `$0013` was written.

Recorded because it is the same shape as eleven measurements in this project: a
clean, confident, wrong answer produced by querying one field of one log line
instead of reading the neighbouring line. **The `[itb]` stream had already shown
`$03D2A7 → $03D2AA` as a 3-byte step, i.e. `A9 01 00 = LDA #$0001`, M = 0 —
the instrument had said so before the hypothesis existed.**

## 7. The year: there is no transform, and `1952` is a property of the poke

`$0B53` is a **plain binary-decimal year**, proved by the guest's own code:

```
$03:C490  AD 53 0B     LDA $0B53
$03:C493  D9 DC C4     CMP ($C4DC),Y        ; 16-bit, absolute-indexed
$03:C4DC  year table:  1900 1900 1900 1901 1901 1905
$03:C49C  AD 55 0B / D9 E8 C4   LDA $0B55 / CMP ($C4E8),Y   ; month table: 1 6 11 1 6 …
```

`$03:C474` is a milestone scanner: it compares `$0B53`/`$0B55` against those two
threshold tables and fires an event. Plain decimal years. `$03:C3AF` does the same
against `$079E`. So **the premise that the HUD does not render the raw decimal
value is false**, and "work out the transform that maps `$076C`→1900" has no
answer to find. (Ledger **R-047**.)

What `1952` actually is: `$0B53` is *also* used as a **divisor** — `bank_03_C500`
(`$03:C500`) reads it and pulls a reciprocal out of a table at `$03:C5B3+Y`, so
`$0B53` indexes tables. A hand-poked `$0FA0` (4000) is far outside every range the
renderer was written for, and the digits it produced are a property of the poke,
not of the encoding.

**NOT ESTABLISHED, and I am not going to guess it:** which routine draws the four
year digits, and why 4000 renders `1952` where 1900 renders `1900`. Two candidate
shapes both fit the two measured points — an 8-bit-low-byte delta
(`display = 1900 + ((lo8(v) − $6C) mod 256)`) and a mod-2048 delta
(`display = 1900 + ((v − 1900) mod 2048)`) — and they **disagree at `$0E6C`**
(3692): 1900 under the first, 3692 under the second. A single poke at `$0B53 =
$0E6C` separates them, and the discriminating measurement is named in §9.

## 8. What was NOT established, stated plainly

* **What re-enters bank `$03` in the peer after f2968.** Not measured. The peer's
  bank-`$03` activity after the scheduler finishes is at `$03:8026` (from f3857),
  reached by a path that is **not** `$00:804D`→`$03D283` (one-shot in both builds)
  and **not** the NMI's ten frame-work JSRs (they run 40× in f3338–f3400 in ours
  and bank `$03` is still silent). **This is now the single open question.**
* **Why `$00:8061`'s `RTS` never executes.** `$8061` resumes after the NMI's `RTI`
  — `$8076` lands at f3329, 52 frames after the stall at `$8073` — so the stall is
  a long suspension inside `$96BE`/its COP, not a lost return address. What
  suspends it for 52 frames, and where it goes between f3277 and f3329, is not
  measured.
* **`$00:86A4` clearing `$7E:2000-$21FF` at f3277** is inferred from the manifest
  disposition, not observed. It matters — that is city-sized WRAM — and it is
  unverified.

## 9. The single next measurement

**A bank-`$03` entry census in the peer.** The existing study patch already logs
`pbr` and `pc` on every WRAM write; widen its window from a single word to all of
WRAM (`T093_WATCH_LO=0x0000 T093_WATCH_HI=0x1FFFF`) over 9000 frames and read the
frame range of every writer with `pbr == $03`. That names the peer addresses that
are still live after f2968, and therefore the code our build never reaches.
**Pre-registered falsifier, same as §4:** the `$0B51` control must still read 27
increments with the first at f3857, or the census is void.

Run on the Deck, foreground, `insns=107365572` is the known-good to reproduce.

## 10. Instruments, and what each one cannot see

| used | for | limit, stated |
|---|---|---|
| `SNESRECOMP_WLOG_ADDR="0010:001F"` + `WLOG_STATE=1` | the `$0012`/`$0014` census | matches the 16-bit address in **any** bank; **blind to DMA/HDMA**; no frame filter |
| `SNESRECOMP_INTERP_TRACE_FRAMES` | frame-exact execution order | **interpreter tier only** (C-039c) — `$00:86A4` and `$00:90DD` are invisible to it |
| `SNESRECOMP_COUNT_PC` + `PHASE_MS=1` | execution counts | interpreter tier only; one PC per run; needs the `0x` prefix (CONF-15) |
| peer study write-watch | the comparative census | WRAM **index** keyed, not bus address; `pc` logged **after** the instruction |

`IPC=` in the write-watch is `g_interp_wlog_pc24`, set only inside
`_interp_run_core` (`snesrecomp/runner/src/snes/interp_bridge.c:2026`), so it is
**stale for AOT writes** — the AOT tag (`bank_03_xxxx`) is the attribution there.
The wlog is opened `_IONBF`, so it survives a force-kill.