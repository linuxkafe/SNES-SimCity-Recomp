# Measurement — T109: the seventeen pictures, what the city's last visible act is, and a RETRACTION of T108

**The question T108 left.** 5 000 presents, 206 picture changes, the last at
**f3381**, then 1 619 bit-identical presents; and `make clock` independently
reports its date crop's last change at **f3378**, which T108 called *"two
instruments, one boundary, within 3 frames"*. **They are not one boundary.
They are two different events, in two different places on the screen, two frames
apart, and T108's reading of them is retracted here.**

**Deck-native** (`ssh deck@steamdeck`), `build-instr`, `scripts/d_city.script`,
**host-only: none.** Four runs; every one carries its own
`COUNT_PC=0x009311` + `PHASE_MS=1` positive control, and the three
3 400-frame runs read it **byte-identically**.

| run | frames | `EXIT` | completion | `COUNT_PC=0x009311` |
|---|---|---|---|---|
| **A** — `SCREENSHOT_DIR` f3360–f3390 + `PRESENT_LOG` | 3 400 | **0** | `exit: RUN_FRAMES reached after 3400 frames` | **6 804 046** = 2001.2/f |
| **B1** — `PRESENT_LOG` only, `SCREENSHOT_FROM=0` | 40 | **0** | `exit: RUN_FRAMES reached after 40 frames` | **0** — see §5, explained and bounded |
| **C** — `WLOG_ADDR="2100:213F"` + `WLOG_STATE=1` | 3 400 | **0** | `exit: RUN_FRAMES reached after 3400 frames` | **6 804 046** = 2001.2/f |
| **D** — `WLOG_ADDR="2100:437F"` + `WLOG_STATE=1` | 3 400 | **0** | `exit: RUN_FRAMES reached after 3400 frames` | **6 804 046** = 2001.2/f |

**On the control:** 2 001.2/frame is inside the established band
(1 929.9–2 007.9/f) and there is **no known-good *total* at 3 400 frames** —
the known totals are 6 626 029 @ 3 300 f and 9 855 088 @ 5 000 f. The
honest comparison is per frame, and it is in band. **Three runs agree to the
unit.**

---

## 1. The seventeen pictures, transition by transition

**31 presents captured** (f3360–f3390 inclusive), one per frame, `P6` PPM,
336×224. `presents.csv` maps `present_NNNNNN` → frame; **NNNNNN is a window
index, not a frame number** (§4). Every transition, measured pixel by pixel:

| frame | changed px | bounding box | what it is |
|---|---|---|---|
| f3360–f3364 | — | — | **identical, and `crc32 = 00000000`, `luma = 0.000`** — the presented frame is **entirely black** |
| f3365 | 27 017 | x 40–295, y 0–223 | **the city appears.** `luma` 0.000 → 3.591 |
| f3366 | 27 017 | x 40–295, y 0–223 | `luma` → 7.278 |
| f3367 | 53 619 | x 40–295, y 0–223 | `luma` → 17.476 |
| f3368 … f3378 | **52 215 each**, 11 consecutive frames | x 48–295, y 7–223 | `luma` 23.342 → 82.423, **+5.9 per frame, every frame** |
| **f3379** | 52 215 | x 48–295, y 7–223 | `luma` → 88.627. **The last change anywhere inside the date crop** |
| **f3380** | **0** | — | **bit-identical to f3379.** Not a "change" |
| **f3381** | **120** | **x 165–178, y 124–136** | **the tile cursor is drawn.** `luma` 88.627 → 88.560 |
| f3382 … f3390 | **0** | — | bit-identical. And it stays identical for the remaining 1 619 presents of the 5 000-frame run |

**So the shape is 16 changes over 17 frames, not 17.** T108's "17 consecutive
changes, f3365 → f3381" counts a frame (**f3380**) in which nothing changed at
all: `crc32` is `5509053c` at **both** f3379 and f3380.

### What is at x 165–178, y 124–136

Pixel values, f3380 → f3381, over the two alternating dither colours of empty
terrain `(181,148,115)` / `(189,173,123)`:

```
(181,148,115) -> (214,189,99)     (189,173,123) -> (0,0,0)
(189,173,123) -> (0,0,0)          (181,148,115) -> (214,189,99)
```

**Nothing was there at f3380 — pure terrain dither. At f3381 a 14×13 yellow
reticle with black corners is drawn over it**, 120 of 182 pixels changed. That is
the SimCity **tile cursor** for the selected tool, and the tool palette on the
left of the same picture reads **`Bulldoze` / `Area` / `$ 1`**.

> **The city's last visible act before the framebuffer goes bit-identical for
> 1 619 presents is: the guest draws its tile cursor, once, at f3381.**
> **Not the date. Not the money. Not the RCI toolbar. A cursor.**

## 2. The burst is one register's brightness nibble — measured at the PPU, not inferred from the pictures

The f3365–f3379 run of ~52 215 changed pixels per frame with a *constant*
bounding box and a *linear* `luma` ramp is a global brightness change, not a
scroll and not a re-render. **`WLOG_ADDR="2100:213F"` + `WLOG_STATE=1` (run C)
shows exactly which register, and it is one byte:**

```
$2100 = INIDISP, bits 0-3 = master brightness, bit 7 = forced blank.
```

| frames | `$2100` written |
|---|---|
| **f3258 – f3362** | **`$038F` — bit 7 SET: the screen is FORCED BLANK** |
| f3363 | `$0300` — unblank, brightness 0 |
| f3364 | `$0301` |
| … one step per frame … | |
| **f3378** | **`$030F` — brightness 15, full** |
| f3379 → f3399 | `$030F`, unchanged |

**The measured `luma` column tracks it exactly**: 0.000 while blank (f3360–f3364),
3.591 at f3365 (brightness 2), rising ~5.9/frame to 82.423 at f3378
(brightness 15), and 88.627 at f3379 once the last of the picture is in.

**The same register is ramped seven times in one run** — f300→f505 up,
f506→f520 down, f565→f580 up, f1132→f1146 down, f1148→f1163 up, f3244→f3258
down, **f3363→f3378 up**. **Menus fade the same way the city does.**
**[MEASURED, Deck-native, from run C's register log — same run, no new one.]**

> ### ⚠️ **Therefore `make clock`'s "last change" is the end of a FADE, not a date advance.**
> Hashing the gate's own crop (x 55–125, y 2–21) over the 31 pictures: it
> changes on **every** frame f3365…f3379 and **never again**. Its last change is
> **the last frame of the game's own brightness ramp.** The date glyphs did not
> move; the light changed.
>
> **This does not weaken the gate.** `make clock` requires ≥2 distinct date
> images **after f3600**, and after f3600 there are none — which is the correct
> verdict for a frozen city, and the gate must stay red. **What is wrong is the
> number the gate prints.** `last change f3378` is a *fade* boundary presented as
> a *date* boundary, and it is also off by one (§4). **Nothing in this section
> says the gate should be weakened, and nothing here weakens it.**

## 3. ⚠️ RETRACTED (T109): "two instruments, one boundary, within 3 frames"

T108 §2 wrote:

> *"That lands **inside** the f3365–f3381 burst. **Two instruments, one
> full-framebuffer `crc32` per present, and the gate's own date-crop hash, agree
> on the same boundary to within 3 frames.**"*

**Refuted, and the refutation is a coordinate, not an opinion.** The two
instruments measure **different rectangles of the screen**:

| instrument | rectangle | last change |
|---|---|---|
| `make clock`'s crop | x 55–125, y 2–21 | **f3379** (printed as f3378, §4) — the last frame of the brightness ramp |
| full-framebuffer `crc32` | all 336×224 | **f3381** — the cursor, at **x 165–178, y 124–136** |

**x 165–178 is outside x 55–125. The cursor is invisible to the gate's crop *by
construction*** — the gate's own header says the crop "covers the date and
nothing else". So the full-framebuffer boundary being 2 frames later is not two
instruments agreeing on one thing; it is **two instruments reporting the last
thing that happened inside their own rectangle.** They happen to be 2 frames
apart, and coincidence is not agreement.

**What survives of T108 §2:** the freeze is a short, frame-resolved event at the
end of a fade — **which is now much better characterised than T108 had it** — and
two instruments do independently bracket it. **What does not survive:** the claim
that they measure the same boundary, and the word "agree".

**And one more of T108's legs does not survive contact with §2.** T107 offered,
as a reason that the rendered date is not read live from `$0B53`, *"the
framebuffer is pixel-identical for 3 400+ frames"*. T108 already withdrew that
one (`$0B53` itself never changes). The **remaining** leg — *"a poke of `$0FA0`
into the year field at f4260 left the picture unchanged"* — is also weaker than
it looks, and for a reason this run measures:

> **The picture's last change is f3381. The first cheat poke is at f4025. There
> are 144 frames between them on `d_city.script`, and on `cheat_probe.script`
> (T107's route, last picture change f1459) there are 2 566.**
> **A screen that stopped moving before the poke cannot report that the poke did
> nothing.** So *"the picture did not change"* carries **no information** about
> whether the display reads `$0B53` live. The conclusion may still be true; **this
> evidence does not support it, and this file does not carry it as support.**
> What *does* support it is the §2 register measurement: the date glyphs are not
> redrawn by anything after the city is built (§4), so there is no live path from
> `$0B53` to the screen for a poke to travel.

## 4. ⚠️ `clock-gate.sh`'s frame numbers are **present indices**, and they are one less than the frame

`clock-gate.sh` captures with `SCREENSHOT_FROM=0`, and `SNESRECOMP_SCREENSHOT_DIR`
names files `present_NNNNNN.ppm` where **`NNNNNN` is a counter over captured
presents** (`host_main.c:1663-1665`), **not** the frame. The detector then derives
the frame from the filename (`clock-gate.sh:212`):

```python
frame = int(os.path.basename(f).split("_")[-1].split(".")[0])
```

`presents.csv` sits in the same directory with the true frame, and is not read.

**Measured directly (run B1), `SCREENSHOT_FROM=0`, 40 frames:**

```
present,frame,alpha,crc32,luma
0,1,1.0000,00000000,0.000
1,2,1.0000,00000000,0.000
...
39,40,1.0000,00000000,0.000
```

**`present 0` is frame 1. So `LAST_CHANGE` is `frame − 1`, always.** And the two
independent measurements agree on the same picture: run A's crop hash over its own
window puts the last crop change at **f3379**, and the gate prints **3378**.

> **Every frame number `make clock` prints is off by one, and its one printed
> boundary that is quoted in three documents — "last change f3378" — is f3379.**
> **[MEASURED, Deck-native.]** This changes no verdict: `f3600` → `f3601` is
> still far from both boundaries, and the gate is still red. **It does mean the
> number should not be quoted as a frame.** *Not fixed here* — it is in
> `scripts/`, it is a one-line change to read `presents.csv`, and doing it inside
> a measurement ticket is how this project ends up with two numbers for one
> thing. **Recorded as CONF-24.**

## 5. The positive control, and the one run where it read `0`

Run B1's control read **`0` over 40 frames.** Explained and bounded, not waved at:
`$00:9311` is the vblank spin body, and `scripts/d_city.script`'s first
instruction is **`wait 250`** — the guest has not reached its main loop at f40.

Falsified by measuring the ramp rather than asserting it:

| frames | `COUNT_PC=0x009311` |
|---|---|
| **40** | **0** = 0.0/f |
| **200** | **67 382** = 336.9/f |
| **400** | **544 150** = 1 360.4/f |
| 3 300 (known-good) | 6 626 029 = 2 007.9/f |
| **3 400 (runs A, C, D)** | **6 804 046** = 2 001.2/f, three times identically |

**So the control has no known-good value below ~f250, and any run shorter than
that which reports `0` is reporting the script's `wait`, not a dead instrument.**
This is the first recorded instance in this project of the control reading `0`
for a *legitimate* reason, and it is the reason the rule is *"reproduce the
known-good value **or explain the difference**"* rather than *"it must be
non-zero"*.

## 6. Where the rendered date comes from — **NOT ESTABLISHED**, and here is exactly why

The brief asks for the tilemap/sprite source of the four date digits and the
month name. **It is not established here, and the reason is measured, not
skipped.**

**What IS measured (run D, `WLOG_ADDR="2100:437F"`, 3 400 frames, 1 050 742
logged writes):**

| | measured |
|---|---|
| **CPU writes to `$2119` (VMDATA) — the only way a CPU can put a byte in VRAM** | **2 082, and every one is to bank `$7E`, i.e. WRAM. ZERO to bank `$00`** |
| CPU writes to `$2155` (HDMAEN) | 2 079, **all to bank `$7E`** — the shadow, not the register |
| **HDMA/DMA channel registers `$4300-$4306` (ch 0) and `$4310-$4315` (ch 1)** | **thousands of writes, bank `$00` — real.** `$4304/$4305` and `$4314/$4315` exist only on HDMA channels |
| PPU register writes in the steady state (f3355–f3390, bank `$00`) | **~29 per frame**, and **not one of them is `$2117` or `$2119`** |

> **So VRAM is filled by HDMA on channels 0 and 1, and the CPU never writes the
> VRAM data port once in 3 400 frames.** ~~The date glyphs were uploaded during
> city creation and are thereafter never rewritten — which is exactly why no poke
> can move them, and it is a *stronger* statement than T107's.~~
>
> ### ⚠️ **RETRACTED (R-045), 2026-10-03 by T111. The measurements above stand;
> the inference drawn from them does not.**
>
> The date tiles **are** rewritten, and a poke **does** move them. Measured
> inside the live window, compared frame by frame against an identical no-poke
> run: `$0B53` → `$0FA0` renders **`1952 JAN`** instead of `1900 JAN` (36 px,
> x 73-88, y 12-19), and `$0B55` → `$0005` renders **`1900 MAY`** (100 px,
> x 97-120, y 12-19). Both land within **one frame** of the poke, at f3365.
>
> **The two measurements in this section are not in conflict with that — they
> are the mechanism.** The CPU never writes `$2119`, **and** VRAM changes,
> because HDMA writes it. "The CPU never wrote VMDATA" was read as "VRAM never
> changed"; that inference is what fails.
>
> > **A register-write census bounds what the CPU did. It says nothing about
> > what HDMA did.**
>
> This also refutes the *conclusion* T107 was reaching (that the rendered date
> does not come from `$0B53`): it does. T107's observation was void because
> every one of its pokes landed after f4025, on a screen bit-identical since
> f3381 — T109's own §3. Full data:
> `docs/measurements/2026-10-03-t111-date-display-path.md`.

**A near-miss worth recording, because it is this project's own recurring trap.**
`WLOG_ADDR` filters on the **16-bit address only, ignoring the bank**
(`cpu_state.c:157-159`), so the range `2100:437F` also captured a **WRAM shadow
of the PPU register block at `$7E:2100-$213F`**, written every frame by
`PPU_Bitpack_8EA9_M0X0`. Read carelessly, `7E:2100=0F` is a beautiful
`INIDISP = $0F`; it is **a byte written to WRAM**. The real `INIDISP` writes are
the `00:2100=` ones and they carry the bank. **Every claim in §2 is from bank
`$00` lines only.** *This is CONF-20's shape again — a filter that matches a
string without matching the thing the string is about — and it is recorded here
because it nearly became a false PPU measurement.*

**The 32 768-write storm at f3271–f3277 is a DELAY LOOP, not a VRAM upload.**
~4 822 `00:2118` writes per frame for seven frames, **32 768 in total, every one
of them the value `$0000`, with `Y` counting down from `$8000` to `$0000`.** The
ROM agrees (`LoROM`, file offset `0x00690`):

```
$00:8690  C2 10        REP #$10
$00:8692  E2 20        SEP #$20
$00:8694  A2 00 00     LDX #$0000
$00:8697  A0 00 80     LDY #$8000
$00:869A  8E 16 21     STX $2116      ; VMADDL = 0
$00:869D  8E 18 21     STX $2118      ; VMADDH = 0    <-- 32768 times
$00:86A0  88           DEY
$00:86A1  D0 FA        BNE $869D
$00:86A3  60           RTS
```

**32 768 writes × 2 bytes = 65 536 = exactly the size of VRAM.** A write census
alone would have reported *"a full 64 KB VRAM upload on the frame bank `$03`
dies"*, which is a very good story and completely false. **This is C-054's /
R-034's shape for the third time: a loop that writes N times is not a transfer.**
The only real work in f3271–f3277 is at `$00:86AD`, which clears two 64-byte WRAM
blocks (`$7E:2000-$203F`, `$7E:2100-$213F`) to `$80` — the game's own
PPU-register and scroll shadows.

**The concrete next step, fully specified:** HDMA channel 0's table pointer is
`$4302/$4303` and channel 1's is `$4312/$4313`, and **both are written every
frame**, so the pointer's value at any frame is one `WLOG_ADDR="4300:4315"` run
away; then `SNESRECOMP_WRAM_DUMP_AT=<frame>` (decimal — CONF-22) plus
`WRAM_DUMP_LO=0x7E00`-style bounds reads the table itself. An HDMA table entry is
`header` (bits 0-6 line count, 0 = 256; bit 5 indirect; bit 7 do-not-repeat)
followed by `line_count+1` data bytes, so the entry covering the HUD scanlines
names the WRAM offset — and, if that entry is an indirect pointer, the ROM/WRAM
base it points into. **That is one run and it is the next thing to do.**

## 7. Not claimed

- **Not a cause for C-006.** C-006 stays **OPEN**. This is a description of what
  the picture does, not of why the city does not simulate.
- **Not that the freeze is one event.** There are at least three, all measured:
  bank `$03` stops executing at **f3271** (C-039b); the city-state block stops
  being written at **f3259** (C-068); the screen is forced blank **f3258–f3362**
  and unblanks into a brightness ramp that ends at **f3378**; and the last pixel
  written anywhere is the cursor at **f3381**. **Whether any two of those are the
  same boundary is not measured.**
- **Not that the rendered date comes from `$0B53`, or that it does not.** §6
  establishes that **nothing redraws the date glyphs after the city is built**,
  which is a stronger and different statement, and it is the one carried forward.
- **Not a re-measurement of `make clock`.** The gate was not run. It is red and
  it must stay red — DoD Rule 0b.