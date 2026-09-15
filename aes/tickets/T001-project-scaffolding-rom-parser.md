---
ticket: T001
title: Project scaffolding + ROM parser
sprint: sprint-01
priority: high
status: pending
created: 2026-09-11
---

# T001 — Project scaffolding + ROM parser

## Context
This is the foundation of the SimCity SNES PC port. We need a project structure
(CMake, src/, tests/, docs/) and a ROM reader that can parse the SNES LoROM
format and expose game data to the rest of the application.

## Acceptance Criteria
- [ ] CMake project builds with `make build`
- [ ] ROM parser loads `*.sfc` file and validates SNES header
- [ ] ROM parser detects LoROM/HiROM mapping and exposes address translation
- [ ] Unit tests for ROM header parsing, bank mapping, address translation
- [ ] CLI tool can list ROM metadata (title, mapper, size, checksum)

## Scope
**In scope:** CMake scaffolding, ROM file reader, SNES memory mapper, header parser.
**Out of scope:** Graphics extraction (T002), game logic, sound, UI.

## Dependencies
- None (this is the root ticket)

## Rollback
Remove the `build/` directory and `src/`, `tests/`, `docs/` scaffolds if problems.

## Known Risks
- ROM format edge cases (different mappers, extended header)
- Checksum validation may fail on some dumps

## Notes
- ROM in workspace: `SimCity (USA).sfc` (MD5: 23715fc7ef700b3999384d5be20f4db5)
- Format: LoROM, 512KB, header at 0x7FC0