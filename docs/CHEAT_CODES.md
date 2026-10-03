# Cheat codes — verified, not vendored

**What this file is.** A record of a **third-party, community** SNES
Pro Action Replay / Game Genie code table for this ROM, checked **byte by byte
against this project's own measurements** and tagged with what survived.

**What this file is not.** It is not a copy of the published table, and it is
not a feature. Nothing here is implemented, nothing here is enabled in any
shipped build, and no gate runs any of it. Cheat codes for a user-owned
cartridge are functional data; **the published table is someone else's content
and is therefore a set of hypotheses about addresses, not an input.**

Measured Deck-native 2026-10-03 (T107). Full transcript:
[`docs/measurements/2026-10-03-t107-cheat-verification.md`](measurements/2026-10-03-t107-cheat-verification.md).

---

## ⚠️ THE HARD RULE, stated where a future session cannot miss it

> ## A cheat must never make `make clock` pass.
>
> `make clock` must keep requiring the clock to advance **with no cheats and no
> pokes, on a stock build, in a stock configuration.**
>
> **If a cheat or a poke turns the gate green, that is a DIAGNOSTIC, not a
> fix.** It tells us *which flag holds the gate*. It is not evidence that the
> game works, it is not evidence that the emulation is faithful, and it must
> never be reported as either. The gate stays red until the clock advances on
> its own.
>
> This is [`docs/DEFINITION_OF_DONE.md`](DEFINITION_OF_DONE.md) **Rule 0** made
> concrete, and it is enforced mechanically by
> [`scripts/check-cheat-gate.sh`](../scripts/check-cheat-gate.sh)
> (`make check-cheat-gate`), which fails if any gate script acquires the
> ability to write guest memory.

---

## The format hypothesis, and how far it is confirmed

Game Genie SNES codes are normally **XOR-obfuscated**. The reading tested here
is the **plaintext** one:

```
7E AA BB CC      ->  write $CC to the 16-bit WRAM address $AABB
```

**The plaintext reading is CONFIRMED for the three entries that name an address
we can watch, and it is confirmed on a detail that would have been invisible
under a looser test: the byte order.** Decisively, it is confirmed **from the
outside** — the source of this table has no access to this project's
disassembly, its symbols, or its WRAM maps, and it still lands on the right
addresses.

**Independent confirmation of little-endian order, from our own run:** the probe
wrote `$A0` to `$0B53` and `$0F` to `$0B54` as two separate 8-bit writes, and
the intermediate state is observable:

| frame | `$0B53` | `$0B54` | 16-bit at `$0B53` |
|---|---|---|---|
| 3900, 4020, 4030 (before) | `$76` | `$07` | `$076C` = **1900** |
| 4140 (after the first poke only) | **`$A0`** | `$07` | **`$07A0`** |
| 4260 onward (after both) | `$A0` | **`$0F`** | **`$0FA0` = 4000** |

**The low byte went to `$0B53`.** Under a big-endian reading the same two codes
would have produced `$A00F` = 40 975, not 4000, and the intermediate would have
been `$760F`. **The byte order is measured, not assumed** — and `$076C` = 1900
is independently our own measured year for this ROM, which is what ties the
address to the *meaning* rather than to a writable 16-bit field that happens to
be there.

---

## The table, with a verdict on every row

Legend — **VERIFIED**: address, value, width and byte order all confirmed by
measurement, *and* the address's meaning is corroborated by our own reading.
**PARTIAL**: the mechanics are confirmed, the *label* is not.
**UNVERIFIED**: not contradicted, not confirmed. **REFUTED**: contradicted.
**UNRESOLVED**: measured as far as it can be measured here, still open.

### 1 · `7E0B53A0` + `7E0B540F` — "year 4000" → **VERIFIED**

| | |
|---|---|
| decode | `$A0` → `$0B53`, `$0F` → `$0B54`; 16-bit LE `$0B53` = **`$0FA0` = 4000** |
| measured | `$0B53`/`$0B54` went `$076C` → `$0FA0` at the predicted frames, **and persisted to f4899** — the game never rewrote them |
| corroboration | `$0B53` = `$076C` = **1900** is this project's own measured year at city load, from the inside (T101/T102). The address is the year, not a coincidence |
| caveat | **the picture did not change.** See "What the cheats did to the screen" below — this is a property of our build, and it is the reason the *labels* below cannot be settled |

### 2 · `7E0BA520` + `7E0BA64E` — "population 20000" → **PARTIAL**

| | |
|---|---|
| decode | `$20` → `$0BA5`, `$4E` → `$0BA6`; 16-bit LE `$0BA5` = **`$4E20` = 20 000** |
| measured | `$0BA5`/`$0BA6` went `$0000` → `$4E20`, **persisted to f4899** |
| **the tension** | **`$0B9D` is also `$4E20` = 20 000, and this project calls `$0B9D` funds** — measured, 0 in every sample of both builds. So the cheat sets a field to the value funds *already has* |
| label | **UNVERIFIED, and not separable with current instruments.** Either `$0BA5` is population that legitimately starts at 0 while the treasury starts at 20 000, or the published table means `$0B9D` and mis-transcribed the address. **Nothing here settles it and this file does not pretend to** |
| what would settle it | a run in which the city renders and `$0BA5` is observed non-zero **without** a poke. That is C-006 |

### 3 · `7E0B-F9EB` — "start with $49 000" → **UNRESOLVED**

| | |
|---|---|
| decode | `$EB` → `$0BF9` |
| measured | `$0BF9` is real, writable 16-bit WRAM and **read `$0000`** before the poke; after it, `$00EB`, **persisted** |
| the arithmetic fails | **49 000 = `$BF68`.** Writing `$EB` to `$0BF9` yields `$00EB`, not `$BF68`. The brief's own arithmetic — `0xBF98 = 49 048` is also not `$00EB` — does not close it either |
| readings still open | (a) a **different field** than the one the label names; (b) an **obfuscated** entry that this table renders in plaintext while others are plaintext too, which would itself be inconsistent; (c) the published entry is simply **wrong** |
| **status** | **Left open deliberately.** The address is now *measured* to be real and writable, which is more than was known before, and the value mismatch is *measured* rather than argued. **No substitute entry is invented here** |

### 4 · `7E03-F501` … `7E03-F50A` — "special buildings" → **UNVERIFIED**, and "bank 3" is **REFUTED**

| | |
|---|---|
| decode | ten codes, values `$01`…`$0A`, **all to the same address `$03F5`** |
| the shape is coherent | one 8-bit field written with ten different values is a **selector**, not ten settings — i.e. a building-type picker. That is consistent, and it is the reading the confirmed plaintext format forces |
| **"bank 3" is refuted** | under the format confirmed in §1–§3, `AA BB` is a **16-bit WRAM address**, so this is **WRAM `$03F5`**, not anything in ROM bank 3. The characterisation in the brief does not survive the format |
| measured | `$03F5` read **`$00`** at f3900 and **still `$00`** at f4780, ~29 frames after the poke was estimated to land |
| ambiguity, stated | the `poke` mechanism is **proven live in the same script, in the same run** — five other pokes landed within the 120-frame dump spacing at their predicted frames. So either `$03F5` is **overwritten within ~30 frames**, or the write is masked some other way. **These are not separated.** A `WLOG_ADDR="03F5:03F5"` run would separate them in one pass and **was not run** |
| region | `$0300`–`$03FF` held **2 non-zero bytes in 256** at f3900 — a sparse area, consistent with scratch rather than with a persistent building table |

### 5 · `DD67-DFAA` / `DE67-DFAA` → **REFUTED as characterised**

**The brief describes these as "RAM-injection codes patching instructions at
`$67DF`". That does not survive contact with the address.**

`$67DF` **cannot be a code address in this ROM.** As a SNES address, bank `$67`
is ROM, and this ROM is **524 288 bytes** — banks `$00`–`$3F`. Under the LoROM
rule this project uses and byte-verifies (`offset = bank*0x8000 + (addr &
0x7FFF)`), bank `$67` maps to file offset `0x338000`, which is **past the end of
the file**. There is nothing there to patch.

Under the standard SNES Game Genie type table, `DD` and `DE` are a
**16-bit and an 8-bit equal-compare freeze pair** on a 16-bit **WRAM** address
— the familiar "freeze the timer" idiom — so these most likely read *"freeze
WRAM `$67DF` when it equals `$AA`"*. **That is a different thing entirely from
patching an instruction.**

| | |
|---|---|
| **verdict** | the *characterisation* is **REFUTED**; the *decoding* is **UNVERIFIED** |
| what `$67DF` is | **not established.** It is below `$6B00`, so it is **outside** the region our own runaway zero-fill overwrote — that much is arithmetic from T105 and is the only thing said about it here |

### 6 · `C28A-AD61` / `E28A-AD61` → **UNVERIFIED**, with one measured connection

`C2` is a 16-bit constant RAM write and `E2` an 8-bit one, so both say *"write
`$61` to WRAM `$8AAD`"*.

**The connection worth recording:** **`$8AAD` lies inside the region our own
emulator destroyed.** T105 traced the `$03C87x` zero-fill to writing `$00` across
WRAM **`$6B00`–`$F8F3`**, and `$8AAD` is inside that range. So in *this build*
there is nothing at `$8AAD` for the cheat to modify — the emulator had already
zeroed it. That is arithmetic over two measurements, offered as a connection and
**not as a claim about what `$8AAD` means**.

---

## What the cheats did to the screen — a result in its own right

**None of them changed the picture.** Over **1 877 consecutive presents**
(f0–f4890) there were **18 distinct picture states, and the last one begins at
frame 1459**. Every poke landed after f4025. So the framebuffer was
**pixel-identical for 3 400+ frames spanning all seven pokes**, including a
write of `$0FA0` to the field this project has independently measured to be the
year.

**This is not evidence that the cheats are wrong** — the WRAM dumps prove they
landed (§1, §2). It is evidence about **our build**: the date on screen is not
read live from `$0B53` each frame. Two consequences, and they matter more than
the cheat table:

1. **No cheat in this table can be verified by its screen effect in this build**,
   which is precisely why row 2's label and row 3's value are left open rather
   than guessed.
2. **`make clock` reads the date off a screen crop.** If the rendered date is
   not derived live from `$0B53`, then the gate and the memory are reading
   different things, and **that relationship is unexamined.** It is named here as
   a lead for C-006 and **no claim is made about it**.

---

## Is PAR / Game Genie support worth implementing? — an honest answer

**No, not now, and not for the reason usually given.**

The usual argument for cheat support is player convenience. That argument does
not apply to this project yet, because **the game does not run**: there is no
city to cheat in (`make clock` red, C-006 OPEN). Cheats for a game that does not
simulate are a UI for a feature that does not exist.

**What the verification actually showed about the format**, which is the part
that would matter later:

| question | answer |
|---|---|
| Is the plaintext `7E AA BB CC` reading right? | **Yes**, confirmed on 3 entries with byte order measured |
| Do the addresses match ones this project measured from the inside? | **Yes** — `$0B53` (year) and `$0BA5` independently; `$0BF9` is real and writable |
| Does a source with no access to our disassembly land on the right addresses? | **Yes**, twice. That is a genuinely surprising result and it is the strongest argument *for* implementing this later |
| Can cheats be applied with what is already in the tree? | **Yes** — `poke <hexaddr> <hexbytes> [hold]` in the script language (`host_main.c:823`). No engine is needed for the `7E` family |
| Does the full GG type set need new machinery? | **Yes.** `DD`/`DE` compare-and-freeze, the `01`/`03`/`05`/`D0`/`D1`/`D3` arithmetic/logic ops, and the `80`–`BF` **ROM-patch** codes are all unimplemented, and ROM patches need a writable-code-page story this project does not have |

**Size estimate, and it is small.** The `7E` family alone is **zero new code** —
it is `poke`, which exists. The arithmetic/logic family is a loop over a small
op table in `host_main.c` beside `ParseHexBytes`: **~80–120 lines**. Compare-and-
freeze needs a per-frame predicate against `g_ram`: **~40–60 lines**. ROM-patch
codes are the only genuinely invasive part, because they must be re-applied on
every frame that can fault the modified page in: **~150–250 lines, plus a
decision about when re-application is correct.** **Total ≈ 300–450 lines in the
submodule, none of it in this repository, and none of it worth writing before
C-006 closes.**

**What would change the answer:** the game simulating, plus a player asking.
Neither has happened.

---

## Provenance and standing

- **No ROM, and no part of one, is committed anywhere near this file.** The
  verification read the user's own cartridge at runtime, on the Deck, and every
  dump was a guest-memory image written to `/dev/shm` and deleted.
- **`study/peer-linux/` was not read** for this work and no third-party emulator
  source was consulted. The Game Genie **type table** referred to in row 5 is
  general published format knowledge, cited as such and **not** treated as
  evidence for anything measured here.
- **The probe is not a feature.** `scripts/cheat_probe.script` is a script, not a
  gate; no `make` target runs it; `make check-cheat-gate` exists precisely to
  keep it that way.
- **Related:** CONF-22 (`SNESRECOMP_WRAM_DUMP_AT` is base-10 and the `0x`
  prefix this project mandates makes it dump frame 0), T107's measurement,
  and the `$04B7` problem below.

### One more thing this work found, which is not about cheats at all

**Our own shipped `SIMCITY_GODMODE` pokes `$7E:04B7`–`$04B9` and calls it
money.** The comment says *"game uses 24-bit at `$7E:04B7`"*.

- **`$04B7` appears nowhere else in this repository** — not in `docs/`, not in
  `README.md`, not in any measurement. It is an address asserted in a source
  comment and never checked.
- **It contradicts the measured address.** This project measures funds at
  **`$0B9D`** — `$4E20` = 20 000, in **every** sample of **both** builds across
  hundreds of samples. `$04B7` is a different address and is not a plausible
  alias of it.
- **So `SIMCITY_GODMODE=1` is a feature that pokes an address nobody has ever
  verified, and it is the only implementation of "infinite money" this project
  ships.** That is rubric **E-06** (unverified tools labelled unverified) and
  **E-01** (a causal/structural claim without a measurement) failing on shipped
  source.

**Not fixed here — deliberately.** The fix is not "change `$04B7` to `$0B9D`";
it is to *establish* which address is money, and the evidence that would do that
(population/funds rendered in a live city) does not exist yet because C-006 is
open. **Recorded, not patched.**