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
# (the line as it stood BEFORE this phase's fix - unmarked, present tense, false)
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
>
> <!-- (the quotation above is the text as it stood BEFORE this phase's fix.
>      It is false; see the measurement below.) -->

**False since `9624f0e`.** The Deck compiles this project natively (gcc 15.1.1,
cmake 4.0.3, `make test` 2/2). The rootfs damage is real and still true; the
inference from it was not. Measured in `aes/decisions/D009.md`. Ledger rows
**R-028**, **R-029**, **R-030** now cover the three phrases; `make retraction-count`
is the authority on how many retractions there are.

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

## 8. `docs ↔ ROM bytes` — the mapper is named wrong in four tracked files

**CONF-7 · severity MEDIUM · `docs/CLAIMS_REGISTER.md:238,409`, `docs/review/RUBRIC.md:32`**

Every tracked document that states the ROM→file offset rule calls the mapping
**HiROM**. (`README.md` and `docs/CAUSE_CLAIMS.md` said it too and were corrected
at `4ba14c7`; `CLAIMS_REGISTER.md` and the hash-pinned `RUBRIC.md` still carry
the old wording, so the count of affected files is **2**, not 4.) The ROM's own header says otherwise:

```
$ python3 -c "
d=open('SimCity (USA).sfc','rb').read()
print('0x7FC0', d[0x7FC0:0x7FD5])
print('0xFFC0', d[0xFFC0:0xFFD5])
print('len   ', hex(len(d)))"
0x7FC0 b'SIMCITY              '
0xFFC0 b'\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff'
len    0x80000
```

The SNES header sits at `0x7FC0` in a **LoROM** image and at `0xFFC0` in a
**HiROM** one. It is at `0x7FC0`, and `0xFFC0` is filler — so this cartridge is
**LoROM**, which is what `rom_identity.txt:24` says (`mapper = lorom`) and what
`src/main.c` derives at runtime.

**The arithmetic is nevertheless correct**, because
`offset = bank*0x8000 + (addr & 0x7FFF)` *is* the LoROM 32 KiB-bank linear rule;
the documents attached the wrong mapper's name to the right formula. Byte proof,
two independent labels:

```
$ python3 -c "
import re
d=open('SimCity (USA).sfc','rb').read()
print('0x130F ->', d[0x130F:0x1318].hex(' '))       # \$00:930F vblank wait
print('0x18026 ->', d[0x18026:0x18029].hex(' '))    # \$03:8026 INC.w \$0B51
print('EE 51 0B occurs at', [hex(m.start()) for m in re.finditer(b'\xee\x51\x0b', d)])"
0x130F -> 64 b9 e6 c7 a5 b9 f0 fa 60
0x18026 -> ee 51 0b
EE 51 0B occurs at ['0x18026']
```

`64 B9 / E6 C7 / A5 B9 / F0 FA / 60` is exactly the vblank token handshake, and
`EE 51 0B` occurs **once** in the whole 524 288-byte image, at `0x18026`. So the
labels and the mask are right; only the noun is wrong.

**Why it matters beyond tidiness.** "HiROM mask" reads as an optional hardware
detail. It is not: it is the `& 0x7FFF` that makes a 16-bit CPU address a file
offset, and `afceeec` shipped a live deadlock because that mask was missing from
one config line. A reader who trusts the mapper name over the formula can
"simplify" the mask away.

**Deliberately not fixed in the prose.** `docs/review/RUBRIC.md` is
hash-pinned (`RUBRIC.sha256`) and pre-registered; editing criterion C-02 to
repair a noun would invalidate the pin and rewrite the standard after the fact.
`README.md` and `docs/CAUSE_CLAIMS.md` now state LoROM with the header evidence
and name this conflict; the rubric keeps the historical wording.

## Summary

| # | severity | axis | one line |
|---|---|---|---|
| CONF-1 | HIGH | docs↔git | `force_lle 0x009311` asserted present at `RE_SCENARIO_NAV.md:145`; removed by `436b25b`; **and the file is outside the guard's scope** |
| CONF-2 | HIGH | docs↔git | `perf-gate.sh` header says the Deck cannot build this project; it can, since `9624f0e`; **no ledger row covers it** |
| CONF-3 | MEDIUM | docs↔docs | three figures for a healthy `test-rom` run; **257 measured**, `verify-rom-render.sh` still says 206 |
| CONF-4 | MEDIUM | docs↔docs | `CLAIMS_REGISTER.md` §9 ticks a row its own §14 calls false |
| CONF-5 | MEDIUM | tracked↔gitignored | eleven tracked references into `aes/`, which is uncommittable by rule |
| CONF-6 | LOW | aes↔docs | seven stale `docs/*.md` references from local artefacts |
| CONF-7 | MEDIUM | docs↔ROM bytes | the mapper is **LoROM** (header at `0x7FC0`); 2 tracked files still call it HiROM. The **formula** is right — it is the LoROM rule |
| CONF-8 | HIGH | code↔code | `scripts/verify-implementation.sh` reports ✅ for a **ticked** criterion with no check, and `bash -c`s a command lifted from prose. **Still present, still named "Verification Gate"** |
| CONF-9 | MEDIUM | docs↔build flags | the Deck trace tier "cannot link" because its sysroot is "partial" — **refuted, R-032**; the real cause was `-idirafter` on `CMAKE_C_FLAGS` and not on `CMAKE_CXX_FLAGS` |
| CONF-11 | HIGH | code↔gate | both evidence gates read **`git ls-files`** = the INDEX. An untracked new doc asserting a refuted claim passed `check-claims` and turned it red the moment `git add` staged it. **Paid for in `af08ff7`**, whose commit body claims `check-claims RESULT: PASS` and is false. Fixed with `ls-files -co --exclude-standard` |

## 9. `code ↔ code` — a tracked script named "Verification Gate" reports ✅ without checking

**CONF-8 · severity HIGH · `scripts/verify-implementation.sh:88`**

Found by running it against a ticket written to be false. Not introduced by this
phase; found by it, and it is the most dangerous artefact in the tree for the
reason DoD Rule 0 exists.

```
$ cat aes/tickets/T999-probe.md        ## Acceptance Criteria
  - [x] this criterion is a lie: `scripts/definitely-not-here.sh` does not exist
  - [ ] this criterion is also a lie: `scripts/definitely-not-here.sh` exits 0
  - [ ] the moon is made of green cheese
$ bash scripts/verify-implementation.sh T999
  ✅ (already checked) this criterion is a lie: `scripts/definitely-not-here.sh` does not exist
  ❌ this criterion is also a lie — 'scripts/definitely-not-here.sh' FAILED
  ⏭️  Cannot auto-verify: the moon is made of green cheese
  Result: 2 passed, 1 failed, 3 total
```

**A ticked box is a pass with no check at all** (`:88`, `case "x" in "x") pass
"(already checked)"`), and it is counted in the passed total. The file it claims
does not exist. That is prose closing work — the mechanism this project has
retracted repeatedly — implemented as a gate. (The exact number is deliberately
not written here: `make retraction-count` computes it, and a hand-typed count is
what four earlier counts of this same number got wrong.)

Two further properties, both read from the source:

- It reads its subject from `aes/tickets/`, which is **gitignored**, so in a
  fresh clone it can only ever bail (`Ticket file not found for … in
  aes/tickets/`). A gate that cannot run in a clean clone is not a gate, and the
  D4.3 rule and the DoD's own "no criterion is satisfied by a claim" cannot both
  hold while it is called a Verification Gate.
- Its criterion checker **executes a command extracted from prose**: `:150`
  `bash -c "$file"` where `$file` is the first backticked token of the criterion
  text. An acceptance criterion is therefore able to run whatever its own wording
  names.

**Not fixed here, and deliberately.** Wiring a guard that reads `aes/` into the
Makefile would point a committed gate at an uncommittable path — the same rule
breaking as D4.3 and as the `aes/tickets/T058` pointer that `README.md` used to
carry. The honest dispositions are: **delete the script**, or **rename it so it
cannot be mistaken for a gate** and record that it verifies nothing. Both are
edits to a tracked file that a reader may be relying on, so the choice goes in a
ticket (**T096**) rather than into a commit whose message says "docs".

| — | verified | docs↔code | `CODE_009311` = `$03:7649`, a real trap, not a contradiction; F-12 closed |

**Not a conflict, and deliberately not filed as one:** the 78 frames between bank
03 going silent (f3301) and the city appearing (≈f3378). It is a correlation with
no mechanism attached, and turning it into a cause is the seventeenth retraction
waiting to happen.
## 10. `docs ↔ build flags` — a trace build that "cannot link" because a flag was passed to one compiler and not the other

**CONF-9 · severity MEDIUM · `docs/measurements/2026-10-02-c041-bank03-pc-dump.md`**

This is the third instance of one shape, and the shape is what makes it worth
filing rather than fixing and forgetting. A real machine defect — the Deck's
rootfs really is missing 503 of 504 glibc headers — was observed correctly, and
then a *second, unrelated* defect was attributed to it.

| | claim | measured on the Deck, gcc 15.1.1 |
|---|---|---|
| stated | the sysroot prefix does not satisfy a translation unit that includes `<cstdlib>`, so the trace tier cannot link | `g++ -idirafter /home/deck/sysroot/usr/include` compiles `<cstdlib>`, `<cstdint>`, `<cstring>`, `<cstdio>`: rc 0. `stdlib.h` and `features.h` are both **PRESENT** in the prefix (1483 files under `~/sysroot`) |
| actual | the prefix was passed to `CMAKE_C_FLAGS` and not to `CMAKE_CXX_FLAGS` | every `.c` unit compiled; the first `.cc` unit (`debug_server.c`) did not |

```
$ grep '^CMAKE_CXX_FLAGS:STRING=' ~/simcity/build-instr-tr/CMakeCache.txt
CMAKE_CXX_FLAGS:STRING=                     <-- empty; this is the whole bug
$ grep '^CMAKE_CXX_FLAGS:STRING=' ~/simcity/build/CMakeCache.txt
CMAKE_CXX_FLAGS:STRING=-idirafter /home/deck/sysroot/usr/include
```

The reported symptom was real and the diagnosis was wrong. Under it sat an
unlicensed substitution: the host-only AOT histogram was kept, labelled
host-only, and the open question — *does bank 03 run AOT anywhere* — was left open
on the grounds that the machine that could answer it could not run the
instrument. It can. Ledger row **R-032**.

Fixed: `scripts/deck-trace-build.sh` carries the measured configure line and ends
in a guard that refuses to call a mute build a success (`[aotblk]` count must be
non-zero — trap 3). Run on the Deck from a clean `build-tr`, it configures,
links, and reports **9771** `[aotblk]` lines in f1–f50.

**The generalisable part**, and it is the same as CONF-2 and as the paired
`$0B51` retraction: *a verified observation plus an unsupported conclusion about
a second thing.* A damaged rootfs is a real, measurable, hard-to-ignore
condition; it is therefore an attractive explanation, and it will keep absorbing
the next build failure that happens to occur on the same machine. A second
failure on a broken machine is not evidence that the machine's breakage caused
it — that is a measurement, and it takes one command to make.

## 11. `code ↔ gate` — the evidence gates read the INDEX, not the tree

**CONF-11 · severity HIGH · `scripts/check-retracted-claims.sh`, `scripts/check-cause-claims.sh`**

**This is the mechanism behind the failure mode this file keeps recording: a
gate that reports green on a commit that is not.** Found by *being* that failure
mode, in this session, in the commit immediately before it was found.

Both evidence gates built their scope from `git ls-files '*.md'`, which lists
**tracked files only**. A file authored but not yet staged is therefore
**invisible to the gate**. Demonstrated, not argued:

```
$ printf 'The vblank token handshake is the cause. <-- THE GATE\n' \
      > docs/measurements/zz-scope-probe.md
$ make check-claims          # the file is UNTRACKED
  RESULT: PASS                            <-- a refuted claim is waved through
$ git add docs/measurements/zz-scope-probe.md
$ make check-claims
  VIOLATION docs/measurements/zz-scope-probe.md:2  R-024 (refuted) asserted
    without a retraction marker
  RESULT: FAIL
```

Identical behaviour in `check-cause-claims.sh`, with an untracked `scripts/*.sh`
containing an unlabelled causal assertion.

**The cost, paid in this session.** Commit `af08ff7` ("clock: what gates the
entry into bank `$03`") added
`docs/measurements/2026-10-02-f3271-entry-gate.md`, whose disassembly listing
contained this line:

```
$03:D2AA  85 12      STA $0012       <-- the gate flag, set to 1
```

`<-- the gate flag` contains `<-- THE GATE`, which is **R-024's** pattern,
matched case-insensitively. The gate run that preceded that commit reported
`make check-claims RESULT: PASS` — **because the new file was not yet staged and
so was not in scope.** The commit body repeats that PASS. **It was false.**
Verified against the committed tree:

```
$ git stash -u && make check-claims      # tree exactly as af08ff7 left it
  VIOLATION docs/measurements/2026-10-02-f3271-entry-gate.md:68  R-024 (refuted)
    asserted without a retraction marker
  RESULT: FAIL
```

So `af08ff7`'s gate block is wrong in one line, and this entry is the record of
it. The pattern was **not** weakened to make the check pass. The over-broad
annotation in the new document was reworded to `<-- writes 1 to the flag`, which
is also the more accurate description of what `STA $0012` does there.

**Fixed.** Both gates now build their scope from

```bash
git ls-files -co --exclude-standard
```

`-c` cached, `-o` untracked, `--exclude-standard` drops whatever `.gitignore`
covers. That is exactly the set `git add -A` would stage, so a gate run taken at
any point in the working tree reports on the files that are about to be
committed. `aes/` stays out of scope **because it is gitignored**, which is the
mechanism DoD D4.3 asks for — not because it is inconvenient to reach.

**Falsified in both directions, in both gates, after the change:**

| seeded violation | file state | result |
|---|---|---|
| R-024 phrase in a `.md` | **untracked** | `RESULT: FAIL` (was PASS) |
| unlabelled causal assertion in a `scripts/*.sh` | **untracked** | `RESULT: FAIL` (was PASS) |
| the same R-024 phrase under `aes/` | untracked **and gitignored** | `RESULT: PASS` — correctly ignored |
| restored tree | — | both `RESULT: PASS` |
| `check-claims-self-test` / `check-causes-self-test` | — | both `RESULT: PASS` |

**The general rule this establishes, and it is the one the next session needs:**
*run the evidence gates after `git add`, or on a gate that reads the working tree.*
A green evidence gate measured over a subset of the tree is not a green evidence
gate. This is the same class as CONF-1 — a guard whose scope did not include the
file carrying the falsified claim — and the scope was wrong in both cases. In
CONF-1's case it is still wrong: `docs/RE_SCENARIO_NAV.md` remains outside
`SCOPE_FILES`.

---

**CONF-12 · severity MEDIUM · `scripts/check-retracted-claims.sh`, `docs/review/validate-findings.sh`**

**The evidence gate is intermittently red on a line that is not a violation,
and the review validator reports the flake as a completely unrelated finding.**

Found while taking T101, and **not** caused by anything T101 changed — the same
flake was observed before and after the ledger grew by two rows.

### What was measured

`scripts/check-retracted-claims.sh` exits **1** intermittently with a single
false positive:

```
-- 1. no script or doc asserts a refuted cause --
  VIOLATION docs/CAUSE_CLAIMS.md:83  R-033 (refuted) asserted without a retraction marker
```

Observed **3 times in ~110 invocations** (runs 38 and others of a 60-iteration
loop; `make review-check` **2 failures in 8**). `make review-check` and
`make check-claims` are both green on every other run.

**The flagged line is not a violation.** `docs/CAUSE_CLAIMS.md:83` is the C-039c
row, and it carries `**RETRACTED as stated**` **on the line itself**, plus
`ledger **R-033**` two columns further right. The guard's own rule is "a
correction marker within ±6 lines", and that window holds **12** matches of its
`NEG` pattern.

### ADDED 2026-10-03 (T102) — the rate is far higher than recorded, and it is LOAD-DEPENDENT

**This is a measurement, not an estimate, and it changes how much the flake
should worry a reader.**

| experiment | runs | red |
|---|---|---|
| `scripts/check-retracted-claims.sh`, dev host **idle** | **60** | **0** |
| the same script, **8 CPU hogs** on a 6-thread box | **40** | **7** |
| `docs/review/validate-findings.sh` + `check-retracted-claims.sh`, interleaved | **12** | **5** `review-check` red; of those, **4** with `check-claims` **green** |

**So: ~17% under load, 0% idle**, and in 4 of 5 interleaved failures the
companion gate was green — which is exactly the signature CONF-12 describes and
the reason its "re-run before believing it" rule is the right interim answer.
**The 3-in-110 figure in the original entry understated this by an order of
magnitude.** That is not a criticism of the earlier measurement, which was taken
on an idle machine; it is a statement that **the flake cannot be bounded by
running the gate once**, which is the practical consequence.

Seven distinct false positives were captured, on seven different lines, in seven
different files — including `docs/CAUSE_CLAIMS.md:81` and `:111`, which carry
`**RETRACTED**` **on the line itself**, and `docs/ROADMAP.md:225`, whose own
first cell reads `**RETRACTED as stated (2026-10-02)**`.

### What was ruled out, and what was NOT

- **NOT the inner test.** A standalone harness replicating the guard's exact
  `printf '%s' "$ctx" | grep -qE "$NEG"` decision, over the six `(file, line)`
  pairs that actually failed, was **2400/2400 clean idle and 2400/2400 clean
  under the same 8-hog load**. The marker test does not flake in isolation.
- **NOT reproducible as an exit code.** An instrumented copy of the guard
  (written to `/tmp`, never committed) recorded six readings of **141** —
  `128 + SIGPIPE` — for the inner `grep`, and **none of them reproduced** in the
  standalone harness. **`141` is therefore reported as an artefact of the
  instrumentation and no mechanism is claimed from it.** This is the same
  discipline the rest of this file uses and it is applied here to my own probe:
  a reading that cannot be reproduced is not a finding.
- **Therefore the varying input is upstream of the marker test** — in the
  `SCOPE_FILES` list (`git ls-files -co --exclude-standard`) or in the per-pattern
  `hits` list — **and that is not yet separated. Cause: still OPEN**, now with a
  rate and a load dependence attached instead of a bare "3 times in ~110".

### The fix is obvious and is NOT applied here

`docs/review/validate-findings.sh` delegates F-03/F-13/F-14 entirely to
`check-retracted-claims.sh`'s **exit code**, so a false positive is reported as a
finding about a phrase that does not exist. Making the marker test not depend on
a pipeline's exit status at all (`grep -qE "$NEG" <<<"$ctx"`, no pipe) removes
the whole class. **It has not been applied**, because a change to a gate whose
own correctness is in question must be falsified properly first — red on a
seeded violation, and 0/40 under the load that makes the unfixed version fail
7/40. That falsification has not been run, so **the gate is left exactly as it
is and this entry stays OPEN.**

### How to read a red `make review-check`

A `REFUTED F-03`, `F-13` or `F-14` line accompanied by
`make check-claims → RESULT: PASS` is **this flake, not a finding**. Re-run
before believing it. Do not "fix" the doc it names; the doc it names is not the
problem.

---

**CONF-13 · severity MEDIUM · the Deck, `ssh`, and backgrounding**

**A heavy Deck run launched in the background dies silently. A foregrounded one
does not. Every load-bearing number in T101 exists because the runs were
foregrounded, and nothing in the tree would catch a repeat.**

Found during T101 (`9069182`). Carried as **D014 V3** and as finding #3 of
`aes/peer-reviews/T101/REVIEW.md`.

**What was measured.** Three runs launched as `setsid nohup ./run.sh > log 2>&1
< /dev/null & disown` from an `ssh` command line died with **no error message, no
core, and no exit status**, at run-frames **2160, 1440 and 1440**. The identical
command run **in the foreground** through `ssh` completed 30 000 frames in **211 s
with `EXIT=0`**, three times out of three.

```
$ ../jjhead-clean "SimCity (USA).sfc" cold.srm out.srm 30000 s.script .
RESULT failed=0 frames=30000 master_clock=10720929618 insns=356512178 sram_dirty=1
EXIT=0
seconds: 211
```

**Why this is a conflict and not a nuisance.** The project's own measured rule —
*"`exit: SDL_QUIT` on the Deck means you signalled it; a truncated trace is not a
smaller trace"* — exists because a truncated run reads exactly like a short one.
A run that dies without writing any exit status at all is the **same failure with
one fewer clue**, and it is silent in the worst way: the log looks complete up to
the frame it reached.

**The three discarded runs are named here so nobody re-derives them.** Nothing in
T101 rests on them. Every peer number in that commit is a foreground run with an
observed `EXIT=0`.

**Cause: OPEN.** Not measured. The candidates were not separated — a signal from
session teardown, the Deck's user-session reaper, or resource behaviour under
concurrency. The first three attempts also ran **three at once**, so concurrency
is confounded with backgrounding and neither is isolated.

**No guard is written, deliberately.** The closure condition is in the review: the
run wrapper must write `rc=$?` to a file that the analysis step requires to exist
and be non-empty. **A new guard must be falsified before it is committed — red on
a seeded violation — and this session has no budget to do that properly.** Wiring
an unfalsified guard into this repository's evidence path is precisely the
`af08ff7` shape (a commit body claiming a green gate that was never
demonstrated). Recorded instead.

**Interim rule, stated so the next session inherits it:**

> **Foreground every heavy Deck run through `ssh`, and read `EXIT=` before you
> read the log.** If a run must be backgrounded, the wrapper writes
> `rc=$?` to a file and you check that file exists before trusting anything the
> run produced.

---

**CONF-14 · severity MEDIUM · `scripts/check-retracted-claims.sh` check #3 cannot see the ledger's own phrasing**

**The count guard matches `"N retractions"`. The ledger's own output says
`"N refuted"`. So a stale count in the ledger's native phrasing passes.**

## What was measured

`CNT_RE="($WORDNUM|[0-9]{1,3}) +retract[a-zçãõ]*"` (line 366). `README.md`'s gate
table stated:

```
| `make retraction-count` | the retraction ledger, computed | **34 rows = 26 refuted + 6 superseded + 2 invalidated-premise** | 0 |
```

`make retraction-count` said **36 rows / 28 refuted**. The gate's check #3
reported **`(no violations)`** — because the line says `26 refuted`, not
`26 retractions`. **The number in the most-read file in the repository was stale
and the guard built to catch exactly that did not see it.**

**The stale number is fixed** (`36 rows = 28 refuted + 6 superseded + 2
invalidated-premise`). **The hole is not closed.**

## The fix was attempted and REVERTED, and that is the finding

Adding an alternation for `"N refuted"` was implemented and then measured against
the project's own corpus. It produced **three false positives**:

```
VIOLATION   README.md:506 states a count of "0"; the ledger says 28
VIOLATION   README.md:507 states a count of "0"; the ledger says 28
VIOLATION   scripts/check-retracted-claims.sh:128 states a count of "two"; the ledger says 28
```

Both classes are the same defect and neither is the seeded violation: the number
extractor takes **the first integer or word-number anywhere on the line**, so a
markdown table row whose trailing cell is `| 0 |` is read as "0 retractions" if
the word *retracted* appears anywhere on that line, and the guard's own header
text is read as "two retractions".

**A guard that cries wolf on its own corpus is worse than the hole it closes** —
it trains the reader to ignore the check, and this project's own script says so
at line 343. The alternation was removed. The failed attempt is recorded **in the
script's own comment block**, not only here, so the next person meets it before
re-deriving it.

## What a real fix needs

Not a wider pattern. A **number extractor that binds to the token immediately
adjacent to the count noun** — `([0-9]{1,3})[[:space:]]+(refuted|retractions?)`
with the match, not the line, supplying the number. That is a rewrite of the
extraction step, it changes which lines the guard reads, and it therefore needs
its own falsification run over the whole corpus before it is committed.

**OPEN. Deliberately not closed in the same session that discovered it.**

## Interim rule

> **The retraction count has exactly one authority: `make retraction-count`.**
> Do not type it. If a document must state it, state it as
> `N rows = M refuted + …` **and re-read it after running the command** — which is
> what caught this one.

---

**CONF-15 · severity HIGH · `SNESRECOMP_COUNT_PC` and two WRAM-dump bounds parse hex as octal — and the rule was already in the record**

**A base-0 `strtoul` reads a bare leading-zero hex value as octal and stops at
the first digit that is not an octal digit. `SNESRECOMP_COUNT_PC=038026` counted
executions of PC `$000003`, in bank `$00`, and printed a clean, formatted,
entirely plausible zero.**

Found while taking T102 (`f278506` + the T102 commit). **It voids the
measurement behind C-041c and T100's `1b099ce` headline** — ledger row **R-037**
— and it is HIGH because the class is silent, universal in shape, and was
already documented four days before the measurement that walked into it.

## What was measured

`interp816.c:323`:

```c
s_interp_pc_watch = (uint32_t)strtoul(e, NULL, 0);   /* base 0 */
```

Base 0 auto-detects: `0x` is hex, a leading `0` is **octal**, and parsing stops
at the first character that is not a valid digit *in the detected base*.

```
009311     -> strtoul(base 0) = 0 = 0x000000
038026     -> strtoul(base 0) = 3 = 0x000003
0x009311   -> strtoul(base 0) = 37649 = 0x009311
0x038026   -> strtoul(base 0) = 229414 = 0x038026
9311       -> strtoul(base 0) = 9311 = 0x00245F
```

The knob is documented at `interp816.c:195` as **`SNESRECOMP_COUNT_PC=<hex>`**,
and its own default is written `0x009311` — with the prefix. The documented usage
and the documented default are both safe. **The natural reading of `<hex>` is
not.**

### The falsifier that caught it was a positive control, and it is the transferable part

```
COUNT_PC=009311   -> [count] pc watched: 0 executions over 300 frames
COUNT_PC=038026   -> [count] pc watched: 0 executions over 300 frames
COUNT_PC=0x009311 -> [count] pc watched: 239617 executions over 300 frames = 798.7 per frame
COUNT_PC=0x009313 -> [count] pc watched: 239617 executions over 300 frames = 798.7 per frame
COUNT_PC=0x038026 -> [count] pc watched: 0 executions over 300 frames
```

`$00:9311` is `INC $C7`, the vblank spin body. It is the instrument's **own
default** (`interp816.c:198`), it is what C-002 measures advancing 5/5, and
`RE_CITY_FREEZE`'s WLOG census counts the spin 4 110 times in 5 000 frames. **A
zero from it is the instrument, not the world.** `$009311` and `$009313` agreeing
to the unit is what a spin body's two halves should do, and it is the reason the
`0x` reading is believed.

**T100's declared falsifier was "does the counter print anything", and it
passed.** The counter printed a confident, correctly-formatted `0`. What it did
not ask was whether the counter had ever been shown non-zero **for the address it
was given**. Every counter instrument in this repository now needs a positive
control in the same build and run configuration, or its zeros are uninterpreted.
**That is a rule, not a gate, and nothing here should be read as claiming
otherwise.**

## Not one knob

| knob | documented as | parsed with | measured symptom |
|---|---|---|---|
| `SNESRECOMP_COUNT_PC` (`interp816.c:323`) | `<hex>` | `strtoul(…, 0)` | **wrong PC watched**; silent, clean-looking zero |
| `SNESRECOMP_WRAM_DUMP_HI` (`host_main.c:1548`) | `(hex)` | `strtol(…, 0)` | `0B60` → **0**, so `hi-lo = 0`: `[wramdump] wrote … (0 bytes)` |
| `SNESRECOMP_WRAM_DUMP_LO` (`host_main.c:1547`) | `(hex)` | `strtol(…, 0)` | `0B40` → **0**, dumping the whole 128 KB instead of 32 bytes |

`SNESRECOMP_WRITE_WATCH` (`cpu_state.c:519`, `:613`) and
`SNESRECOMP_WRAM_WATCH` (`common_cpu_infra.c:937`) are documented **with** a
`0x` prefix (`0xADDR`) and are safe if the documentation is followed — which is
the only reason this is HIGH and not higher. `strtol(…, 0)` / `strtoul(…, 0)`
appears at **28 sites** in `snesrecomp/runner/src` and `src/`.

## The part that is worse than the bug

**`docs/RE_CITY_FREEZE.md:1546`, dated 2026-09-30, headed "Um bug meu que
invalida probes anteriores", says it exactly:**

> `interp816.c:325` usa `strtoul(e, NULL, 0)` — base 0, portanto um `0` à frente
> significa **octal**. […] **Qualquer resultado de `COUNT_PC` registado neste
> projecto sem prefixo `0x` é nulo.**

T058 used `0x009311` and was correct. **T100 (`1b099ce`) used `038026`.** The
rule was in a tracked file, in the right words, four days before the measurement
that violated it — and it protected nothing, because it sat in an append-only
log at line 1546 and not in the README's instrument list, which is where a
reader starts. CONF-1 and CONF-11 are both "a guard whose scope did not include
the thing"; this is the same failure with **no guard at all**, and the knowledge
present.

## What is deliberately NOT done

- **The parse is not fixed.** The fix is one character per site in a **pinned
  submodule**; `snesrecomp` is pinned to a branch (`T097`), so a local edit
  reaches no clone and creates a divergence the tree does not track. Shipping a
  fix that only exists on this machine is the `af08ff7` shape.
- **No gate is wired.** A guard that rejects a bare leading-zero hex value on a
  base-0-parsed knob is mechanically possible, and it is the obvious next thing —
  **but it must be falsified before it is committed** (red on a seeded
  violation, green on a labelled one, and demonstrated on an **untracked** file
  so CONF-11 cannot recur). That falsification run has not happened, so no guard
  exists. Wiring an unfalsified guard into this repository's evidence path is
  precisely what CONF-13's own note says not to do.

## Interim rule

> **Any knob documented as `<hex>` or `0xADDR` in `snesrecomp` gets the `0x`
> prefix, always — and any execution count gets a positive control on a PC known
> to execute, in the same run configuration, before its zero is believed.**
> Today: `SNESRECOMP_COUNT_PC=0x038026`, `SNESRECOMP_WRAM_DUMP_LO=0x0B40`,
> `SNESRECOMP_WRAM_DUMP_HI=0x0DC8`.

## Where the affected claims stand

| claim | status |
|---|---|
| **C-041c**, T100's `0 executions over f0–f6000` and `f0–f9000` | **RETRACTED (R-037)** — the measurement watched PC `$000003` |
| **C-041** (`$03:8026` not among the bank-`$03` PCs) | **UNAFFECTED** — `INTERP_DUMP_BANK` *enumerates* PCs; a parse bug cannot corrupt an enumeration |
| **C-008** (`INC.w $0B51` executes zero times) | **UNAFFECTED** — same reason |
| **C-066** (`$03:8026` = 0 over 14 000 frames) | the surviving, re-derived claim — prefixed counter, `lle_only` manifest argument, and C-041's enumeration |

---

**CONF-16 · severity MEDIUM · two untracked files in `docs/` that the evidence gates read and a clone does not have**

**Found while running the T102 gates. `make check-causes` was RED on a cause
that exists only on this machine, and the gate would be GREEN in a clone.**

## What was measured

```
$ scripts/check-cause-claims.sh
  scope : 42 file(s), tracked + untracked-not-ignored
-- 1. every causal assertion is labelled --
  VIOLATION docs/QUALITY_GATES.md:13  causal assertion with no provenance marker within 5 lines
  RESULT: FAIL
```

`docs/QUALITY_GATES.md` and `docs/DECK_RUNBOOK.md` are **untracked**
(`git ls-files` lists neither) and are read by the guard, because
`check-cause-claims.sh` derives scope from `git ls-files -co
--exclude-standard` — **CONF-11's fix.** The scope diff, measured by stashing:

```
$ diff <(git ls-files -co --exclude-standard '*.md' 'scripts/*.sh')   # before / after
> docs/DECK_RUNBOOK.md
> docs/QUALITY_GATES.md
```

**CONF-1 and CONF-11 are both "a guard whose scope did not include the thing",
and both were fixed by widening scope. This is the cost of that fix arriving
from the other side: a guard whose scope now includes something the repository
does not ship.** A gate that is red here and green in `git clone` is worse than
one that is red in both, because the local red trains the reader to ignore it.

## The writer — UNIDENTIFIED at the time, then IDENTIFIED, and the correction is kept

**This section is written twice on purpose.** The first version said the writer
was unidentified. It was not, and the second version says so with the evidence,
because a record that quietly drops an OPEN finding is the same defect as one
that quietly drops a retracted one.

**What was excluded before the writer was found** (all measured, all negative):

- **Not present at session open.** The session's first `ls docs/` returned 14
  entries; neither file was among them.
- **Not written by anything in the repository.** Checked and excluded: every
  script under `scripts/`, `docs/review/validate-findings.sh`,
  `docs/review/validate-findings-c041.sh`, both ctest targets, the git
  `pre-commit` hook (`.git/hooks/pre-commit` → `.aes/hooks/pre-commit.sh`, which
  only greps `src/` for `TODO`/`printf`), and the Deck rsync (host → deck, never
  the reverse).
- **Not in `aes/`.** `find aes .opencode .aes -name QUALITY_GATES.md -o -name
  DECK_RUNBOOK.md` returns nothing.

**What identified it.** `aes/decisions/D015.md` — an **`aes-project-manager`
decision record, dated 2026-10-03, file mtime 01:20:00** — names both files as
things that were missing and files its remedy as a ticket:

> **Ticket:** T103 — Create T102-plan + Deck runbook + docs/QUALITY_GATES.md
> **Type:** finding

and its evidence section is literally `ls …/docs/QUALITY_GATES.md` → *No such
file or directory* and `ls …/docs/DECK_RUNBOOK.md` → *No such file or directory*.
**The decision record's remediation created exactly the two files it complained
were absent**, in the same minute. `aes/tickets/T102-plan.md` carries the same
01:20 stamp.

**So the writer is `aes-project-manager`, acting on its own D015 finding — not
an unknown process, and not the gates.** Recorded because it is the more useful
half: **a skill that writes into `docs/` puts files the evidence gates read and
`git clone` does not have, and it does so as a side effect of answering a
question.**

### And the second thing D015 caused: a ticket-number collision

D015 filed its remedy as **T103** and **never materialised it as a ticket file**,
so `ls aes/tickets/T103*` finds nothing and the number looks free. This session's
next measurement — the `$03C87x` scan loop — was about to take it.

**The earlier claim keeps the number.** The scan-loop measurement is therefore
**T104**, and the collision is recorded in `docs/ROADMAP.md`'s CURRENT NEXT PATH
section. **This is the fourth time in this project's history that a ticket
number has been the thing that was wrong** (T097, T098, CONF-1, and now this),
and it is the first time the collision was between two artefacts of the same
system rather than between a doc and a script.

## What was done, and what deliberately was not

- **The violation was fixed in place**, by adding provenance to the three "Why"
  cells in `docs/QUALITY_GATES.md`. The sentence really is a causal assertion
  and really did lack a marker, so this makes the gate **honest** rather than
  green-by-deletion. `make check-causes` and `make check-causes-self-test` are
  both `RESULT: PASS` after it.
- **The files are still untracked and were NOT committed.** Committing
  AES-derived prose of unidentified provenance into the repository would import
  an unexplained artefact; deleting them would have turned a real gate red into a
  fake green and thrown away the finding.
- **No guard was written.** A guard that fails when `docs/` contains an untracked
  markdown file is mechanically trivial and would close this permanently — but
  it must be falsified first (red on a seeded untracked file, green when the
  tree is clean, and demonstrated on an **untracked** file so CONF-11 cannot
  recur). That has not been done, so it does not exist.

## Interim rule

> **A gate result on this machine is only comparable to a gate result in a clone
> if the scope is only tracked files.** `git ls-files -co` means it is not.
> Before believing a red evidence gate, run `git status --short` and check
> whether the file it names is even in the repository. And when something in
> `aes/` names a missing file as its remedy, check whether it has since created
> it — `aes/decisions/` is a ledger of side effects, not only of opinions.

---

**CONF-17 · severity HIGH · `check-retracted-claims.sh` section 1 does not scan `.sh` files outside a four-name hand list — and does not print that it doesn't**

**Found while running `aes-narrative`'s omission-rate metric by hand, 2026-10-03.
Falsified in both directions before being written down.**

## The measurement

Section 1's scope is a hand list of **four** scripts plus every `*.md`:

```sh
SCOPE_FILES=( scripts/clock-gate.sh scripts/perf-gate.sh
              scripts/verify-rom-render.sh scripts/cross-load-peer-save.sh
              README.md docs/RE_CITY_FREEZE.md docs/CLAIMS_REGISTER.md
              docs/ROADMAP.md docs/DEFINITION_OF_DONE.md study/peer-linux/README.md )
…
done < <(git ls-files -co --exclude-standard '*.md' 2>/dev/null)   # <-- *.md ONLY
```

`scripts/check-retracted-claims.sh` is **neither** a named entry nor a `.md`, so
**the guard does not read itself.** Nor does it read `scripts/deck-trace-build.sh`,
`scripts/clock-probe.sh`, `scripts/clock-probe-live.sh`, `scripts/crossplatform-determinism.sh`,
or any `docs/review/validate-findings*.sh`.

### Falsified, both directions, on an UNTRACKED file in each case (CONF-11)

```
$ printf '#!/usr/bin/env bash\n# The gate is $0012, measured 0 in 13 of 13 samples\n' > scripts/.seedtest.sh
$ git ls-files -co --exclude-standard '*.md' 'scripts/*.sh' | grep -c seedtest
1                                   <-- git DOES list it; the guard's glob does not take it
$ scripts/check-retracted-claims.sh
-- 1. no script or doc asserts a refuted cause --
  (no violations)                   <-- RED FAIL EXPECTED, DID NOT COME

$ printf '# seed\n\nThe gate is $0012, measured 0 in 13 of 13 samples\n' > docs/.seedtest.md
$ scripts/check-retracted-claims.sh
-- 1. no script or doc asserts a refuted cause --
  VIOLATION docs/.seedtest.md:3  R-005 (refuted) asserted without a retraction marker
  VIOLATION docs/.seedtest.md:3  R-007 (refuted) asserted without a retraction marker
```

**Identical claim, identical ledger rows, identical untracked status — red in
`.md`, silent in `.sh`.** Both seeds removed; the restored tree is `RESULT: PASS`.

## Why the guard cannot simply scan itself

`scripts/check-retracted-claims.sh:438-442` is a **self-test fixture**:

```sh
cat > "$tmp/seed.sh" <<'SEED'
printf "What is established:\n"
printf "  the gate is \$0012, measured 0 in 13 of 13 samples\n"
SEED
```

That text is an unmarked refuted phrase **on purpose** — it exists so the guard
can be proven to fire. Scanning itself would therefore turn the guard permanently
red. **This is the third instance of one defect in this repository**: a guard's
own text reads as a violation of the guard. CONF-14 found it in the count guard's
header, the first attempt at fixing which produced three false positives and was
reverted. **The pattern is now named: the guard's own fixtures are unmarked
assertions by construction, and every scope decision about them is invisible.**

## The second half: the exclusion is not printed

The script prints what it excluded from the derived list —

```
  scope   : 35 file(s); 4 excluded as dated-or-hashed records
            (docs/review/RUBRIC.md, docs/review/REVIEW-*.md - audit with: …)
```

— and says nothing about the `.sh` files its glob silently drops. **CONF-11's own
rule is "an exemption nobody can see is a gate with a hole in it", and this
exemption is invisible.** A reader who sees `scope: 35 file(s)` reasonably reads
"35 files checked"; the true figure is "35 of the ~41 files in the repository
that could carry a claim".

## What is deliberately NOT done

**The one-character fix is not applied.** Widening the derived glob to
`'*.md' 'scripts/*.sh'` is correct and trivial, and it needs one companion
change — a **visible, printed** exclusion for the guard's own fixture block —
because without that second change the gate is red forever. Two changes to an
evidence gate, one of which touches its own self-test, is exactly the situation
the project has been burned by repeatedly (`8a7340f`, `af08ff7`).

It goes in the hardening pass with the falsification the project requires: **red
on a seeded violation in an untracked `scripts/*.sh`; green on the same sentence
carrying a marker; green on the guard's own fixture; green on the restored
corpus; and the scope line must then *print* the `.sh` exclusion.**

### CLOSED 2026-10-03 — the fix, and its falsification in four directions

The derived glob is now `'*.md' 'scripts/*.sh'`, **and the guard excludes itself
by name, visibly.** The exclusion is *printed* in the scope block, because an
exemption nobody can see is a gate with a hole in it — which is the whole of
CONF-11 and CONF-1 restated.

| falsification | seeded state | result |
|---|---|---|
| **1. red on a seeded violation** | untracked `scripts/.seedtest.sh` carrying an unmarked `R-005`/`R-007` phrase | **`VIOLATION scripts/.seedtest.sh:2 R-005`** + `R-007` — **was `(no violations)` before the fix** |
| **2. green on a labelled one** | same file, same sentence, prefixed `RETRACTED:` | **`(no violations)`** |
| **3. green on the restored corpus** | both seeds removed | **`RESULT: PASS`** |
| **4. the self-test still fires** | `make check-claims-self-test` | **`RESULT: PASS`** |
| **5. idempotence** | 20 consecutive direct runs | **0 red / 20** |
| **6. no new false positives** | the whole tracked corpus, widened scope | **`RESULT: PASS`**, scope 35 → **42 files** |

**Direction 1 is the one that matters and it is the exact before/after of the
hole: the identical claim in an untracked `.sh` went from silent to caught,
while the identical claim in an untracked `.md` was already caught.**

**And the exclusion is now visible where it was not:**

```
  scope   : 42 file(s); 4 excluded as dated-or-hashed records
            1 file excluded as THE GUARD ITSELF (CONF-17):
            scripts/check-retracted-claims.sh - its self-test fixture carries an
            unmarked refuted phrase on purpose, so it cannot scan itself.
```

**What remains open, stated rather than hidden:** the guard still does not read
**itself**. That is a real residual hole, accepted deliberately, because the
alternative is a permanently red gate. It is bounded: the only unmarked refuted
phrases in the guard are its own quoted fixtures, and they are identifiable by
being inside comment or heredoc blocks.

---

**CONF-18 · severity LOW · `README.md` stated `make clock`'s exit code as 1; it is 2**

**Found by running the gate instead of quoting it, 2026-10-03, at `1adede1`.**

`README.md` carried, in bold, *"**`make clock` exits 1, not 2**"* — and the
distinction was called load-bearing. It is wrong **about `make`**. All four
measured:

| command | exit | why |
|---|---|---|
| `scripts/clock-gate.sh --frames 6000` | **1** | the FAIL verdict |
| **`make clock`** | **2** | GNU Make 4.3 maps any failed recipe to exit 2 |
| `scripts/clock-gate.sh --help` | 0 | usage |
| `scripts/clock-gate.sh --nonsense` | 2 | unknown flag |

**`scripts/clock-gate.sh` is correct and has no exit-2 path of its own.** The 2
is make's. The error was in the most-quoted sentence in the file about the
project's most important gate, and it failed in the direction that matters: it
invited a reader to treat a **`make` failure as evidence about the gate**.

**Why it survived so long, and why this one is not a retraction.** It is not a
cause claim about the guest, so `check-retracted-claims.sh` correctly has no
ledger row for it and correctly stayed silent — **the guard is not a fact-checker
and was never supposed to be**. It is also not in `docs/CAUSE_CLAIMS.md`. **No
gate in this repository recomputes a prose claim about a gate's own exit code,
and `docs/review/validate-findings-c041.sh` says so explicitly** — *"no command
recomputes README's numbers"* — which is the D3.3/D3.4 retirement showing its
cost a second time.

**Cost:** four commands, ~4 minutes. It was found in the last twenty minutes of
the session by doing the one thing the brief and the `aes-project-manager` skill
both insist on and this file had not been doing: **running the thing and reading
its status, instead of transcribing it.**

**Corrected in place in `README.md` and `docs/DECK_RUNBOOK.md`,** with all four
codes recorded rather than one. Not entered in the ledger: a retracted claim is a
claim about the *guest*, and inventing a row for a claim about the build would
make the retraction count mean two different things — which is precisely how
CONF-14's "26 refuted" and a prose "two retractions" in a script header came to
disagree.

---

## CONF-19 · severity HIGH · `SNESRECOMP_CYC_WATCH` prints the fetched opcode byte, and four sessions of ROM-byte archaeology never asked for it

**The conflict.** C-069 read `$03C87F` as "the second byte of `8D D0 F6`" from
ROM bytes and, applying C-056's rule faithfully, recorded it as **"not an
instruction boundary"** and **"unattributed, not as an executed instruction"**.
That is a claim about the **executed stream**, and it was derived entirely from
a **ROM decode**.

T104 measured the executed stream. The opcode byte **fetched** at `$03C87F` is
**`$D0`**. It is the loop's **only** branch and its back-edge **36 339** times.
`$03C87E` — the byte C-069's reading made the instruction start — **never
executes**: **0** occurrences in 2 676 196 trace lines, absent from the whole-run
bank-`$03` dump. **Ledger R-038 retracts the clause. C-069's cost figures are not
retracted and reproduce to the unit.**

**The instrument was already in the tree, and it prints `op=$%02X`.**
`SNESRECOMP_CYC_WATCH="lo-hi"` (`interp_bridge.c:2084`) is documented in
`README.md` trap 1 and `docs/RE_CITY_FREEZE.md:3082` — **as a cycle-accounting
tool**. Nobody had read past "blind to AOT" to notice that its log line also
carries the fetched opcode. It answers the ROM-versus-stream question directly,
and it took one run to do it.

**Why this is a conflict and not a retraction.** Both. The retraction is R-038,
about C-069's clause. The conflict is the *class*: **`README.md` and
`RE_CITY_FREEZE.md` both describe `CYC_WATCH` by its limitation and never by its
capability**, so a reader deciding which instrument to reach for has been given
one half of the answer. A guard document that lists only what an instrument
cannot do will steer every future session away from it.

**The generalisation, and it is now the third appearance of C-056's rule**
(`$03:D947`/`$03D94B`; R-034's "the logged next-PC does not match its length";
now this):

> **A byte-boundary question about the ROM cannot be answered by, or exported
> into, a claim about execution. Where the two disagree, the fetched opcode byte
> settles it — and `SNESRECOMP_CYC_WATCH` prints it.**

**Falsified, in both directions and on untracked files, CONF-11 style** — see
the commit body for the full transcript. Seeding R-038's refuted phrase —
`03C87F is the second byte of`, quoted here so the falsification is
reproducible — into `docs/.t104seed.md` → `VIOLATION … RESULT: FAIL`; the
identical phrase in `scripts/.t104seed.sh` → `VIOLATION … RESULT: FAIL`; both
removed → `RESULT: PASS`.

**This guard fired on the paragraph above, on its first run, on text written to
document it.** That is recorded rather than quietly reworded, because a guard
that only ever catches other people's prose has not been shown to work on the
author's. The `*.sh` half of that is the CONF-17 fix holding, which is the first
time it has been exercised by a *new* row rather than by its own self-test.

**Fixed in place** in `README.md` (instrument list) and
`docs/DECK_RUNBOOK.md` (recipe + the rule), in the same commit.

---

## CONF-20 · severity HIGH · the ledger's `where` column is checked for *existence*, not for *completeness* — so R-038's refuted clause survived **unmarked in a tracked file** and both evidence gates passed it

**The conflict, measured 2026-10-03.** R-038 retracted C-069's `$03C87F`
"not an instruction boundary" clause. Three tracked files still asserted it.
Two carried a retraction marker (`README.md`, `docs/ROADMAP.md`,
`docs/CLAIMS_REGISTER.md`, `docs/CONFLICTS.md`). **One did not:**

```
docs/CAUSE_CLAIMS.md:240   One of the four, `$03C87F`, is **not an instruction
                           boundary** and is reported unattributed. What the
                           loop scans, what branches back, and what terminates
                           it are **unmeasured**, and **no cause is claimed**.
```

That is the retraction, restated as present-tense fact, in the one document that
`README.md` itself names as *"the classification of **every** causal claim in this
project"*. It also presented **T104 as the next measurement** a commit after T104
closed.

**Both gates passed it. Measured, not asserted:**

```
$ make check-claims   ;  # RESULT: PASS
$ make check-causes   ;  # RESULT: PASS
```

**Why, measured — the mechanism is two independent holes:**

1. **R-038's ledger phrase is a narrow literal.** Its `phrase` field is
   `03C87F is the second byte of`. `check-retracted-claims.sh` §1 is **lexical**:
   it greps for that string. `CAUSE_CLAIMS.md:240` says *"`$03C87F` is **not an
   instruction boundary**"* — a faithful **paraphrase** of the retracted claim
   that does not contain the retracted **words**. A lexical guard cannot see a
   paraphrase. Falsified in both directions, on **untracked** files (CONF-11
   style):

   | seeded into `docs/.seed-r038*.md` | `make check-claims` |
   |---|---|
   | the ledger's exact phrase, unmarked | **`VIOLATION` → exit 2** ✅ positive control |
   | the `CAUSE_CLAIMS.md` paraphrase, unmarked | **`(no violations)` → exit 0** ❌ the hole |
   | neither | `RESULT: PASS` |

   So the guard is **live** and its blind spot is **measured**, not theorised.

2. **DoD D3.2 checks the `where` column for existence, never for completeness.**
   The whole of D3.2's implementation is: non-empty, and `f="${w%%:*}"`
   resolves to a real file. R-038's `where` names **one** file
   (`docs/measurements/2026-10-03-t104-c87x-scan-loop.md`), which exists, so
   D3.2 passes. **Nothing in the ledger or in D3.2 can represent that the same
   retraction is asserted in three other places**, and nothing enumerates the
   places a refuted phrase was ever written into.

**The generalisation, and it belongs beside CONF-14 rather than beside CONF-19:**

> **A lexical retraction guard proves that a retracted *string* is not restated.
> It does not prove that a retracted *claim* is not restated.** The moment a
> document paraphrases instead of quoting, the guard's coverage ends — and the
> paraphrase is the more natural way to write a summary, so the hole widens
> exactly where documents are most read.
>
> **The two live holes in `scripts/check-retracted-claims.sh` are now named
> separately and both are OPEN:** **CONF-14** — the count guard is
> forward-only, so `retracted **29** claims` escapes a guard that catches
> `29 retractions` (demonstrated on `README.md:22`, the most-read line in the
> repository, which carried a stale **29 of 37** against a ledger of **30 of
> 38**). **CONF-20 (this)** — the phrase guard is literal, so a paraphrase
> escapes, and D3.2 cannot see it because the ledger has one `where` per row.

**NOT FIXED HERE, DELIBERATELY.** A claim-level guard needs the claim, not the
string, and this project has neither a machine-readable claim identity nor a
list of assertion sites per row. Adding an alternation for the paraphrase would
be CONF-14's mistake repeated — the last attempt at widening the count guard
produced **three false positives on this project's own corpus** and was reverted.
**The instance is fixed in `docs/CAUSE_CLAIMS.md`; the class is recorded here and
stays OPEN.** `docs/DEFINITION_OF_DONE.md` D3.1/D3.2 are therefore **floors, not
checks**, which is what its own "Not criteria" section already says about them.

---

## CONF-21 · severity HIGH · the differential suite compares AOT against `interp816`, and its corpus generator encodes the *same wrong width rule* — so 1 599 000 checks cannot see this class of defect

**The conflict, measured 2026-10-03 (T105).** `interp816` computes
`CPX #imm` against a **2-byte** operand and advances the PC by 3 when `xf=0`
(**C-073**). The submodule's own accuracy report records this suite passing:

```
snesrecomp/SNES_ACCURACY_BURNDOWN.md:108
  LDX/LDY/STX/STY/CPX/CPY — 323 opcode variants, all 0 divergences (969k checks).
  ... Now 533 opcode variants, 0 divergences (1.599M checks) vs interp816.
```

**Those checks compare the AOT code generator against `interp816`.** Read the
last four words: *"vs interp816"*. It is an **internal-consistency** suite
between two implementations of this project, not a conformance suite against
hardware. Two implementations that share a mistake agree perfectly.

**And they demonstrably share this one, because the corpus generator states it:**

```
snesrecomp/tests/cpu_diff/gen_ops.py:30
  # index-immediate compares/loads (width = X flag)
  IMMX_OPS = [("ldx", 0xA2), ("ldy", 0xA0), ("cpx", 0xE0), ("cpy", 0xC0)]
  ...
snesrecomp/tests/cpu_diff/gen_ops.py:67
        emit(f"{label}_{imm:02x}_lo_x0", [op, imm, 0x00], 1, 0)   # 16-bit index
```

Line 30's comment **is the defect, restated as a test-corpus invariant** —
"width = X flag" is exactly the rule `CPX #imm` does not obey. Line 67 then
emits `[op, imm, 0x00]`, a **3-byte** encoding, for the `x=0` case. So the
operand bytes planted in the test ROM are wrong in the same direction as the
decoder, and the differential sees two implementations making the *same*
mistake and calls it agreement.

**Why this is filed as a conflict and not a ticket.** The generalisation is the
project's own, and this is its purest instance:

> **An instrument that cannot see the answer looks exactly like an instrument
> that found nothing.** Trap 1 of `README.md` records it for a driver that
> printed `$0B53` under the label `$0B55`. Here it is a **test suite** whose
> pass count is the project's strongest accuracy evidence, and the blindness is
> in the *corpus*, not the comparison — so no amount of adding opcodes or checks
> to it can help. **Coverage is not the problem. Independence is.**

**NOT FIXED HERE, and the reason matters.** A fix needs an **external
conformance reference** — a real 65816, or a disassembler with a published,
independently-derived length table. This repository has none:
`study/peer-linux/` is four tool scripts and a README, not an emulator;
`snesrecomp/tools/cyc_watch/` has a cycle hook, not a length oracle. So the
65816 rule C-073 says `$A0/$A2/$C0/$E0` violate is recorded as **[INFERRED]**,
and the honest next step is to obtain that reference rather than to guess twice.
**The measurement stands either way** — *our* decoder reads two operand bytes
for `$E0` and advances three; that is true regardless of what hardware does.

**Also recorded, because it will bite the next reader:** the corpus generator is
in the **submodule**, and `gen_ops.py` is not in any gate's scope. Fixing it in
this repository would reach no clone without a submodule push, exactly as
`snesrecomp`'s own history shows for every other instrument change here.
