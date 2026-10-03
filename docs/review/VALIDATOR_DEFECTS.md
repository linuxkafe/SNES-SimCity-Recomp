# The T101 peer-review validator: three self-defects, and what a clone inherits

**Read this before treating `aes/peer-reviews/T101/validate.sh` as an approval.**
It lives under `aes/`, which is **gitignored permanently and by rule** (DoD D4.3),
so **this file is the only part of the subject that reaches a clone.**

The script was attacked on 2026-10-03 with seeded violations. Two of its three
known self-defects were real and are **fixed**; the third was already fixed and
was **re-verified**. Each line below is a *seeded violation* and what it produced
— not a description.

## D1 — VACUOUS SUBJECT · **fixed, and it was producing false GREEN**

> The block read `if <grep finds a refuted phrase> then bad else ok`.

**Seeded:** rename the string `CLOCK: FAIL` in `scripts/clock-gate.sh`.
**Result before the fix:** `PASS  no refuted-cause phrase in the FAIL text`.
**Because** the extraction was empty, `grep` matched nothing, and the `else`
branch ran. **A check that passes when its subject has been deleted is not a
check.**

**Fixed:** the extraction is asserted non-empty **and** asserted to cover **both**
FAIL branches (`no city was loaded` / `the date did not advance`) before anything
is concluded, and the scan now reads the emitted text rather than the script's
source.

| re-falsification | result |
|---|---|
| rename `CLOCK: FAIL` | **`FAIL  E-03 could not find the FAIL text at all`** (was PASS) |
| delete **one** of the two FAIL branches | **`FAIL  E-03 found only 1 of the 2 FAIL branches`** (was PASS) |
| restore | **`ALL CHECKS PASS`**, rc 0 |

## D2 — PINNED COUNT · **fixed, and it was producing false RED**

> The block read `[ "$n" = "28" ]`.

**Seeded:** nothing. The legitimate T102 retraction **R-037** moved the ledger
from 28 to 29, and the validator **FAILED** — *"review says 28,
`make retraction-count` says '29'"*. **Adding a true statement to the record
invalidated the thing that checks the record.** That is **CONF-14 again, inside
the validator**: a count typed as a literal instead of cited.

**Fixed:** the review's 28 is a **floor, not an equality**. The count may only
go up.

- count **< 28** → **FAIL** — a retraction was *undone*, which is the direction
  that actually matters;
- count **> 28** → **pass, and say so**, printing how many were added since and
  naming the ledger as authoritative.

## D3 — CASE-INSENSITIVE MARKER REGEX · **already fixed; re-verified**

The first version used `re.I` on the marker pattern, so a seeded refuted claim
appended to the end of `README.md` **PASSED** because an unrelated sentence six
lines above said a framebuffer claim "was wrong".

**Re-falsified 2026-10-03:** an unmarked `stops ticking productively / rising is
not simulating` sentence appended to `README.md` →
**`FAIL  README.md has an unmarked refuted claim -> 1 site(s)`**. The fix is
present and works. The marker list is now case-sensitive by explicit
enumeration, matching `scripts/check-retracted-claims.sh`.

## STILL OPEN — and this validator must not be cited as a project-wide gate

- **It reads exactly two files** for unmarked retractions (`README.md`,
  `docs/RE_CITY_FREEZE.md`) and inherits `make check-claims` for breadth.
- **It hard-codes substrings** of the T101 measurement doc (`28 month rolls`,
  `1902 MAY`, `f13080`, `f2460`). Those are a *record* of what that review rested
  on; they are not re-derived.
- **It performs no heavy run and no Deck access.** Every Deck number it restates
  is quoted from the session and marked as such.
- **It cannot settle the limitation that matters.** The four personas in
  `REVIEW.md` were written by the same model in the same session and are **not
  independent**; the verdict is **REJECT WITH CONDITIONS**; and the candidate
  **stays CANDIDATE** until someone who did not author the session runs it and
  records the output.

## Why the fixes are not in a tracked script

Because there is nowhere tracked to put them: `aes/` is gitignored by rule, so
the fixed script exists only on the machine that fixed it. **That is the honest
state and it is the same state as the review itself** — a clone inherits this
file, the caveat, and the rubric, and inherits neither the review nor its
validator.
