# Vision — SimCity SNES PC Port

> **Rewritten 2026-09-28.** The previous version described a clean-room C++17
> *reimplementation*. That approach was analysed in T026/T027 and abandoned; the
> project pivoted to static recompilation in T031 (see "Why it changed" below).
> This document was the one place still describing the dead approach, which is
> what made "where are we in the migration?" an unanswerable question.

## 1. Problem Statement

SimCity for the Super Nintendo (1991) is a seminal city-building title with
iconic visual aesthetic, Mode 7 rendering, distinctive music, and streamlined
simulation mechanics. Playing it on a modern PC otherwise means running a
general-purpose emulator — with its own legal grey area, its own accuracy
trade-offs, and no native integration with the host OS.

## 2. Proposed Solution

**Static recompilation**: the game's own 65816 code is translated ahead of time
into native C, linked against an emulation of the SNES hardware, and run natively
on the host. The shipped binary executes *the original game's logic* — its
menus, its city simulation, its disasters — at native speed, while every pixel
it draws still comes from the game's own tiles and palettes.

Concretely:

- **snesrecomp** (submodule) provides the recompiler and the hardware runtime:
  the 65C816 interpreter and AOT recompiler, PPU, APU, DMA/HDMA, joypad, and the
  SDL3 desktop host (launcher, window, presenters, audio, save states, rewind).
- **This repository** supplies the title: bank configuration (`recomp/*.cfg`),
  the frame model (`src/game_rtl.c`), the host contract and ROM identity checks
  (`src/host_contract.c`), and the build.
- **AOT, not only interpretation.** Functions declared in `recomp/*.cfg` and
  seeded by `tools/regen.sh` compile to native C — currently 187 of them. The
  rest runs in the interpreter.

## 3. Core Values

**Legal Purity**: 0 bytes of proprietary copyrighted data committed. Every tile,
palette, map and audio byte is read at runtime from a ROM **the user owns**.
`src/gen/` — the generated C, which is derived from the ROM — is gitignored for
the same reason, and is regenerated per machine by `tools/regen.sh`.

**Faithfulness over convenience**: the game is the game. Fixes belong in the
hardware emulation (a missing bus mirror, a wrong PPU register) and not in
replacements for the game's own logic. This is what T046/T047/T050/T057 are:
each a case of the recompiler or the runtime being wrong about the hardware,
never the game being rewritten.

**Measured, not asserted**: every claim in this repository carries a number.
`make test-rom` is a gate that exists because `ctest` could not see a black
screen, and because `src/gen/` is unversioned — a regeneration can change the
code from 0 to 216 AOT functions without leaving a trace in any commit.

**Engineering Rigor**: the Ambrósio Engineering System (AES) — plan, build,
verify, review, learn, with ticket artefacts under `aes/`.

## 4. Non-Goals

- No ROM hacking or fan-translation support.
- No 3D or modern-graphics overhaul: the renderer is the game's own.
- No mobile ports.

## Why it changed

T026 chose a clean-room reimplementation over recompilation, and T027 re-affirmed
it. T031 reversed that decision and the project pivoted: recompilation *is* the
purpose. The reasoning that survived, and is still the reason:

- **SimCity has no coprocessor.** Super FX is StarFox's differentiator, and
  SimCity does not use it, so recompilation's headline advantage does not apply
  — this was the original argument against.
- **But**: a reimplementation must re-derive the game's behaviour, and every
  visual and mechanical detail that differs is a permanent defect. A
  recompilation runs the original logic, so fidelity is the default rather than
  the goal. The widescreen renderer and the soft mouse, which are additive and
  impossible in a reimplementation, are the clearest evidence.

The original objection to recompilation (licence) was resolved by accepting
PolyForm Noncommercial 1.0.0 in T031.
