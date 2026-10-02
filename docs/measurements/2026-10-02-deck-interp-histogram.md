# Measurement — interpreted PC histogram over the whole run (Deck, 2026-10-02)

**Question this answers.** Entry (r) of `docs/RE_CITY_FREEZE.md` bracketed the
interpreted histogram to the live-city window `f3400–f3600` and found zero bank-03
execution there. That left one fact unmeasured, and it is the fact the whole
investigation turns on: **does bank 03 ever execute in this build, during boot or
attract, or not at all?**

**Answer: yes, it executes — 921 distinct PCs and 515 043 interpreted steps over
frames 0–3700.** "The city simulates by another path" is therefore not available
from bank 03 in the live window *because bank 03 is silent there*, and the
silence has a measured boundary: bank 03 executes up to and including f3300 and
executes **zero** steps from f3301 onward.

**Machine and build.** Steam Deck `steamdeck`, gcc 15.1.1, cmake 4.0.3, Zen 2,
8 threads. **The Deck compiled this natively** — it is not a copied binary. The
build carries an **environment-fidelity caveat**: the Deck's SteamOS rootfs is
damaged (503 of 504 glibc headers under `/usr/include` are absent from disk while
pacman reports the package installed), so the build resolves libc headers from a
hand-assembled prefix at `/home/deck/sysroot` via `-idirafter`. Every number on
this page inherits that caveat. Runs are headless with `SDL_VIDEODRIVER=dummy`
and `SDL_AUDIODRIVER=dummy`.

**Binary.** `build-instr/` — Release, `-O3`, plus `-DSNESRECOMP_INTERP_PROFILE`,
which no CMake option in this repository exposes. Every run used
`scripts/d_city.script` (the script that reaches a live city), and
`SNESRECOMP_WRAM_DUMP_AT=3600` so the same run that produced the histogram also
produced the city-loaded proof below.

---

## 1. The command

```sh
cd /home/deck/simcity
env SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy \
    SNESRECOMP_MOUSE=1 SNESRECOMP_SOFT_MOUSE=1 \
    SNESRECOMP_RUN_FRAMES=4000 \
    SNESRECOMP_INTERP_PROFILE_START=0 \
    SNESRECOMP_INTERP_PROFILE_END=3700 \
    SNESRECOMP_WRAM_DUMP=/home/deck/prof-runs/whole/wram/w \
    SNESRECOMP_WRAM_DUMP_AT=3600 \
  build-instr/SimCitySNESRecomp \
    --script /home/deck/simcity/scripts/d_city.script \
    "/home/deck/rom/SimCity (USA).sfc"
```

ROM md5 `23715fc7ef700b3999384d5be20f4db5`, 524 288 bytes, as required by
`rom_identity.txt`. The profile window is a **frame** window: frames
0–3700 inclusive. `RUN_FRAMES` is larger than the window so the run continues
past it and the dump at f3600 lands inside it.

Result: `RC=0`, `simulations=4000 presentations=4000 seconds=151.202`,
`exit: RUN_FRAMES reached after 4000 frames`.

## 2. City-loaded proof from the same run

Read from `wram.f3600.bin` (128 KiB), two independent runs agreeing:

| address | value | meaning |
|---|---|---|
| `$0B53` | `076C` | absolute year = **1900** — a city object exists |
| `$0B55` | `0001` | month = January |
| `$0B9D` | `20000` | treasury, the exact `$20000` the HUD shows |
| `$00B9` | `01` | the vblank token **survives** the frame boundary |
| `$00C7` | `C8` | the per-frame counter inside the spinlock advances |
| `$0012` | `01` | not 0 — the retracted "$0012 is the gate" premise stays refuted |
| `$0014` | `8000` | bit 7 set — also refutes that premise |
| `$0B51` | `0000` | the master city tick has not advanced |
| `$0DC7` | `0000` | two instructions after `INC $0B51`; also dead |

## 3. The histogram, per bank, whole run

`[coverage] per-bank LLE (distinct PCs / steps)`, frames 0–3700:

| run | profile window | distinct PCs | `$00` | `$01` | `$02` | `$03` | `$04` | `$05` | `$06` | `$07` |
|---|---|---|---|---|---|---|---|---|---|---|
| whole | 0–3700 | 6055 | 1822 / 26 856 340 | 1814 / 5 730 728 | 542 / 601 366 | **921 / 515 043** | 0 / 0 | 956 / 1 510 936 | 0 / 0 | 0 / 0 |
| boot | 0–200 | 916 | 478 / 1 664 752 | 0 / 0 | 0 / 0 | **10 / 10** | 0 / 0 | 428 / 932 195 | 0 / 0 | 0 / 0 |
| attract | 201–1200 | 2502 | 494 / 5 336 356 | 575 / 5 411 938 | 542 / 601 366 | **499 / 11 035** | 0 / 0 | 392 / 328 481 | 0 / 0 | 0 / 0 |
| menus | 1201–3380 | 3707 | 1753 / 17 808 456 | 1231 / 56 967 | 0 / 0 | **532 / 503 998** | 0 / 0 | 191 / 250 260 | 0 / 0 | 0 / 0 |
| citywin | 3381–3700 | 1228 | 813 / 2 046 776 | 415 / 261 823 | 0 / 0 | **0 / 0** | 0 / 0 | 0 / 0 | 0 / 0 | 0 / 0 |
| w3k | 3000–3300 | 1026 | 566 / 2 834 471 | 0 / 0 | 0 / 0 | **333 / 169 693** | 0 / 0 | 127 / 51 405 | 0 / 0 | 0 / 0 |
| w31 | 3100–3300 | 1026 | 566 / 1 923 612 | 0 / 0 | 0 / 0 | **333 / 160 693** | 0 / 0 | 127 / 30 253 | 0 / 0 | 0 / 0 |
| pre | 3300–3380 | 2790 | 1559 / 872 464 | 1231 / 56 967 | 0 / 0 | **0 / 0** | 0 / 0 | 0 / 0 | 0 / 0 | 0 / 0 |

Cells are `distinct PCs / interpreted steps`. A zero cell means the instrument
recorded nothing there — see §5 for what a zero from this instrument is and is
not evidence of.

**The brackets partition exactly.** `boot + attract + menus + citywin` sums to the
`whole` row for every bank's step count, to the unit: `$00`
1 664 752 + 5 336 356 + 17 808 456 + 2 046 776 = 26 856 340; `$03`
10 + 11 035 + 503 998 + 0 = 515 043; `$01` 0 + 5 411 938 + 56 967 + 261 823 =
5 730 728; `$02` 601 366; `$05` 932 195 + 328 481 + 250 260 = 1 510 936. Distinct-PC
counts do not sum and are not expected to — the sets overlap.

## 4. What the counts characterise

- **Bank 03 executes.** 921 distinct PCs, 515 043 interpreted steps, before the
  city exists. It is not code the build never reaches.
- **Bank 03 goes silent at f3301.** Live at 3100–3300 (160 693 steps over 200
  frames ≈ 803/frame); **zero** in 3300–3380 and zero in 3381–3700. The city is on
  screen from ≈f3378, so bank 03 stops roughly **78 frames before** the city
  appears, not at the moment it appears.
- **The live-city window is confined to banks 00 and 01** — 813 + 415 distinct PCs,
  2 308 599 steps, nothing in `$02`, `$03`, `$05`. This independently reproduces
  entry (r)'s f3400–f3600 bracket (same 813 and 415 distinct-PC counts) and
  extends it: `$02` and `$05` are zero there too, not only `$03`.
- **Bank 02 executes only during attract** — 542 PCs / 601 366 steps inside
  201–1200, and zero in every other window including boot and the city.
- **Banks 04, 06, 07 never execute** in frames 0–3700.
- **The live-city window's interpreted time is dominated by the vblank
  handshake.** Top PCs by host-ms in 3381–3700: `$009313` (610 206 steps),
  `$009311` (610 210), `$009315` (610 234), and `$0080B2` — the NMI handler —
  at 320 executions ≈ once per frame. That is ~1 800 spin iterations per frame
  alongside one NMI per frame, which is consistent with the token surviving
  (`$00B9 = 01` at f3600, §2) and does not indicate a hang.

## 5. What this instrument cannot see, stated before anyone over-reads it

1. **It is blind to AOT.** `interp_hist_add` is called from inside
   `_interp_run_core`, the per-interpreted-opcode loop
   (`snesrecomp/runner/src/snes/interp_bridge.c:1132`). A bank-03 routine that
   executes as a compiled block contributes **zero** to every number on this
   page. The complement instrument is `SNESRECOMP_AOTBLK`, which takes a **frame**
   window, not a PC range. At `9624f0e` it was measured over f3400–f3600 only
   (206 806 block entries, 30 distinct PCs, banks 00 and 01, zero in `$03`).
   **There was no whole-run AOT histogram** when this page was written, so
   "bank 03 executes no AOT blocks anywhere" was **NOT measured** and was not
   claimed. **It is measured now** — 1,430,539 block entries over f0–f3700 of
   which bank `$03` has 18, all in f3270 — see
   `2026-10-02-c041-bank03-pc-dump.md` §5. Note that this took an instrument
   *build* change as well as a run: `cpu_trace_block()` is a no-op without
   `SNESRECOMP_TRACE=1`, so `AOTBLK` silently logged nothing at all.
2. **`$03:8026` specifically was not resolved by this page.** RESOLVED, elsewhere:
   **it does not execute** — 0 hits in the full per-bank dump on both machines,
   and 0 AOT entries (`2026-10-02-c041-bank03-pc-dump.md`). What this page could
   not do: the dump prints only the top 60 PCs by host-ms, and with
   `SNESRECOMP_INTERP_MS_PROF` unset it prints **none of them** (see §6).
3. **`INTERP_PROFILE` is dev-only.** No CMake option in this repository defines
   `-DSNESRECOMP_INTERP_PROFILE`; this build was configured with
   `-DCMAKE_C_FLAGS="-idirafter /home/deck/sysroot/usr/include -DSNESRECOMP_INTERP_PROFILE=1"`.
   A default `make build` cannot produce this histogram.

## 6. A third instrument trap, found while running this

`[interp_profile] N distinct PCs, top 60 by host-ms` prints **nothing** unless
`SNESRECOMP_INTERP_MS_PROF=1` is set. Without it every entry's `ms` is `0.0`,
`_hist_cmp` returns 0 for every pair, the sort is stable, and the leading array
slots are unused hash-table entries (`PROFILE_HIST_CAP` is 65 536 with only
~6 000 used) — so the `s_interp_hist[i].n` guard fails before 60 entries are
reached. Observed directly: the whole-run dump reported `6055 distinct PCs` and
then printed **zero** PC lines.

This is the same class as entry (r)'s two findings: an instrument that looks mute
and is not, or that looks silent and is merely printing nothing. **A section
header with no rows under it is not a negative result.**

## 7. What this does NOT establish

- **Why** bank 03 stops. Not measured — and the boundary this page reported,
  f3301, was **too coarse**: the real one is f3271 (same page's successor). Any
  cause asserted from this page would be the same error as the fifteen retracted
  claims.
- That the city tick lives in bank 03. That label comes from the reference
  implementation's trace, which is **inferred**, not measured here.
- Anything about AOT execution outside f3400–f3600.
- Anything about frames after f3700, beyond the fact that entry (r)'s settle
  protocol ran 9 000 frames and read the date at every snapshot.