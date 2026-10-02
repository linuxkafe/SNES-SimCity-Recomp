# Definition of Done — SimCity SNES PC Port

Pre-registered 2026-10-02 at `afceeec`. Companion to `docs/review/RUBRIC.md`
(same directory discipline, same reason: `aes/` is gitignored permanently, so
anything that must survive a fresh clone lives here).

This document exists because of a specific, measured failure. Twelve commits
produced one root cause, and then a run of retractions of this project's own
claims — their number is **not stated here**, because every prose count this
project has ever carried was wrong: it carried four of them and none in
agreement. The count is computed from the one ledger that records them:
`scripts/check-retracted-claims.sh --count`. Every retraction was
individually reasonable. The pattern that produced them is that **prose was
allowed to close work.** So this DoD is written to make that structurally
impossible rather than merely discouraged.

---

## Rule 0 — the rule the other rules implement

> **No acceptance criterion may be satisfied by a claim.**
> A criterion is satisfied only by a command that exits 0.
> If a criterion cannot be expressed as a command, it is not a criterion —
> it is a wish, and it goes in the "Not criteria" section at the bottom,
> where it is visible as a wish.

Concretely, and this is the part that matters:

- A commit message is **not** evidence. It is a claim with no exit code.
- A ticket marked `done` is **not** evidence. A ticket is a claim.
- A number written in prose is **not** evidence until the command that
  produces it is in this file, and a reader can run it.
- **An author is not a source.** "I measured it" is the exact sentence that
  precedes each retraction in this project's record. The measurement is
  evidence; the sentence is not.

---

## The delivery gate

`make clock` is the gate. It is red and must stay red until it is genuinely
green. Measured 2026-10-02 at `afceeec`, dev host and Deck:

```
1 distinct date images after f3600 (last change f3378 of 6000)
```

**A `make clock` that passes is not sufficient for delivery on its own.** It
must pass *and* satisfy D2 below, because a green clock gate on a build that
loads no city would satisfy the gate and defeat the purpose. The rubric names
this as its BLOCKER class: "`make clock` passing on a build that does not
simulate".

---

## D1 — Build and test

| # | Criterion | Command | Pass |
|---|---|---|---|
| D1.1 | The project builds from a clean build dir | `make clean && make build` | exit 0 |
| D1.2 | The unit/deterministic suite passes | `make test` | exit 0, and `test_deterministic_replay` is in the list |
| D1.3 | The rendered picture moves | `make test-rom` | exit 0 |
| D1.4 | Build works from a path that is not the author's | `cp -r . /tmp/relocated && cd /tmp/relocated && make build && make test` | exit 0 |

D1.1-D1.3 are **necessary and insufficient**. All three pass on a build whose
city does not simulate, because all three finish before the city exists. That
is not a defect in them; it is why D2 exists.

## D2 — The city runs *(the part that is actually hard)*

| # | Criterion | Command | Pass |
|---|---|---|---|
| D2.1 | The gate passes | `make clock` | exit 0 |
| D2.2 | **The city is real, not a still screen.** An independent of D2.1, because a green gate must not be satisfiable by a frozen city | `scripts/clock-gate.sh` must also emit a **city-loaded proof**: a WRAM sample showing the guest's own year word non-zero, printed by the gate itself | gate prints `city loaded: year=0x???? (non-zero)` |
| D2.3 | Time advances, not just the picture | `make clock`, which reads the **date** off the screen crop and requires >= 2 distinct date images after the city is live | `DISTINCT_AFTER >= 2` |
| D2.4 | The simulation is more than a cursor | WRAM diff between two samples >= 200 frames apart, in a live city, must exceed a **dead-city baseline** that the gate computes itself | printed bytes > printed baseline |

**D2.2 is the anti-claim clause.** It exists because entry (q) of
`docs/RE_CITY_FREEZE.md` asserted "the city does not load" (RETRACTED
2026-10-02 - the city does load) and the assertion
survived several sessions unchallenged, and because the previous generation of
this DoD would have been satisfied by that assertion. A gate that can pass
without proving the city exists is a gate that a claim can satisfy.

**D2.4 exists because of the same disease.** `verify-rom-render.sh`'s
`>= 10 distinct crc32` threshold is satisfiable by a city that sits frozen and
moves four times per 1,000 frames. A threshold that a dead build can meet is
not a gate; it is a decoration. The threshold must be set against a measured
dead-build number, and that number must be printed next to it so the reader can
see the margin.

## D3 — Evidence integrity

These are the criteria that would have caught the retractions.

**Each row now states whether its command exists.** Rule 0 applied to this table
retired two rows on 2026-10-02; their original text is kept under
`RETIRED — D3.3, D3.4` below, and the reason is stated rather than left implicit.

| # | Criterion | Command | Pass | Command exists? |
|---|---|---|---|---|
| D3.1 | No gate failure text asserts a retracted or unmeasured cause | `scripts/check-retracted-claims.sh` | exit 0 | **yes** — exit 0 at `0b8927d` |
| D3.2 | Every retracted claim is visible **at the point it was made**, not only in a changelog | same script, second pass | exit 0 | **yes** — the ledger's `where` column is checked to resolve to a real file |
| ~~D3.3~~ | ~~Numeric claims in `README.md` match a committed measurement~~ | ~~`scripts/check-numbers.sh`~~ | — | **NO — retired 2026-10-02** |
| ~~D3.4~~ | ~~A claim marked MEASURED in the claims register names the command that produces it~~ | ~~`scripts/check-numbers.sh`~~ | — | **NO — retired 2026-10-02** |
| D3.5 | Every load-bearing open question is labelled **open** in every file that mentions it | `scripts/check-retracted-claims.sh --open-labels` | exit 0 | **yes, but see below** |
| **D3.6** | **No prose count of the retractions disagrees with the ledger.** The count is *computed* from `scripts/retracted-claims.tsv`; it is never written by hand | `scripts/check-retracted-claims.sh` (section 3) · `make retraction-count` prints it | exit 0 | **yes** — added 2026-10-02, falsification demonstrated |
| **D3.7** | **Every causal assertion in a tracked file carries provenance** — a measurement, a retraction, an `OPEN` label, or an explicit "inferred" — within five lines. Applies to claims **no ledger row knows about yet** | `make check-causes` · `make check-causes-self-test` | exit 0 | **yes** — added 2026-10-02 at `8a7340f`, and **RED from the moment it was added until this commit**; see the correction below. Seeded direction still fires (3 assertions at `9624f0e`) |

**D3.6 and D3.7 exist because D3.1/D3.2 are lexical and therefore partial.**
`check-retracted-claims.sh` holds strings that have *already been refuted*. A
cause claim that nobody has retracted yet is invisible to it, and four of the five
retracted claims this audit found in tracked files were caught by **reading**, not
by running anything. D3.7 asks the structural question instead — does this line
assert a cause, and does it say where the cause came from — so it needs no ledger
row to catch a new false claim. It normalises shell escapes, markdown emphasis
and Unicode punctuation first, because the earlier guard missed
`printf "the gate is \$0012"` for exactly that reason.

### CORRECTED 2026-10-02 — D3.7's "exit 0" was a claim, and it was false for four commits

This row said *"falsification demonstrated"* and the commit that added the guard
(`8a7340f`) said the same in its message. Measured on this tree:

```
$ make check-causes; echo $?
  VIOLATION scripts/check-cause-claims.sh:30
            # A line is a candidate if it contains a CAUSE cue ("the cause is"...
  RESULT: FAIL
1
$ make check-causes-self-test 2>&1 | tail -2
  SELFTEST FAIL: the guard fires on the CURRENT tree as well as the old one
1
```

Cause, measured: `CLOCK_NOUN` contains the bare alternation `date`, and line 30 of
the guard's own header contains `candiDATE`, so the guard fires on its own
documentation. **It has been red since the commit that introduced it.**

**This is Rule 0 applied to the DoD itself**, and it is the reason the row is
corrected here rather than left with a ticket reference: a criterion table that
asserts a green gate which is red is a claim, and this document exists to make
that structurally impossible.

**Closed in this commit, by fixing the guard and not by weakening anything.** The
fix is `\b` on the bare alternatives in `CLOCK_NOUN`; no scope exclusion, no
marker word near the offending line, no deleted header. Falsified three ways
before it was committed (T094): reverting the boundary alone turns the guard red
again, a synthetic unlabelled causal sentence still fires, and the same sentence
labelled `HYPOTHESIS` does not. `make check-causes` and its self-test both exit
0, and the self-test still reports **3** seeded assertions against `9624f0e` — so
the false positive went and no detection power went with it.

`docs/review/validate-findings-c041.sh` now checks this class of claim directly:
it compares the README's stated verdict for each cheap gate against that gate's
real exit code, and **fails when they disagree**. That check has already earned
its place — it reported REFUTED the first time it ran against a fixed guard and
un-updated prose.

**D3.7's known limit, stated so it is not oversold:** it checks *labelling*, not
*truth*. A confidently wrong cause that carries the word "measured" passes it. No
lexical guard can do better, and a guard that implied otherwise would be the same
disease it was written against.

D3.1 and D3.2 are the guard whose absence produced this session's work. The
gate's failure text asserted `$0012` was clear; `$0012` measured `0001` in 5/5
samples (`aes/decisions/D003.md`). A gate that teaches the wrong answer is
worse than a gate that reports none, because the wrong answer is what the next
session starts from.

**D3.5 is implemented and still inadequate, and both halves of that are load
bearing.** Its check is: if a file matches `why (does|did) the cit(y|ies) not
simulate|not simulate`, the *same file* must contain `OPEN` or `not established`
somewhere. A single occurrence anywhere in a 3,400-line document satisfies it.
**It would have passed on every retracted claim in the ledger**, every one of which
lived in a file that also contained the words "not established" somewhere else.
It is a floor, not a check. It is listed as a criterion because it is a real
command that exits 0, not because it does what its sentence says.

### RETIRED — D3.3, D3.4 (2026-10-02, at `0b8927d`)

Retired for the reason this document specifies for a criterion with no command:
**`scripts/check-numbers.sh` does not exist.** Measured:

```
$ ls scripts/check-numbers.sh
ls: cannot access 'scripts/check-numbers.sh': No such file or directory
$ grep -n "check-numbers" Makefile
(no output)
```

Original text, kept verbatim:

> | D3.3 | Numeric claims in `README.md` match a committed measurement | `scripts/check-numbers.sh` | exit 0 |
> | D3.4 | A claim marked MEASURED in the claims register names the command that produces it | `scripts/check-numbers.sh` | exit 0 |

**What was given up:** nothing was verified by this retirement. Two criteria
that were never satisfiable are now named as such, which is strictly more honest
than a green-looking table. **What it costs:** rubric criterion **E-04** ("numeric
claims are current") is **UNVERIFIED, not passing**, and stays that way until
the script exists. Writing it is ticket T091. Until then, the numbers in
`README.md` are maintained by hand and by `docs/measurements/`, and a reader who
wants them checked has no command to run — which is the same position this
project was in before `check-retracted-claims.sh` existed, and the reason the guard took
this long to build.

## D4 — Legal

| # | Criterion | Command | Pass |
|---|---|---|---|
| D4.1 | The ROM is not in the tree | `git ls-files \| grep -ci '\.sfc$'` | `0` |
| D4.2 | No battery save is in the tree | `git ls-files \| grep -ciE '\.(sav\|srm)$'` | `0` |
| D4.3 | `aes/` and `.aes/` are gitignored and never committed | `git ls-files \| grep -cE '^(aes\|\.aes)/'` | `0` |
| D4.4 | No unlicensed peer source is vendored | `git ls-files study/` matches the allowlist only | empty diff |

**D4.3 is a permanent rule, not a preference.** It was violated once
(`92fe59c`), reverted (`4b974f5`, `54f5079`), and a process requirement was
allowed to justify it in the first place. The rubric lives in `docs/`
*precisely because* of this rule. **A process requirement yields to D4.3.**

---

## Not criteria

Named here so they cannot be smuggled in as criteria later.

- **"The root cause is known."** It is not, as of `afceeec`. `$0B51 = 0000` is
  measured; why is open (`aes/tickets/T080`). Writing a cause here would
  satisfy nothing and would satisfy it *visually*, which is the failure mode.
- **"The picture is correct."** A city rendering plausibly is not correctness.
  No pixel-assertion suite exists.
- **"60 fps."** `make perf` is frame-locked on the Deck — five runs, identical
  to the millisecond — so it cannot detect guest slowdown. It is a
  "does it still run" check. See `aes/decisions/D006.md`.
- **"Tests pass."** Two tests. One of them runs 30 frames.
- **Anything the author asserts about their own intent.** The retraction record
  is eleven deep precisely because "I checked" preceded each one.

---

## How a criterion gets added

1. It must come with a command and an exit code, in this file, before the work.
2. If no such command exists, the thing is added to **Not criteria**, not to D.
3. Adding a criterion to D1 after the work is done is permitted; adding one to
   D2 after the work is done is not, and neither is softening one. A criterion
   that has never failed has not been tested — see the project's own record on
   `verify-rom-render.sh`, whose first run failed *on the good build* and had to
   be fixed before it meant anything.

## How a criterion gets removed

Only by weakening it into a claim, which this document exists to make visible.
If a criterion must be removed, the removal is a commit whose subject says what
was given up, and the retired criterion's text stays in the file under a
`RETIRED` heading with the date. Nothing is deleted outright — that is the one
practice this project's retractions share a common cause with.
