# Measurement — verifying a third-party cheat table against our own (T107, 2026-10-03)

**The question.** A published SNES PAR/Game Genie table for this ROM exists.
It is **third-party community content and unverified**. Two of its entries land
on addresses this project has measured *from the inside*, with the right byte
order, from a source with no access to our disassembly — which is either a
remarkable coincidence or a real signal. **Which?**

**The answer: the plaintext format is CONFIRMED, including the byte order and
from outside.** And the exercise turned up three things nobody asked for: a new
instrument defect, an unverified address in our own shipped source, and a hard
mechanical rule that a cheat must never be able to satisfy the delivery gate.

Deck-native throughout (`ssh deck@steamdeck`), Release `build/`, headless,
`scripts/cheat_probe.script`. **Host-only: none.** ROM md5
`23715fc7ef700b3999384d5be20f4db5`, 524 288 bytes, verified on the Deck.

**Deliverable:** [`docs/CHEAT_CODES.md`](../CHEAT_CODES.md) — the table, tagged
verified / partial / unverified / refuted / unresolved, with the evidence.

---

## ⚠️ ADDENDUM 2026-10-03 (T111): one of this document's conclusions is REFUTED

**The plaintext format finding stands. The byte-order finding stands. The WRAM
transcripts stand. One conclusion drawn from them does not, and it is the one
that mattered.**

This document observed that poking `$0FA0` into the year field at f4260 left the
picture unchanged, and used it to argue that **the rendered date does not come
from `$0B53`**.

> **That conclusion is false. The rendered date is a live, direct function of
> `$0B53` (year) and `$0B55` (month).**

Measured by T111 with the poke placed **inside the live window** and compared
frame by frame against an identical no-poke run:

| poke | rendered date | pixels changed |
|---|---|---|
| none | `1900 JAN` | — |
| `$0B53`/`$0B54` → `$0FA0` | **`1952 JAN`** | 36 px, x 73-88, y 12-19 |
| `$0B55`/`$0B56` → `$0005` | **`1900 MAY`** | 100 px, x 97-120, y 12-19 |

**Why the observation here was void, in one number:** the picture's last change
anywhere is **f3381**, and every poke in this document landed at **f4025 or
later** — on a screen that had been bit-identical for 640+ frames. A frozen
screen cannot report that a write did nothing.

This is the same error this document itself records elsewhere (its `$03F5`
ambiguity), and it is the third time in this project that **a clean measurement
on a frozen screen has been read as evidence about a live one**. The general
rule, now written down because it has cost three measurements:

> **A negative result is only evidence if the instrument was in a state where a
> positive was possible. On a screen that has stopped changing, "nothing
> happened" is the screen's property, not the write's.**

Full data: [`2026-10-03-t111-date-display-path.md`](2026-10-03-t111-date-display-path.md).

---

## 1. Falsifiers and instruments, before the results

| # | stated before running | outcome |
|---|---|---|
| **F1** | the probe script silently no-ops (relative `--script`, or `poke` not honoured) | **did not fire** — five pokes landed within the 120-frame dump spacing at their predicted frames, and an earlier host smoke test showed `[poke]`-equivalent writes. **The `poke` verb exists already**: `host_main.c:823`, `poke <hexaddr> <hexbytes> [hold]`, implemented as `memcpy(g_ram + addr, bytes, count)` |
| **F2** | a run not ending `exit: RUN_FRAMES reached` is void | **did not fire.** All three runs `EXIT=0`, `RUN_FRAMES reached`, 0 `SDL_QUIT` |
| **F3** | every count carries a positive control | **satisfied on all three.** `COUNT_PC=0x009311` → 10 239 582 / 5200 f (1969.2/f), 9 667 063 / 4900 f (1972.9/f, twice). The prior known-good readings are 6 626 029 / 3300 f (**2007.9/f**) and 27 019 166 / 14 000 f (**1929.9/f**). These runs are **in that band and byte-identical to each other**, and are *not* byte-identical to the 3300- and 14 000-frame readings because the frame counts differ — which is the point of the band, and the reason a raw total is not the right comparison here |

**A note on F3 worth keeping.** The control could not reproduce a prior number
*exactly*, because the runs are different lengths. Comparing per-frame is the
only honest comparison, and it lands inside the established spread. **A control
that is compared the wrong way looks like a failing control**, and this project
has retracted enough claims to know what that costs.

---

## 2. The measurement

Sixteen WRAM dumps of `$0B40`–`$0C00` at decimal frame numbers, one run,
`SNESRECOMP_SCREENSHOT_DIR` over the same window, `EXIT=0`, 88.3 s:

```
 frame    0B51 tick    0B53 year   0B55 month   0B9D funds   0BA5 ??pop  0BF9 cheat3
 3900      $0000        $076C        $0001        $4E20        $0000        $0000
 4020      $0000        $076C        $0001        $4E20        $0000        $0000
 4030      $0000        $076C        $0001        $4E20        $0000        $0000
 4140      $0000        $07A0        $0001        $4E20        $0000        $0000
 4150      $0000        $07A0        $0001        $4E20        $0000        $0000
 4260      $0000        $0FA0        $0001        $4E20        $0000        $0000
 4270      $0000        $0FA0        $0001        $4E20        $0000        $0000
 4380      $0000        $0FA0        $0001        $4E20        $0020        $0000
 4390      $0000        $0FA0        $0001        $4E20        $0020        $0000
 4500      $0000        $0FA0        $0001        $4E20        $4E20        $0000
 4515      $0000        $0FA0        $0001        $4E20        $4E20        $0000
 4620      $0000        $0FA0        $0001        $4E20        $4E20        $00EB
 4635      $0000        $0FA0        $0001        $4E20        $4E20        $00EB
 4740      $0000        $0FA0        $0001        $4E20        $4E20        $00EB
 4760      $0000        $0FA0        $0001        $4E20        $4E20        $00EB
 4899      $0000        $0FA0        $0001        $4E20        $4E20        $00EB

bytes differing between the first and last dump:
  0x0B53, 0x0B54, 0x0BA5, 0x0BA6, 0x0BF9      <- exactly the five poked
```

**Five bytes changed in a 1 000-frame window, and they are exactly the five
poked.** The game never rewrote any of them — consistent with T102's finding
that the city-state block is written once at f3259.

### 2a · Byte order is MEASURED, not assumed

`7E0B53A0` + `7E0B540F` was applied as the two 8-bit writes it is: `$A0` to
`$0B53`, then `$0F` to `$0B54`. The **intermediate state is visible at f4140**:

```
$0B53 = $A0,  $0B54 = $07   ->  16-bit $07A0
```

**The low byte went to `$0B53`.** Under a big-endian reading the same two
published codes would have produced `$A00F` = 40 975 with intermediate `$760F`;
under the plaintext reading they produce `$0FA0` = **4 000** with intermediate
`$07A0`. **Only one of those is what the machine did.**

And `$0B53` read **`$076C` = 1 900** before the poke — which is this project's
own independently measured year for this ROM (T101/T102). **That is what ties
the address to the *meaning*, and it is the part a lucky guess would not
produce.**

### 2b · The second entry's address is real, its label is not

`7E0BA520` + `7E0BA64E` → `$0BA5`/`$0BA6` = `$4E20` = 20 000, little-endian,
again confirmed with a visible intermediate (`$0020` at f4380). **The mechanics
are confirmed.**

**The tension, stated and not resolved:** **`$0B9D` is also `$4E20` = 20 000**,
and this project measures `$0B9D` as funds across hundreds of samples of *both*
builds. So the cheat writes the value funds already holds into a field that
reads 0. Either `$0BA5` is population that legitimately starts at 0, or the
table means `$0B9D` and mis-transcribed. **Left open — see §4.**

### 2c · `7E0B-F9EB` — the address is real, the value does not decode

`$0BF9` read **`$0000`** before the poke and **`$00EB`** after, and persisted. So
`$0BF9` **is** real, writable, 16-bit WRAM.

**49 000 = `$BF68`.** The published code writes `$EB` to `$0BF9`, giving `$00EB`.
**The value does not correspond to the claim**, and `0xBF98` (49 048) does not
either. Left **UNRESOLVED** with the address now measured — which is more than
was known — and **no substitute entry is invented.**

---

## 3. What the cheats did to the screen — a result nobody asked for

`md5sum` over **1 877 consecutive presents** (f0–f4890):

```
18 distinct picture states; the last begins at frame 1459.
```

**Every poke landed after f4025. The framebuffer was pixel-identical for
3 400+ frames spanning all of them** — including a write of `$0FA0` into the
field measured to be the year.

This is **not** evidence the cheats are wrong; §2 proves they landed. It is
evidence about our build: **the rendered date is not read live from `$0B53`.**

Two consequences, and they matter more than the cheat table:

1. **No cheat here can be verified by its screen effect in this build** — which
   is exactly why §2b and §2c are left open rather than guessed at.
2. **`make clock` reads the date off a screen crop.** If the rendered date is not
   derived live from `$0B53`, the gate and the memory are reading different
   things. **That relationship is unexamined**, it is named here as a lead for
   C-006, and **no claim is made about it.**

---

## 4. What this does NOT establish

- **Not that `$0BA5` is population.** §2b. Unresolved.
- **Not that `$03F5` is a building selector.** The ten-code family has a coherent
  shape (one address, ten values) but `$03F5` read **`$00` both before and
  ~29 frames after** the poke. The `poke` verb is **proven live in the same
  script in the same run**, so either `$03F5` is overwritten within ~30 frames
  or the write is masked some other way. **Not separated** — a
  `WLOG_ADDR="03F5:03F5"` run would separate them in one pass and **was not
  run.** Recorded as a gap, not a finding.
- **Not what `$67DF` or `$8AAD` are.** `$67DF` is **below** `$6B00` and so
  outside the region T105's runaway zero-fill overwrote; `$8AAD` is **inside**
  `$6B00`–`$F8F3` and therefore sits in memory **our own emulator had already
  zeroed**. That is arithmetic over two measurements and **no more**.
- **Not that the Game Genie type table is right.** The `DD`/`DE` reading used to
  refute the brief's characterisation is general published format knowledge,
  cited as such, and is **not** treated as evidence for anything measured here.
  What *was* refuted is the claim that `$67DF` is a patchable code address, and
  that is arithmetic: bank `$67` maps to file offset `0x338000` in a 524 288-byte
  ROM.
- **The debug menu was NOT reached**, and the reason is concrete — §5.
- **No ROM or part of one is committed.** All dumps were guest-memory images
  written to `/dev/shm`.
- **`study/peer-linux/` was not read.** No third-party emulator source was used.

---

## 5. The hidden debug menu — not reached, and why, with a size estimate

The published sequence needs **controller 2**: Left, A, Right, Y, Up, B, Down,
X, Select, Start, Start, Select, R, R, L, L on the *"See you soon!"* screen.

**Our harness cannot deliver that, and the blocker is one line:**

```
snesrecomp/runner/src/desktop/host_main.c:3774
    uint32 inputs = human | (g_gamepad[1].axis_buttons << 12);
```

`GamepadInfo` (`host_main.c:125`) carries `modifiers` — the **button** mask —
and `axis_buttons` — the d-pad-as-axis mask used by the SNES Mouse shim.
**Line 3774 reads only `axis_buttons` for the second pad.** So controller 2's
**face buttons (A/B/X/Y/L/R/Start/Select) are dropped on the floor.** The script
language's `press` verb sets `g_pad_buttons`, which feeds **player 1** only
(`host_main.c:1690`: `g_input_state | g_pad_buttons | g_gamepad[0].axis_buttons`).

**So six of the fourteen presses the cheat needs — A, Y, B, X, Select, Start —
cannot be delivered at all**, and `R`/`L` are likewise face buttons.

**Size estimate, not done here:** read `g_gamepad[1].modifiers` into the pad-2
half of the joypad word at that line (**~2 lines**, the layout is in
`snes/joypad.h`), plus a script verb that targets pad 2 (**~10 lines**). **Call
it ~15–30 lines in the submodule, plus a verification run.** It is *not* a
one-line change, because the joypad word packs two pads and getting the byte
order wrong produces a controller that answers to the wrong player — a bug that
looks exactly like "the cheat did not work".

**And even with that, the menu may not be reachable**, because reaching
*"See you soon!"* requires choosing END and this project's route never gets
there. **Recorded as the blocker it is, not worked around.**

---

## 6. CONF-22 — `SNESRECOMP_WRAM_DUMP_AT` is base-10, and the `0x` prefix this project mandates makes it dump frame 0

**Measured, and it cost the first run of this ticket.**

The first attempt passed the frame list as `0x0CEC,0x0D2C,…`. Result: **one**
dump file, named `f0`.

```
snesrecomp/runner/src/desktop/host_main.c:1539
        long f = strtol(a, &end, 10);          // <-- base 10, unconditionally
```

`strtol("0x0CEC", …, 10)` consumes the leading `0`, then **stops at the `x`**,
yielding **0**. `end == a` is false, so the loop does not break; the next
iteration stops. `wram_at_n` becomes **1**, holding frame **0** — and the run
produces **one clean, correctly-formatted dump of the wrong frame**, with no
warning.

**This is CONF-15's shape but not CONF-15's bug, and the difference is the
interesting part.** CONF-15 is `strtoul(e, NULL, 0)` reading a bare leading `0`
as octal; the project's remedy is a standing rule in `README.md` instrument trap
8: *any knob documented `<hex>` or `0xADDR` gets the `0x` prefix, always.*
**Applying this project's own rule to this knob is what broke it.** The
mandated prefix silently converts a frame list into frame 0.

| written | parsed | dumps at |
|---|---|---|
| `3900,4020,4030` | 3900, 4020, 4030 | those three ✔ |
| `0x0CEC,0x0D2C` | **0** | **frame 0** ✘ |

**Not fixed here.** It lives in the submodule, and the honest fix is to accept
both forms (`strtol(a, &end, 0)` plus a `0x` retry on failure) rather than to
pick one — but changing a shared parse in a pinned submodule to suit this
project's needs is not a call to make inside a measurement ticket.

**Interim rule, and it is the opposite of trap 8's:** *`WRAM_DUMP_AT` takes
**decimal**, always. Its siblings `WRAM_DUMP_LO`/`_HI`/`_FRAME` use
`strtol(…, 0)` and **do** want the prefix.*

---

## 7. The hard rule, and the guard that enforces it

> **A cheat must never make `make clock` pass.** The gate must keep requiring the
> clock to advance on a stock build with no cheats and no pokes. A cheat that
> turns it green is a **DIAGNOSTIC** — it names the flag that holds the gate —
> and it is not evidence the game works.

Stated in [`docs/DEFINITION_OF_DONE.md`](../DEFINITION_OF_DONE.md) as **Rule 0b**
and in [`docs/CHEAT_CODES.md`](../CHEAT_CODES.md), and enforced by
`scripts/check-cheat-gate.sh` (`make check-cheat-gate`), which fails if any
script under `scripts/` acquires the ability to write guest memory on a run it
drives, or if the rule is deleted from either document.

### Falsified in both directions before committing, with positive controls

| seeded | expected | result |
|---|---|---|
| a script containing `poke 0B53 A0` | detected | **ok** |
| a script with no write verb | **not** flagged | **ok** (positive control) |
| a gate invoking `SNESRECOMP_WRAM_POKE` | detected | **ok** |
| a *comment* naming the knob | **not** flagged | **ok** (positive control) |
| **a dotfile** gate invoking `SIMCITY_GODMODE` | detected | **ok** — after a fix, below |
| `clock-gate.sh` repointed at a script containing `poke` | detected | **ok** — `VIOLATION scripts/clock-gate.sh drives scripts/dirty_city.script, and that script WRITES GUEST MEMORY` |
| restored tree | **green** | **ok**, `RESULT: PASS` |
| `make check-cheat-gate-self-test` | 5/5 | **ok**, `SELFTEST PASS: 5/5` |

### The new guard was wrong twice, and both times the self-test caught it

1. **It fired on the delivery gate's own clean route.** `clock-gate.sh` passes
   `"$PWD/scripts/d_city.script"`; the checker could not resolve the literal
   `$PWD` and reported the gate as unsafe. **A guard that cries wolf on this
   project's own corpus is worse than the hole it closes** — CONF-14's lesson,
   learned again one commit after it was written down. Fixed by resolving through
   a shell-variable / absolute prefix to a repo file.
2. **A leading dot hid a gate script from it.** `scripts/*.sh` does not match
   `scripts/.hidden.sh`, so a hidden script invoking a write knob passed
   silently. **Measured, then fixed** with `dotglob`, and the case is now a
   permanent self-test assertion.
   A third defect was in the *self-test itself*: `"$0" | grep -q …` under
   `set -o pipefail` reported FAIL while the guard was working, because
   `grep -q` exits at the first match and SIGPIPEs the producer. Fixed by
   capturing instead of piping.

**All three were found by the self-test rather than by reading, which is the
only reason the guard is in this commit at all.**

---

## 8. Found incidentally, and not about cheats: our own `SIMCITY_GODMODE` pokes an address nobody has ever verified

`src/gen_stubs.c:70-73`:

```c
/* Keep money at max (999,999 = 0x0F423F, but game uses 24-bit at $7E:04B7) */
g_ram[0x04B7] = 0x3F;  g_ram[0x04B8] = 0x42;  g_ram[0x04B9] = 0x0F;
```

- **`$04B7` appears nowhere else in this repository** — not in `docs/`, not in
  `README.md`, not in any measurement. Measured: `grep -rn '04B7' docs/ README.md
  scripts/` returns nothing.
- **It contradicts the measured address.** Funds are measured at **`$0B9D`**
  (`$4E20` = 20 000, every sample, both builds, hundreds of samples) — a
  different address, and not a plausible alias.
- **`SIMCITY_GODMODE=1` is therefore the only implementation of "infinite money"
  this project ships, and it writes to an address asserted in a comment.**

Rubric **E-06** and **E-01** failing on shipped source. **Recorded, not
patched:** the fix is not "use `$0B9D` instead", it is to *establish* which
address is money, and the evidence that would do that requires a city that
renders. **C-006 again.**

---

## 9. Related

- **C-073 / R-039 / CONF-21** (T105) — the `CPX #imm` width defect. `$8AAD`'s
  connection in §4 is arithmetic over T105's measured zero-fill extent.
- **C-006** — **OPEN and untouched.** Named here three times as the blocker for
  everything this ticket could not settle.
- **CONF-22** — new; `WRAM_DUMP_AT` is base-10.
- **CONF-14** — its lesson applied to the new guard, which cried wolf first.
- **CONF-11** — both new guard checks were seeded on **untracked** files.
- **R-038** — untouched; `$03C87E`/`$03C87F` are unaffected by this ticket.
- **Ledger: 39 rows, 31 refuted.** No row was added by this ticket: nothing here
  retracted a claim. The `$04B7` finding is **recorded**, not retracted, because
  no ledger row ever asserted it.