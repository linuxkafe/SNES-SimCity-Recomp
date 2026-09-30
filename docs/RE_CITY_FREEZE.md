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
deste repositório consegue ver este bug**, e nenhum把它们 faz:

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
