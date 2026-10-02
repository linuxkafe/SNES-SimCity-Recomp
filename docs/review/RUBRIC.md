# Review Rubric — SimCity SNES PC Port

**Candidate under review:** the clock investigation and the working tree it lives in.
**Pre-registered before review.** A review against an unregistered rubric is invalid.
**Hash:** see `RUBRIC.sha256`, same directory, committed with this file.

**Why this lives in `docs/` and not `aes/`.** `aes/` is local project management
and is gitignored, permanently and by rule. The protocol wants a committed,
hashed rubric; putting the rubric inside `aes/` would have meant force-adding it
past that rule to satisfy a process. The process yields to the rule.

This rubric replaces nothing. It exists because the only other rubric available
(`templates/review-rubric.md` in the AES skill) is `NARRATIVE-INTEGRITY-SUITE`,
whose criteria are all about the AES toolkit's own shell scripts and cannot
legally be cited for a finding about a SNES emulator. Every finding must name a
criterion below, or it is rejected by format.

The four dimensions exist because this project's characteristic failure is
**assertion outrunning measurement**. Twelve commits, one root cause, then eleven
retractions of the same claim in both directions. A rubric that only asked "is the
code good" would have passed all of that.

---

## C — Correctness

Claims about what the guest CPU executes, and about our own implementation of it.

| ID | Criterion | Verifiable Check |
|---|---|---|
| C-01 | Any instruction located by CPU address is verified at ROM-byte level, with the ROM offset stated | Every such claim in `README.md` / `docs/*.md` cites an offset; spot-check 3 of them: `python3 -c "d=open('SimCity (USA).sfc','rb').read(); print(d[0x18026:0x18029].hex())"` gives `ee510b` for `$03:8026` |
| C-02 | ROM offset arithmetic is stated and self-consistent | Docs state HiROM `offset = bank*0x8000 + (addr & 0x7FFF)` for banks 00-3F; and no doc simultaneously claims `$00:930D` is at file offset `0x930D` |
| C-03 | `recomp/*.cfg` `exclude_range` values resolve to the CPU addresses they claim | For each `exclude_range` in `recomp/*.cfg`, mask with `0x7FFF` and confirm the bytes there are the code the comment names |
| C-04 | `make test` passes on a clean checkout at a path other than the author's | `cp -r . /tmp/relocated && cd /tmp/relocated && make build && make test` exits 0; no absolute path to the author's home appears in `tests/` |
| C-05 | Determinism holds | `make test` includes `test_deterministic_replay` and it passes |

## D — Determinism and the frame model

| ID | Criterion | Verifiable Check |
|---|---|---|
| D-01 | Game time is driven by master cycles, not wall clock | `src/game_rtl.c` computes the frame boundary from `master_cycles`; no frame boundary derives from a host timer |
| D-02 | Host slowness cannot change emulated behaviour | The only host-side work cap is a *resume* (guest re-enters at the recorded PC), never a truncation; a cap that discards work is documented as a correctness risk |
| D-03 | Measured per-frame work fits the frame budget with stated headroom | `SNESRECOMP_HOST_PROFILE=1` on 1200 frames: sum of non-`deadline-wait` stages < 16.667 ms, and the figure is written down |
| D-04 | Budget figures are not double-counted | Work and wait are summed separately; no doc adds `deadline-wait` to the work total when claiming oversubscription |

## E — Evidence integrity

The dimension this project fails most, and therefore the one weighted hardest.

| ID | Criterion | Verifiable Check |
|---|---|---|
| E-01 | Every causal claim in `README.md` points at a measurement, or is labelled a hypothesis | Every "because"/"root cause" sentence resolves to a command, a file:line, or the words "hypothesis"/"not established" |
| E-02 | Retractions are visible where the claim was made, not only in a changelog | `grep -c "retract" docs/RE_CITY_FREEZE.md` > 0 **and** the retracted claim no longer appears as fact in `README.md` |
| E-03 | No retracted claim survives as an assertion in the clock-gate's failure text | `scripts/clock-gate.sh` FAIL message contains no claim that appears in the retraction log |
| E-04 | Numeric claims are current | Every fps, ms/frame, byte-count and address figure in `README.md` matches a measurement recorded in `docs/` |
| E-05 | A negative result is recorded as a negative | Where a measurement produced nothing, the doc says so, rather than omitting the experiment |
| E-06 | Unverified tools are labelled unverified | Any instrument that did not produce output is marked as such, with what was tried and what it cost |

## I — Intellectual property

| ID | Criterion | Verifiable Check |
|---|---|---|
| I-01 | The ROM is never committed | `git ls-files \| grep -i "sfc$"` returns nothing |
| I-02 | No peer source is committed or vendored | `git ls-files study/ \| grep -vE "^(study/peer-linux/(README.md\|build-peer-linux.sh\|jjhead.c\|jjwin.c\|bgra2png.py))$"` returns nothing |
| I-03 | The unlicensed reference is confined to a private scratch clone | `/tmp/...` only; nothing under the repo root; the licence status is stated in `study/peer-linux/README.md` |
| I-04 | Reading an unlicensed implementation did not become copying it | Each behavioural fact taken from the reference is expressed as a mechanism, cited by file:line, with no code transcribed |

## G — Gates

| ID | Criterion | Verifiable Check |
|---|---|---|
| G-01 | `make test`, `make test-rom`, `make perf` pass | All three exit 0 on the candidate |
| G-02 | `make clock` fails, and its message does not assert an unproven cause | Exits non-zero; message matches E-03 |
| G-03 | The delivery gate is red and the status line says the build is not deliverable | `README.md` status names `make clock` as FAIL and states not deliverable |
| G-04 | Heavy work runs on the Deck, not the dev host | Measurement reports name the machine they ran on |

---

## Verdict rule

| Condition | Verdict |
|---|---|
| Any BLOCKER open | REJECT |
| Any MAJOR open | REJECT WITH CONDITIONS |
| MINOR only | APPROVE WITH CONDITIONS |
| All closed | APPROVE |

**A BLOCKER is a finding where the project's stated purpose is defeated or a
published claim is false.** In this project that includes: `make clock` passing
on a build that does not simulate; a retracted cause presented as fact; or the ROM
or unlicensed source present in the tree.

## Weighting note

E is weighted heaviest and deliberately so. The failure mode here is not bad
code — the gates pass and the emulator holds 60 fps. It is a confident narrative
outrunning its evidence, which cost twelve commits and produced eleven
retractions. A rubric that scored this project well on code quality and poorly on
truthfulness would have rated the whole investigation as a success.
