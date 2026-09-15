# Operational Contract — SimCity SNES PC Port

## Intent & Scope
Native C++17 reimplementation of SimCity SNES for PC platforms using SDL2.
The application extracts all graphics, palettes, map terrain, and audio from a user-supplied ROM at runtime.

## Core Rules & Invariants
- **Zero Asset Distribution**: No Nintendo/Maxis copyrighted assets (ROMs, ripped tiles, palettes, audio) may ever be committed to git.
- **Strict AES Adherence**: Every non-trivial task must go through Plan, Build, Verify, Review, and Learn phases with associated ticket artifacts in `aes/tickets/`.
- **Deterministic Simulation**: `sim::City` core must remain decoupled from rendering/audio, fully deterministic, and headless-testable.
- **Standard Quality Gates**: All tests in `ctest` must pass before finishing any ticket.

## Essential Commands
- Build: `make build` (runs CMake and builds all targets)
- Test: `make test` (runs ctest with failure outputs)
- Run CLI ROM info: `./build/rominfo "SimCity (USA).sfc"`
- Run Tile viewer: `./build/tileview "SimCity (USA).sfc" 0x10000`
- Run Game: `./build/simcity "SimCity (USA).sfc"`
- Headless Smoke Test: `SDL_VIDEODRIVER=dummy timeout 3 ./build/simcity "SimCity (USA).sfc"`

## Critical Files
- `src/snes/rom.h` / `src/snes/rom.cpp`: Low-level LoROM memory mapping and header parsing.
- `src/sim/city.h` / `src/sim/city.cpp`: Core simulation rules (budget, RCI, zoning, terrain).
- `CMakeLists.txt`: Build configuration and test registrations.

## Never-Do List
- Never bundle proprietary binary game assets in repository commits.
- Never add external dependencies without updating documentation and verifying availability via system packages.
- Never commit broken builds or skipped tests without explicit documentation.
