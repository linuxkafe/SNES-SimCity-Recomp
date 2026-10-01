# T060 — the clock does not advance in a running city

Entry into the game is solved (`RE_SCENARIO_NAV.md`). This is the remaining
bug, and it is narrow: **the city runs, animations play, and the date stays at
1900 JAN.**

## A correction worth keeping

The first version of this document concluded that the city started and then
froze with a corrupted screen. **That was wrong**, and the reason it was wrong
is the most useful thing in it.

The evidence was a savestate taken from a running city: load it headlessly and
one byte of WRAM moves in 3600 frames, on a black screen with scattered tiles.
The inference drawn was that the game corrupts itself while creating the city.

But the owner reports the live game does not freeze — the animations keep
running. So the frozen state is **our savestate, not the game**, and every
conclusion drawn from it was about the wrong subject. Two things follow:

- The measurement was invalid. One byte in 3600 frames measures the state
  *load*, not the city.
- **Saving and loading a state from a running city is itself a bug**, separate
  and worth its own ticket. The garbled picture after a load says the state is
  incomplete.

The diagnostic detail gathered along the way still stands as exclusions, and
they are worth having:

- NMI is delivered every frame and `pre-nmi` / `post-nmi` return with the same
  stack pointer and the same resume PC, so the interrupt path is clean.
- Every AOT entry in `src/gen/bank00_v2.c` falls back to authoritative LLE, so
  bank 0 is faithfully interpreted and the T057 class of bug is not in play.
- The resume PC oscillating through `$0084E1-$0084FA` is a 16-bit jump table,
  not a loop. A jump table is not a bug.

## Measuring it properly

`scripts/clock-probe-live.sh` measures the **live** game: it takes two WRAM
dumps by frame number while the city is on screen, plus a screenshot, and
diffs them. No savestate, nothing to trust but the game itself.

```bash
scripts/clock-probe-live.sh                 # dumps at frames 8000 and 14000
scripts/clock-probe-live.sh "$ROM" 12000 20000
```

The screenshot matters as much as the diff: the dumps are taken by frame
number, and if the run was not in a city when they landed, the picture says so
instead of the table quietly explaining a wrong moment.

## What the readings mean

- **Thousands of bytes moving** → the simulation is running and the clock is
  specifically stuck. The slow runs are then the tick candidates.
- **Dozens of bytes moving** → not a live city; the screenshot will show which
  screen it actually was.

The 27-byte baseline is the number to keep in mind: a still title screen, a
still naming screen and a broken state load all move about that much in 3600
frames. A still screen and a dead clock are indistinguishable by byte counts
alone, which is why the probe also screenshots and judges.

## Where the month probably is

Unmeasured, so stated as a guess and nothing more: SimCity advances time from
its NMI handler, once per vblank, against a frame counter. NMI is delivered and
returns clean, so if the month does not move, the handler is running and
deciding not to — gated on a counter, a speed setting, or a paused flag the
port does not carry. That is a question about a specific value, and the live
diff is what would name it.

---

## Two savestate bugs, one fixed

Both were found by measuring rather than by reading, and both had the same
shape: something the snapshot does not carry, which is silent until you look.

### 1. Nothing carried the resume point — fixed

`g_resume_pc` is a static in `src/game_rtl.c`. It is not in the `Snes`, and it is
**not in the CPU either** — logging the restored register right after a load
shows `pc=0000`, so `snes_saveload` does not restore the program counter at all.

So a state restored into a fresh process leaves the resume point at 0, the frame
loop reads `booting = (g_resume_pc == 0)` as true, and the guest is answered
with the reset vector. Measured: resuming at `$008000` — the reset vector — with
`S=01FF` and `DP=0000`, boot-time register values, drawn over WRAM that still
held the city.

The fix puts it in the game's own chunk, which is what `state_save_extra` and
`state_load_extra` exist for. The state grows by exactly four bytes, and the
cross-check is the good part: a state saved on the naming screen now reports
`state loaded: resume PC restored as $009311`, and `$009311` is the address this
project already carries in `recomp/bank00.cfg` as
`force_lle 0x009311  # VBlank wait loop main polling address`. The resume point is
the game's vblank wait, which is where it should be.

A state written before this fix has no chunk and cannot be resumed; it falls
back to the reset vector. Those files are not recoverable, which is why the
city state has to be taken again.

### 2. The snapshot does not carry the render state — open

With the resume point fixed, a loaded state still draws the wrong picture.
Capturing the frame one frame before a save and the frame after the load, both
from the naming screen:

| | CRC |
|---|---|
| before the save | `a3bc4b3bf5` (the naming screen) |
| after the load | `90a7a5ffbd` (not it) |

So VRAM, OAM, CGRAM or the PPU registers are not in the blob, or the load resets
the PPU. This is why a loaded state looked like corruption in the first place,
and it is a separate bug from the resume point.

**It is also the good news for measuring the clock.** The simulation lives in
WRAM, WRAM *is* in the snapshot, and the clock advances in WRAM. So a loaded
city state runs the real simulation with a wrong picture, and the picture is not
what the clock is being measured from.

---

## Determinism has a fourth input, and it is not the ROM

A cross-platform battery (`scripts/crossplatform-determinism.sh`, commit
`82456e7`) ran the same binary on two machines and found the framebuffers and
WRAM byte-identical across 23,340 simulated frames - and one real trap on the way.

**The guest's battery-backed SRAM is a determinism input.** It lives in
`<exe dir>/saves/save.srm`, it is gitignored, it is per-machine, and the game
reads it. On a 3000-frame menu run it moves **44 of 131072 WRAM bytes**, and an
absent image, an all-`0x00` image and an all-`0xFF` image give three different
WRAM hashes. 41 consecutive runs on one machine and 5 on the other each collapse
to exactly one value, so it is contextual, not random.

So **"same ROM, same script, same frame count" is not sufficient** for a
byte-identical WRAM image. The framebuffer is *invariant* to the SRAM - all
three hash the same - which means **no picture-based gate can see this**,
`verify-rom-render.sh` included. Any WRAM-diffing gate has to pin the SRAM cold
or it measures the battery instead of the thing under test.

### A second trap: relative output paths do not fail

The host `chdir()`s to the exe directory. A relative `--script` path now exits 2
loudly, but a relative **output** path - `SNESRECOMP_SCREENSHOT`,
`SNESRECOMP_WRAM_DUMP` - resolves against the exe directory and **succeeds**:

```
SNESRECOMP_SCREENSHOT=relout.ppm  ->  build/relout.ppm
```

No warning, exit 0, and the file is in the wrong place. `clock-probe-live.sh`
had exactly this bug with `DUMP=states/live`; both probes now use absolute paths.

## A correction to the widescreen claim

`README.md` said the emulated picture is "byte-identical either way" with
16:9. **That was wrong, and the battery caught it.** In 16:9 the PPU frame is
256 wide instead of 336, so the *presented framebuffer* is a different width and
cannot hash equal. The substantive claim does hold, and it is the one that
matters: **the guest's own 256 columns are identical** - cross-platform, the 256
columns inside the 4:3 margins hash equal to the whole 16:9 framebuffer, and the
margins are solid black. An earlier version of `test_display_aspect` could not
have caught this, because it only checks the arithmetic and never hashed a
frame.

## Also worth knowing

- The 4:3 window is **1008x672, which is 3:2** - the 7:6 pixel-aspect
  correction is not applied to the window. Pre-existing, and 16:9 (1194x672) is.
- Scripted input is **frame-indexed, not wall-clock**: `TickScript()` runs inside
  the frame loop and `wait` is flushed into the next entry. The earlier
  "naming screen appeared two `B` presses later on the Deck" did not reproduce in
  any form; case C's framebuffer and WRAM are byte-identical across machines.
  Whatever produced that observation was not guest state.
- **The Deck has a full toolchain** - cmake 4.0.3, gcc 15.1.1, make 4.4.1. An
  earlier note in this project asserted it did not, and that assertion was
  wrong: the shell probe that produced it was mangled by quoting, and the broken
  output was reported as fact. A native build on both machines is now available
  as an independent check.

---

## 2026-09-30 — the city is reachable, and the clock is still frozen

**The explanation above was wrong, and it is worth being precise about which
half is wrong.** This document concluded that "the clock did not advance because
the city was never created, and the city was never created because the last two
inputs are pointer inputs that no script can make."

The second half is false. `scripts/d_city.script` reaches a running city,
headlessly, and it is deterministic — two runs, same SRAM, byte-identical WRAM
at frame 7998 (`b2038ffa4a34560b5931c2dbec20f2cf7c04a6b5079298d1961f2d7b484a3c30`).
The route is in the script's own header; the short version is that `mouseclick
right` does reach the guest, `press left` wraps the key list to its end, and
four `press down` walks the hand onto `ENT`.

So the first half now stands alone, and it is the part that matters: **the city
is created, it is on screen, and the clock does not advance.** That makes this
a genuinely different bug from the one this file spent three weeks describing.

### The measurement

20,000 frames — 333 seconds of emulated time — with the city live from ~frame
3600:

| window | WRAM bytes changed |
|---|---|
| f4000 → f6000 | 32 |
| f6000 → f8000 | 28 |
| f8000 → f10000 | 23 |
| f10000 → f12000 | 32 |
| f12000 → f14000 | 27 |
| f14000 → f16000 | 32 |
| f16000 → f18000 | 32 |
| f18000 → f19998 | 21 |

And the date, read off the screen rather than inferred: **`1900 JAN` at frame
4000 and `1900 JAN` at frame 19998.** The population counter in the HUD reads 0
throughout.

One of the 36 bytes that do move is diagnostic:

```
$0406-$0407   u16 639 -> 16637   delta +15998 over 15998 frames
```

That is **exactly +1 per frame** — a frame counter, doing its job. So the host
is advancing frames, the guest is consuming them, and the game is choosing not
to turn them into months. Whatever gates the month tick is not the frame count.

### What this rules out

- **Not a savestate artifact.** This is a live run with no state load anywhere.
- **Not "the city was never created".** It is created; the picture shows it.
- **Not a stopped host.** `$0406` ticks once per frame, every frame, for 20,000
  frames.
- **Not a build or AOT regression.** The city renders, the tool palette is
  drawn, the map is there.

### What it does not yet tell us

Nobody has located the value that gates the month. The four bytes at
`$2510`-`$2516` move slowly (+15, +13, +11, +11 over 16k frames) and are
candidates, but "moves slowly" is the same signature as the idle counters in
`$007C`/`$01B3`/`$01D5` that this file already listed as noise. They have not
been shown to be the date, and this file has a history of reading a slow byte as
a signal. The next step is to find the actual month field — which is a
disassembly question, not another diff.

The most likely place remains what this file already guessed: the NMI handler
runs, the frame counter advances, and the handler declines to convert it into a
month. That guess has now survived one more round of elimination and no more.

---

## 2026-09-30 (later) — it is not a frozen clock. It is a hang.

The section above frames this as "the city runs and the month tick is gated".
That framing is wrong, and the evidence is uncomfortable: **the game hangs.**

### The picture freezes too

Per-present crc32 over 12,000 presents, `scripts/d_city.script`, cold SRAM:

| window | distinct crc32 | changes |
|---|---|---|
| 0–2500 (attract, menus) | 126 | 129 |
| 3000–6000 (city loading) | 34 | 47 |
| **6000–12000 (city "running")** | **1** | **0** |

**The last picture change in the entire run is frame 3382.** For the following
8,618 frames — 143 seconds of emulated time — the rendered image does not
change by one bit.

This file, and T058 before it, both describe a city that "runs, animations
play, and the date stays 1900 JAN". Nothing here animates. The `~4 changes per
1000 frames` figure used elsewhere in this repo as the signature of a slow
city is wrong for this build: the real figure is **zero**.

### And input does nothing

`scripts/d_city.script` followed by 12 rounds of `press right` / `press down` /
`mouseclick right` — 1,700 frames of deliberate input after the city is live:

```
total presents 9000, distinct-change events 148, LAST CHANGE at frame 3534
```

Nothing. The game does not respond to the controller at all.

### The registers prove it

`SNESRECOMP_WLOG_ADDR=0400:0410` with `SNESRECOMP_WLOG_STATE=1` samples the
whole CPU state at every write to the frame counter. Sampled at frames 3400,
3500, 3600, 4000 and 4199:

```
A=0028 X=0008 Y=0000 S=1FE4 D=0000 DB=00 M=0 Xf=0 IPC=0092E3
A=008C X=0008 Y=0000 S=1FE4 D=0000 DB=00 M=0 Xf=0 IPC=0092E3
A=00F0 X=0008 Y=0000 S=1FE4 D=0000 DB=00 M=0 Xf=0 IPC=0092E3
A=0280 X=0008 Y=0000 S=1FE4 D=0000 DB=00 M=0 Xf=0 IPC=0092E3
A=0347 X=0008 Y=0000 S=1FE4 D=0000 DB=00 M=0 Xf=0 IPC=0092E3
```

**`A` is the only register that changes, and it is the counter being
incremented.** `X`, `Y`, `S`, `D`, `DB`, the flag bytes and every stack peek
(`p34`…`p57`) are byte-identical across 800 frames. `S=1FE4` never moves, so
the guest is not entering or leaving a subroutine. The only interpreter PC that
ever executes after the freeze is `$0092E3`.

PPU register writes confirm it: 224,189 writes to `$2100-$213F` before frame
3382, then **23,557 across 817 frames** — and every single one of them is
`$210F`/`$2110`/`$2111`/`$2112` written with the *same value*, twice per frame.
That is an idle loop poking the OAM address register, not a renderer.

`$009313` is the hottest interpreter PC in the whole run (2.3% of 1.7M
samples), and `recomp/bank00.cfg:32` already pins
`force_lle 0x009311  # VBlank wait loop main polling address`.

### What this means

**The game is spinning in its VBlank wait loop.** `$9311` waits for a flag that
something else is supposed to set, and the thing that sets it never runs. The
`$0406` counter is incremented by the NMI path, which is why it ticks — so the
interrupt *is* being delivered — but the main loop never gets past the wait.

That is the same family as T050 ("VBlank wait loop at $00927C forced to
interpreter") and T057 (an AOT function that runs but produces nothing). It is
**not** a clock bug, not a pause gate, and not game flow. It is a livelock
discovered now that the city is reachable.

### What has been eliminated, with the evidence

| Ruled out | How |
|---|---|
| Game flow / city never created | `scripts/d_city.script` reaches a live city |
| Headless environment | Deck run on a real 1920x1080 X display with real PulseAudio: byte-identical WRAM **and** byte-identical screenshot to the headless run |
| Cross-machine divergence | Deck and dev machine WRAM sha256 identical at f11998 |
| Timing / frame pacing | real display and real audio change nothing; the guest is frame-indexed |
| A slow simulation | picture is bit-identical for 8,618 frames; input does nothing |
| A pause or speed gate | forcing `$0408=0` arms `$040A=$8080`, and the consumer at `$01:8A92` then **executes every frame** — and still no month. Opening the gate did not help. |
| The "slow-moving bytes" as the date | `$2510-$251F` is periodic with a ~10,000-frame cycle and is byte-identical at f4000 and f14000. That is animation phase, positively excluded. |

### The actual next step

Not more WRAM diffing. The guest is not looping through game logic, so there is
no slow-moving game state to find — which is why three weeks of diffing a
"frozen simulation" kept producing the idle counters at `$007C`/`$01B3`/`$01D5`.

The question is narrow and mechanical: **what is `$9311` waiting for, and why
does it never arrive?** That is a disassembly of the loop and of whatever is
supposed to set the flag, plus the AOT/LLE boundary question — the fork
documents that *AOT code never advances the PPU beam* while the interpreter
advances it per opcode, and a VBlank wait is exactly the kind of code where
that asymmetry produces a hang rather than a wrong picture.

---

## 2026-09-30 (root cause) — the AOT wait loop clears the token in the same frame the NMI sets it

**Found by comparing against the three games that work on this framework**, then
confirmed here by direct measurement. This is the end of the line for this bug.

### What the peers do differently

Three other games run on snesrecomp and are believed playable end-to-end:
Super Mario World, Zelda ALttP, Mega Man X. Read across all three repos'
`recomp/*.cfg`:

```
force_lle : 0 occurrences
lle_only  : 0 occurrences
```

**Not one working game uses `force_lle`.** SimCity uses eight of them. The
peers all handle the same construct — a per-vblank token byte, cleared by the
main loop and bumped by the NMI — by *excluding the spinlock from AOT and
driving it from the host*:

- **SMW** `recomp/bank00.cfg:18-27`, read verbatim:
  > `# HLE-replacement: the asm main loop at $806B-$8078 is 'LDA $10 ; BEQ
  > $806B ; CLI ; INC $13 ; JSR ProcessGameMode ; STZ $10 ; BRA $806B' — a
  > busy-wait spinlock against an NMI flag. [...] Without this directive, the
  > JMP at the tail of I_RESET auto-promotes a bank_00_806B function whose
  > recompiled body re-enters the spinlock from inside the C frame and hangs
  > the watchdog at frame 0.`
  > `exclude_range 806B 8079`

  and in `src/smw_rtl.c`:
  ```c
  waiting_for_vblank = 0xFF;
  interp_bridge_run_scheduler(&g_cpu, 0x00806B, 0x00806B, 0x0010);
  ```

- **Zelda** `src/zelda_rtl.c:353` — same shape, `interp_bridge_run_scheduler(&g_cpu, 0x008034, 0x008034, 0x0012)`.

- **MMX** `recomp/bank00.cfg:50,108-114` — `hle_func` on the scheduler and the
  vblank-yield, token `$0B9D`.

`interp_bridge_run_scheduler` (`interp_bridge.h:68-73`) runs the *real ROM*
spinlock under the interpreter and yields when it reaches `yield_pc` with the
token byte cleared. The host sets the token each frame.

`snesrecomp/docs/GAME_PROJECT_SETUP.md:91-96` says in as much:
> "A real ROM's reset vector never returns — it enters a main loop that waits on
> vblank — so the host chooses the yield point. The template uses the general
> LLE-first shape [...] which is the right starting point but is not tuned to
> any particular game."

`src/game_rtl.c:239-251` is that untuned template, verbatim. All three working
games moved off it.

### The SimCity construct, decoded from the ROM

```
$930D: E2 20        SEP #$20
$930F: 64 B9        STZ $00B9      ; clear the token
$9311: E6 C7        INC $00C7
$9313: A5 B9        LDA $00B9      ; <- the hottest PC in the whole run
$9315: F0 FA        BEQ $9311
$9317: 60           RTS
```

and the only setter, inside the NMI handler's early-exit branch:

```
$80B2: 78           SEI
$80B6: AF B1 00 00  LDA $00B100     ; ROM byte $A9, N clear -> short path
$80BA: 30 04        BMI $80C0
$80BC: E6 B9        INC $00B9       ; <- the only writer of the token
$80BE: 68           PLA
$80BF: 40           RTI
```

A scan of the ROM for absolute writers of DP `$B9` returns **zero** hits. Those
two instructions are the whole protocol.

### The measurement that closes it

`SNESRECOMP_WLOG_ADDR=00B9:00B9`, every write to the token, tagged with the
function that made it:

```
3383 00:00B9=01  interp@$0080B2          <- NMI sets it
3383 00:00B9=00  bank_00_930D_M0X0       <- AOT loop clears it
3384 00:00B9=01  interp@$0080B2
3384 00:00B9=00  bank_00_930D_M0X0
... 817 frames, exactly 817 of each ...
```

**817 increments and 817 clears, perfectly paired, forever.** The NMI sets the
token and the compiled loop clears it in the same frame, so `$9313` never
observes it non-zero and `BEQ $9311` never falls through. The loop is not
waiting for an event that does not happen — the event happens and is destroyed
before it can be seen.

And the AOT loop should not exist: `recomp/bank00.cfg:32` declares
`force_lle 0x009311`, but `src/gen/program_manifest.json` has

```
00930D:M0X0   aot_eligible   instr=6   reasons=[]
```

`reasons=[]` means the analyser has no opinion, and it compiled the spinlock to
native C. `force_lle` at `$9311` pins one PC inside a function that begins at
`$930D`; the function as a whole is still emitted. That is why it did not
prevent this, and it is the same gap T057 found in `bank_00_8D65_M1`.

### So: three bugs, not one

1. `force_lle 0x009311` does not stop `$930D` being compiled AOT — the
   declaration is off-label. `snesrecomp/docs/MULTI_TIER.md:181-195` is the
   only documentation of `force_lle` and scopes it to *architectural* ABI
   boundaries, not timing. A `LDA/BNE` spin is not an ABI boundary.
2. The host runs the untuned generic template instead of a scheduler-shaped
   frame step, so nothing drives the token.
3. The token's only writer is an `INC` inside an NMI early-exit branch, which
   makes it fragile in a way the peers' tokens are not.

### The fix, in the peers' shape

```c
g_ram[0x00B9] = 0xFF;                                    /* host vblank token */
interp_bridge_run_scheduler(&g_cpu, 0x009311, 0x009311, 0x00B9);
```
plus `exclude_range 0x930D 0x9318` in `recomp/bank00.cfg` so AOT can never
re-enter the spin from inside a C frame — SMW's documented failure mode — and
dropping the five redundant `force_lle` lines around the loop.

**One known wrinkle, so it is not a surprise later:**
`interp_bridge.c:1713-1720` hard-codes the canonical wait-loop byte pattern as
`LDA <abs> ; BNE -5` (`AD .. .. D0 FB`). SimCity's loop is `LDA dp ; BEQ -3`
(`A5 B9 F0 FA`) and **will not match**. The primary yield is not pattern-gated
and should still fire, but the M/X and DB width repair and the stale-flag
step-cap containment will not run. Widening that matcher is a one-line
framework change if the containment turns out to be needed.

### Corrections to earlier claims in this file

- The AOT/PPU-beam asymmetry (`common_rtl.c:1358-1367`) is real and documented,
  but it is **not** what causes this. `$4212` is synthesised from
  `vPos >= 225` (`snes.c:594-606`) and reads go through a tier-independent
  path. The beam is not frozen. That hypothesis is dead.
- `force_lle` is described in `docs/MULTI_TIER.md` as being for ABI boundaries.
  Using it for a timing boundary, as this project does in five places, does not
  mean what the comment on line 32 claims.

---

## 2026-09-30 (confirmado pelo dono) — 30.000 frames, uma única imagem

O dono relata, jogando: *"a imagem mexe-se porque tem animação, não significa
que o tempo passe, as estações não avançam mudando a coloração de todo o mapa,
não aparece FEV nem MAR"*.

**Isto está correcto, e é o teste mais forte que o projecto tem.** Também
explica a aparente tensão com "a imagem é bit-idêntica": são camadas
diferentes, e o registo de escritas na PPU separa-as exactamente.

### Animação é OAM; estações são CGRAM

Escritas na PPU depois do frame 3382 (8.000 frames, `SNESRECOMP_WLOG_ADDR=2100:213F`):

```
$2100 = $030F   4599x     <- endereço OAM
$2102 = $00     4599x     <- endereço OAM
$210F..$2112 = 00  9198x cada   <- writes de OAM, SEMPRE o mesmo valor
```

O guest continua a enviar OAM 4.599 vezes — o cursor/hand a piscar. É
animação, e é exactamente o que o dono viu. Mas os valores são sempre os
mesmos, por isso a imagem não muda.

E a paleta, que é o que muda com as estações:

| janela | escritas CGRAM (`$2120`/`$2121`) |
|---|---|
| f1000–1999 | 1.673 |
| f2000–2999 | 2.001 |
| f3000–3999 | 544 |
| **f4000–4999** | **83** |
| f5000–5999 | 83 |
| f6000–6999 | 84 |
| f7000–7999 | 83 |

~2.000 por 1.000 frames antes, **83** depois — as estações pararam. As 383
escritas que restam depois de f3382 são todas `$2121 = $00`.

### 30.000 frames é mais uma prova

Screenshot a cada frame, 30.000 frames (500 s de tempo emulado; um mês de
SimCity são ~2 s, portanto são >8 meses de jogo):

```
distinct images in 30,000 frames: 164
3922329d6279  first@ 003381  count 26619
```

As 164 imagens distintas são todas do attract e dos menus, antes de f3381. A
partir daí há **uma** imagem, repetida **26.619 vezes**. Nenhum FEV, nenhum
MAR, nenhuma estação.

### O que isto fecha

A causa-raiz acima — o loop AOT `$930D` a limpar o token `$00B9` no mesmo frame
em que o NMI o põe — explica tudo isto por um único mecanismo, e a previsão
que faz é correcta: **o jogo nuncaexecuta uma iteração de simulação**, porque
nunca sai do spinlock. Logo:

- o mês nunca avança (não há código que o faça correr);
- as estações nunca mudam (a paleta é escrita pela simulação);
- a animação continua (OAM vem do sprite/cursor, não da simulação);
- o input não faz nada (o loop principal está preso).

A animação é a **única** coisa que sobrevive a um guest pendurado, e é
precisamente por isso que este bug enganou o projecto durante semanas: havia
movimento no ecrã, e movimento foi lido como "o jogo está vivo".

### Consequência para os gates

`make test-rom` mede `crc32` distintos numa janela de 600 frames depois do
boot. Com esta rota ela passa com folga, porque passa nos menus. **Nenhum gate
deste repositório consegue ver este bug**, e nenhum deles faz:

- `test_deterministic_replay` (30 frames) — nem chega à cidade;
- `verify-rom-render.sh` (f200–800) — ainda está nos menus;
- `perf-gate.sh` (600 frames) — idem;
- o clock probe — exige entrar na cidade à mão, que era o que o tornava
  inaplicável.

O gate que faltava é um que **rode `d_city.script` e verifique que a data
avança depois de f3382**. É um gate de uma linha de lógica, e é a razão de o
roteiro `scripts/d_city.script` existir como ficheiro versionado em vez de
nota num documento.

---

## The gate (`make clock`)

`scripts/clock-gate.sh`, wired as `make clock`. It runs
`scripts/d_city.script` for 6,000 frames and reads the HUD date off the screen
— the same thing the owner does — and fails if the date did not advance after
the city is live.

Current result, and it is the honest one:

```
run 1/1 ... 1 distinct date images after f3600 (last change f3379 of 6000)
CLOCK: FAIL - the date did not advance in a live city.
```

The date crop is x 55–125, y 2–21 of the 336x224 framebuffer: the `1900 JAN`
glyphs and nothing else. It deliberately excludes the tool palette, the RCI bar
and the treasury. A whole-frame comparison would be worthless here — the OAM
writes that animate the cursor do not change the pixels, and the ones that do
would be exactly the false positives this gate must not have.

### Why it is not a ctest

Same reason as `test-rom` (it needs the ROM, which is never committed) plus a
second one: it needs the game to be *reached*, which takes 3,600 frames of
scripted input. A ctest has to be fast enough to run on every build, and this
one takes about 100 seconds to reach the thing it is testing.

### The calibration problem, and how it is handled

**There is no build in this repository where the clock advances.** So a gate
written against it has never seen a PASS, and a gate that has never passed is
indistinguishable from a gate that cannot pass. `scripts/clock-gate.sh
--self-test` (`make clock-self-test`) exists for exactly that: it points the
same detector at a window of the same run that is demonstrably alive — the
menus animate, frames 0–1200 — and fails if the detector reports no motion
there.

```
frames captured               : 1200
distinct date images, all     : 16
last frame the date changed   : 1164
SELF-TEST: PASS - the detector sees motion where motion exists.
```

A PASS from the real check only means something because the detector has been
shown to fire on a screen that is alive.

### Both outcomes were reached, deliberately

Falsification is the only evidence that a gate works:

| check | result |
|---|---|
| real check on the current build | **FAIL**, `rc=1`, "last change f3379 of 6000" |
| window moved to the pre-city animation | **PASS**, `rc=0`, 16 distinct |
| missing script / ROM / binary | **FAIL**, `rc=1` each |
| `--frames` below the city frame | **FAIL**, `rc=1` |
| no screenshots captured | **FAIL**, `rc=1` — "would pass on an empty directory" |

The last row matters more than it looks. Three separate bugs in this gate's own
first draft all had the same shape — a path that did not exist, so the host
logged `cannot open ...` per present and exited 0, and the gate read that as
"the game produced nothing". A gate that cannot distinguish *no output because
the game is dead* from *no output because I passed the wrong directory* is a
gate that will eventually pass for the wrong reason.

### What the other gates say, for contrast

```
make test       100% tests passed, 0 tests failed out of 2
make test-rom   PASS: the emulated picture moves.
make perf       PERF: PASS
make clock      CLOCK: FAIL
```

Three green and one red, on a build whose city is a still image. That is the
whole argument for this gate existing.

---

## 2026-09-30 — CORRECTION: the vblank handshake is NOT the cause

Everything above this line that blames `$930D` / `$00B9` is **wrong**, and it
was wrong in a way that survived three rounds of measurement because every
measurement confirmed the symptom rather than testing the claim.

**What I claimed**: the AOT-compiled wait loop clears the token `$00B9` in the
same frame the NMI sets it, so `LDA $00B9 / BEQ` never sees it set.

**What is actually true**, measured by dumping every interpreted PC for one
whole frame:

```
[   0..653 ] NMI handler $0080B2 .. RTI at $0081A3
[ 654..657 ] $9315 BEQ / $9311 / $9313 / $9315    one loop pass
[     658  ] $9317 RTS                            <-- LEAVES THE SPIN
[ 659..1523] guest main-loop body                 865 steps
[1524..1525] $930D SEP #$20 ; $930F STZ $00B9     arms the wait again
[1526..5592] spin $9311/$9313/$9315 x1357
```

**The guest leaves the spin every frame and completes a full main-loop
iteration.** `INC` (NMI) then `STZ` (guest) is the *correct* order: the
interrupt releases the wait, and the guest re-arms it. It is not an
inversion, and reordering the NMI would break a handshake that already works.

Confirmed independently of that trace: `$00C7` is the `INC` inside the spin, so
it counts passes through the wait. It reads 156 at f3400, 2 at f3500, 58 at
f3600 — it **wraps**, repeatedly, every frame. A livelock would pin it or
advance it without end.

Two more things the earlier sections got wrong:

- **`$0080BC` is never executed.** The NMI takes the *long* path: `$80B6 LDA`
  → `$80BA BMI` **taken** (A = `$0081`, bit 7 set) → `$80C0`. The token is
  still set each frame, by a different instruction in that path — the write log
  tags a store with the *scope entry* `$0080B2`, not the store site, and I read
  that tag as the store address.
- **The spin has run since boot.** `$00C7` advances ~1400–2600×/frame from
  frame 0, menus included. It is not something that started at the city.

### What the fault actually looks like

The guest is **structurally healthy at the frame boundary** — NMI delivered and
returned, spin entered and left, one main-loop iteration completed, frame
counter ticking. And the game is **idle**. So the fault is in *what the main
loop does*, not in the vblank handshake, and it is a game-semantics question
this project has been reading as a timing one.

The most concrete lead: the NMI handler's short/long path is selected by **bit
7 of `$00B1`** (`$80B6` → `$80BA`). In our runs `$00B1` is `$81` at NMI time,
so the handler always takes the long path and the fast path never runs.
`$00B1` is written from ~140 sites. **Whether hardware presents `$01` at the
vblank edge, and we present `$81`, is the next thing to test** — an open
question, not a finding.

### An alternative I could not rule out

`scripts/d_city.script` reaches a screen that *looks* like a city. Given the
guest completes a full main-loop iteration per frame while the picture is
bit-identical, "live city that is idle" and "static screen the guest loops on"
are not yet separated. That should be settled before more work on the main
loop — by proving the city *state* changes when it should (population,
treasury, a zoned tile), not by looking at it.

### What this cost, recorded honestly

Three pieces of work were built on the wrong diagnosis and are now known to be
wrong or irrelevant:

- `exclude_range 0x930D 0x9318` in `recomp/bank00.cfg` (commit `436b25b`) —
  harmless and defensible on its own terms (SMW has the same exclusion and
  quotes the failure it prevents), but it was applied to fix this and does not.
  Left in place, not because it fixed anything here.
- A reorder of `GameRunOneFrame` — reverted, it does not converge.
- Widening the framework's `_canonical_wait_loop` matcher — reverted, it
  changed nothing.

The failure was mine and it is worth naming precisely: **I inferred a
mechanism from a write-trace and treated the tag as the store site.** Every
number I quoted was real; the conclusion drawn from them was not.

---

## 2026-09-30 — o renderer está VIVO. A simulação é que não corre.

O dono jogou e disse: *"existe animação, no ecrã, o tempo é que não passa, a
população não cresce, as estações não aparecem"*. **Está certo, e refuta a
metade seguinte do meu diagnóstico anterior.**

### A reconciliação

| medição | distinct crc32 | o que é |
|---|---|---|
| run passiva (o que o gate amostra) | **1** em 2400 presents | o gate nunca dá input |
| run com input | **26** que a passiva nunca produziu | o renderer responde |
| `pokefor $0B9D` | muda no **present seguinte** | não é bitmap em cache |

O gate (`scripts/clock-gate.sh`) amostra **passivamente**, por isso nunca veria
animação de renderer. "Bit-idêntico" e "animado" são simultaneamente verdadeiros
e a aparente contradição era do instrumento, não do jogo. A mesma limitação
explica o `1 distinct crc32` que eu reportei três vezes.

### O que o main loop faz — medido

Trace de um frame completo, 1144 stores WRAM no body, 4 tags de autor:

```
bank_01_C772_M0X0       7657   AOT   setup HDMA/OAM
bank_01_B274_M0X0       1280   AOT   loop raster
PPU_Bitpack_8EA9_M0X0    ~12   AOT
interp@$0080B2         2089         o NMI
interp@$009313/11/15   17406         o spin + a parte interpretada do body
```

O body é **100% setup de raster/HDMA**: 32 iterações de `$01B274`, cada uma com
dois `JSR $C772`, indexando a tabela `$7F0200,X` (a SRAM da bateria). Só bancos
00 e 01 correm; **bancos 02–07 não correm nada**.

De 110 endereços do body, **85 escrevem valores byte-idênticos em 10 frames**.
Os 25 que variam são posição de raster, índice e pilha. **O controlo de fluxo
nunca varia** — logo não há nada a montante "a escolher não trabalhar", porque
não há outro trabalho lá dentro.

### Os diagramas

O loop do guest e o loop do host estão em `/tmp/opencode/loop/*.mmd` e foram
derivados da medição acima, com cada nó rotulado pelo seu endereço e marcado
como medido ou inferido. Resumo estrutural:

```
HOST: NMI PRIMEIRO (game_rtl.c:223-229)
  -> NMI $0080B2..$81A3, o caminho LONGO (o curto nunca corre)
     -> $819A STA $00B9   (único writer do token que corre, 1/frame)
  -> guest sai do spin em $9317 RTS
  -> MAIN LOOP BODY: 32x raster/HDMA, 1144 stores
  -> $930F STZ $00B9 (rearma)
  -> SPIN $9311/$9313/$9315 até ao deadline do host
```

### Três correções a afirmações anteriores

1. **`$00B1` nunca foi uma flag.** É um byte temporário de endereço DMA dentro
   de `$01C772` (`LDA $B3 / AND #$7F / STA $B1`, depois `STA $4202`, depois
   restaura). Escrito 62–208×/frame. O jogo **inicializa-o a `$81` no boot**
   (`$008044 LDA #$81 / STA $B3 / STA $B1`) — bit 7 set é o estado projectado.
2. **O caminho longo põe o token ele próprio**, em `$819A`, 236 bytes depois de
   onde este documento supunha. 840 stores, exactamente 1 por frame.
3. **`$0406` não é "+1 por frame desde o frame 0".** Tem **zero escritas nos
   frames 0–3338** e 1/frame a partir de f3339. O jogo está ocioso **de
   propósito** fora da cidade — e continua ocioso dentro dela. Esta é a segunda
   prova independente de que o idle é deliberado.

### A pergunta que fica

**A rotina de mês/estação/população não é alcançável a partir do main loop
body.** Não foi identificada por endereço, e não vou adivinhar um.

O que se sabe: o guest é estruturalmente saudável (NMI entregue e retornado,
spin entrado e saído, uma iteração do main loop por frame, contador a avançar),
o renderer está vivo e segue o estado do guest com um frame de atraso, as
estruturas da cidade existem e estão inicializadas (2.387 bytes que são zero no
ecrã de nomes e no attract), e `$0B9D` injectado produz `162774` no HUD — um
valor **calculado pelo jogo**, não o nosso byte ecoado. Portanto o jogo sabe
ler, formatar e apresentar estado. Simplesmente nada escreve nesse estado.

Onde vive a rotina, e o que a impede de correr, é a pergunta em aberto.

### Sobre o gate

Não foi adicionada a asserção do poke, e a decisão é do owner. Passaria no
build actual — e um gate que passa num build onde o jogo está visivelmente
partido é um segundo semáforo verde, não cobertura. Quando a causa for
encontrada e corrigida, ela passa a valer como **precondição** ("o caminho de
render funciona, logo um relógio parado é falha de simulação e não de
render"), que é exactamente a distinção que este documento precisa.

---

## 2026-09-30 — a terceira refutação: os bancos 02/03/05 correm

Duas das minhas premissas anteriores estavam erradas, ambas sobre o mesmo
erro: inferir a partir de uma ausência.

### "Os bancos 02–07 nunca correm" — falso

MEASURED, `SNESRECOMP_PHASE_MS=1`, 4200 frames, 4,349,843 amostras:

```
bank$00  3505153  80.6%
bank$01   599072  13.8%
bank$05   106189   2.4%
bank$02    80527   1.9%
bank$03    58902   1.4%
```

Bancos 04, 06, 07 estão genuinamente a zero — e esses são dados. Eu medi
"escritas em `$0000-$7FFF` por tag de autor" e vi só `bank_00` e `bank_01`, e
concluí que os outros não correm. **Não correm o suficiente para escrever ali.**

### E o `bank02.cfg` está a declarar tiles como código

`recomp/bank02.cfg` tem `func Res_D01_TL 0x03AC` e comentários sobre *"tile
IDs"* e *"tile range 940-1022"*. Esses são **índices de tile VRAM**, não
endereços de código. O dano está no manifesto: **41 de 41 nós do banco 02 têm
`instruction_count: 0`.** Nenhum código é gerado, e nada do banco 02 está
disponível para o linker.

`bank03.cfg`–`bank07.cfg` declaram endereços redondos (`0x8000`, `0x9600`) com
nomes inventados. Só o banco 03 tem funções analisadas com extensões reais
(180 nós, `instruction_count` até 639) — e o banco 03 **corre**.

**`$03:8B42`, a única rotina que poderia armar `$0BB9` por frame, não existe no
manifesto.** E `recomp/bank03.cfg` tem `func SFX_Play 0x8600`, que o manifesto
diz ser `03:8600-03:8840 ic=289` — ou seja, os nomes inventados do cfg sao duvidosos
código real por acidente, e o resto dasfunctionalidades do banco 03 nunca foi
declarado.

### Onde a cidade realmente vive: **SRAM**, não WRAM

`$01:F8E9` (`LDA $7F0200,X / AND #$03FF`) corre **78.392** vezes; `$01:F8AF`
(`STA $7F0200,X`) 12.403. Chamadas a partir de uma família de rotinas de tile
(`$01:F22C`, `$F311`, `$F380`, `$F3A3`, `$F444`, `$F502`, `$F5B9`, `$F600`,
`$F647`, `$F6AE`, `$F71D`, `$F794`) — um walker de tilemap 120×100 com
bounds checks contra `$0078`/`$0064`.

`save.srm` vai de 0 para **32.768 bytes** durante uma run. **"Bateria fria" só
descreve o primeiro frame.**

### Os gates encontrados são do caminho de DISPLAY, não de simulação

`$00:85EC` (JSL'd de `$01:89C1`, 220 execuções) exige `$D7 == 1`; na cidade
`$D7 == 0`, por isso devolve em `$85F3` todas as vezes. O seu companheiro
`$00:84AD` correu **1 vez**.

**Mas `$84AD`/`$85EC` são reconstruidores de tabelas de display** — copiam das
tabelas da página `$0B00` (`$0B53`, `$0B55`, `$0BA5`, `$0B9D`) para
`$7E2021-$7E2056` e `$7E38EE`, e constroem uma lista de 6 ponteiros. **Não são a
simulação de mês/população.** E isto explica o poke do `$0B9D`: `$0B9D` é um
*cursor de tabela* passado em X (`LDX #$0B9D / JSR $8FEF`), não um campo
apresentado. Poke-lo mexe no ecrã porque muda onde a cópia começa.

### O dispatcher da cidade corre

`$01:894A`, **221 execuções** em 3600 frames. Com `$01:8B21` (220), `$01:8A92`
(220), `$00:85EB` (220), `$01:EF9F` (220). E as rotinas de criação de cidade
são one-shot: `$01:C6C8` e `$01:8907` correm **exactamente 1 vez** cada. A
cidade aparece entre f3200 e f3300 (54.546 bytes mudam nesse passo).

### Resultado negativo limpo: o campo do mês

**Não encontrado, e não faço claims.** O que foi procurado e o que exclui:

- `"1900"` e os doze nomes de mês em ASCII na WRAM (131.072 bytes): **zero
  hits.** São tiles, confirmado.
- Tabela de 12 entradas com stride constante no ROM: 764 hits, todos sliding
  windows sobre rampas de valores. Excluído — ruído.
- Varrimento estático de `$0400-$04FF` em todos os bancos: nenhum cluster com
  forma de data. `$0406` tem exactamente **4** referências em todo o ROM.
- `$2510-$2516` já excluído antes como fase de animação.

**O que o encontraria**: o leitor da data está no motor de texto/UI do banco 03
(que corre, 1.4%), e esse evento precisa de um conjunto de PCs de execução
completo — `SNESRECOMP_PHASE_MS` só imprime os 16 PCs mais quentes, que não
basta para enumerar.

### Turbo/pause: não corre, e não responderia

`Turbo = Tab`, `Pause = Shift+p` — ambos via eventos de teclado SDL
(`host_main.c:1810-1818`), que um run `SDL_VIDEODRIVER=dummy` não recebe. O
parser de script não tem verbo de turbo nem de pause.

E mesmo com o Tab entregue **não responderia à pergunta**: `g_turbo` só põe
`disableRender` em 15 de 16 frames e limpa o pacing realtime. O emulador corre
exactamente um frame de guest por iteração de host em qualquer modo. Turbo muda
a taxa de apresentação, não o tempo simulado.

### Estado

`make clock` **FAIL**. O jogo não é jogável. O que está estabelecido é
substancial — o guest está saudável, o renderer está vivo e segue o estado, as
estruturas da cidade estão inicializadas, o dispatcher corre — e o que falta é
uma coisa: **a rotina que escreve o tempo**.

---

## 2026-09-30 — as duas fontes que mudam a investigação

### Existe uma descompilação pública do NOSSO ROM

**`Yoshifanatic1/SimCity-SNES-Disassembly`**, USA, MD5
`23715fc7ef700b3999384d5be20f4db5` — **idêntico ao nosso ROM**, verificado por
`md5sum`. É exactamente o input que `SuperMarioWorldRecomp` usa
(`SMWDisX`) para gerar `data_region` / `name` / `symbol` automaticamente.

O nosso `recomp/*.cfg` tem **zero** `data_region`, **zero** `symbol`, **zero**
`name`. O SMW tem 747 / 6.817 / 82. Esta é a peça que falta, e existe.

### E existe OUTRO port de recompilação do MESMO jogo

**`Junior-Jones/SimCity-SNES-Static-Recomp`** — "Static recompilation of SimCity
(SNES) for Windows 10 and 11. ROM not included." Não é peer de framework; é
outra tentativa do mesmo jogo. A forma mais barata de saber se alguém já
chegou onde nós não chegámos.

### O que a descompilação já responde, sem uma única medição nova

Notas de research público sobre SimCity (gist `freem/e0e88ed`, mais o
descompressor de `bbbradsmith`):

- **A cidade vive em `$7E8000`, e a SRAM `$7F0200` é o buffer comprimido.** Isto
  **confirma a medição** de que `$01:F8E9` (`LDA $7F0200,X`) corre 78.392
  vezes. A decompressão de `$7F0200` → `$7E8000` está em **`$03D1C4`**.
- **SimCity usa o COP-Interrupt com A como índice de uma jump table.** Isto
  importa muito: o manifesto lista `cop_at_*` como motivo de `lle_only`, e eu
  tratei isso como erro de decode. **É código real do jogo.** `ANALYZER_GAPS_INVENTORY.md`
  classifica `cop_at` como problema — para a maioria dos jogos é-noite, mas aqui
  é o mecanismo principal de dispatch.
- **O mapa do cenário é comprimido**, com o código de descompressão em
  `$03D1C4`, e `#$FFFF` é o fim de dados.
- **$7E00B5 contém as flags de HDMA**, e o OAM buffer está em `$7E2000`
  (via `$00/8D65` — a função que o T057 travou com `force_lle`).
- Os nomes dos cenários estão em `$03/CF32`, lidos para `$7E0B5B`.
- O formato de compressão é o **LC_LZ5 da Nintendo**, e o decompressor do jogo
  está em **`$0090A6`**.

### Consequências directas para o que eu fiz de errado

1. **`cop_at_01894F` no dispatcher não é necessariamente um erro de decode.** Se
   SimCity despacha por COP, então `cop_at` é a forma *correcta* de o descrever,
   e a directiva `indirect_dispatch` que o agente de pesquisa recommended pode
   estar a resolver o problema errado.
2. **`recomp/bank03.cfg` declara `func LC_LZ5_Decompress 0x8000`** — que eu e o
   agente registámos como "nome inventado num endereço redondo". **O nome está
   certo**: o decompressor LC_LZ5 existe, em `$0090A6` no bank 00. Não é
   invenção, é conhecimento real mal colocado. Isto é um bom exemplo de quanto
   custou working sem authority.

### O que falta, em ordem

| # | Gap | Porquê |
|---|---|---|
| 1 | Ingerir a descompilação como **authority** | Desbloqueia `data_region`/`name`/`symbol` automáticos e o `audit_disassembly.py` do upstream |
| 2 | Merger os **147 commits** do upstream | O fork está 147 atrás e 38 à frente; o commit dos peers (`8867499`) está no `main` |
| 3 | `dma_initHdma` / `dma_doHdma` / `dma_primeHdmaFirstLine` | `docs/LLE_SCHEDULER.md` e `dma.h` exigem-no; nós usamos `dma_startDma(…, true)` para init de frame, que o framework proíbe. Explica as 6.619 escritas PPU em f3385-87 e depois nada |
| 4 | `snesref` como oráculo | 701 linhas atrás; falta `SNESREF_SCRIPT` (com `until`), `SNESREF_SRAM_IN`, `SNESREF_CORE_OPTIONS`. Resposta directa: *o hardware real alguma vez tira `$7E:00C5` do índice 0?* |
| 5 | Gate sobre estado semântico, não sobre o HUD | Nenhum peer faz gate sobre uma string. Zelda tem `debug_harness.py` com símbolos de jogo. O nosso `clock-gate.sh` lê o HUD |
| 6 | Declarar o dispatch correcto | **Depende de (1)** — e pode ser COP, não `JSR (abs,X)` |

**Nota de método, de `SuperMarioWorldRecomp/docs/GOLDEN_TESTING.md`**, que
proíbe explicitamente o que fizemos: *"Don't sync by frame number… A test that
asserts 'at frame 200, player Y = 0x0150' will fail on both sides' valid
states. Sync on gameplay state."* E: *"Don't trust `g_last_recomp_func` — it's a
single global that lags behind actual execution under tail-call patterns."*
Nós citámos essa tag como se fosse o local da escrita. **O peer documentou esse
erro exacto antes de nós o cometermos.**

---

## 2026-09-30 — QUEBREDO: a descompilação reassembla byte-idêntica, e nomeia a data

`Yoshifanatic1/SimCity-SNES-Disassembly` com o framework em **V1.0.1** e
asar 1.91 reassembla para **exactamente os nossos bytes**:
`23715fc7ef700b3999384d5be20f4db5`, 524.288 bytes, checksum `$39B3`
restaurado. Sem mismatch de header ou região — o nosso ROM é LoROM 512 KB
headerless, que é precisamente o que o mapa do framework declara.

Isto torna-o uma **authority** no mesmo sentido que o SMWDisX é para o
SuperMarioWorldRecomp. E o copyright fica limpo: o repositório da
descompilação não contém assets, e só labels e endereços passariam para cá.

### Os campos que três semanas de diff não encontraram

De `SIMC/RAM_Map_SIMC.asm`:

| campo | endereço |
|---|---|
| **`CurrentYear`** (16-bit LE) | **`$7E:0B53`** |
| **`CurrentMonth`** (16-bit LE) | **`$7E:0B55`** |
| `CurrentPopulation` (24-bit) | `$7E:0BA5` |
| `CurrentFunds` (24-bit) | `$7E:0B9D` |
| `DifficultyLevel` | `$7E:0B57` |
| `CityCategory` / `CurrentCityScore` | `$7E:0DEB` / `$7E:0DED` |
| `TaxRate` | `$7E:0DC5` |
| **`$0B51`** | **contador de fase de 4 ticks** |

Medido no nosso build, `d_city.script`, SRAM fria:

```
addr        f3300  f3380  f3400  f4000  f4199
$0B53 year  076C   076C   076C   076C   076C     <- 0x076C = 1900
$0B55 month 0001   0001   0001   0001   0001     <- 1 = Janeiro
$0B51 tick  0000   0000   0000   0000   0000     <- NUNCA avanca
$02BF pause 0000   0080   0080   0080   0080     <- bit 0 limpo = NAO pausado
$0BA5 pop   000000 ...
$0B9D funds 004E20 ...
```

**`$0B53`/`$0B55` é literalmente a string do HUD.** E um log de escritas em
`$0B53-$0B56` mostra **exatamente uma escrita em 4.500 frames**, em **f3258**,
por `interp@$009311` em **bank `$03`**, com o valor `6C 07 01 00` = 1900 /
Janeiro. O setup do cenário escreveu a data. Nada escreve desde então.

**O tick que a avança é `CODE_038000`/`CODE_038016` em bank 03**: `INC.w $0B51`,
acumula impostos em `$0DC7`, e a cada 4 ticks `INC.w CurrentMonth` com o wrap
Dezembro→Janeiro e `INC.w CurrentYear`. **Estações**: `CODE_00961C` deriva
`$7E:0B4D` do mês. **Pausa**: `LDA.w $02BF / AND #$0001` — bit 0 limpo = não
pausado.

### O `cop_at_*` é o mecanismo real, não um erro de decode

`!NativeModeCOPVector = CODE_008211`. O handler faz `ASL / TAX /
JSR.w (DATA_008223,x)` — **tabela de 11 entradas em `$01:8223`**, índice `A>>1`:

| A | destino |
|---|---|
| 0 | `$00930D` **vblank wait** |
| 8 | `$0090DD` **descompressor LC_LZ5** |
| 1, 10 | `$0086A4`, `$0086C8` (OAM) |

**Não "consertes" os `cop_at_*`.** E a frame boundary do guest é dela:
`CODE_008061` chama `LDA.w #$0000 / COP` **duas** vezes e depois
`JSL CODE_018907`, que faz um terceiro. Ou seja, **o guest estaciona três ou
mais vezes por iteração do main loop** — o host entrega um NMI por frame.

`GAME_MASTER_CYCLES_PER_FRAME` + o slice loop é uma **palpite** a substituir
por `interp_bridge_run_scheduler(0x009311, 0x009311, 0x00B9)`. O nosso cfg já
tem `force_lle 0x009311` com o comentário certo — **falta o token, `$7E:00B9`.**

### Duas refutações minhas

1. **A tabela de `$00C5` tem 12 entradas, não 16**, e `$C5` guarda um índice
   *par*. O site é `LDA.b $C5 / REP #$10 / ASL / TAX / JSR.w (DATA_0188EF,x)`.
2. **O cliff de PPU em f3387 não reproduz.** Com log de escrita em
   `$2100-$213F` durante 4.050 frames: o pico é f3271–f3315 (4.000–4.800
   escritas/frame — é a *carga* da cidade), e a partir de **f3339 são 28–30
   escritas/frame até f4049**, nunca zero. **O trabalho PPU do NMI não parou.**
   O número que eu citei media outra coisa.
3. `$03/CF32 → $7E0B5B` (nomes dos cenários) está **refutado** — não existe
   essa tabela. E o decompressor LC_LZ5 está em `$0090DD`, não `$0090A6`.

### O peer do mesmo jogo

`Junior-Jones/SimCity-SNES-Static-Recomp` — **mesmo ROM** (SHA-256 igual ao
nosso). Windows-only, sem screenshots no repo, por isso **não se pode
confirmar que chegue a uma cidade a correr**; é uma afirmação, não uma
medição.

Mas duas coisas são leitura de código e valem muito:

- **Não implementa HDMA.** `sc_machine.c:309` guarda `hdma_enabled_mask` e as
  tabelas, e **nunca os relê**. Chegam a uma cidade a correr sem HDMA. O que
  **confirma por via independente que HDMA não é o nosso bloqueio** — e o
  teste Q5 abaixo mediu o mesmo.
- **O frame model é real**: `sc_v11_scheduler.c`, 262 linhas × 341 hclock,
  NMI no scanline 225 hclock 2, captura de PPU por scanline, "one guest frame
  per host deadline". É o prior art a adoptar.

### HDMA: negativo limpo

Implementado exactamente como `dma.h` e `FRAME_MODEL_HOSTS.md` especificam
(`dma_initHdma` + `dma_primeHdmaFirstLine` no init, `dma_doHdma` antes de cada
linha). **Medido, antes vs depois:**

| | antes | depois |
|---|---|---|
| escritas PPU do guest f0–f4049 | 243.450 | **243.450** |
| média na cidade f3400–f4049 | 28.8/frame | **28.8/frame** |
| framebuffer f4049 | `ad95abe7…` | **idêntico** |
| framebuffer f1199 no attract (HDMA **ligado**, `$7E:00B5 = $08`) | `54370fd6…` | **idêntico** |

Porque é um no-op: na cidade `$7E:00B5 = 0` e `CODE_008C28` faz commit de
`$420C = 0`, portanto nenhum canal está activo. **Revertido, `git diff`
vazio.**

### snesref corre o nosso ROM — e concorda connosco

Medido: `c++ -std=c++11 -O2 -o snesref frontend.cpp $(pkg-config --cflags
--libs sdl2) -ldl`, core snes9x libretro construído em `/tmp`. **9.820 frames
headless**, `SNESREF_WRAM_FILL=0`, `SNESREF_SRAM_IN` fria, trace JSONL de
low-WRAM com 93.384 registos.

**No hardware real `$00C5` é escrito uma vez e nunca muda** em 9.820 frames de
boot. E **o nosso recomp concorda com isso em todos os estados comparáveis**.
Portanto o pinning de `$00C5` **não é um artefacto do recomp** nos estados que
conseguimos alcançar — o que enfraquece muito a hipótese "dispatch quebrado".

O `snesref` **não consegue chegar a uma cidade**: a rota só com pad encrava no
ecrã de nome. `SNESREF_SCRIPT` não pode reproduzir a conversão
soft-mouse→d-pad do nosso host. O caminho é `SNESREF_INPUT_FILE` alimentado
pelo stream de d-pad *efectivo*.

### Onde fica, e a próxima unidade de trabalho

**O tick de bank 03 nunca corre.** `$0B51` é zero em todos osamples. A
descompilação mostra que o main loop **arma a corrotina** em `$01:825F`
(escreve `$1F7C-$1F7F = $038000`, e o NMI muda para uma segunda pilha via
`TSC/TCS` em `$01:817C`/`$01:8193`). **A instrução que transfere o controlo
para lá não foi encontrada** — nada na descompilação lê `$1F7C`, portanto tem
de ser um `RTS`/`JSL` por endereço computado.

**Recomendação: ingerir a descompilação como authority, no motor actual, antes
do merge.** Um `tools/ingest_simcitydis.py` modelledado em
`tools/ingest_smwdisx.py`, emitindo `name` (~3.000 labels), `symbol` (todas as
constantes de `RAM_Map_SIMC.asm`) e `data_region`. Depois `regen.sh`, os três
gates verdes, e os seis probes de WRAM acima como conjunto de aceitação.

**Não fazer o merge dos 147 antes disso.** Quando se fizer, pôr
`exit_mx_set` nos critérios — o upstream nomeia o SimCity na própria doc do
directivo, e é a correcção do nosso defeito de banco 02.

**Uma ressalva, no espírito das duas refutações**: a identificação de
`$0B53`/`$0B55` é *authority + round-trip do valor* (1900 / Janeiro, escrito
uma vez em f3258), **não um screenshot**. O poke não pode provar nada porque
o guest deixou de redesenhar o HUD neste build.

---

## 2026-09-30 — o `$1F7C` nunca é lido. E um bug de medição meu que invalida probes antigas.

### `$1F7C` é write-only, e a hipótese do "transferimento computado" está refutada

`CODE_00825F` (medido: corre **1 vez**, em f3277, o frame em que a cidade é
criada) não é um salto — é um **guardar contexto de stack**:

```
LDA #$03 / STA $1F7F     \
LDA #$80 / STA $1E       |  $1F7C..$1F7F = $038000, um PC de 24 bits
LDA #$00 / STA $1D / STA $1C /
LDX #$1EFF / STX $1FA    ; stack PRINCIPAL
LDX #$001F72 / STX.b $AB ; segunda stack
LDX #$0000 / STX.b $AD   ; flag de fase
```

**7 hits no ficheiro de descompilação inteiro para `1F7C`-`1F7F`**: os quatro
stores acima, mais três colisões de label `CODE_01F7E7`. **Nada lê.**

E o ping-pong de stacks do NMI (`$817C`/`$8193`) lê e escreve apenas
`$A9`/`$AB`/`$AD` — `TSC`→`$A9`, `TCS`←`$AB` em `$8198`; `TSC`→`$AB`,
`TCS`←`$A9` em `$818B`. **`$1F7C` não está nesse caminho.**

Log decisivo em `$A9`/`$AB`/`$AD` (3.292 eventos): `$1F72` aparece em `$AB`
**exactamente uma vez em toda a run** — a f3277, o próprio arming
(`IPC=008280`). De f3280 em diante `$AB` vale `$1FE6`, o stack principal *vivo*,
e o ping-pong alterna `$A9=$1FE6` ⇄ `$AB=$1FE6` — **o mesmo valor dos dois
lados. A stack da corrotina nunca é entrada.**

E `CODE_00804D` (o main loop) corre **2 vezes em 5.000 frames**. O guest
deixou o main loop permanentemente e vive no caminho NMI/vblank. A hipótese
"o transferimento tem de ser um RTS/JSL por endereço computado" está
**refutada**: não existe leitura computada.

### "3 esperas por iteração, 1 NMI por frame" — também refutado

`SNESRECOMP_WLOG_ADDR=00B9:00B9`, cidade viva, 5.000 frames:

- escritas de token pelo NMI: **4.992**
- inícios de espera (`STZ $B9` em `$930F`): **4.110**
- → **0.998 NMI/frame, 0.822 esperas/frame**

Na janela f3600–3700 é **exatamente 1 NMI e 1 espera por frame, todos os
frames**. O guest não está esfomeado nem em deadlock no spin. As 1.640
execuções/frame no spin são o **sintoma**, não a causa.

### Um bug meu que invalida probes anteriores

`interp816.c:325` usa `strtoul(e, NULL, 0)` — base 0, portanto um `0` à frente
significa **octal**. `SNESRECOMP_COUNT_PC=00930D` faz parse de `00` como octal
e pára no `9`, devolvendo **0**. Os primeiros 9 probes de um agente deram
sempre 0, incluindo a vblank wait que o WLOG prova correr 4.110 vezes.

**Qualquer resultado de `COUNT_PC` registado neste projecto sem prefixo `0x` é
nulo.** O ticket T058 usava `0x009311`, portanto está correcto; os números que
agentes trouxeram em relatórios separados não têm auditoria de prefixo.

Nota relacionada: o contador vive em `interp816_runOpcode`, portanto **só vê
opcodes interpretados**. `$03:8000` está dentro de `func Menu_Main 0x8000`
(AOT) — um zero ali não significa nada.

### Resultados no Steam Deck

Binário `279e253e0df544f00259fd026e99fc40`, **idêntico** ao local.

| gate | Deck |
|---|---|
| `test-rom` | **PASS** — 254 crc32 distintos |
| `perf` | **PASS** — pior de 5: **60.05 fps** |
| `clock` | **FAIL** — "1 distinct date images after f3600" |
| `ctest` | **não corre no Deck** — `test_deterministic_replay.c:39` tem o caminho do ROM fixado; passa no dev box (2/2) |

### O que fica em aberto, e é menos do que pensávamos

- **Porque é que `$1F7C` é escrito se nada o lê.** Hipótese do agente: é um
  idioma de corrotina/task-switch em que o leitor é o **RTI do hardware** a
  rebentar a segunda stack. **Não há medição de um `RTI` a regressar por
  `$1F72`** — e `$1F72` nunca é carregado em `S` (`TCS` em `$8198` corre 4.663
  vezes, sempre com `$AB`, que vale `$1FE6`).
- **`$C3` alterna `00`/`FF` 1.621/1.620 vezes** e `$C3 != 0` salta para
  `CODE_0081A4` *antes* de qualquer escrita de stack. Pode estar a bloquear o
  switch em ~metade dos NMIs. **Não foi correlacionado** com o ping-pong
  por-NMI.
- `$0B51` foi escrito **uma vez**, em f3258, por `IPC=03C77E` — código de bank
  03 corre e toca nos campos do tick na criação da cidade, depois pára. Não
  foi perseguido se `03C77E` está no caminho do próprio tick.

---

## 2026-09-30 — o main loop não corre na cidade, e um valor armado não bate certo

Duas medições adicionais, com `0x` no `COUNT_PC` (a armadilha do octal está
documentada acima).

### O main loop não é executado depois da cidade

`SNESRECOMP_COUNT_PC` em 4.200 frames de cidade viva:

```
COUNT_PC=0x009311 -> 881646 execuções    (o spin de vblank)
COUNT_PC=0x008061 -> 0                   (corpo do main loop)
COUNT_PC=0x00804D -> 0
COUNT_PC=0x00825F -> 0
```

Isto **não** é o artefacto AOT-vs-intérprete que o agente accionou como
cautela: `$008061` está no manifesto como `lle_only`
(`ic=6, reasons=['cop_at_00806C','structural_poison', …]`), portanto é
interpretado, e mesmo assim conta zero. O guest executa
`$009311` 881.646 vezes e **nunca entra no corpo do main loop**.

A máquina de estados de bank 03 é despachada a partir de lá, e o NMI só
executa handlers. **É por isso que `$0B51` nunca é escrito**: o tick não é
inalcançado por uma condição interna, é inalcançável porque o código que o
despacha não corre.

Isto substitui a formulação anterior — "o main loop vive na rota NMI/vblank" —
por uma mais precisa e verificável: **ele não corre de todo**.

### Um valor armado que não bate certo

O relatório anterior diz que `CODE_00825F` escreve `$1F7C-$1F7F = $038000`.
Medido nas três janelas:

```
f3277: $1F7C..$1F7F = $000000   $1F7A = 0000
f3400: $1F7C..$1F7F = $800000   $1F7A = 1EFF
f4198: $1F7C..$1F7F = $800000   $1F7A = 1EFF
```

`$800000`, **não `$038000`** — o byte de banco em `$1F7E` é `$80`, não `$03`.
E `$1F7A = $1EFF` bate certo com o stack principal declarado. Portanto ou a
leitura do valor imediato no relatório anterior estava errada, ou algo
reescreveu o byte de banco depois. **Não resolvi qual**, e a diferença
`$80` vs `$03` é exactamente o tipo de detalhe que faz um bank malcodificado
saltar para o espaço errado — por isso vale a pena, e vale mais agora que se
sabe que nada lê este valor de qualquer forma.

### O estado, sem floreados

- Guest estruturalmente saudável, renderer vivo, cidade carregada, estruturas
  inicializadas.
- **`CODE_008061` (o main loop) não é executado na cidade.** O spin de vblank
  consome 881.646 execuções em 4.200 frames.
- `$0B51` (fase de 4 ticks) nunca escrito; `$0B53`/`$0B55` escritos uma vez, na
  criação da cidade.
- `$1F7C` é write-only. O bloco bank-03 compilado (o tick é agora C nativo) e
  ainda assim não corre — porque o despachante não corre.

**A pergunta passou de "que condição bloqueia o tick" para "o que impede o main
loop de correr".** É uma pergunta diferente e mais pequena.

---

## 2026-09-30 — as peers: o que foi útil, e três correções minhas

### Existe exactamente UM outro port de recompilação deste jogo

Pesquisa no GitHub devolve **um** candidato: `Junior-Jones/SimCity-SNES-Static-Recomp`
— **o mesmo ROM** (SHA-256 igual ao nosso). O
`Yoshifanatic1/SimCity-SNES-Disassembly` não é um port (já ingerido), e
`linuxkafe/SNES-SimCity-Recomp` é **o nosso próprio repositório** re-carregado.
Nenhum outro.

### O peer compila e corre no Deck — e tem 57.944 templates contra os nossos 302 nós

**B3 — o dispatch COP está resolvido lá, e a forma é a nossa.** O handler é
`COP → $008211 → PHB/PEA $0000/PLB/PLB/REP #$20/REP #$10/ASL A/TAX/
JSR ($8223,X)/PLB/RTI`, com a tabela de 11 entradas em **`$00:8223`** — bank 0,
não `$01:8223` como eu tinha dito. Todas as entradas medidas:
`A=0,5,6 → $930D` (vblank wait), `A=2 → $8EA9` (o nosso `PPU_Bitpack`),
`A=8 → $90DD` (LC_LZ5), `A=1,10 → $86A4/$86C8` (OAM).

**B2 — o frame model é o beam, e a constante não é o problema.** A constante
dele medida é `357366`; a nossa é `357368` — **diferença de 2 clocks**. O que
importa é **onde o NMI é(assertido)**: ao nível do beam no scanline 225
hclock 6, entregue nos limites de instrução (`before_instruction` /
`finish_instruction`), e o **spin do próprio guest é a fronteira de frame**.
O host dele é literalmente `while (scheduler.frame < target_frame) step();` —
sem `MASTER_CYCLES_PER_FRAME`, sem slice loop, sem `lle_took_wai()`.

### Três correções minhas, e a segunda é grave

1. **`$008061` NÃO é o "corpo do main loop".** Corre uma vez, não é
   `lle_only`-morto: corre **exactamente uma vez**, no boot. É uma cadeia
   one-shot de init de hardware (`JSR $8288` / `JSR $8690` / `COP #$00` /
   `JSR $825F`). **O nosso próprio `recomp/bank00.cfg:60-66` já a nomeava
   `Init_Hardware 0x8061`.** Eu li um label e escrevi "main loop".

2. **`$009311` são 6.996.200 execuções, não 881.646.** 1.665,8 por frame é
   **comportamento correcto**: o guest entra na espera ~0,79×/frame e roda
   ~1.666 vezes até o `INC $00B9` do NMI a libertar. **A espera de vblank está
   saudável. O pacing de frame está saudável.** As 881.646 eram o meu
   `COUNT_PC` sem prefixo `0x` — a armadilha do octal.

3. **O dispatch COP não está partido.** `$8211` e `$821E` correm **10.775
   vezes** em 4.200 frames, 2,56/frame. A `indirect_dispatch` em falta é uma
   lacuna de *análise estática*, não a causa do relógio.

Números do Deck, todos `lle_only`/`force_lle` para que um zero signifique algo:

```
$0080B2  NMI_Handler              4,191 = 1.00/frame
$008211  COP_Handler             10,775 = 2.56/frame
$00821E  JSR ($8223,X)           10,775 = 2.56/frame
$009311  spin head               6,996,200 = 1665.8/frame
$00804D  VBlank_Wait                   2 = boot only
$038000  bank-03 tick                 0 = NEVER
$038026  INC $0B51                    0 = NEVER
```

### A directive `indirect_dispatch` é correcta masinerte — medido

Declarei `indirect_dispatch 821E 11 idx:X`. O loader lê-a
(`{'site_pc16': 33310, 'count': 11, 'idx_reg': 'X', 'table_bases': ()}` — 33310
= 0x821E). Mas um **A/B com e sem ela dá manifesto idêntico**: 239 AOT / 63
LLE, e a lista de razões de `$008061` inalterada.

Porque: os `cop_at_*` nomeiam um **COP dentro de cada função envenenada**
(`$806C` dentro de `$8061`, `$8E5A` dentro de `$8E43`, …), não este site de
dispatch. Autorizar o dispatch não os limpa. **Mantida porque é correcta e
custa nada**, e anotada para que ninguém a volte a derivar.

### O peer tem os mesmos defeitos, e isso é informação

- **HDMA é write-only** — confirmado por leitura: `sc_machine.c:309` é a única
  escrita de `hdma_enabled_mask` e a única leitura em toda a árvore é a própria
  declaração. O README dele diz "HDMA" na lista de implementações. **Não está
  implementado.** E `$00B5` foi 0 em todas as amostras, nos dois.
- **Sem SNES Mouse** — `grep -ril mouse` sobre todos os `.c`/`.h` não devolve
  nada. **A "running city" dele não é reproduzível por nenhuma interface que
  publica**, porque o passo que cria a cidade é `mouseclick right`.
- **Só 2 gates**, ambos de áudio. Nenhum afirma o relógio, o frame model, ou que
  o main loop corra.
- **Tem o corpo do tick gerado** (`v35_group_000E0.c:121-185`) **e também não
  tem caller** — a sua única referência a `0x038000` é o próprio caso de
  entrada. Varri todas as tabelas `$82xx` de 11 entradas em todos os banks:
  **zero hits** para `$0380xx`.

### O B1 real, e é este

NMI 1/frame ✓, dispatch COP 2,56/frame ✓, spin 1.666/frame ✓, e
`$038000`/`$038026` **zero**. O tick é inalcançável por um **terceiro caminho**
que ninguém — nem nós, nem o peer — encontrou. E o corpo está analisado
(`038000:M1X1`, 134 instruções, `$038026` dentro do range) e agora compilado.
**O que falta é o caller, e é uma pergunta de análise estática sobre bytes do
ROM, não de emulação.**

### Sobre o `snesref` e o peer, para o registo

O `snesref` com snes9xbuilt em `/tmp` corre o nosso ROM. No hardware real
`$00C5` é escrito uma vez e nunca muda em 9.820 frames — **o nosso recomp
concorda**, o que desfaz a hipótese "dispatch quebrado".

O peer, corrido no Deck: **112,94 fps** em modo core-only (sem SDL, sem
present) — 1,88× o seu próprio nominal de 60,098 fps. O nosso, no Deck com
apresentação: **60,0 fps**, `guest` 3,599 ms/frame (21,6% do orçamento) contra
`upload-present` **7,761 ms/frame (46,6%)**. **O caminho de apresentação do
host custa 2,16× o guest.** Isso é um achado sobre o renderer, não sobre o
recompilador.

---

## 2026-09-30 — O mecanismo de `$038000` e o fork privado (estudo, sem publicar)

### Decisões do dono

**Fork privado para estudo** do peer sem licença, e **adotar o frame model dos
pares**. O peer (`Junior-Jones/SimCity-SNES-Static-Recomp`) **não tem licença
declarada** — `license: null` na API, zero ficheiros LICENSE — logo é todos-os-
direitos-reservados por omissão, e fundir o nosso PolyForm Noncommercial nele
não é legítimo. Clonado em `/tmp/opencode/peerstudy`, **privado, nunca
publicado, nunca redistributed**.

### O peer funciona sem `$038000` — e tem o corpo dele noutro sítio

Corrido sob **Proton 9.0 (Beta)** no Deck (`$HOME/.steam/root/steamapps/common/
Proton 9.0 (Beta)/proton`, `DISPLAY=:0`, `STEAM_COMPAT_APP_ID=480`), binário
Windows pré-construído `Launcher.exe`:

```
frame  9872  1900 JAN
frame 11780  1900 FEB
frame 16361  1900 JUN
frame 30356  1901 APR
```

**`1900 JAN → 1901 APR` em 20.484 frames. O relógio avança.** É o resultado que
nos falta, e prova que a implementação é possível sem executar o tick de bank
03 — o que invalida a premissa de que `$038000` tem de correr.

Duas armadilhas que custariam uma hora: o processo **morre quando a sessão ssh
fecha** (não é crash), e o `x11grab` devolve **preto** porque o Xwayland do
gamescope é rootless (o peer expõe `F8` para screenshots, que contorna).

E no peer, `$0380B2` faz `LDA.w $0B51` — a leitura do contador de 4 ticks —
com o dispatch derivado de `switch(address>>10)`, um **fall-through por índice**
que não é uma tabela de ponteiros. `$038000` tem uma única referência em toda a
árvore (`sc_v34_group_000E0.c:7`, o próprio caso de entrada), e nenhum caller.

### O mecanismo, encontrado na nossa descompilação

Varredura completa por qualquer `JSL`/`JML`/`JMP`/`BRA` para `CODE_038*`:
**zero**. O bank 03 **não é alcançado por um salto estático.** A cadeia é:

1. `$038000` — setup (`SEP #$20 / REP #$10 / PLB $03 / JSR $0390A7 / …`), e
   **cai por fall-through em `$038016`**, que é o tick.
2. O tick em `$038016` faz `INC.w $0B51` e, a cada 4 (`AND #$0003 / BNE
   CODE_0380B0`), `INC.w CurrentMonth` com o wrap Dezembro→Janeiro. **É
   exactamente o tick que medimos nunca correr.**
3. `$0380B0` é o merge point local (os testes por `$0193` reconvergem aí).
4. **Quem chega ao bloco é o town-route dispatcher, indexado por `$0193`.** E
   `$0193` é escrito em três sítios: após um `JSL $0098A0` (o dispatcher de
   input), e em dois handlers que escrevem `#$0002` e um valor do **teclado**.

**Isto é a resposta ao B1.** `$038000` é alcançado pela máquina de estados do
town route, cujo índice é posto pelo input e por handlers internos — **não por
uma tabela de dispatch que uma análise estática possa seguir.** A cadeia de
dispatch por índice do peer codifica isto; o nosso `recomp/*.cfg` não declara
nada disto, e o `$0193` nunca chega ao valor que selects o tick.

Isto também explica por que o `$038000` não aparece no manifesto como
chamado: **é uma entrada por fall-through a partir de um bloco cuja
seleção é dinâmica.** O `038000:M1X1` está analisado (134 instruções) e
compilado, mas nada o alcança porque o índice nunca aponta para lá.

### O que adoptamos: o frame model dos pares

Passamos a `GAME_MASTER_CYCLES_PER_FRAME` + 64 slices + 1 NMI/frame para o
padrão `interp_bridge_run_scheduler`, que é o que SMW, Zelda e MMX usam e o que
o peer valida. A nossa constante `357368` já está a 2 clocks da do peer
(`357366`); **o que muda é onde o NMI é(assertido)** e que a fronteira de frame
é o spin do próprio guest, não um deadline adivinhado. Isto é a mensagem que
a pesquisa de peers e o Proton nos trouxeram, e é a mudança de maior impacto
possível do lado do nosso frame loop.

---

## 2026-09-30 — frame model dos peers: tentativa, **REVERTIDA**, e porquê

O dono aprovou adoptar o padrão dos pares. Implementei-o em `GameRunOneFrame`:
o guest passa a correr com `interp_bridge_run_loop(g_resume_pc, 0x009311, 0x00B9,
flag_value=1)` até estacionar no seu próprio token de vblank, e o NMI é
entregue **depois** do estacionamento, com o guest retomado para simular. O
deadline deixou de ser o que decide a fronteira de frame.

**Os gates partiram.** `make test-rom` → **FAIL** (a imagem deixou de se mexer)
e `make perf` → **FAIL**.

A/B, com o mesmo `src/gen` e o mesmo ROM:

| | `test-rom` | `perf` |
|---|---|---|
| baseline (sem a mudança) | **PASS**, 254 crc32 distintos | **PASS**, pior 54.88 fps |
| com `run_loop` + NMI depois | **FAIL** | **FAIL** |

**Revertido, e nada commitado.** Um gate que falha existe exactamente para
isto: a mudança *parecia* correcta, seguia o padrão documentado de três jogos,
e mesmo assim partiu a renderização. Sem os gates teria sido publicada como
progresso.

### Porque é que "parecia correcta" e não era

1. **A forma do nosso loop não é a forma que `run_loop` pressupõe.**
   `run_scheduler` é `run_loop(..., flag_value = 0)` — MMX e Super Metroid
   libertam a flag **depois** de um slot walk. A nossa é(assertida) *enquanto
   se espera* e o NMI põe-na a 1. Com o `STZ $00B9` no topo do bloco, trocar a
   ordem NMI/guest limpa o token outra vez antes de o `BEQ` cair. Os pares
   não encontrados porque são **transparentes**: o host põe o token e o bloco
   não o repõe. O nosso repõe.
2. **`$009311` como `yield_pc`.** `$9311` é o `INC $00C7`, a cabeça do spin; o
   `LDA $00B9` está em `$9313`. Apontar o yield ao `INC` cede no sítio errado
   do ciclo.
3. **Mover o NMI depois do estacionamento inverte o handshake.** Esta é
   precisamente a inversão que um diagnóstico anterior mediu (817 `INC` e 817
   `STZ` emparelhados) e que eu próprio reverti nessa altura.

Isto é uma **lição negativa registada, não uma hipótese**: a adopção do padrão
dos pares não é mecânica, e este jogo não tem a forma que esse padrão assume.

### O que fica

O diagnóstico e o `exclude_range` ficam. A mudança de frame model **não**, até
alguém entender porque é que o `$930D` repõe o token que o host acabou de pôr —
que é, em última análise, a mesma pergunta do handshake, agora com a resposta
de que o problema não era o NMI chegar cedo mas **o guest repôr o token**.

E o peer, que **não tem o nosso handshake**, funciona. A diferença entre nós e
ele no MainLoop não é o frame model — é que o `STZ $00B9` no topo de `$930D`
existe. **Se o token fosse reposto só depois do `LDA`, o `run_loop` com
`flag_value` teria a forma que assume.**

---

## 2026-09-30 — o peer compila para Linux, e passa à mesma parede sem rato

### Sim, compila para Linux — e agora há um harness

`libsimcity-static-recomp.a` compila limpo em Linux (o core é C++ — precisa de
`-lstdc++` no link). O harness anterior saltava os `mouseclick`; agora
conduz-se o peer **com a nossa rota exacta**, `scripts/d_city.script`, porque a
API pública `simcity_recomp_advance_headless(inst, input_mask, frames, res)`
aceita a mesma máscara de botões que nós.

Corrida medida, 5.000 frames, ROM e script idênticos aos nossos:

```
video_standard=NTSC nominal_fps=60.098814 avg_master_clocks_per_frame=357366
RESULT failed=0 frames=5000 insns=58981392 sram_dirty=1
```

**E o peer chega ao ecrã de nome com a nossa rota** — renderizado, correcto,
`11111_`, mão sobre o SPACE. Isto confirma que a rota é boa e que o core do peer
corre o jogo até ao mesmo ponto onde nós corremos.

### Mas o rato é a parede, e é uma parede partilhada

Substituímos cada `mouseclick` por `press down` + `press a`. **A mão não se
moveu.** E o motivo está medido:

```
$0193 (town-route index):  0 -> 0
$00C5 (dispatch index):    0 -> 0
```

~~**O cursor do ecrã de nome é controlado pelo rato, não pelo d-pad.**~~

**Isto está errado, e é a retractação mais cara desta investigação.** Testei
**uma** direcção — substituí cada `mouseclick` por `press a`, a mão não se
mexeu, e generalizei. **Dez `press right` no ecrã de nome levam a mão do `SPACE`
para o `P`/backspace.** Renderizado e medido, não argumentado.

O launcher do próprio peer confirma: `grep -ri mouse frontend/` sobre o frontend
inteiro dele é **vazio** — só teclado e XInput. Nunca houve rato neste caminho.

**E isto atinge-nos a nós, que é a parte que importa.** O nosso
`d_city.script` chega a uma cidade viva usando `mouseclick`. Se o ecrã de nome
navega-se com o d-pad, a rota que construímos à volta de um rato pode estar a
resolver um problema que não existe — e o nosso suporte de rato pode ser
load-bearing ou pode ser só hábito. Não sei qual. É uma pergunta aberta, e é
muito menor do que parecia.

E aqui está o ponto que importa: **o `$0B51` (tick de 4) fica a 0 no peer
também**, exactamente como no nosso. Porque está parado no ecrã de nome, como
nós. **O clock do peer só avançou porque o Proton o conduziu com `F8` e
`xdotool` até lá** — ou seja, com um rato a sério, dentro do frontend Win32 que tem
o interface para o receber. **O core Linux, sozinho, não chega lá.**

Isto é a resposta à pergunta que eu não tinha resolvido: **o peer funciona,
mas só através do seu frontend Windows e um rato.** O core é bom; o interface é
que não é scriptável. E nós temos o oposto: interface scriptável, core
travado.

### O que isto ensina, e o que não

**Não ensina** que o relógio é insolúvel — o peer provou que é solúvel.
**Ensinha** que o obstáculo é de *entrada*, não de execução: tanto nós como o
peer precisamos de um clique de rato para criar a cidade. nós temos o
`mouseclick` no script; o peer tem de o fazer à mão no Proton.

**Isto reabre uma pergunta que eu tinha marcado como fechada.** Escrevi
anteriormente que `$038000` "não é alcançado por um salto estático" e que o
town-route index nunca é posto. **No peer, com um rato a sério, `$0193` é posto e o
relógio avança.** Portanto o índice é posting — por input, que é o que a
medição acima de `$0193=0` confirma. O que nos falta é precisamente o
**clique de rato que põe o índice**, e nós já o temos (`mouseclick`) e o
usamos — mas no nosso build o `$0193` continua a 0.

**Essa é a próxima pergunta, e é mais específica do que era:** no nosso build,
`mouseclick right` põe `$0193`? Já medi que o token `$00B9` e o cursor mexem,
mas nunca medi `$0193` depois de um clique. **É uma medição de um minuto.**

---

## 2026-10-01 — o port não está "congelado": nunca arranca. E a fechadura é `$12`

Esta é a entrada mais longa e a mais útil. Baseia-se num savestate tirado
**de dentro de uma cidade a correr** (`saves/save1.sav`, 330248 bytes), que
finalmente nos dá o estado certo para medir — sem isto, tudo o que veio antes
media o ecrã de título.

### O que está medido

**1. A cidade está a correr e nada simula.** O screenshot do estado carregado
é uma cidade cheia — prédios, stellas, painel com população e tesouro. Entre o
frame 200 e o frame 3800 (3600 frames, 60 s) mudam **29 dos 131072 bytes de
WRAM**, e todos eles são churn de ponteiros na tabela de ponteiros de `$1Fxx`.
Nenhum contador. Nenhuma data.

**2. O convidado vive na espera de VBlank do próprio jogo.** Na descompilação:

```
CODE_00930D:
	STZ.b $B9
CODE_009311:
	INC.b $C7
	LDA.b $B9
	BEQ.b CODE_009311     ; gira até o NMI mexer $B9
	RTS
```

`$B9` é posto pelo handler de NMI `CODE_0080B2` (`INC.b $B9`). Medido: `$C7`
(o contador do spin) muda violentemente entre frames e `$B9` está a 0 na
maioria das amostras. O guest passa a vida neste laço.

**3. O CPU está no handler do COP.** O PC registado no savestate é
`$00:821C` em **24 de 24** frames consecutivos, com `SP=$3809` e `DP=$005E`
inalterados. `$821C` é o `ASL` dentro de `CODE_008211`, a rotina para onde o
opcode `COP` salta.

*Caveat que tenho de escrever:* o savestate é tirado na fronteira de frame, portanto
isto diz onde o guest está quando o cortamos — não prova que nunca sai de lá.

**4. `$12` nunca é posto, e portanto o corpo de vblank nunca corre.** O loop
principal é:

```
CODE_00804D:
	LDA.b $12
	BNE.b CODE_00805C          ; -> JSR CODE_008061
	STZ.b $B7
	JSR.w CODE_008D65
	JSL.l CODE_03D283          ; o scheduler round-robin
	BRA.b CODE_00804D
```

`$12` é escrito em **um único sítio** em todo o ROM: a cauda de `CODE_03D283`,
depois de o seu laço `BPL CODE_03D287` sair. E `$12` está a 0 em **13 de 13**
amostras de fronteira de frame. Logo `CODE_008061` — que é o que regista a task
de simulação — nunca corre.

**5. Prova causal do ponto 4.** `pokefor 0012 01 400` — forçar `$12=1` durante
400 frames. Imediatamente `$1F7D/$1F7E/$1F7F` passam a `00 80 03`.
`CODE_00825F`, alcançado só a partir de `CODE_008061`, escreve exactamente
esses bytes: guarda `CODE_038000` em `$1F7D..$1F7F`. Em jogo normal esses bytes
são `00 00 00 00` em **todas** as amostras. **`CODE_008061` demonstra não correr
em jogo normal, e forçar `$12=1` fá-lo correr.**

**6. E mesmo assim a task de simulação não acontece.** Com o hook instalado
(`$1F7D..F = 038000`) à força, `$0B51`/`$0B53`/`$0B55`/`$0BA5`/`$0B9D` continuam
todos a zero 400 frames depois. Instalar o hook não basta.

### O elo seguinte — e é uma contradição que vale registar

`$1F7A..$1F7F` é **escrita-só em todo o ROM**: a descompilação tem 5
referências, todas escritas, todas dentro de `CODE_00825F`. Procurei no ROM
(bytes, não mnemónicos) todos os leitores plausíveis — `LDA/LDX/LDY/INC/DEC/
ASL/LSR/ROR/BIT` absoluto, `JMP abs`, `JML abs`, `JMP (abs)`, `JSR (abs)`,
`JSL (abs)`, `JMP (abs,X)`, `JMP (ind),Y` — contra `$1F7A`, `$1F7C`, `$1F7D`,
`$1F7E`. **Um único acerto nos 512 KB: `JSL ($1F7A)` no offset ROM `0x76DE9`.**
E `CODE_038000` não tem `JSR` nem `JSL` de lado nenhum.

Verifiquei a aritmética de offset porque já me enganou uma vez: este ROM é
**HiROM**, `offset = addr & 0x7FFF` para a bank 00 — a cauda de `CODE_008211`
cai em `0x218` e `CODE_00930D` em `0x130D`, ambos exactos. E os bytes em
`$00:825F` são `E2 20 C2 10 A9 03 8D 7F 1F ...`, que é `SEP #$20 / REP #$10 /
LDA #$03 / STA $1F7F` — a escrita é mesmo lá, o opcode é `8D` (abs) e não `8F`
(long), o que só muda a mnemónica.

Portanto ou o leitor é calculado em runtime (aritmética de ponteiros que a
descompilação desenha como tabela), ou a crença de que este campo é write-only
está errada por algum lado que ainda não vi.

### A pergunta precisa que fica

O laço do scheduler é:

```
CODE_03D287:
	LDA.b $14
	REP.b #$10
	ASL
	TAX
	JSR.w (DATA_03D255,x)
	REP.b #$20
	LDA.w #$0000
	COP.b #$00
	LDA.b $14
	BPL.b CODE_03D287        ; sai só quando $14 tem o bit 7
```

Sai quando `$14` tem o bit 7 posto. Verifiquei os 14 `STA.b $14` da região do
scheduler: os valores carregados são `$07, $50, $07, $06, $02` e o resto vem de
tabelas. **Nenhum carrega um valor com o bit 7.** Ou o bit 7 de `$14` é posto
por algo que a descompilação desenha como dado de tabela, ou há uma saída que
não encontrei.

**Encontrar o que torna `$14` negativo é a porta.** `$14` fecha `$12`, `$12`
fecha `CODE_008061`, e `CODE_008061` é a única coisa que instala o hook de
`CODE_038000`.

### Retractações: três coisas que eu escrevi e estão erradas

**`$0B51` não é evidência de nada.** É um contador **mod-4 livre**
(`INC $0B51` … `AND #$0003` … `INC month`). Está a 0 um frame em quatro *por
desenho*. Eu tratei "fica a 0" como sinal de bug; não é. O sinal real é
`$0B53`/`$0B55`, que nunca mudam.

**`$02BF` não é um flag de pausa.** É escrito **uma vez em todo o ROM** —
`$0080`, no fim de `CODE_008061`. O nosso RAM map chama-lhe `Pause`; o nome
está errado.

**`$0B53 = 0 / $0B55 = 0` é "1900 Janeiro", não "por definir".** Confirmado
pelo peer: sem cidade, o peer também fica em `000/000` para sempre. E o
`1900 JAN` que vemos no ecrã é a codificação `0/0` — o que concorda com o
ecrã e não o contraria.

**E uma afirmação anterior que não se sustenta:** "a espera de vblank em
`$009311` é saudável, não é livelock". Isso **não está demonstrado**. `$C7` e
`$B9` dizem que o guest passa a vida naquele laço, e nenhuma medição que eu
tenho separa "à espera normal" de "nunca satisfeita".

### O peer, conduzido com a nossa rota

O peer compila para Linux (`-lstdc++` no link; o core é C++) e há um harness
que o conduz com `scripts/d_city.script` pela mesma API
`simcity_recomp_advance_headless(inst, input_mask, frames, result)` que nós:

```
nominal_fps=60.098814 avg_master_clocks_per_frame=357366
RESULT failed=0 frames=5000 insns=58981392 sram_dirty=1
```

Chega ao ecrã de nome com a rota exacta — `11111_`, mão sobre o SPACE. E
`$0193` (o índice do town-route) fica `0 -> 0`.

O que escrevi aqui em cima — que o cursor é do rato e não do d-pad — está
**retractado**. Ver a secção de 2026-09-30 acima. Dez `press right` movem a
mão, e o frontend do peer não tem uma linha de código de rato.

### O que NÃO é prova

- `save.srm` é a SRAM de pilha, não o savestate da cidade. Carregá-la no peer
  dá o ecrã de título (`sram_dirty=0`), portanto esse teste foi
  inconclusivo — não prova nada sobre o core.
- O PC `$821C` ser constante é artefacto de *quando* tiramos o savestate, não
  uma prova de travamento. Está escrito acima para não ser mal citado.

---

## 2026-10-01 (b) — `$14` é uma word, e eu estava a ler o fim errado

### A correcção de largura, verificada nos bytes

A入口 da secção anterior estava errada, e por minha causa. O terminador do
scheduler existe, e é `LDA.w #$8000`, não um valor com bit 7.

Quatro sítios, todos na cauda das tasks:

| bank:addr | offset ROM | linha da descomp. | valor |
|---|---|---|---|
| `03:DA9E` | `0x01DA9E` | 41925-41926 | `$8000` |
| `03:E20F` | `0x01E20F` | 42804-42805 | `$8000` |
| `03:E27A` | `0x01E27A` | 42864-42865 | `$8000` |
| `03:E2C1` | `0x01E2C1` | 42901-42902 | `$8000` |

Bytes crus em `0x01DA9B`:

```
A9 00 80   LDA #$8000
85 14      STA $14
```

`85` é *sempre* dp-relativo e a largura segue M. Com `REP.b #$20` em
`03:DA91` (M=0) isto é uma **store de 16 bits**. E o `REP.b #$20` em
`03:D292` torna o `LDA.b $14` do laço também 16 bits, portanto o `BPL` em
`03:D29B` testa o **bit 15**, não o bit 7 do byte baixo.

A descompilação imprime `STA.b $14` com o `.b` errado. Por isso nenhum dos 14
candidatos que eu enumerei tinha bit 7: **o valor terminador nunca tem bit 7.
Tem bit 15.**

### Os meus próprios dados confirmam — e refutam-me

Re li os dumps que eu já tinha, desta vez como word:

```
frame 31-60   $0014 = $0003    bit15 = 0
frame 70      $0014 = $FFF7    bit15 = 1
```

O `$FFF7` é a prova. Eu tinha lido `$14 = F7` como "bit 7 posto, logo o laço
sai" e escrevi isso. **Estava errado pela mesma razão duas vezes**: o campo é
uma word. Lido como word, `$FFF7` é negativo, e o `BPL` **não** salta — o laço
continua, exactamente como o resto da evidência exige.

Isto é a segunda vez nesta investigação que a largura do operando, e não a
lógica, me baralhou. Registado para o próximo não perder tempo.

### `DP = $0000`, não `$005E`

O `DP=$005E` que eu li vinha do meu parse do savestate, com um offset de struct
que assumi. Medido directamente, em 51.978 frames de uma corrida de cidade:

```
[fslog] f=51977 frame-end S=1FF5 PB=00 DP=0000 resume=009311
```

`DP=0000` em todas as amostras. Confirmado estaticamente: os 49 sítios `TCD` do
ROM são frames `PHD`…`PLD` balanceados, e os únicos quatro com imediato
absoluto são `LDA.w #$0000 / TCD`, todos no handler de NMI. **Nenhum caminho de
código no ROM pode produzir `DP=$005E`.** Portanto `$14` é WRAM `$0014/$0015` e
`$12` é `$0012` — os endereços que eu assumi. O meu `DP` é que estava errado.

### O que bloqueia o laço, medido

```
$0012 (porta) = $0000     $0014 (word) = $0001     $00C9 = $0000
```

`$14` está em `$0001` — índice da task 1, `CODE_03D2C6`. A cauda em `03:D2F4`:

```
03:D2F4  A5 C9     LDA $C9          ; 16-bit
03:D2F6  29 00 90  AND #$9000
03:D2F9  F0 08     BEQ $03:D303     ; <-- TOMADO
03:D301  E6 14     INC $14          ; nunca alcançado
```

`$C9` é uma **latch de edge do joypad**, escrita só por `STA.b $C9,x` em
`CODE_00929B` (`$00:929E`) a partir das quatro leituras de comando. É `$0000`
em todas as amostras, logo `AND #$9000` dá zero, o `BEQ` é tomado, e o
`INC $14` — a única coisa que avança o round-robin — nunca corre. O índice
re-despacha-se a si próprio para sempre.

Portanto **a porta não é `$12` nem um store negativo em falta. É
`$C9 & #$9000` em `03:D2F6` (ROM `0x01D2F6`), dentro da task 1, a controlar o
`INC $14` em `03:D301` (ROM `0x01D301`).** A cadeia até à data congelada
mantém-se; tem mais um elo do que eu tinha documentado — e esse elo é uma latch
de joypad, o que explica porque a falha é total e não parcial: o scheduler é
uma lista *sequencial* e está preso no elemento 1 de ~23.

Numa corrida mais longa o mesmo mecanismo foi visto com `$14` em `$0003` durante
40+ frames sem avançar. O índice exacto preso depende do ponto de entrada, não
do mecanismo.

### O que NÃO está resolvido, e um teste que falhou

**Não sei porque é que `$C9` lê `$0000` numa cidade a correr.** Os bits `$9000`
são os dois botões altos do joypad 1. Uma latch permanentemente limpa sugere
que a detecção de edge em `CODE_00929B`
(`STA.b $BF / EOR.w $011B,x / AND.b $BF / STA.b $C9,x`) não está a ver edges, ou
`$011B,x` está a ser pré-carregado com o estado corrente do pad e o XOR
cancela sempre. **Não medi qual das duas.**

`forcepoke 00C9 0090` não serve: o poke é reescrito cada frame pela leitura do
joypad, e `$C9` continuou `$0000` em 6 dumps até ao frame 4500. Isso é
consistente, não contraditório — `$C9` não é uma latch que se possa manter, é
recalculada por frame.

**Teste que falhou, e falhou mal.** Tentei responde se input de joypad
destranca o scheduler, com `press` de todos os botões dentro da cidade viva. O
ecrã foi **a preto** e o WRAM encheu de `$48` — a sequência de input levou o
game a um estado morto, não a um resultado. Inconclusivo, e o caminho
óbvio é mais suave: um botão de cada vez, poucos frames, e ver `$14` mexer.

### Onde isto deixa a investigação

A cadeia está assim, e cada elo tem medição própria:

```
$009311  o guest espera por $B9 (NMI)          medido: $B9 = 0, $C7 a rodar
   |
$03D287  o laço do scheduler                    medido: $14 não avança
   |
$03D2F6  AND #$9000  ->  INC $14                ROM 0x01D2F6   <== A PORTA
   |
$03D2A3  $12 = 1                                 medido: $12 = 0 em 13/13
   |
$008061  corpo de vblank                        provado por pokefor: corre
   |                                           quando $12 é forçado
$00825F  escreve CODE_038000 em $1F7D..$1F7F    medido: 00 00 00 00 em jogo normal
   |
$038000  a task que incrementa o mês             medido: não corre nem com o
   |                                           hook instalado à força
$0B51/$0B53/$0B55                                  $0B55 nunca sai de 0
```

O elo a fechar é o de baixo: com o hook instalado à força, `CODE_038000` ainda
não corre. E o elo de cima é o que precisa de uma explicação de *porquê*: um
scheduler que só avança com um edge de joypad é uma coisa estranha para um
jogo, e é o primeiro ponto desta lista que cheira a **bug nosso** e não a
característica do ROM. `$C9` a zero pode ser o jogo a fazer a coisa certa com
informação que nós não lhe damos.

---

## 2026-10-01 (c) — o cross-load corre, e ainda não prova nada (mas a tubagem está provada)

O frontend windowed ficou com autosave, portanto já existe um `jj.srm` gerado
por nós — de uma sessão que chega ao **ecrã de nome**, não a uma cidade, porque
o botão de confirmar continua por identificar. Meti-o no cross-load na mesma,
para provar a mecânica antes de ter o artefacto certo.

```
loaded the peer's save: 5b4053a64602e0cb8b86d196112ad610
field          frame 600    frame 4600   moved
  $0B51 tick    $0000       $0000      no
  $0B53 year    $0000       $0000      no
  $0B55 month   $0000       $0000      no
  $0BA5 pop     $0000       $0000      no
  $0B9D funds   $0000       $0000      no
  $0014 sched   $0001       $0001      no
  $0012 gate    $0000       $0000      no
  $00C9 joypad  $0000       $0000      no

WRAM bytes moved in 4000 frames: 159 of 131072
```

E o screenshot é o que decide: **a nossa build mostra o ecrã de título**, com o
save do peer a apontar para o ecrã de nome. O estado do ecrã de nome não está no
save de pilha — é UI de fluxo, e o SRAM só persiste a cidade depois de criada.

Portanto: **inconclusivo, e é assim que deve ser lido.** Não é o veredicto
"o relógio fica congelado", porque não há cidade nenhuma para simular.

O que fica provado:

- a nossa build **aceita** o SRAM de 32 KiB do peer sem queixa, e arranca. A
  formato não é rejeitado.
- o scheduler fica preso em `$0014 = $0001`, com `$0012 = 0` e `$00C9 = 0` —
  exactamente o estado documentado para a nossa cidade viva congelada. Coerente,
  ainda que não prova nada.
- 159 bytes em 4000 frames, a mesma ordem de grandeza dos 29 em 3600 da cidade
  viva. Também coerente, também não é prova.

Continua a faltar o botão que confirma o nome. Quem já chegou a cidade把它.

---

## 2026-10-01 (d) — o cross-load é impossível, e eu devia tê-lo visto

O peer chegou a uma cidade e a data andou de JAN para FEV. Trouxe o `jj.srm`.
**Não há nada para carregar.**

```
run/jj.srm        32768 bytes  distintos=7   nao 00/FF: 10   topo=[255 x 32746, 0 x 12, 'S' x 2]
saves/save.srm    32768 bytes  distintos=5   nao 00/FF:  8   topo=[  0 x 32760, 'S' x 2]
```

Os dois SRAM são **basicamente vazios**. O do peer é `SIM` seguido de 32746
bytes de `0xFF`; o nosso é `SIM` seguido de 32760 bytes de `0x00`. A única
diferença entre eles é o byte de apagamento. **Nenhum dos dois contém uma
cidade.**

Portanto **a cidade não está no SRAM de pilha.** Está em WRAM, que é volátil.
Isto não é uma surpresa para quem leu o cabeçalho do meu próprio script — ele
diz, em linhas, que a data vive em WRAM e que por isso um save carregado vai
ler `1900 JAN` mesmo que venha de uma cidade que chegou a 1901 APR. **Eu escrevi
essa ressalva e não a segui até à conclusão.** Ela não era um detalhe sobre a
data: era a prova de que o teste inteiro não podia funcionar.

O cross-load é impossível nas duas direções, e por razões que não se resolve com
esforço:

- O `simcity_recomp_snapshot_save/load` do peer é um estado runtime *matched à
  build dele*, e o próprio header diz que não é formato de save de utilizador. Só
  o peer o carrega.
- Os nossos savestates (`save1.sav`) são o nosso WRAM no nosso formato. Nem
  intercompatíveis, nem com um caminho para lá chegarem.

**O que eu fiz:** projetei um teste sobre uma suposição que nunca verifiquei — que
um save de pilha de 32 KiB leva a cidade dentro. Escrevi o script, escrevi o
README, e chamei-lhe "o teste decisivo". Não era. Era um teste que só podia
produzir um de dois resultados, e um deles era indistinguível de "não há cidade".

### O que fica, e é melhor

A referência que eu queria **não pode ser transferida, mas pode ser observada.**
O que eu queria saber era o que um relógio do SimCity a funcionar toca. Isso
não exige mover nada entre corações: exige WRAM do peer com uma cidade viva.

E há uma第二条 via que é mais limpa do que a que eu estava a tentar, e que não
depende de mim adivinhar o botão de confirmar: **o frontend windowed já sabe
conduzir-se por script, e o peer é determinístico.** Quem já chega à cidade com
o teclado gera um traço de WRAM, e nele vemos exactamente que bytes mudam entre
JAN e FEV. Isso é a verdade de terreno que fecha a cadeia — e é a única coisa
que este teste devia ter sido desde o início.

---

## 2026-10-01 (e) — o peer tem relógio, e eu tinha a data mal

O peer, com a rota de teclado, chega a cidade e a data anda. Capturado do
frontend com `--date`:

```
[date] 1900 FEB  (raw $0B53=076C $0B55=01)
[date] 1900 MAR  (raw $0B53=076C $0B55=02)
```

Isto **corrige o que eu escrevi duas entradas atrás.** Disse que `$0B53=0 /
$0B55=0` era a codificação de "1900 Janeiro" e que por isso o nosso `0/0` não era
"por definir". É o contrário:

- `$0B53` é o **ano absoluto**, não um offset a partir de 1900. Numa cidade viva
  lê 1900 = `0x076C`, não 0.
- `$0B55` é o **índice do mês, base 0**: 0=JAN, 1=FEB, 2=MAR.
- Logo `$0B53 = 0` significa **não há cidade**, e é o que o ecrã de título mostra.

Portanto, no nosso port o `1900 JAN` que vemos no ecrã **não** é `$0B53=0`. É
`$0B53=1900, $0B55=0`. E o nosso WRAM mostra `$0B53 = $0000` em todas as
amostras — ou seja, **o nosso estado não tem sequer o ano inicializado.** Isso é
uma leitura diferente da que fiz, e mais forte: não é "a data está presa", é "a
data nunca foi escrita".

Isto está agora no código: `--date` imprime `no city yet` em vez de `0 JAN`, que
parecia uma data e não é.

### O SRAM está definitivamente fora

O `jj.srm` voltou **byte-idêntico** — mesmo md5 `5b4053a6...` — de uma sessão que
vai de JAN a MAR, e o mesmo md5 de uma sessão parada no ecrã de nome. Duas
estados de jogo completamente diferentes, o mesmo ficheiro. Com a medição
anterior (32746 bytes de `0xFF` mais `SIM`), está provado: **o SRAM de 32 KiB
não transporta a cidade.** Não há cross-load a fazer, em nenhuma direção, e
depois disto não vale a pena voltar a essa ideia.

### O que falta

Os dumps de WRAM. Correr com `--wram 300` escreve `jjwram.f*.bin` ao lado do
executável, mas a scp bringing only `jj.srm`. **São esses ficheiros que quero** —
o conjunto de endereços que mexe entre JAN e FEV é a resposta ao que um relógio
do SimCity a funcionar toca, e é a única forma de fechar a cadeia contra uma
referência que sabemos funcionar.

---

## 2026-10-01 (f) — o rato nunca foi a causa

`scripts/d_city_kbd.script` chega a uma cidade viva **sem um único
`mouseclick`**, só com o d-pad e `B`:

```
$009311  wait on $B9                                    medido
$03D287  scheduler loop                                 $14 nao avanca
$03D2F6  AND #$9000 gates INC $14   ROM 0x01D2F6        <== A PORTA
$03D2A3  $12 = 1                                        $12 = 0 em 13/13
$008061  per-vblank body                                provado por pokefor
$00825F  CODE_038000 -> $1F7D..$1F7F                    00 00 00 00
$038000  a task do mes                                  nao corre
$0B53/$0B55  a data                                     nunca sai de 0
```

```
frame    $0B53   $0B55  $0BA5(pop)  $0B9D(funds)  $0B12
  3000        0    00      0000        000000        00
  5000     1900    01      0000        004E20        00
  7000     1900    01      0000        004E20        00
  9000     1900    01      0000        004E20        00
 11000     1900    01      0000        004E20        00
 13000     1900    01      0000        004E20        00
```

E o screenshot é uma cidade completa: HUD, `$20000`, toolbar RCI, "BullDoze Area
$1", terreno e árvores. **A cidade é criada e renderizada, e o relógio está
congelado exactamente igual.** `$0B9D = $004E20` são os $20000 iniciais, o que
confirma que é uma cidade verdadeira e não um ecrã de过渡.

**Isto fecha uma variável, e é a que eu mais promotei.** O `mouseclick` no
`d_city.script` não era load-bearing. Durante semanas treatei o rato como
suspeito principal e a理由 era "precisamos dele para passar o ecrã de nome". A
verdade é que o ecrã de nome navega-se com o d-pad e confirma-se com `B` — como
o agente provou no peer, e agora reproduzido aqui.

O espaço de busca fica menor e mais honesto: **já não é "a nossa execução do
scheduler do jogo" nem "o nosso rato". É só a primeira.**

### E uma correcção de codificacao

O nosso HUD diz `1900 JAN` com `$0B55 = $01`. Logo **`$0B55` é 1-based e `$01` é
JAN**, não FEV. A tabela de meses que escrevi no `--date` do `jjwin.c` era
0-based, e imprimia "1900 FEB" para o valor que o ecrã chama Janeiro. Corrigida.

Isto mede-se contra um HUD renderizado, que é a forma certa — ao contrário da
tabela anterior, que foi inferida de um nome que eu próprio escrevi.

---

## 2026-10-01 (g) — a verdade de terreno, e duas retractações

O Deck produziu o traço de WRAM do peer com uma cidade viva ao longo de 33.700
frames e 23 meses, e o mesmo traço do nosso build. A comparação fecha a cadeia
numa só frase.

### `$0B51` é o endereço, e eu retractei a afirmação certa

O peer, cidade viva:

```
$0B51  f3800=00 f4600=01 f4800=02 f5000=03 f5200=04 ... f30000=006D
$0DC7  +7 por tick: f4600=0007 f4800=000E f5000=0015 f5200=001C f30000=005B
$0DAD  0,1,2,...,11, depois 0 a cada Janeiro
$0CE7  00 em JAN 1900, 01 do primeiro tick em diante, para sempre
```

**`$0B51` é um contador de 16 bits que sobe monotonicamente. NÃO é um contador
módulo 4.**

E é aqui que eu me engano duas vezes, e a segunda foi a pior:

1. Afirmei primeiro que "`$0B51` fica a 0 → o tick nunca corre". **Estava certo.**
2. Retractei isso: "é um contador livre módulo 4, lê 0 um frame em quatro por
   desenho, logo não prova nada". **Estava errado.**

Retractei a afirmação correcta. E foi a minha segunda retractação do mesmo
endereço, em direcções opostas, o que devia ter-me dito para não tocar nele sem
ler a rutina. A rotina está em `CODE_038016`:

```
CODE_038016:  ... JSR CODE_03ADDF
  REP #$20
  INC.w $0B51                 ; contador mestre, +1 por tick
  $0DC7 += $0DC5              ; dinheiro de imposto acumulado
  LDA.w $0B51
  AND.w #$0003
  BNE  CODE_0380B0            ; o mes so avanca quando (ticks & 3) == 0
  INC.w $0B55                 ; CurrentMonthLo
  se $0B55 == 13: $0B55 = 1; INC.w $0B53
  $0DA9 = $0B53 - 10 ; $0DAD = $0B55 - 1 ; $0CE7 = 1
```

`AND #$0003` lê os **dois bits baixos** de um contador de 16 bits. Não é
módulo 4 no sentido de "ciclo de 4". `$0B51` é o contador mestre da cidade.

### A cadeia inteira reduz-se a um facto

Nossos, cidade viva, do frame 3150 ao 30000:

| | nosso | peer |
|---|---|---|
| `$0B51` tick | `0000` **em todas as amostras** | `0000 -> 001C -> 0033 -> 006C -> 006D` |
| `$0DC7` imposto | `0000` sempre | `+7` por tick |
| `$0CE7` | `00` sempre | `00 -> 01` no primeiro tick |
| `$0DAD` | `0000` sempre | `0,1,...,11,0` a cada Janeiro |
| `$0B55` | `0001` sempre | `01 -> 0C -> 01` |
| `$0B53` | `076C` sempre | `076C -> 076D -> 076E` |

O argumento é aritmético e não admite fuga: **com `$0B51 == 0`, `AND #$0003` dá
zero, pelo que a PRIMEIRA execução da rotina avançaria o mês imediatamente.** E
nunca acontece. Logo **`INC.w $0B51` executa zero vezes** — `CODE_038016` nunca é
alcançada depois do carregamento da cidade. `$0DC7` são duas instruções depois e
também está morta, o que é corroboração independente do mesmo bloco.

O nosso build muda **53** endereços em 30.000 frames de cidade. O peer muda
**102.158**. A nossa cidade está quase inerte.

### `$0B12` era uma pista falsa — retractada

O agente reportou antes que o peer punha `$0B12 = 01` numa cidade viva. **Não
reproduz.** `$0B12` é `$00` nos 337 dumps do peer ao longo de 33.700 frames e 23
meses, e nos 61 dumps nossos. Não aparece na rotina do tick. É um par `$0B11/$0B13`
usado por aritmética sem relação. **A minha pista mais afiada era ruído**, e eu
promovi-a a "lead" no README com base num único relatório sem repetir a medição.

### A contradição de `$1F7C` continua, e é agora mais séria

`CODE_00825F` escreve `CODE_038000` em `$1F7D..$1F7F` com `$1F7C = 0`, e é a
única coisa que instala a task. **`$1F7A..$1F7F` é escrita-só em todo o ROM.**
Procurei de duas maneiras:

- opcodes que lêem o endereço directamente — `LDA/LDX/LDY/INC/DEC/ASL/LSR/ROR/
  BIT` absoluto, `JMP/JML/JSR/JSL` (abs), (abs,X), (ind),Y — contra `$1F7A`,
  `$1F7C`, `$1F7D`, `$1F7E`: **um único acerto em 512 KB**, `JSL ($1F7A)` em
  ROM `0x76DE9`.
- **imediatos** que carreguem o endereço para o usar como ponteiro — `LDA/LDX/
  LDY #$1F79..$1F7F`: **nenhum**.

Ou a descompilação está incompleta, ou o mecanismo não é um hook. O peer corre o
mesmo ROM e o relógio funciona, logo o mecanismo existe. **Esta é a pergunta, e
é a única que fica.**

---

## 2026-10-01 (h) — `CODE_038000` tem zero entradas. E a descompilação é exacta.

O agente不再是 está a procurar uma coisa:沸 fez um census de cobertura e um scan
de bytes, e o resultado é negativo e rigoroso.

### A descompilação cobre o ROM inteiro, byte a byte

Duas provas independentes:

- **Rebuild byte-exacto.** Asar sobre uma cópia do ROM reproduz o md5
  `23715fc7ef700b3999384d5be20f4db5` — idêntico.
- **Census.** 4.730 labels, 96.7% dos bytes atribuídos. Os 17.462 bytes não
  atribuídos são 16.657 de `$FF`, mais o header de 32 bytes, os vectores, quatro
  blobs de dados pequenos e uma citação ASCII do Will Wright. **Não há código
  escondido.** E as `warnpc` do próprio framework disparam exactamente nos
  limites calculados.

Onde vive o código, por bank: `00`:743, `01`:1019, `02`:580, `03`:1191, `05`:91.
Banks 04 e 06-0F: **zero código**. E `DATA_0593C1` é uma tabela de saltos viva,
`JSR (DATA_0593C1,x)` em `$05:93BD` — o bank 05 não é só paletas.

### O meu "acerto único" era um bug meu

Eu reportei `JSL ($1F7A)` em ROM `0x76DE9` como o único leitor de `$1F7C`. **O
opcode `JSL (abs)` é `$FC`, portanto a sequência tem de ser `FC 7A 1F` — e há
zero ocorrências nos 524.288 bytes.** O meu scanner casou o *operando* `7A 1F` e
renderizou-o sem verificar o opcode. Os bytes reais em `0x76DE9` são
`22 7A 1F DD EF`, e `$22` é `JSL long` com operando `$DD1F7A`. Está no bank `$0E`,
que é 100% graphics comprimido lz5 — inalcançável, não é código.

**O mesmo bug que eu cometi com o `$0B51`:** casar um valor e concluir sem
verificar o que o circunda.

Census completo de leitores de `$1F7A..$1F7F`, por scan de pares de bytes — e
isto é completo para **todos** os modos de endereçamento absolutos e indexados:
nas banks de código só há as escritas de `CODE_00825F` ($1F7F @ 0x000266,
$1F7E @ 0x00026B, $1F7D @ 0x000270, $1F7C @ 0x000275, $1F7A @ 0x00027B). Três
acenos n��o-escrita, todos eliminados como dados.

### `CODE_038000` não é chamado. De todo o modo.

`INC.w $0B51` são os bytes `EE 51 0B` e ocorrem **exactamente uma vez** em
524.288 bytes, em ROM `0x18026` = `$03:8026`. O mesmo para a data: `INC.w $0B55`
uma vez, `INC.w $0B53` uma vez.

Todas as rotas possíveis para `$03:8000` foram verificadas: `JSL` (0), `JML` (0),
`JSR $8000` (6, todos dados), `JMP $8000` (1, graphics), fall-in de `$02:FFFF`
(16 bytes de `$FF`), ponteiros de 3 bytes (todos imediatos `LDA #$0080` ou
`AND #$0080`), transferência indirecta via `$1F7C` (0), e as **32 tabelas de
saltos** do ROM — li cada palavra de 16 bits das 20 tabelas distintas: **nenhuma
entrada vale `$8000`**.

E `CODE_038000` acaba em `JMP.w CODE_038016` e **nunca retorna**: é um laço
autocontido.

### A contradição acentua-se

O peer corre **o mesmo ROM** — `SIMCITY_RECOMP_ROM_SIZE` é 524288, o nosso tem
524288, mesmo md5. E o `$0B51` do peer chega a `$006D`. Logo o peer **executa**
`$03:8026`. Logo existe uma rota que o meu opcode-scan não vê.

**A única brecha que resta no argumento: as tabelas de saltos que o jogo copia
para WRAM.** Todas as 32 tabelas que foram escaneadas vivem no ROM. Uma tabela de
saltos em RAM, construida no arranque, com `JSR (dp),Y` ou `JSR (abs,X)` a
partir de `$8000`, éidiomatica e não aparece em nenhum scan de ROM. **É aí que
procurar a seguir.**

### E uma pista concreta que não encaixa

`CODE_00825F` escreve `STX.w $1F7A` com `X = $1EFF`, o que tem de deixar
`$1F7A = EF`, `$1F7B = 1F`. **O observado é `FF 1E`.** Ou seja, `$1F7A` **não**
tem o valor que `CODE_00825F` lhe escreve. E `$1F7D..$1F7F = 00 80 03` **é** o
ponteiro correcto para `CODE_038000`.

E `pokefor $1F7A DEADBEEF` e `pokefor $1F7C DEADBEEF` persistem até ao dump —
**o guest nunca reescreve nenhum dos dois.** Logo `CODE_00825F` não está a correr
periodicamente; o slot foi escrito uma vez na criação da cidade e congelou.

Isto é compatível com o resto: se `CODE_00825F` corre uma vez e escreve o
ponteiro, e nada no ROM lê esse ponteiro, então **o ponteiro não é o mecanismo**,
e o mecanismo é a tabela de saltos em WRAM.
