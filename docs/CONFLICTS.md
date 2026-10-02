# Conflicts — every contradiction found, with the command that found it

Durable record for Phase 5 (`aes-conflict`), 2026-10-02, at `166c82b`. Each item
names the command that produced it, so any reader can re-derive it and disagree
with the finding rather than with the author.

## 0. The instrument, and why nothing here is its output

`make conflict-check` **does not exist in this repository.** It is
`/opt/aes/Makefile:1153`, and the target body is:

```make
conflict-check:
	@./scripts/conflict-detection.py || true
```

**`|| true`** — the upstream gate cannot fail even where it runs. It prints
`GATE: BLOCKED` and exits 0. That is the same shape as this project's own
`smoke || true` CI job that `d816afc` removed, and it means "aes-conflict passed"
has never meant anything anywhere. Three of its five classes also require
`aes/shadow/access.log`, which this project does not have, so orphan-access,
causality and session-conflict detection are **structurally unmeasurable here**.

The five classes were therefore mapped onto this repository's equivalents and run
by hand. What follows is measurement, not tool output.

## 1. `docs ↔ git history` — a falsified claim, unmarked, in an unguarded file

**CONF-1 · severity HIGH · `docs/RE_SCENARIO_NAV.md:145`**

```
$ grep -n "force_lle" recomp/bank00.cfg
22:force_lle 0x008000   25:force_lle 0x0080B2   28:force_lle 0x00927C
29:force_lle 0x009280   30:force_lle 0x009287   31:force_lle 0x00928F
60:force_lle 0x00804D
$ git log --oneline -S"force_lle 0x009311" -- recomp/bank00.cfg
436b25b recomp: exclude the vblank spinlock from AOT (force_lle did not cover it)
$ sed -n '145p' docs/RE_SCENARIO_NAV.md
- `force_lle 0x009311` is correct because the NMI must be delivered *during* the
```

The line is in the **present tense**, in backticks, as file content, and it is
**false**: the directive was removed by `436b25b`, before that sentence was
written. Every other mention in the tree carries a retraction marker
(`RE_CITY_FREEZE.md:262, 524, 671, 685, 1368`; `CLAIMS_REGISTER.md` §12; ledger
row **R-018**). This one does not.

**And it survives the guard**, because:

```
$ grep -c "RE_SCENARIO_NAV" scripts/check-retracted-claims.sh
0
```

`docs/RE_SCENARIO_NAV.md` is **not in the checker's `SCOPE_FILES`**. A falsified
claim is sitting in a file the guard never reads. This is a guard gap, not a
documentation slip, and it is the only CONF in this file that a reader following
the docs would act on: the line tells the next person to re-introduce a
directive that was removed *because* it pinned one PC inside a function
beginning at `$930D`.

## 2. `docs ↔ git history` — a claim refuted by a commit nobody told the docs about

**CONF-2 · severity HIGH · `scripts/perf-gate.sh:68-71`**

The gate's own header states:

> Also: the Deck cannot build this project. SteamOS has an immutable rootfs with
> no glibc headers, so `make build` there fails at configure. Every Deck number
> in this repo comes from a binary built on the dev host and copied over.

**False since `9624f0e`.** The Deck compiles this project natively (gcc 15.1.1,
cmake 4.0.3, `make test` 2/2). The rootfs damage is real and still true; the
inference from it was not. Measured in
`aes/decisions/D009.md`; ledger row for the retraction is pending.

**No ledger row covers this claim**, so `check-retracted-claims.sh` cannot catch
it at this or any other site. A guard only guards what the ledger names.

## 3. `docs ↔ docs` — three different figures for "a healthy run"

**CONF-3 · severity MEDIUM · numeric (rubric E-04)**

| site | figure |
|---|---|
| `scripts/verify-rom-render.sh:14` and `:95` | "206 distinct crc32 … when healthy" / "gives ~206" |
| `README.md:202` | **257** |
| `docs/CLAIMS_REGISTER.md` §14 | "254 is stale; `make test-rom` printed **257**" |

Measured this phase, resolving it:

```
$ make test-rom
  distinct crc32   : 257
  peak luma        : 41.751
PASS: the emulated picture moves.
```

**257 is current. The `206` in the gate's header and in its own FAIL text is
stale** — and it is the figure a failing reader is told to expect, which makes it
worse than a stale comment in a doc nobody opens. Note the ledger's existing
`254` rows (R-013, R-022) are a *different* wrong number about the same quantity.

## 4. `docs ↔ docs` — a file contradicting itself, one screen apart

**CONF-4 · severity MEDIUM · `docs/CLAIMS_REGISTER.md`**

```
§9  (line 238):  | `recomp/bank00.cfg:44` contains `exclude_range 0x930D 0x9318` | ✅ T062 AC #1 satisfied |
§14 (line 348):  - **§9's row** … is **false**: it has read `0x130D 0x1318` since `afceeec`.
```

```
$ grep -n "exclude_range" recomp/bank00.cfg
44:exclude_range 0x130D 0x1318
```

The §9 row still carries a ✅ four hundred lines above the line that says it is
false. A reader who stops at the evidence table — the section whose stated purpose
is "re-verified in this session" — gets a wrong answer with a tick next to it.
**The register's §9 is a snapshot of 2026-10-01 and §14 is its correction; §9 is
not marked as superseded in place.**

## 5. `tracked ↔ gitignored` — dangling references out of a fresh clone

**CONF-5 · severity MEDIUM · eleven sites**

`aes/` and `.aes/` are gitignored **permanently, by rule** (DoD **D4.3**). Every
tracked file that points into them therefore points at nothing in a fresh clone:

```
$ grep -rn "aes/" README.md docs/*.md Makefile | grep -v "is gitignored\|is permanently\|never committed\|by rule\|under \`aes/\`\|aes/graph/island-clock\|/opt/aes\|aes/decisions/D0"
README.md:496                   See `aes/tickets/T058-city-clock-does-not-advance.md`.
docs/RE_SCENARIO_NAV.md:219     See `aes/tickets/T058-city-clock-does-not-advance.md`.
docs/ROADMAP.md:6               `aes/kanban.md` is the live tracker
docs/ROADMAP.md:102             See `aes/kanban.md`.
docs/CHECKLIST.md:31            Ticket statuses match actual state in aes/kanban.md
docs/VISION.md:56               ticket artefacts under `aes/`.
docs/DEFINITION_OF_DONE.md:111  (`aes/decisions/D003.md`)
docs/DEFINITION_OF_DONE.md:172  (`aes/tickets/T080`)
docs/DEFINITION_OF_DONE.md:178  (`aes/decisions/D006.md`)
```

This is the **same rule that produced D4.3** producing its own violation: the
process wanted durable content, the rule forbade it living in `aes/`, and the
resolution was correct — but nothing replaced the *pointers*. A rule that makes a
directory uncommittable obliges you to move what matters out of it, and these
eleven lines are the residue of not finishing that job.

## 6. `aes ↔ docs` — stale ticket references (conflict class 5)

**CONF-6 · severity LOW (local-only) · seven missing files**

```
$ grep -rhoE "docs/[A-Za-z_/-]+\.md" aes/ | sort -u | while read f; do [ -e "$f" ] || echo "MISSING $f"; done
MISSING docs/CLOCK_GATE_CALIBRATION.md
MISSING docs/LLE_SCHEDULER.md
MISSING docs/MOUSE.md
MISSING docs/PEER_LICENCE_DECISION.md
MISSING docs/PERFORMANCE.md
MISSING docs/ROM_ADDRESS_MAP.md
MISSING docs/TICKETS.md
```

These are references **from** local, gitignored artefacts **to** files that no
longer exist. They cannot mislead a reader of the repository, because the reader
cannot see the referring document either. Recorded because the skill's class 5
("stale ticket references") is otherwise unrepresented, and because two of them
(`ROM_ADDRESS_MAP.md`, `PERFORMANCE.md`) are exactly the documents that would have
prevented CONF-3 and CONF-4 from needing a Phase 5 to find.

## 7. Verified — a suspected conflict that is NOT one

Recorded because a conflict scan that only reports hits teaches nothing about its
own coverage.

**`CODE_009311` is an alias of `$03:7649`, not `$00:9311`.** Confirmed at the
source, not inferred:

```
$ grep -n "009311" recomp/funcs.h
1665:void CODE_009311(CpuState *cpu);  /* $03:7649 alias */
$ grep -n "CODE_009311" recomp/bank00.cfg
344:name 0x37649 CODE_009311
```

So `grep 009311 recomp/` finds a routine in the **wrong bank**, exactly as
`CLAIMS_REGISTER.md` §12 warns. **This is correct behaviour with a real trap, not
a contradiction** — and it is why the trap is worth stating: the name looks like
the vblank spinlock address and is not.

**Review finding F-12 is closed.** `recomp/bank00.cfg:44`'s comment now reads
`INC $C7 (direct page, E6 C7)` and explains why the range starts at `0x130D`:

```
$ python3 -c "d=open('SimCity (USA).sfc','rb').read(); print(d[0x130D:0x1319].hex(' '))"
e2 20 64 b9 e6 c7 a5 b9 f0 fa 60 e2
```

`e2 20` = `SPC $20` (the preceding instruction), `64 b9` = `STZ $B9` at `0x130F`,
`e6 c7` = `INC $C7`. The comment is accurate about the addressing mode and states
the over-exclusion. **Closed, verified against the ROM bytes.**

## Summary

| # | severity | axis | one line |
|---|---|---|---|
| CONF-1 | HIGH | docs↔git | `force_lle 0x009311` asserted present at `RE_SCENARIO_NAV.md:145`; removed by `436b25b`; **and the file is outside the guard's scope** |
| CONF-2 | HIGH | docs↔git | `perf-gate.sh` header says the Deck cannot build this project; it can, since `9624f0e`; **no ledger row covers it** |
| CONF-3 | MEDIUM | docs↔docs | three figures for a healthy `test-rom` run; **257 measured**, `verify-rom-render.sh` still says 206 |
| CONF-4 | MEDIUM | docs↔docs | `CLAIMS_REGISTER.md` §9 ticks a row its own §14 calls false |
| CONF-5 | MEDIUM | tracked↔gitignored | eleven tracked references into `aes/`, which is uncommittable by rule |
| CONF-6 | LOW | aes↔docs | seven stale `docs/*.md` references from local artefacts |
| — | verified | docs↔code | `CODE_009311` = `$03:7649`, a real trap, not a contradiction; F-12 closed |

**Not a conflict, and deliberately not filed as one:** the 78 frames between bank
03 going silent (f3301) and the city appearing (≈f3378). It is a correlation with
no mechanism attached, and turning it into a cause is the seventeenth retraction
waiting to happen.