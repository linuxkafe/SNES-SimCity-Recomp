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

**`make clock` must stay red.** Red is the correct state of this project and a
run that reports otherwise is the finding, not the goal.

**Exit codes, measured 2026-10-03 — read the script's, not make's:**

| command | exit |
|---|---|
| `scripts/clock-gate.sh --frames 6000` | **1** — the FAIL verdict |
| **`make clock`** | **2** — GNU Make 4.3 maps any failed recipe to 2 |
| `scripts/clock-gate.sh --help` | 0 |
| `scripts/clock-gate.sh --nonsense` | 2 — usage |

`scripts/clock-gate.sh` has **no exit-2 path of its own**; the 2 you see from
`make clock` is make's.

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

**Use an ABSOLUTE path for `--script`, every time.** The host `chdir()`s to the
exe dir, so `--script "$PWD/scripts/d_city.script"` is required and a relative
`--script scripts/d_city.script` dies in four seconds with
`script: cannot open '...'` / `script: the host chdir()s to the exe dir; pass an
absolute path.` **[MEASURED again 2026-10-03, T104]** — the runbook said this
above, in a form easy to read as optional, and T104 lost the launch of all three
of its first runs to it. The recipe throughout this file is
`"$HOME/simcity/scripts/d_city.script"`.

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

## The three instruments that answer "what does this code actually do"

These three were all in the tree before T104 and all three are documented in
`README.md` **by their limitations only**. T104 needed all three and had to find
them by reading source. **CONF-19.** Read the capability, not just the trap.

### 1. `SNESRECOMP_CYC_WATCH=LO-HI` — prints the **fetched opcode byte**

```
SNESRECOMP_CYC_WATCH=03C877-03C87F
```

```
[cyc] f=3259 pc=$03C877 op=$9F cyc=... bus_xfers=... bus_master=... internal=... master_delta=...
```

Range is `sscanf("%lx-%lx")` against `pc_before`, so **plain hex, no octal trap
here** — but it is still `PC`-range, still inside `_interp_run_core`, and still
**blind to AOT** (`interp_bridge.c:2034`). It is documented in `README.md` trap 1
as a cycle-accounting tool and that is all it was ever described as.

> **The capability nobody had read: the `op=$%02X` field is the opcode byte the
> CPU actually fetched at that PC.** It settles, in one run, any question of the
> form *is this address an instruction boundary in the executed stream, or only
> in the ROM decode?* — which is **CONF-19 / ledger R-038**, and which four
> sessions of ROM-byte archaeology had answered from the ROM instead.
>
> **The rule this earns, and it is the third failure of the same rule**
> (`$03:D947`/`$03D94B`; R-034's next-PC length; now R-038):
>
> **A byte-boundary question about the ROM cannot be answered by, or exported
> into, a claim about execution. Where the two disagree, the fetched opcode byte
> settles it.**

### 2. `SNESRECOMP_WLOG_ADDR="LO:HI:PATH"` + `SNESRECOMP_WLOG_STATE=1`

```
SNESRECOMP_WLOG_ADDR="6B00:6FFF:/dev/shm/w.log" SNESRECOMP_WLOG_STATE=1
```

One line per write whose **16-bit** address is in `[LO,HI]`, with frame, value,
width, the AOT function tag, and the register file:

```
f3259   7F:6B00=00 w1 interp@$03C877 A=0000 X=0000 Y=... S=... D=0000 DB=00 M=0 Xf=0 IPC=03C877 p34=...
```

- Funnels through `cpu_write8/16` in `cpu_state.c`, so it sees **both** engines —
  which is why C-039c's tier blindness does not apply to it.
- **`IPC=` is the writing interpreter PC.** Filtering on
  `IPC=03C877` is how one loop's stores are separated from the rest of the
  traffic in a 64 KB window.
- **`Xf=0` does not bound X at `$FF`.** The `$9F` long-indexed form
  (`STA long,X`) adds the **full 16-bit X** regardless of the X flag, so
  `X=04FF` writing `$7F6FFF` is correct 65816 behaviour, not a bug. Measured
  2026-10-03; recorded so the next reader does not "fix" it.
- **`LO`/`HI` are `%x` with no base prefix in `sscanf("%x:%x:%511[^\n]")`** —
  this one really is hex-16 and never octal. It is the `strtoul(...,0)` knobs
  (`COUNT_PC`, `WRAM_DUMP_LO/HI`, `PROFILE_START/END`) that need `0x`.
- `SNESRECOMP_WLOG_ADDR_CAP` defaults to 2 000 000 lines.
- **Positive-control it before believing an empty log.** R5 in T104 watched
  `$0B40-$0BFF` and read **66** writes to `$0B51-$0B5F`, reproducing C-052's 66
  to the unit — that is what made R7's `$6B00-$6FFF` result interpretable.

### 3. `SNESRECOMP_INTERP_TRACE_FRAMES=LO-HI` — but test the filter

Prints `[itb] f=N pc=$XXXXXX` for **every interpreted PC, all banks, in
execution order**. R1 of T104 over f3000–f3272 = **2 676 196** lines / 68 MB in
119 s. Affordable; the runbook's "narrow window" advice is right.

> **Do not answer a transition question from a filtered stream.** Testing
> adjacency over `$03C87x` lines only produced four phantom `C877 -> C877`
> "re-entries" that were frame boundaries, with other banks' code running between
> iterations. Over the **full** stream the same analysis returns 5 entries and 1
> exit. **A filter that removes the thing that explains the artefact will always
> invent the artefact.**
>
> And if you build an opcode-length table to check successors against linear
> decode: **falsify the table first.** My first table labelled `$D0` (BNE rel8)
> as length 1 and flagged **460** sites, all of them my error. Only sites whose
> opcode byte came from `CYC_WATCH` are evidence.

---

## Parallel runs: how many, and the rule that comes with them

**Up to 3 Deck runs concurrently. `make perf` is never one of them — see
below.** The Deck is measurably not saturated: **~36% CPU-busy** against
~10.55 ms of work per frame with **deadline-wait 6.930 ms** of the 16.67 ms
budget spent sleeping. Parallelism buys wall-clock; it does not buy validity.

> ### ⚠️ THE RULE: **every parallel run carries its own embedded positive
> ### control.** Not one control for a batch — one per run.
>
> Under concurrency, **"prints nothing" is ambiguous.** A dead machine, a dead
> instrument and a genuine zero all look identical in the log, and running three
> of them at once triples the number of things that can be silently dead. This
> is not theoretical: it is exactly how R-037 happened, where `COUNT_PC` printed
> a confident, formatted `0` while watching PC `$000003`.
>
> **`COUNT_PC=0x009311` is the control, and it has a known-good value.** It has
> read **6 626 029** in five separate runs at 3 300 frames, byte-identical, and
> **27 019 166** at 14 000 frames. **Reproduce the known-good number or explain
> the difference.** A run whose control reads `0` produced no measurement.
>
> **The control is per-run, never per-batch, because the failure it guards
> against is per-run.**

**`make perf` is strictly solo.** It is the only wall-clock gate and the one that
has flapped: the same unchanged binary measured **FAIL at 48.38 fps / PASS at
51.52 / FAIL at 46.99** against a threshold of 50. Under concurrency it measures
the scheduler, not the emulator. **Run it with nothing else on the machine, and
record the load average before it** (T104: `load average 1.20`).

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
---

## The picture instruments — and the one-line rule that has to precede them

Two knobs write the presented framebuffer, and they are the same code path
(`host_main.c:1601-1668`):

| knob | what it writes |
|---|---|
| `SNESRECOMP_PRESENT_LOG=<csv>` | `present,frame,alpha,crc32,luma` — **one row per present, no pictures** |
| `SNESRECOMP_SCREENSHOT_DIR=<dir>` | `present_NNNNNN.ppm` **plus** `presents.csv` with the same columns |

Both are gated by `SNESRECOMP_SCREENSHOT_FROM=<a>` / `_TO=<b>`, which filter on
**`g_present_frame`** — so a range picks a scene the way a per-frame dump would.

> ### ⚠️ **Per present, not per frame, and that is the whole point.**
> `host_main.c:1601-1620` says it, and T108 is the run that made it matter: a
> whole 5 000-frame session was scanned with `PRESENT_LOG` — **no pictures at
> all** — to find the 17 presents where the picture actually moves, and only then
> were those 17 dumped. **Do not dump 5 000 PPMs to find a 17-sample event.**
>
> `crc32` is over the **presented** buffer (`w*4` bytes per row, `w =
> g_snes_width * render_scale`), and `luma` is the mean of R+G+B over the same
> pixels — so both are properties of **what the host presented**, not of the
> guest's VRAM.

### `SCREENSHOT_FROM` / `_TO` are `strtol(v, NULL, 0)` — base 0 (CONF-15's family)

`host_main.c:1627-1630` (`from = v ? strtol(v, NULL, 0) : 0;`). A plain decimal
`3360` is unambiguous and safe. **Write them without a leading zero.** `03360`
would be read as **octal** and silently give you a different range. This is the
same `strtol(…, 0)` family as `COUNT_PC` and `WRAM_DUMP_LO/HI`, and it is a
**sixth** knob to add to the per-knob table in `README.md` instrument trap 8 —
**not** to the `WRAM_DUMP_AT` row, which is base 10 (CONF-22).

### The measured recipe — the 17 presents

```bash
ssh deck@steamdeck
cd /home/deck/simcity
rm -rf /dev/shm/pics && mkdir -p /dev/shm/pics
env SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy \
    SNESRECOMP_RUN_FRAMES=3400 \
    SNESRECOMP_SCREENSHOT_DIR=/dev/shm/pics \
    SNESRECOMP_SCREENSHOT_FROM=3360 SNESRECOMP_SCREENSHOT_TO=3390 \
    SNESRECOMP_COUNT_PC=0x009311 SNESRECOMP_PHASE_MS=1 \
    timeout 900 build-instr/SimCitySNESRecomp \
      --script "$HOME/simcity/scripts/d_city.script" \
      "/home/deck/rom/SimCity (USA).sfc" 2>&1 | tail -20
echo "EXIT=$?"
```

**Every line of that is load-bearing and three of them have cost this project a
run:**

- **`--script "$HOME/simcity/scripts/d_city.script"`** — **absolute**, and
  **before** the ROM. A relative path silently no-ops and a misplaced flag is
  *silently ignored* (exit 0, output identical to an unscripted run).
- **`SNESRECOMP_COUNT_PC=0x009311` and `SNESRECOMP_PHASE_MS=1` together.** The
  `0x` prefix or the count is octal (CONF-15); `PHASE_MS` missing and the
  reporting `fprintf` never runs, so the run is a clean-looking no-op (trap 7).
  **A run whose control is missing produced no measurement.**
- **`/dev/shm`, and foreground.** `setsid nohup … &` from an `ssh` command line
  has killed three runs with no error, no core and no exit status (CONF-13). Read
  `EXIT=` **before** you read the log, and accept the run only on
  `exit: RUN_FRAMES reached`.
- **`tail -20` on the output** loses the `[count]` line if the run is long — pipe
  to a file in `/dev/shm` and grep it, do not scroll.

### Reading the pictures back

The files are **binary PPM (`P6`)**, one per present, named
`present_000000.ppm` … **numbered from 0 at the first in-range present** — so the
number is an index into the window, *not* a frame number. **The frame number is
in `presents.csv`, third column.** Pairwise-diffing them is ordinary work on the
dev host and needs no emulator:

```bash
scp deck@steamdeck:/dev/shm/pics/presents.csv /tmp/opencode/
scp 'deck@steamdeck:/dev/shm/pics/*.ppm' /tmp/opencode/pics/
```

`python3 -c "import PIL"` is **not** assumed to exist; a `P6` header is
`P6\n<w> <h>\n255\n` and the rest is raw RGB, so `numpy` or `bytes` is enough.
**Report the bounding box and the pixel count of each transition, not a
subjective description of the pictures** — this project has retracted eight
findings and every one of them was the instrument, and "it looks like the date"
is the exact shape of the claim that has to be replaced by a coordinate.
