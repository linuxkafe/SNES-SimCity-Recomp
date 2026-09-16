# Operational Contract — SimCity SNES PC Port

## Intent & Scope
Native C++17 static recompilation of SimCity SNES for PC platforms using SDL3.
The application extracts all graphics, palettes, map terrain, and audio from a user-supplied ROM at runtime.

## Core Rules & Invariants
- **Zero Asset Distribution**: No Nintendo/Maxis copyrighted assets (ROMs, ripped tiles, palettes, audio) may ever be committed to git.
- **Strict AES Adherence**: Every non-trivial task must go through Plan, Build, Verify, Review, and Learn phases with associated ticket artifacts in `aes/tickets/`.
- **Deterministic Replay**: The recompiled 65C816 core must remain decoupled from rendering/audio, fully deterministic, and headless-testable (`test_deterministic_replay`).
- **Standard Quality Gates**: All tests in `ctest` must pass before finishing any ticket.

## Essential Commands
- Build: `make build` (runs CMake and builds all targets)
- Test: `make test` (runs ctest with failure outputs)
- Run Game: `./build/SimCitySNESRecomp "$PWD/SimCity (USA).sfc"` (absolute ROM path; the host chdirs to the exe dir)
- Headless Smoke Test: `SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy timeout 3 ./build/SimCitySNESRecomp "$PWD/SimCity (USA).sfc"`

## Critical Files
- `src/main.c`: Host entry, config, ROM identity wiring.
- `src/game_rtl.c` / `src/game_rtl.h`: Frame loop, NMI/IRQ, VBlank wait, CPU state bridge.
- `src/host_contract.c`: ROM contract validation against `rom_identity.txt`.
- `src/gen/`: Recompiled 65C816→C output (regenerated via `tools/regen.sh`; never committed).
- `recomp/bank*.cfg` / `recomp/funcs.h`: Bank mapping configs and function tables.
- `CMakeLists.txt`: Build configuration and test registrations.

## Never-Do List
- Never bundle proprietary binary game assets in repository commits.
- Never add external dependencies without updating documentation and verifying availability via system packages.
- Never commit broken builds or skipped tests without explicit documentation.
