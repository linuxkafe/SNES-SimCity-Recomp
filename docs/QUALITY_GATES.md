# Quality Gates — SimCity SNES Port

> **Provenance, because it is not obvious and it matters.**
> This file and `docs/DECK_RUNBOOK.md` were created as the stated remedy of
> **`aes/decisions/D015.md`**, an `aes-project-manager` decision record dated
> 2026-10-03 — a *skill* writing into `docs/` as a side effect of answering a
> question. Neither existed when the T102 session opened. They were **untracked**,
> and both evidence gates read untracked-not-ignored files (CONF-11's fix), so
> `make check-causes` was **red on this machine for a reason no clone would ever
> see**, and `docs/DECK_RUNBOOK.md` was **teaching two of the mistakes this
> session measured** — the octal `COUNT_PC` parse (backwards) and backgrounding
> Deck runs (CONF-13). **Both are now TRACKED, both are corrected, and the
> runbook's original wrong text is quoted rather than deleted.** See
> `docs/CONFLICTS.md` **CONF-16** and **CONF-15**.
>
> Tracking them also closes the asymmetry CONF-16 exposed: a tracked
> `docs/CHECKLIST.md` was citing a file the repository did not contain, which is
> CONF-1's shape.

Domain-specific gates beyond the generic checklist in `docs/CHECKLIST.md`.

## ROM-dependent gates

These need a user ROM and are **not** ctest targets. A tree with no ROM must
**fail** them, not skip them — a gate that reports green on work it did not do
teaches a false fact.

| Gate | Command | Threshold | Why |
|------|---------|-----------|-----|
| `make test-rom` | `scripts/verify-rom-render.sh` | >= 10 distinct crc32 over frames 200-800 | **[MEASURED, dev host `seyon`, 2026-10-02]** Proves the picture moves. It also **passes on a frozen city** — a frozen city still moves 4×/1000 frames, **[MEASURED]** — so it is a floor and **not** a simulation gate. |
| `make clock` | `scripts/clock-gate.sh` | >= 2 distinct date images after f3600, **and** city loaded proof | **[MEASURED]** The only gate in this list that can see a dead city. Requires `--rom` and `--frames > 3600`. The floor is **calibrated, not assumed**: the reference build produces **29** distinct date images in 30 000 frames, Deck-native. |
| `make perf` | `scripts/perf-gate.sh` | >= 50 fps | **[MEASURED]** Machine-dependent; frame-locked on the Deck, so it cannot detect guest slowdown **there**. |

## Evidence-integrity gates

| Gate | Command | Purpose |
|------|---------|---------|
| `make check-claims` | `scripts/check-retracted-claims.sh` | Fails when a script/doc asserts a refuted cause without a retraction marker. |
| `make check-causes` | `scripts/check-cause-claims.sh` | Structural: every causal assertion carries provenance. Checks labelling, not truth. |
| `make retraction-count` | `scripts/check-retracted-claims.sh --count` | Computes the retraction count from the ledger. No prose count may differ. |

## Review gates

| Gate | Command | Purpose |
|------|---------|---------|
| `make review-check` | `docs/review/validate-findings.sh` | Validates review findings against the tree. |
| `make review-check-c041` | `docs/review/validate-findings-c041.sh` | Re-reads ROM-byte claims from the ROM; fails if a tracked file asserts a green gate while the gate is red. |

## How a gate is added

1. It must come with a command and an exit code, in this file, before the work.
2. If no such command exists, the thing is a **not a criterion** — see `docs/DEFINITION_OF_DONE.md` Rule 0.
3. A gate that has never been seen to fail has not been tested. The first use of a new gate must prove it fails on the good build.