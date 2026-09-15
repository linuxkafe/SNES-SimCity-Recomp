# Vision — SimCity SNES PC Port

## 1. Problem Statement
SimCity for the Super Nintendo (1991) is a seminal city-building title with iconic visual aesthetic, Mode 7 rendering, distinct music, and streamlined simulation mechanics. However, playing it on modern PCs typically requires general-purpose SNES emulation rather than a native, high-performance, moddable executable.

## 2. Proposed Solution
A clean-room native C++17 reimplementation that:
- Runs natively on Linux and other desktop operating systems with SDL2.
- Requires no copyrighted asset distribution: all tiles, palettes, audio, and initial maps are extracted from the user's authentic SNES ROM at runtime.
- Preserves the deterministic simulation behavior of SimCity while offering crisp scaling, high frame rates, and keyboard/mouse native controls.

## 3. Core Values
- **Legal Purity**: 0 bytes of proprietary copyrighted data committed.
- **Architectural Modularity**: Decoupled engine layers (ROM extractor, simulation, renderer, presentation).
- **Engineering Rigor**: Strict adherence to the Ambrósio Engineering System (AES), full verification loops, and deterministic headless tests.
