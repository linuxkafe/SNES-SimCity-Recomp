# AES chain — run by hand, 2026-10-03

**This is the record of running the AES chain in this session, and it leads with
the places the chain's own inputs were wrong about this project.** Everything
below is measured on this machine and in this repository unless it says
otherwise.

**Two things were corrected against a brief I was given, and both corrections
matter more than the chain's output.** See §1.

---

## 1. Corrections first

### 1.1 `extraction_confidence` is NOT absent — trigger #2 is ARMED on 51 nodes

The brief stated that `aes-epistemics` trigger #2 *"needs
`extraction_confidence > 0.85` and the field appears **zero** times"*.

**Measured, `aes/graph/island-clock.yaml`:**

```
extraction_confidence: 0.95   x 22
extraction_confidence: 0.90   x 24   (0.9 x23, 0.90 x1)
extraction_confidence: 0.85   x  5
extraction_confidence: 0.92   x  2
extraction_confidence: 0.94 / 0.93 / 0.88  x 1 each
                          total 56 fields on 64 nodes
nodes with extraction_confidence > 0.85:  51
```

**The field is present, well-populated, and 51 nodes clear the 0.85 threshold.**
The premise "the field appears zero times" is **false**.

**This changes the reason `aes-epistemics` cannot run, and the reason is the
one that was always true: there is no solver.** `which z3` → nothing.
`aes/graph/island-clock.yaml` says so itself (`z3_available: false`,
`sat_run_performed: false`).

> **The distinction is load-bearing.** "The trigger is disarmed" and "the
> validator cannot execute" lead to different future work. The first says
> *nothing to do*; the second says *51 nodes are waiting on a solver and the
> graph is currently validated `structural` only*. **A graph that says
> `validated_by: structural` while its nodes clear a SAT trigger by a wide
> margin is a graph waiting for a capability, not a graph that has nothing to
> say.** The graph's own header already refuses to be read as a proof, which is
> correct and should not be changed.

### 1.2 `/opt/aes` has **724** tickets, not 269 — and none of them are ours

Brief: *"`/opt/aes` is real but measures **AES's own 269 tickets**"*.
**Measured: `ls /opt/aes/aes/tickets/ | wc -l` → 724.**

**No number from `/opt/aes` is imported anywhere in this session**, and none
should be: it is a different project with its own history. The count is recorded
only so the next reader does not repeat a figure that is wrong.

### 1.3 The `aes-epistemics` assets are not missing — they are in the wrong place

Brief: *"5 scripts + 2 templates ABSENT"*. **Measured:**

```
harness skill dir  ~/.config/opencode/skills/aes-epistemics/   -> SKILL.md only
/opt/aes/skills/user/aes-epistemics/scripts/                    -> 6 scripts
/opt/aes/skills/user/aes-epistemics/templates/                 -> 2 templates
```

`gmif-check.sh`, `install-z3.sh`, `gmif-staleness-check.sh`,
`aes-epistemics-wire.sh`, `test-gmif.sh`, `verify-external.sh`, plus
`templates/island.yaml` and `templates/self-ticket-learn.md`.

**So the scripts exist and are reachable; what is missing is the solver.** That
is a different problem with a different fix — `install-z3.sh` is present, which
suggests the capability was intended to be installable. **Not attempted here**:
installing a solver to enable an epistemic check on a project whose delivery gate
is a city that does not simulate is not a decision to make inside a session that
was asked to verify a cheat table. Recorded as a decision for the operator.

**And `gmif-check.sh` is still not wired**, for the stated reason: it is
fail-closed on a missing solver and it reads `aes/graph`, which is gitignored,
so wiring it would put a gate in the tree whose scope **does not exist in a
clone** — CONF-16's exact failure.

### 1.4 Island graph: 64 nodes ✓, **6** edges (brief said 19)

```
nodes : 64   (confirmed)
edges : 6    (measured; the brief's 19 does not reproduce)
```

**Counted, not estimated.** `grep -cE '^\s*(from|to|source|target):'` → 6.
**Edges also inherit no state** — this is a live structural gap, not a
counting dispute: a graph whose edges carry no state cannot express "this claim
was refuted *because of* that one", which is the only reason an island graph is
better than a list. Recorded, not fixed (§4).

> #### ⚠️ CORRECTION 2026-10-03 (later session, head `cfc7a99`) — **both numbers
> are grep artefacts, and the real ones are 56 nodes / 25 edges**
>
> The text above is left exactly as it stood. The claim in it is **false**, and
> the `grep` printed beside it is **why** — it is the fourth instance of this
> project's most productive failure shape (see `docs/CONFLICTS.md` CONF-24:
> *"an identifier that looks like the thing you want is not the thing you want"*).
>
> ```
> $ grep -cE '^\s*- id:' aes/graph/island-clock.yaml      -> 64   # nodes + INVARIANTS
> $ grep -cE '^\s*(from|to|source|target):' …             ->  6   # block-style edges ONLY
> $ python3 -c "import yaml;d=yaml.safe_load(open('aes/graph/island-clock.yaml'));
>               print(len(d['nodes']), len(d['edges']), len(d['invariants']))"
>   56 25 8
> ```
>
> | quoted | what it actually counts | truth |
> |---|---|---|
> | **64 nodes** | `- id:` at any indent — **matches `nodes:` (56) *and* `invariants:` (8)**, since both use `- id:` | **56 nodes** |
> | **6 edges** | only the 6 edges written in block style (`- from:` … `note:`) | **25 edges** |
> | the brief's **19** | only the 19 edges written inline (`- {type: … why: …}`) | same 25 edges |
>
> **So three artefacts have quoted three different numbers for one file and none
> of them is the count.** `docs/AES_CHAIN_RUN.md:84` says 64/6;
> `aes/epistemics/EPIGMIF-2026-10-03c.md:71` says "64 nodes, **19** edges";
> `aes/decisions/D017.md:133` derives "**47 of 64** nodes (73%)" from it. The true
> figures are **56 nodes / 25 edges**, and the 19/6 split is not a dispute at all —
> it is the two YAML styles the file happens to use for its explanatory key
> (`why` inline, `note` in block form).
>
> **The orphan count is wrong for the same reason, and this is the part that
> mattered:** the graph has **25 orphans of 56 nodes = 45%**, not "47 of 64 =
> 73%". The claim "most of the graph is unconnected" survives — it is still the
> largest structural gap — but the numbers attached to it did not, and the
> overstated figure was the one doing the arguing.
>
> **This is recorded here rather than fixed in place because `aes/` is gitignored
> (D4.3) and never survives a clone.** The wrong number lived in this tracked
> file, so the correction had to live here too. **No gate would have caught it:**
> `make check-claims`, `check-causes`, `check-entrypoints` and `retraction-count`
> all exit 0 on the uncorrected text — measured before this edit.
>
> **And no gate is added for it, on purpose.** The subject is a gitignored file,
> so a guard that validates prose counts against `aes/graph/island-clock.yaml`
> would have **nothing to read in a fresh clone** and would exit 0 — the
> "passes on an absent file" defeat, in a new costume. See
> `docs/RE_CITY_FREEZE.md` for the two known-real defeats of that shape.

### 1.5 `aes-debt` cannot be run — its own "How to run" block is empty

```
## How to run
```bash
# Full debt scan
```
```

**There is no command.** The fenced block opens, contains one comment, and
closes. The skill directory holds `SKILL.md` and nothing else. **Measured:
`grep -c 'debt-measure\|epistemic-debt.sh'` over its own `SKILL.md` → 0.**

**So `aes/metrics/epistemic-debt.log` cannot have been written by this skill in
this install**, and any entry in it was written by hand. Nothing from it is used
below.

### 1.6 `aes-project-manager` is on disk but NOT registered with the harness

**The brief said "not installed in the harness".** Precisely: the directory
exists in **both** `~/.config/opencode/skills/aes-project-manager/` and
`~/.claude/skills/aes-project-manager/`, with a substantive 171-line `SKILL.md`
— and the harness's skill registry **does not list it**, so
`skill(name="aes-project-manager")` returns *"Skill not found"*.

**So the brief was right about the effective state and wrong about the
filesystem.** The missing thing is the *registration*, not the file.

**I read its `SKILL.md` from disk and applied its contract by hand below. I did
not run it as a skill and I do not claim to have.** Its three-verdict contract
is reproduced faithfully because it is the right shape for §3.

### 1.7 A structural conflict between that skill and this project's D4.3

`aes-project-manager`'s stated product is *"a decision record written to the
repository BEFORE the answer is spoken"*.

**This project cannot do that.** `aes/` is **gitignored permanently** by DoD
**D4.3**, so a decision record placed there reaches no clone, and force-adding it
is forbidden. `docs/ROADMAP.md` already carries the tracked mirror for exactly
this reason.

**The project's own rule settles it and the rule is not in doubt:** a process
requirement yields to D4.3. The decision record for this session is therefore
**this file**, in `docs/`, and the skill's requirement is met in the only form
available. Saying so is better than quietly omitting the requirement.

---

## 2. What the chain produced, in the manager's own three verdicts

Reproduced from the `SKILL.md` read in §1.6, and applied to the four questions
this session actually had.

### 2.1 "Is the `$03C87x` overrun the game's bug or ours?" → **SUPORTADA** (ours)

Mechanical falsifier, **run**: the flags at `$03C87C` were read with
`SNESRECOMP_CYC_WATCH`, which prints the core's own `interp816_getFlags()`. Z is
clear at `X=$00F4` and set at `X=$8DF4`; `NPC` from `$03C87C` is `$03C87F` on
all 36 340 iterations. A game-side cause cannot survive the `LDA #imm` control in
the same log. Full falsifier transcript in the T105 commit body.

### 2.2 "Is the third-party cheat table usable?" → **NÃO-SUPORTADA** (as stated)

The brief's characterisation is unsupported in two specific places and
partially supported in the rest: the `7E` plaintext format **is** confirmed
(§VERIFIED in `docs/CHEAT_CODES.md`), and `DD/DE` **are not** instruction
patches. The mechanical falsifier — "write `$A0` to `$0B53`, then `$0F` to
`$0B54`, and read the intermediate" — **was run**, and it produced `$07A0`.

### 2.3 "Can the hidden debug menu be reached?" → **NÃO-VERIFICÁVEL** (here)

The blocker is `host_main.c:3774`, which reads only
`g_gamepad[1].axis_buttons` for the second pad, so controller 2's face buttons
are never delivered. **This is verifiable by reading and was**, and the estimate
is ~15–30 lines. But whether the menu is *reachable* also depends on reaching
*"See you soon!"*, which this project's route does not. **Two unknowns, one
answerable, one not** — so the honest verdict is neither of the two positives.

### 2.4 "Is this session's work a delivery?" → **NÃO-SUPORTADA**

`make clock` is red, and was measured red on the Deck in this session:
`clock-gate.sh` exit **1**, `make clock` exit **2**. Nothing here changes that
and nothing here was allowed to.

---

## 3. `aes-debt`, run by hand — and the one metric I decline to publish

The skill cannot run (§1.5). The three debt measures were taken by hand from
this repository's own artefacts.

### 3.1 Aging debt — claims not re-verified in > 7 days

**Measured: 4 of the load-bearing claims are older than 7 days and were not
re-verified by me.** From `docs/CAUSE_CLAIMS.md`'s own `measurement` column:
C-041/C-041b (`2026-10-02-t100`), C-052/C-053 (`2026-10-02-t100`),
C-059 (`2026-10-02-t101`). Two of those are **the duplicated ids from §4.1**.

Everything dated 2026-10-03 — T102, T104, T105, T107, C-070…C-073 — is
same-day and re-measured.

### 3.2 Protocol fatigue — phases skipped or gates overridden

**Measured: 0 gate overrides in this session, and 1 gate that was red on purpose
and stayed red.** `make clock` is red by design and by DoD. That is not fatigue;
fatigue would be reaching for the override.

**One real fatigue signal, and it is mine:** I built a WRAM-poke instrument
before grepping for one, and `poke` already existed in the script language. I
reverted it. **A guard built before searching for the guard is the same shape as
a retrained claim**, and the fix — grep first — is already trap 1's lesson about
an instrument that cannot read the answer.

### 3.3 Criteria leakage — acceptance criteria drifting between phases

**Measured: 1.** The T105 acceptance criterion was *"do the flags at
`$03C87F` show a correct Z for `CPX #$F4`, and what does that imply"* — and the
**implication half drifted** between registration and result, from *"no correct Z
⇒ a game bug"* to *"no correct Z ⇒ evidence about us"*. The measurement half did
not drift.

**This session refutes the previous entry's `criteria_leakage: 6`, and the
measured number is 1.** One drifted implication clause in one ticket.

### 3.4 The metric I will not publish

`aes-debt`'s documented output includes `verification_rate`. **I decline to
compute or report it**, because as defined it counts a **retracted** row as a
verified one: a claim that was measured and then refuted would raise the rate.

**In this project that is not a rounding error.** **30 of 39 ledger rows are
`refuted`** — the single largest category by far. A "verification rate" that
counts those as successes would report this repository as ~77% verified when the
honest description is that most of what was ever asserted here was wrong and
caught. **The metric is worse than absent on this corpus, and the brief's
instruction not to repeat it is correct.**

---

## 4. Hardening

### 4.1 Carried forward, re-measured rather than repeated

| item | measured state 2026-10-03 |
|---|---|
| **CONF-8** `scripts/verify-implementation.sh` | **still in tree** (6 677 bytes, executable, dated Sep 13). **No longer wired to any `make` target** — so the drift is that a gate *shaped* file survives as a file. Not deleted: deleting it would remove the evidence, and `aes/`'s inconsistencies are currently the only evidence of themselves |
| **CONF-1** scope hole | **live.** `scripts/check-retracted-claims.sh` is the only gate that reads `aes/` at all (grep-confirmed), and `aes/` is gitignored — so a gate whose scope is a directory no clone has |
| **CONF-12** mechanism | **OPEN, unchanged.** rate not re-measured this session |
| **CONF-13** cause | **OPEN, unchanged** |
| **CONF-14** count guard | **OPEN, and it fired again in this session** — on `README.md:22` at `79a4064`. Documented in that commit and in CONF-20 |
| **duplicate claim ids C-052/C-053** | **CONFIRMED, and worse than "duplicate"**: each is defined **twice with entirely different content** — `CAUSE_CLAIMS.md:94/95` are the f3259 date-write claims, `:131/132` are the `$0012` gate claims. **A reader looking up C-052 finds two unrelated claims, both marked MEASURED** |
| **island graph staleness / stateless edges** | **OPEN.** 64 nodes / **6** edges; edges carry no epistemic state |
| **`aes/*.yaml` in no gate's scope** | **CONFIRMED** — no `make` target reads `aes/graph/island-clock.yaml` |
| **10 dangling `SD-*` references** | **CONFIRMED.** Present: 10 shadow docs. Missing: `SD-CI-003`, `SD-CI-005`, `SD-CI-006`, `SD-META-007`, `SD-META-017` … `SD-META-022` |

**On the renumbering question: nothing under `aes/` was edited.** Where a fix
would mean renumbering or deleting inside a gitignored directory, the
inconsistency is documented instead — **because it is currently the only
evidence of itself**, and a fix applied to a directory no clone contains cannot
be reviewed by anyone else.

### 4.2 Two new guards, both falsified before committing

Both landed in this session's earlier commits; recorded here so the chain's
output and the hardening output are in one place.

| guard | what it holds | falsified |
|---|---|---|
| `scripts/check-cheat-gate.sh` | **a cheat can never make `make clock` pass** (DoD Rule 0b) | 5/5 seeded assertions, **both directions**, on **untracked** files, with positive controls. **It was itself wrong three times** and its self-test caught all three: fired on the gate's own `$PWD` route; a leading dot hid a gate script from it; and its own `pipefail`+`grep -q` reported FAIL while the guard worked |
| CONF-20/21/22 documented | the lexical-guard blind spots | each with a two-directional seed |

### 4.3 The one thing I would harden next, and why it is not this session's job

**A per-knob table for the parse conventions** (CONF-22). Four conventions
across five sibling knobs; two are the same `strtol` family one argument apart;
and the project-wide rule in `README.md` trap 8 is *wrong for one of them*, so
following it faithfully breaks an instrument. **That is a table of five rows,
and it belongs next to trap 8 rather than in a session record.** It is the
cheapest outstanding fix in this document and I did not do it only because it
belongs to a different ticket.

---

## 5. What this chain did not do

- **No phase was run by the tool that normally runs it.** Every verdict in §2 is
  applied by hand from a `SKILL.md` read off disk. **That is not the same as
  running the pipeline, and nothing here should be read as a pipeline result.**
- **No SAT was performed, and none is claimed.** `z3` absent. The graph's own
  `validated_by: structural` is correct and was not changed.
- **`/opt/aes`'s 724 tickets were not imported, read for content, or compared.**
- **`aes/` and `.aes/` remain gitignored and nothing was committed under
  either.** `git ls-files | grep -c '^aes/'` → **0**.
- **`aes-peer-review` was not run as a verdict.** Its protocol needs reviewers
  independent of the author; this session's author is the author of the work, so
  **the multi-perspective fallback the T101 review already used applies again**,
  and the candidate stays **CANDIDATE**. The single-perspective result of §2 is
  labelled as such and is not an approval.
- **`aes-narrative` and `aes-conflict` were not run.** `aes-narrative` ships
  `SKILL.md` only. `aes-conflict` types 1 and 3 are **uncomputable** here: both
  require an `access.log`, and **no `access.log` exists** in this install
  (`ls aes/*access*` → no such file). **Reporting PASS on those two would be
  reporting a number I do not have**, so they are reported as uncomputable.