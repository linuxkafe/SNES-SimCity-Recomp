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

### The isolation that clears the logic and does not explain the flake

| experiment | repetitions | violations |
|---|---|---|
| the exact ±6-line `ctx` test for line 83 | **200** | **0** |
| the complete section-1 loop for `CAUSE_CLAIMS.md`, all **36** ledger patterns | **12 full iterations** | **0** |
| `scripts/check-retracted-claims.sh` direct | **10 consecutive** | **0** |

So the guard's arithmetic is right every time it is evaluated in isolation, and
it is nonetheless wrong about one run in roughly thirty. **The cause is OPEN and
none is asserted.** The candidates were not separated: a transient read of the
working tree, the scope list being rebuilt by `git ls-files -co` while the tree
is being written, or something in the subshell/`$( )` plumbing further down the
script. Nothing here measured which.

### The second defect, which is deterministic and is the more harmful one

`docs/review/validate-findings.sh` F-13 and F-14 **delegate the entire decision
to that script's exit code**:

```sh
if scripts/check-retracted-claims.sh >/dev/null 2>&1; then
  ok "F-13 every 'force_lle 0x009311' mention carries a retraction marker"
else
  bad "F-13 an unmarked 'force_lle 0x009311' quote remains (run check-retracted-claims.sh)"
fi
```

Any nonzero exit is therefore reported as *"an unmarked `force_lle 0x009311`
quote remains"* — a claim about a specific deleted config directive that has
nothing to do with whatever actually failed. A reviewer reading that line is told
to go and look for a quote that does not exist. **A gate that names the wrong
cause is worse than one that names none**, which is the same principle the
`make clock` verdict text already states about itself.

### What is deliberately NOT done about it

`validate-findings.sh` is **not** modified. It is the thing that checks the
reviews, it is deliberately conservative, and changing a validator while its own
correctness is in question is how a gate gets weakened by accident. The fix is
to make F-13/F-14 name the failing section instead of a phrase — a change that
can only make the check *stricter* and more honest — but it is **OPEN** and
belongs to whoever can falsify it properly.

### Until then: how to read a red `make review-check`

A `REFUTED F-13` or `REFUTED F-14` line accompanied by
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
