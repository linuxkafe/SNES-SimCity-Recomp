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

**T058: the city loads and then zero simulation ticks run.** The view is
correct, the frame loop runs once per frame for 12 000 frames, and the date
stays `1900 JAN`, the population stays 0, and the treasury stays at exactly its
initial value — so it is not a stopped clock, it is a game state that never
leaves its initial values. Random input does not unblock it.

Two hypotheses remain live and they have different owners:

1. **State transition** — the game never receives the confirmation that starts
   the city. `docs/RE_SCENARIO_NAV.md` step 10 (confirm `ENT`) is marked BLOCKED,
   and that step is exactly this transition. Not yet discriminated from (2).
2. **Emulation defect** — the input or the state it needs never arrives, for a
   hardware-modelling reason of the same family as T046/T047/T050/T057.

Discriminating them is the next piece of work, and it decides whether the fix
belongs to this repository or to the snesrecomp host.

## Next

| # | Item | Why |
|---|------|-----|
| 1 | Discriminate T058's two hypotheses | Without it there is no playable game |
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
| **`upload-present` costs more than the whole guest** | 8.13 ms/frame on the Deck against a 2.45 ms guest. That is the host's SDL present path, not the recompiler, and optimising it is a different piece of work with a different owner. It is also the obvious next lever on performance, once the clock bug is fixed. |
| **`interp816_opcode_hist_dump()` has no caller** | The per-opcode nanosecond histogram is already being collected at `interp816.c:288` and thrown away. Wiring it up would separate raw interpreter dispatch cost from the bridge's PPU-beam catch-up cost, which the current measurement cannot do. Cheap, and it belongs with a performance investigation rather than inside one. |
| **99 `lle_only` functions are ~89% of guest-execution time** | The single largest lever on emulation cost, and the reason "native" needed qualifying. The reasons are 97 `structural_poison` / 41 `empty_decode` / 8 `unproven_callee_exit` — an analysis limitation in the recompiler, not hardware. Raising the AOT fraction is a recompiler project, not a game project. |
| **No script reaches a running city** | Every performance figure in this repo is attract mode and menus, because city creation needs a mouse the scripted path cannot provide. So the 89% interpreted share and the 2.45 ms guest figure are both *unverified for gameplay* and should not be quoted as if they were. |

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
