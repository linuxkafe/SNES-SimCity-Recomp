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
---
---

# SECOND CHAIN RUN — 2026-10-03, fresh context, head `cfc7a99`

**This section is the record of a second, independent run of the six-phase chain,
in a fresh session with no memory of the first. It is appended, not merged: the
run above is left exactly as it stood, because a chain record that rewrites its own
history is not a chain record.**

Artefacts for this run live in `aes/` and are **gitignored permanently** (D4.3),
so **this section is the only part of it that survives `git clone`.**

---

## S1. `aes-project-manager` → three verdicts, one of them a retraction

**Could not be run as a skill — third session running.** The brief asserted it
*"is installed and in your registry. Run it as a skill."* Measured: the skill tool
returns `Skill "aes-project-manager" not found` and prints the available list. It
**is** on disk in both `~/.config/opencode/skills/` and `~/.claude/skills/` with a
171-line `SKILL.md`. **Executed as a protocol by hand**, per its own instruction
that every verdict carry a mechanical falsifier that was actually run. Record:
`aes/decisions/D018.md`; correction ticket `aes/tickets/T159`.

### S1.1 NÃO-SUPORTADA — the island graph is **56 nodes and 25 edges**

Three artefacts quote three different counts and **none of them is the count**:

| artefact | says |
|---|---|
| `docs/AES_CHAIN_RUN.md:84` (**tracked**, marked ✓ *confirmed*) | 64 nodes, **6** edges |
| `aes/epistemics/EPIGMIF-2026-10-03c.md:71` | 64 nodes, **19** edges |
| `aes/decisions/D017.md:133` | **47 of 64** nodes (73%) are orphans |

```
$ grep -cE '^\s*- id:' aes/graph/island-clock.yaml          -> 64
$ grep -cE '^\s*(from|to|source|target):' aes/graph/…yaml    -> 6
$ python3 -c "import yaml;d=yaml.safe_load(open('aes/graph/island-clock.yaml'));
              print(len(d['nodes']),len(d['edges']),len(d['invariants']))"
56 25 8
```

**64 = nodes (56) + invariants (8)** — both use the key `id`, so one regex counts
both. **6 = the block-style edges; 19 = the inline edges; 6 + 19 = 25.** The
apparent 6-vs-19 dispute is not a dispute: it is the two YAML styles the file uses
for its explanatory key (`why` inline, `note` in block form).

**The orphan figure was the one doing the arguing: 25 of 56 = 45%, not 47 of 64 =
73%.** "Most of the graph is unconnected" survives — it is still the largest
structural gap. Every number attached to it did not.

**This is CONF-24's shape a fourth time** (`docs/CONFLICTS.md:1591`): *an identifier
that looks like the thing you want is not the thing you want.* The `present_NNNNNN`
filename index, the 32 768-write delay loop, the `$7E:2100` WRAM shadow — and now a
`grep` regex that reads like a node counter. **All four were caught by asking "what
is this number *actually* a count of?", and none by reading the code that made it.**

### S1.2 NÃO-SUPORTADA — the graph's own *headline finding* is already in the ledger

`INV-5` records `violations: 1` and calls it *"the headline finding of this graph"*:
**"C-009 has no retraction anywhere in the repo."**

**Refuted.** `scripts/retracted-claims.tsv:84-87` carries **R-005, R-006, R-007,
R-008** — all `refuted`, whose phrases are *"the gate is $0012"* and *"13 of 13"*,
which are C-009's exact claim and its exact stated evidence. `docs/CAUSE_CLAIMS.md:70`
marks it **RETRACTED**. `make check-claims` exits 0.

`INV-3`'s 9 recorded violations are stale in the **other** direction:
`scripts/clock-gate.sh:339-343` now says in its own text that a *previous* version
asserted the cause, and asserts none.

**The graph's headline finding is a retraction the project already made, and it has
been carried unchallenged through D016, D017, `EPIGMIF-2026-10-03c.md` and a
tracked file — because `aes/graph/island-clock.yaml` is in no gate's scope.**
Measured: `grep -l 'island\|\.yaml' scripts/*.sh` → nothing.

**D3.6 guarantees no prose retraction *count* disagrees with the ledger. It cannot
reach a gitignored YAML file, which is exactly where this instance lives. The
guard's blind spot and this finding are the same shape.**

### S1.3 NÃO-VERIFICÁVEL — and the gate is rejected **for a reason**, not left unwritten

The subject of S1.1 and S1.2 is a **gitignored** file. A guard that validates prose
counts against `aes/graph/island-clock.yaml` would, **in a fresh clone, find no YAML
and no claim, and exit 0** — the documented absent-file defeat in a new costume.

The sound alternative is a hardcoded literal ("any tracked file stating a node count
states 56"). **Rejected:** it permanently fails the moment the graph gains a node, and
a permanently-failing gate is how this project ends up with gates that get deleted.
**Recorded so the next session does not reinvent it.**

## S2. `aes-debt` — three metrics, one of them declined

**Its "How to run" block is empty**: the fence opens, holds one comment
(`# Full debt scan`), and closes. `grep -cE 'debt-measure|epistemic-debt[.]sh|[.]sh'`
over its own `SKILL.md` → **0**. The skill directory contains `SKILL.md` and nothing
else. **The metrics below are computed by hand against commands**, and the record is
appended to `aes/metrics/epistemic-debt.log` — **8 valid JSON entries, prefix
byte-identical, verified with `cmp` before and after.**

| metric | value | why |
|---|---|---|
| `verification_rate` | **`null` — DECLINED** | every candidate formula scores a **refuted** row as a verified one, and **35 of 43** ledger rows are refuted. **A metric that rewards being wrong is not a debt metric.** Declined in three consecutive entries now |
| `aging_debt` | **0 — by construction** | all 15 files in `docs/measurements/` are dated 2026-10-02/03 and today is 2026-10-03. **It would read 0 on a project nobody had worked on.** Reported *with* the caveat, never as a health signal |
| `criteria_leakage_count` | **1** | **D2.4** only: its check column is the prose `printed bytes > printed baseline`. D1.4 and D3.2 name executable commands in prose. D3.3/D3.4 are **retired** and their script is **absent** (`ls scripts/check-numbers.sh` → no such file) |
| `fatigue_rate` | **0.0** | 0 gates skipped, 0 weakened, 0 overrides, 0 targets added to turn anything green. `make clock` not run and not modified; `make perf` not run (out of scope) |
| ledger | **43 / 35 / 6 / 2** | `make retraction-count`. **Delta 0** — no row added this phase, deliberately |

### S2.1 The fifth classifier, and a hypothesis of mine that the instrument killed

**`docs/CAUSE_CLAIMS.md`'s own row count cannot be computed unambiguously.** Five
classifiers have now produced five numbers for one file — **6, 1, 3, 67, 75** across
five debt entries. **Mine, stated because the classifier *is* the finding:** 75 table
rows carry a `C-NNN` id in the first cell; 1 of those is a cross-reference rather
than a definition; so **74 definitions, 72 distinct ids**.

> **I predicted a THIRD duplicated claim id** beyond the documented `C-052`/`C-053`,
> on the strength of a loose regex that returned `C-055` twice.
> **MEASURED: refuted.** `docs/CAUSE_CLAIMS.md:99` is the `C-055` definition; **:480**
> is a cross-reference row in the C-006-lead table (`C-055's +96 updater, picture
> side`). The only genuine duplicates are `C-052` and `C-053` at 2 each — **exactly
> what the file's own warning block at `:131` says.**
>
> **Recorded because the prediction was reasonable and the instrument killed it,
> which is the only reason the numbers that survived deserve anything.**

**This is D3.3/D3.4's retirement arriving inside the metric that was meant to
measure it, for the third time in two days.** Those criteria were retired because
`scripts/check-numbers.sh` does not exist. It still does not.

## S3. `aes-narrative` — 1 of 5 dimensions computable, and **dimension 5 has been scored three incompatible ways**

Record: `aes/narrative/NARRATIVE-2026-10-03d.md` (gitignored).

**First, the substrate, because four dimensions have none:**

| # | dimension | substrate it names | measured | verdict |
|---|---|---|---|---|
| 1 | omission rate (% excluded from `INDEX.md`) | `aes/shadow/INDEX.md` | **ABSENT** | **UNCOMPUTABLE** |
| 2 | pinning bias (📌 vs unpinned **in the hot index**) | hot index | **ABSENT** | **UNCOMPUTABLE as defined** |
| 3 | access concentration (Gini over `access.log`) | `access.log` | **ABSENT** | **UNCOMPUTABLE** |
| 4 | synthesis coverage (% of *accessed* docs with `/synthesis`) | log + markers | **0 markers / 10 docs** | **UNCOMPUTABLE — denominator 0** |
| 5 | score clustering | per-doc composite scores | **PRESENT, 10 of 10** | **COMPUTABLE — see below** |

The **only** `access.log` on this machine is `/opt/aes/aes/shadow/access.log`, which
measures **AES's own tree**. No number from it is imported. The five `/synthesis`
hits are this project's own prior reports *discussing* the marker —
`grep -l '/synthesis' aes/shadow/*.md | wc -l` → **0**.

### S3.1 A retraction: the first run's HIGH verdict was scored on the wrong quantity

`SKILL.md` names dimension 5 **"Score Clustering — whether composite scores cluster
tightly or spread across range."** Three same-day reports computed it three
incompatible ways:

| report | what it scored as dimension 5 | verdict |
|---|---|---|
| `NARRATIVE-2026-10-03.md:88` | **claim-state distribution** in the claims register (`MEASURED 48 / RETRACTED 22 / OPEN 2 / INFERRED 1`, 73 rows) | **2** |
| `NARRATIVE-2026-10-03b.md:113` | **claim-state distribution** again (54% MEASURED, 1 pure-OPEN) | **2** |
| `NARRATIVE-2026-10-03c.md:31` | the **shadow docs' composite scores** — the quantity the dimension names | **COMPUTABLE, and DEGENERATE** |

**The dimension is named after the shadow docs' scores, and those are two
constants:**

```
$ grep -hoE '(activation|centrality): *[0-9.]+' aes/shadow/*.md | sort | uniq -c
     10 activation: 0.0        # distinct values: 0.0   (one)
     10 centrality: 1.0        # distinct values: 1.0   (one)
```

**Zero variance in both fields. That is not "the scores agree" — it is the absence
of a computation.** `NARRATIVE-2026-10-03c:85` had it right and declined to score
it; the two earlier reports substituted a *different quantity* and scored it 2.

**This matters because it was load-bearing.** `NARRATIVE-2026-10-03.md:89` reports
**`5 / 10 HIGH`,** and line 90 states the exit **"is earned by dimension 5
alone."** Under the dimension as defined, dimension 5 is degenerate and unscoreable,
and the other four have no substrate — so **the first chain run's headline risk
verdict rests on scoring claim-state distribution as if it were a composite score.**
`NARRATIVE-2026-10-03b.md:113` carried the same substitution forward, which is how
one wrong quantity produced two wrong verdicts.

> **Retracted:** *"the AES narrative risk for this project is 5/10 HIGH, and the
> exit is earned by dimension 5 alone."* The quantity is not the dimension's.
> **What survives:** the *concern* behind it is real and independently stated at
> `NARRATIVE-2026-10-03.md:66-71` — a register that is 66% `MEASURED` **reads** as
> more resolved than it is, and the mitigation is the sentence at the top of
> `docs/CAUSE_CLAIMS.md`, not the ratio. **A true observation, filed under the
> wrong dimension.**

### S3.2 Risk score: NOT REPORTED, and the reason is not "the tool was busy"

One dimension of five is computable and it is degenerate. **A score needs five.**
Inventing zeros for the other four would produce a **lower score for a worse
state**, which is `NARRATIVE-2026-10-02:33`'s own words: *"A harness that prints
0/8 is not 'low risk', it is no signal."*

**Reported instead: `1 of 5 dimensions computable`; risk score UNCOMPUTABLE.
`PASS` is not available and is not reported.**

### S3.3 The substrate measurement, reproduced to the unit in a fresh session

```
$ compare `created` with `last_accessed` in each of the 10 shadow docs
last_accessed == created : 10 of 10
```

**No shadow document in this project has ever been read after it was written.**
Reproduces `NARRATIVE-2026-10-03c` §3 exactly. Therefore **dimension 3's Gini is
`0/0` — undefined, and it must not be reported as `0`.** Zero Gini means *uniform*
access; the truth is **there is no access at all**, which the dimension's own
interpretation ("few docs dominate the system's memory") cannot express, because
**the system has no memory to dominate.**

### S3.4 What would make it computable — and why none of it is done here

1. **D1/D2** — generate `aes/shadow/INDEX.md` from the ten docs. **Caveat that must
   travel with any resulting number:** it would be built from documents that have
   never been read, so an omission rate over it measures the generator, not the corpus.
2. **D3** — needs something to log accesses. **No proxy is invented.**
3. **D4** — needs D3 first.

**All three are decisions about the AES install, not project work.** Same
disposition D017 and D018 reached for the missing solver. **Also measured and
reproduced:** the skill's own stated invocation does not exist —
`make narrative-analysis` → `No rule to make target`, exit **2**.

## S4. `aes-epistemics` — **self-ticket trigger NOT ARMED**, and it fails on the conjunct nobody checked

Record: `aes/epistemic-proof/EPISTEMICS-2026-10-03.md` (gitignored).

### S4.0 `sat_run_performed: false` — preserved from the original, and nothing here changes it

```bash
$ command -v z3            # ABSENT
$ python3 -c "import z3"   # ABSENT
$ make gmif-check
make: *** No rule to make target 'gmif-check'.  Stop.    exit=2
```

**No SAT was run. No UNSAT exists. No core exists. `sat_run_performed: false`
stands unaltered.**

### S4.1 The bootstrap's step 1 is DECLINED, explicitly

SKILL.md's Bootstrap Protocol runs on load, and step 1 is:

```bash
if ! command -v z3 &>/dev/null; then
  echo "⚠ Z3 not found — installing..."
  bash "$AES_EPISTEMICS_HOME/scripts/install-z3.sh" || warn ...
```

**Not run. Installing a solver is the operator's decision, not the agent's.**
Worth recording that this was not even reachable: `AES_EPISTEMICS_HOME` resolves to
`~/.config/opencode/skills/aes-epistemics/`, which contains **`SKILL.md` and nothing
else** — no `scripts/`, no `templates/`. The bootstrap's own `|| warn` branch is what
would have run. **The skill's harness install is documentation-only.**

### S4.2 The assets are misplaced, and the count reconciles exactly

| directory | epistemics shell scripts |
|---|---|
| `/opt/aes/scripts/` | **8** — `gmif-check.sh`, `gmif-check-z3.sh`, `gmif-check-flybrain.sh`, `gmif-staleness-check.sh`, `install-z3.sh`, `verify-external.sh`, `test-gmif.sh`, `aes-epistemics-wire.sh` |
| `/opt/aes/skills/user/aes-epistemics/scripts/` | **6** — same set minus the two `gmif-check-*` variants |
| **total** | **14** ✓ |

**SKILL.md's wiring recipe points at the *second* path (6 scripts). The larger set
(8) lives in `/opt/aes/scripts/`**, which SKILL.md never mentions. So the path the
documentation prescribes holds **half** the implementation.

**Neither was executed.** They belong to AES's own install, and the solver they
require is absent.

### S4.3 Auto-wire declined — and the reason is better than "it would fail"

```makefile
# AES-Epistemics gates
gmif-check:
	@bash ~/.config/opencode/skills/aes-epistemics/scripts/gmif-check.sh
check: gmif-check gmif-staleness  # appended to existing check deps
```

Three independent reasons, in order of weight:

1. **It would fail-closed and stay red.** `gmif-check.sh` has
   `require_z3() { … die "Z3 not available…" }`, and `die()` is `exit 1`. So this
   is not a gate that silently passes without a solver — it is one that correctly
   refuses to report a result it did not compute. **That is the single best-written
   failure behaviour in this entire audit, and wiring it would convert a correctly
   absent capability into a permanently failing `make check`.** Making `check` red
   is not a fix; it is the removal of information.
2. **It reads `aes/graph/`, which is gitignored.** `docs/CONFLICTS.md:321` already
   records this exact hazard for `scripts/verify-implementation.sh`: *"a committed
   gate pointing at an uncommittable path is the same rule breaking as D4.3."*
   Confirmed: `aes/tickets/T058` is the rule. **`gmif-check` would repeat it.**
3. **The script lives in AES's own `/opt/aes` tree**, not in this repo.

**Not wired. `aes-epistemics-wire.sh` not run. Recorded as an operator decision,
with the fix (install z3, then wire) available and stated.**

### S4.4 The trigger, evaluated conjunct by conjunct

SKILL.md, **Self-Ticket Trigger Conditions — "(ALL must be true)"**:

| # | condition | status | evidence |
|---|---|---|---|
| 1 | `make gmif-check` returns UNSAT for an island | **CANNOT BE SATISFIED** | no target (exit 2); no solver |
| 2 | unsat core contains ≥2 claims with `extraction_confidence > 0.85` | **CANNOT BE EVALUATED** | no core exists, because no SAT ran |
| 3 | island referenced by ≥1 other island (`edges.to`) **OR** active ticket | **SATISFIED** | **18** distinct islands are a `to` endpoint; **9** tickets name an island id |

**Verdict: NOT ARMED — and it fails on conjunct 1, not on conjunct 2.**

### S4.5 Two prior readings of this trigger were wrong, in *opposite* directions

This is the finding, and it is the second one in this chain produced by the same
error class.

- Prior reports concluded the trigger was **"NOT ARMED because the confidence
  fields are missing."** **Wrong on the trigger's own terms.** The trigger never
  mentions field presence. It asks whether a *core* contains ≥2 nodes with
  `extraction_confidence > 0.85` — and the population test is met **51 of 56**.
  On the terms the rule actually states, conjunct 2's threshold is comfortably
  satisfied.
- A second reading treats that as **"ARMED, because 51 nodes exceed 0.85."**
  **Also wrong.** 51 of 56 is a statement about the *population*; the condition is
  about the *core*. A conjunct requiring an UNSAT result cannot be satisfied by
  counting nodes.

**Both substituted a computable proxy for the quantity the rule names.** Phase 3
found the identical move in dimension 5, where claim-state distribution was scored
as "composite-score clustering." **Two occurrences in one chain is not yet a
pattern**, and I will not call it one — but it is now the *second* time a number
was available, the *named* quantity was not, and the available number was used
instead. The rule that follows is the one the whole chain has been asserting on
matters: **`docs/DEFINITION_OF_DONE.md` Rule 0 — a command exiting 0 is
satisfaction; a proxy number is not.**

### S4.6 Not done, and why that is the honest outcome

- **No self-ticket created.** The trigger's three conjuncts are not jointly true.
- **No `SD-GMIF-*-article` / `SD-GMIF-*-feynman` shadows produced.** Those are
  outputs *of a triggered* self-ticket. Producing them without the trigger would be
  manufacturing the evidence the trigger exists to require.
- **`aes/epistemic-proof/`** written with `sat_run_performed: false` and every
  unevaluable conjunct marked as such.

## S5. `aes-conflict` — **FAIL**: 1 of 5 checks fails, 3 of 5 have no input

Record: `aes/conflict/CONFLICT-2026-10-03.md` (gitignored).

`make conflict-check` → `No rule to make target 'conflict-check'.  Stop.` exit **2**.
Harness dir holds `SKILL.md` only; the machinery is in `/opt/aes/scripts/`
(`conflict-detection.py`, `migrate-access-log.sh`, `verify-access-log.sh`,
`write-access-log.sh`).

| # | check | substrate | verdict |
|---|---|---|---|
| 1 | orphan access entries | `access.log` | **UNEVALUABLE** — no log |
| 2 | epistemic state contradictions | docs + log | **UNEVALUABLE as specified** (residue measured below) |
| 3 | causality violations | `access.log` | **UNEVALUABLE** — no log |
| 4 | session action conflicts | `access.log` | **UNEVALUABLE** — no log |
| 5 | stale ticket references | tickets + shadow dir | **FAIL — 10 dangling** |

### S5.1 Check 5 is the FAIL, and it is not resolvable as documented

Tickets cite **20** distinct SD ids; **10** have no file:

| dangling | cited by |
|---|---|
| `SD-CI-003` | T031-learn |
| `SD-CI-005` | T007-plan, T008-plan, T004-learn |
| `SD-CI-006` | T008-learn |
| `SD-META-007` | T031-learn |
| `SD-META-017` … `SD-META-022` | T007-plan, T008-plan, T004-learn, T008-learn |

The ids stop dead at **`SD-META-006`** — the corpus ceiling. Everything past it is
referenced and absent.

**SKILL.md's fix is "update the ticket to reference current SD IDs", and that is
impossible: the missing ids have no current counterpart.** The only two honest
options are author the 10 docs, or delete the references. **Authoring them is
rejected — that would manufacture the evidence this skill exists to demand.**
Deleting references from 5 historical learn artefacts is a content decision outside
this chain. **Left as an open FAIL, escalated.**

### S5.2 Check 2: the corpus is internally consistent and externally unverifiable

`epistemic_state`: **8 SUPORTADA, 2 HIPÓTESE.** The check as written is unfalsifiable
here — without the log there is no *time* of verification, and two docs verified at
different times are not a contradiction.

What *is* measurable underneath, and it is not flattering:

```
$ grep -hoE 'content_hash:.*' aes/shadow/*.md | sort | uniq -c
     10 content_hash: ""
$ grep -c '^last_verified:'  aes/shadow/*.md   →  0 across all 10
$ grep -c '^access_count:'   aes/shadow/*.md   →  0 across all 10
```

Every doc names its source ticket, **all 10 targets exist**, and
`provenance.generated_by` agrees with `pointer.path` in **10 of 10**. So the corpus is
internally tidy and **MEASURED externally unverifiable**: the one field that would
show whether the pointed-to ticket had drifted is `content_hash`, and it is `""` in
**10 of 10**. **UNVERIFIED — no check in this project can detect a shadow doc going
stale.**

And `aes-sleep`'s rule — `last_verified > 30d AND access_count < 3` → `STALE` —
has **both** inputs at zero occurrences. **It cannot demote and cannot confirm.**

### S5.3 What is currently protecting the corpus is the calendar, not a control

Only **6 distinct learn tickets** (T009, T021, T026, T027, T031) produced the 10
docs — **`T009`–`T031` of 159 project tickets.** 138 tickets produced none.
All 10 docs are **19 days old** (3× 2026-09-13, 7× 2026-09-14), so **not one is yet
>30d stale.** The corpus is unmarked only because **30 days have not elapsed.**

### S5.4 The error class is now three-for-three, and the substrate is not "not built yet"

| phase | rule names an input | this project produced | what got used instead |
|---|---|---|---|
| 3 · narrative | per-doc **composite scores** | two constants (`0.0`, `1.0`) | claim-state distribution, scored 2 |
| 4 · epistemics | an **UNSAT core** | none — no solver | population count, 51 of 56 |
| 5 · conflict | **`last_verified`** + **`access_count`** | none — 0 occurrences | nothing; check skipped |

In phase 4 I wrote that two occurrences "is not yet a pattern", and named the obvious
counter-hypothesis: **a fresh project legitimately has not built its substrate yet.**
That counter-hypothesis is now **testable and false for this project**, which is not
fresh: **159 tickets**, **26,020 bytes** of debt log, **5** narrative reports, a
**43-row** retraction ledger, **10** shadow docs, and this is **chain run #2**.

**So the pattern is not onboarding. It is that three skills, run in sequence against
the same mature project, each found the named input missing and each did not
manufacture it — which is correct — while the chain still produced three verdicts that
read as if the input existed.** In phase 3 that produced `5/10 HIGH`; in phase 4 a
confident trigger reading; here a skip. The correction is the same each time and is
already written down: **`docs/DEFINITION_OF_DONE.md` Rule 0 — a command exiting 0 is
satisfaction, a proxy number is not.**

What would actually break the pattern is one substrate, built once: an `access.log`
with `created`/`accessed` events. It would make checks 1, 3 and 4 runnable, give
narrative dimension 3 a denominator, and supply `access_count` to `aes-sleep`. **It is
a change to the AES install, not to this project** — same disposition D017 and D018
reached. Recorded for the operator, not built here.

### S5.5 A gate failed once and then would not fail again — recorded, not explained away

While running the S5 gates, `make check-claims` returned **exit 2 / `RESULT: FAIL`**
with a violation on `docs/CLAIMS_REGISTER.md:132` (R-021, `2.45 ms`, *"asserted
without a retraction marker"*). **It has not reproduced in 11 subsequent runs.**

```
run1..run6 after the event : EXIT=0  RESULT: PASS   (6/6)
S5 edit stashed, re-run    : EXIT=0  RESULT: PASS
```

What is established, and what is not:

- **Established:** the violation is **not** caused by the S5 text. With the S5 block
  stashed the gate still passes, and the S5 block matches **zero** of the 43 ledger
  phrases. The reported line, `docs/CLAIMS_REGISTER.md:132`, carries its marker
  (`RETRACTED as stated`) on **line 131** — one line above, against a documented
  six-line window (`scripts/check-retracted-claims.sh:260`).
- **Not established:** the cause. The failing invocation is distinguishable only in
  that it ran a full CMake configure **in the same `make` call**; the passing runs
  did not. I could not reproduce it on demand and I am **not** going to name a cause
  I have not proven.
- Failing output preserved verbatim at
  `aes/evidence/check-claims-FAIL-once-2026-10-03.log` (gitignored).

**This is a real defect regardless of cause: a gate that can report FAIL on
correctly-marked committed content cannot be used as evidence in either direction.**
Every PASS in this chain is therefore reported as *"passed on N of N runs",* not as
a single observation. **Confidence in today's green is qualified, and the
qualification travels with it.** `scripts/check-retracted-claims.sh:266` — its own
comment says the cost of a false positive "is one line to read"; this run shows the
cost is occasionally a false *alarm* on a line that is already marked.
