# Roadmap — SimCity SNES PC Port

> **Rewritten 2026-09-28.** The previous version listed Sprint 01 as current and
> a backlog of T007–T016 — tickets from the abandoned reimplementation, several
> of which were archived in T038 and one (T016, multiplayer) which is still
> marked a non-goal in the same file. `aes/kanban.md` is the live tracker; this
> file is the *shape* of the work, not the queue.

## Where the project is

**The migration to static recompilation is done.** T031 closed it. The shipped
binary runs the game's original 65C816 logic through snesrecomp, and 187
functions are compiled ahead of time to native C. What is *not* done is the
game being playable end to end, and the reason is precise and narrow.

### Working

| Area | Evidence |
|---|---|
| Recompiled 65816 drives the PPU | `test_deterministic_replay`, `ctest` 1/1 |
| Title screen, city view, HUD, tool palette | frame 400 = ROM title logo; frame 1200 = city |
| Runtime stability | 12 000 frames headless, no watchdog, no `TRAP_BADPB` |
| Game-native graphics and palettes | decoded from the user's ROM at runtime |
| AOT of declared functions | 187 functions; behaviourally identical to 0 AOT, byte for byte |
| Mouse (P2), resolution presets, save states, rewind, turbo, config bar | T041–T054 |
| Headless capture gate | `make test-rom` — proven to fail a black build |

### Broken — the one thing that matters

**The game hangs as soon as the city loads.** It is not a slow simulation and
not a frozen clock.

- The last picture change in a 12,000-frame run is **frame 3382**. After that
  the screen is bit-identical for 8,600 frames.
- **Input does nothing** — 1,700 frames of deliberate presses and clicks after
  the city is live produce no change at all.
- CPU state sampled 800 frames apart is byte-identical except for `A`, which is
  the frame counter. `S=1FE4` never moves, so no subroutine is entered. The only
  interpreter PC that executes is `$0092E3`.

That is a livelock in the **VBlank wait loop** at `$9311` — the same address
`recomp/bank00.cfg:32` already pins with `force_lle`, and the same family as
T050. The `$0406` counter still ticks at +1/frame because the NMI path runs;
the main loop just never gets past the wait.

**What has been eliminated**, each by measurement rather than argument: game
flow (solved, `scripts/d_city.script`), headless mode (a real display and real
audio on the Deck produce byte-identical WRAM *and* screenshot), cross-machine
divergence, frame pacing, a pause/speed gate (forcing `$0408=0` arms `$040A`
and its consumer then runs every frame without producing a month), and the
"slow-moving bytes" as the date field (`$2510-$251F` is periodic on a ~10,000
frame cycle — animation phase).

**The next step is narrow and mechanical**: what is `$9311` waiting for, and why
does it never arrive? That is a disassembly of the loop and of whatever sets its
flag, plus the AOT/LLE boundary — the fork documents that AOT code never
advances the PPU beam while the interpreter advances it per opcode, which turns
a VBlank wait into a hang rather than into a wrong picture.

Three weeks of WRAM diffing produced the idle counters at `$007C`/`$01B3`/
`$01D5` because a livelocked guest has no slow-moving game state to find. Do not
diff again; disassemble.

## Next

| # | Item | Why |
|---|------|-----|
| 1 | Disassemble the `$9311` VBlank wait and find what sets its flag | The game livelocks there. Not a diff question — the guest has no game state to diff |
| 2 | Gate the clock, not the motion | `make test-rom` proves the picture moves; a frozen city moves 4×/1000 frames, so the gate would pass. The gate must ask whether the date advances |
| 3 | T025 content assertions | Prove the screen is the *right* screen, not just a moving one |
| 4 | Config bar auto-hide (F1) | Requested; the bar covers 21 of 224 rows |
| 5 | snesrecomp: "writes a hardware register → `lle_only`" | T057's root cause, defence in depth. Class size measured at 1 function, already pinned |

## Discovered, not yet ticketed

Found while measuring T061. Recorded here rather than fixed, because each is
either a different owner or a change that should not ride along on a
performance ticket.

| Item | Why it is not fixed here |
|---|---|
| **`<exe> <rom> --script X` silently ignores `--script`** | A real bug, in the snesrecomp fork (`host_main.c:2717`, `2759`): flags are consumed from the front, so `--script` must precede the ROM. It fails *silently* — exit 0, no warning, and the run is indistinguishable from one without a script. Worth noting that this invalidated a first round of scripted evidence in T061, caught only because the output was byte-identical to the unscripted run. |
| **`upload-present` costs more than the whole guest** | **RETRACTED as stated (2026-10-02).** The 2.45 ms Deck guest figure was never re-verified; the register's own re-measurement of the same stage was **4.511 ms**, i.e. 27% of the budget rather than 15%. At 4.511 vs 6.540 the ratio is 1.45x, not 3.3x, and the three stages sum to 16.97 ms against a 16.67 ms budget, so the frame is oversubscribed on the Deck. **"The 65816 is not the bottleneck" is therefore UNPROVEN and is not asserted.** No current per-stage split exists: the Deck cannot build this project (SteamOS, no glibc headers), so every Deck figure comes from a copied binary. Re-measure before using this row to justify any work. |
| **`interp816_opcode_hist_dump()` has no caller** | The per-opcode nanosecond histogram is already being collected at `interp816.c:288` and thrown away. Wiring it up would separate raw interpreter dispatch cost from the bridge's PPU-beam catch-up cost, which the current measurement cannot do. Cheap, and it belongs with a performance investigation rather than inside one. |
| **99 `lle_only` functions are ~89% of guest-execution time** | The single largest lever on emulation cost, and the reason "native" needed qualifying. The reasons are 97 `structural_poison` / 41 `empty_decode` / 8 `unproven_callee_exit` — an analysis limitation in the recompiler, not hardware. Raising the AOT fraction is a recompiler project, not a game project. |
| ~~**No script reaches a running city**~~ **RESOLVED (2026-09-30) — false.** `scripts/d_city.script` reaches a live city headlessly; `mouseclick right` does reach the guest, `press left` wraps the key list, four `press down` land on `ENT`. Deterministic across runs. The performance figures above are therefore still unmeasured *in a city*, but the excuse is gone: the route exists and the next person can just run it. |

## Non-Goals (Explicitly Out of Scope)

- ROM hacks and fan translations.
- 3D rendering or a modern-graphics overhaul.
- Mobile ports.
- **Multiplayer.** T016 appears in older revisions of this file as a backlog
  item; it is not planned. Local hotseat is not a goal.

## Archive

T010, T012, T013, T019, T020, T022, T024, T028, T029 and T038-touched tickets
referenced the deleted pre-recompilation `src/gfx` architecture and were archived
in T038. T011, T030 and T033 were re-scoped onto the recompiled game. T026, T027,
T031, T032, T035, T036, T037, T039, T040–T044, T046, T047, T049, T050, T053–T057
are closed. See `aes/kanban.md`.
