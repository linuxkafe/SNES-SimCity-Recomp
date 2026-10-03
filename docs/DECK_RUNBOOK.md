# Deck Runbook — SimCity SNES PC Port

Heavy tests run on the Steam Deck via SSH. This is the quick reference.

> **⚠ REWRITTEN 2026-10-03 (`f6f1d12` + the hardening pass). The previous
> version of this file taught two mistakes that this session then measured, and
> it was untracked, so no gate could see it.** Both are corrected below and both
> corrections are falsified. **The original claims were not deleted — they are
> quoted in the "What this file got wrong" section at the bottom, which is the
> practice this project follows everywhere else.**

## Access

```bash
ssh deck@steamdeck
```

Repo: `/home/deck/simcity`

## ROM

User-supplied, never committed. **Verified on the Deck** (md5
`23715fc7ef700b3999384d5be20f4db5`, exactly 524 288 bytes):

```
/home/deck/rom/SimCity (USA).sfc
```

## Build

Plain Release — `cd /home/deck/simcity && make build` → `build/SimCitySNESRecomp`.

**The instrumented and trace tiers need their own recipes, and they are
different from `make build`:**

| tier | recipe | what it is for |
|---|---|---|
| `build-instr` | `-DSNESRECOMP_INTERP_PROFILE=1` on **both** `-DCMAKE_C_FLAGS` and `-DCMAKE_CXX_FLAGS`, plus `-idirafter /home/deck/sysroot/usr/include` | the interpreted PC histogram, `INTERP_DUMP_BANK`, `INTERP_PROFILE_START/END`, `COUNT_PC` |
| `build-tr` | `bash scripts/deck-trace-build.sh` | the AOT tier. Adds `SNESRECOMP_TRACE_BUILD=ON` and `SNESRECOMP_TRACE=1`, and **ends in a guard that refuses a mute link as success** |

**The header prefix must be on `CMAKE_CXX_FLAGS` as well as `CMAKE_C_FLAGS`.**
Leaving the CXX one empty is what broke the trace tier before, and it is
**CONF-9**; `scripts/deck-trace-build.sh` exists so nobody re-derives it.

## Heavy gates

| Gate | Command |
|------|---------|
| Clock | `scripts/clock-gate.sh --rom "/home/deck/rom/SimCity (USA).sfc" --frames 6000` |
| Perf | `scripts/perf-gate.sh` |
| ROM render | `scripts/verify-rom-render.sh` |

`clock-gate.sh` takes the ROM through **`--rom`** (`clock-gate.sh:109`); there is
no `SIMCITY_ROM` variable in it — `grep -n "SIMCITY_ROM" scripts/clock-gate.sh`
returns nothing. **[MEASURED, dev host 2026-10-03]**

**`make clock` must stay red.** It exits **1**, not 2. Red is the correct state
of this project and a Deck run that reports otherwise is the finding, not the
goal.

## Running headless

```bash
env SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy \
    SNESRECOMP_RUN_FRAMES=14000 \
    <your instruments> \
    timeout 1200 build-instr/SimCitySNESRecomp \
      --script "$PWD/scripts/d_city.script" \
      "/home/deck/rom/SimCity (USA).sfc"
```

**`--script` must precede the ROM** — flags are consumed from the front
(`host_main.c:2717`), and a misplaced one is **silently ignored**: exit 0, no
warning, output identical to an unscripted run. **[MEASURED]**

## `SNESRECOMP_COUNT_PC` — and the octal trap

```
SNESRECOMP_COUNT_PC=0x038026 SNESRECOMP_PHASE_MS=1 ./build/SimCitySNESRecomp ...
```

Counts executions of one 24-bit PC. **Print `[count] pc watched: N executions
over F frames` only when `SNESRECOMP_PHASE_MS=1` is also set** — the reporting
`fprintf` sits behind `if (!HostGetenv("PHASE_MS")) return;`
(`host_main.c:2535`), so the knob alone produces a clean-looking no-op run.

### ⚠ ALWAYS write the `0x` prefix. This is measured, and it voided a result.

`interp816.c:323` parses the value with **`strtoul(e, NULL, 0)`**. Base 0
auto-detects a leading `0` as **octal** and stops at the first digit that is not
an octal digit:

| written | parsed as | watches |
|---|---|---|
| `038026` | **3** | PC `$000003`, bank `$00` |
| `009311` | **0** | PC `$000000` |
| `0x038026` | `0x038026` | the intended PC |
| `0x009311` | `0x009311` | the intended PC |

**[MEASURED, Deck-native 2026-10-03]** A whole measurement was retracted for
this: ledger **R-037**. `T100` and `C-041c` reported "0 executions" that were
counts of `$00:0003`. The generalisation is in `README.md` instrument trap 8 and
in **`docs/CONFLICTS.md` CONF-15**; it hit `SNESRECOMP_WRAM_DUMP_LO`/`_HI` too,
and `strtol(…, 0)` appears at **28 sites**.

### Every execution count needs a positive control

Before you believe a zero, count a PC that is certain to execute, in the same
build and run configuration:

```
COUNT_PC=0x009311  ->  239617 executions over 300 frames = 798.7 per frame
```

`$00:9311` is the vblank spin body and the instrument's own default. **A zero
from a counter that has never read non-zero for the address it was given is not a
measurement.**

## ⚠ FOREGROUND every heavy run. Do not background it.

**Measured, three times:** runs launched with `setsid nohup … & disown` from an
`ssh` command line **died with no error message, no core, and no exit status**,
at run-frames **2160, 1440 and 1440**. The identical command run **in the
foreground** through `ssh` completed 30 000 frames with `EXIT=0`, three times out
of three. **[MEASURED, Deck-native; `docs/CONFLICTS.md` CONF-13]**

**Why it matters more than a lost run:** a truncated trace is not a smaller
trace. Every dead run above still produced 250 000+ `[aotblk]` lines and a
plausible per-bank split, which is exactly how a partial census gets reported as
a whole one. And a run that dies **without writing any exit status at all** is
the same failure with one fewer clue — the log looks complete up to the frame it
reached.

> **The rule: foreground it, and read `EXIT=` before you read the log. Only
> `[host +T s] exit: RUN_FRAMES reached` is a clean completion. `exit: SDL_QUIT`
> on this Deck means _you_ signalled it.**

Logs go to **`/dev/shm`** (tmpfs, 7.2 GB free), not `/home`: the same binary ran
at ~4 300 lines/s to `/dev/null` against 154.2 s for 4 000 frames, and the T102
AOT run wrote **457 MB** for 10 080 frames.

## What this file got wrong

Kept, because a correction with the original attached is the practice this
project uses and the alternative is a runbook that has quietly rewritten history.

| the old text | why it was wrong | measured replacement |
|---|---|---|
| *"`038026` is parsed as **octal** — leading zero matters. Write `$03:8026` as `038026`, not `0x038026`."* | **Exactly backwards.** It named the mechanism and then drew the opposite conclusion from it. `038026` → **3**; `0x038026` → `$038026`. | `0x038026`, always. R-037, CONF-15 |
| *"Background and redirect:"* followed by a `nohup … &` recipe | **Prescribes the exact thing that destroyed three runs**, silently. | Foreground. CONF-13 |
| *"`clock-gate.sh` requires `--rom`; it does not read `SIMCITY_ROM`."* | **Correct**, and now carries the command that proves it (`grep`) rather than being a bare assertion. | verified above |

**Why no gate caught the first two.** `scripts/check-retracted-claims.sh` is
**lexical**: it fires on phrases the ledger already holds rows for, and there was
no row for "octal" or "background this way". `scripts/check-cause-claims.sh` is
**structural** — it asks whether a causal sentence carries provenance — and these
two did not trip it either, because its trigger set is narrower than
"any sentence that could mislead". **The gap is real and it is named rather than
papered over: a new failure *shape* needs a new ledger row before any lexical
guard can see it, and writing that row is a human act.**