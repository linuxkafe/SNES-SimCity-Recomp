# Measurement — T108: the per-present crc32 timeline, and the freeze is at f3381

**The question.** T107 found `md5` over its presents giving 18 distinct states
with the last beginning at **f1459**, and concluded the rendered date is not read
live from `$0B53`. **The conclusion survives; the reasoning offered with it does
not**, and a per-present checksum settles why — see §3.

**Deck-native** (`ssh deck@steamdeck`), `build-instr`, `scripts/d_city.script`,
**5 000 frames**, `EXIT=0` and `exit: RUN_FRAMES reached after 5000 frames`,
positive control `COUNT_PC=0x009311` → **9 855 088** = **1971.0/frame**, inside
the established 1 929.9–2 007.9 band. ROM md5 verified on the Deck.
**Host-only: none.**

**Instrument:** `SNESRECOMP_PRESENT_LOG=<csv>`, which writes
`present,frame,alpha,crc32,luma` — **one row per present, no pictures**. It is
the same code path as `SNESRECOMP_SCREENSHOT_DIR`, which `host_main.c:1601-1620`
documents as *"Per present, not per simulated frame, and that is the whole
point … a per-frame dump hides exactly the pair that differs."*

---

## 1. The timeline

**206 picture-state changes in 5 000 presents. The last is at frame 3381, and the
next 1 619 presents are bit-identical.**

| | |
|---|---|
| presents | **5 000**, frames **1…5000** — **one present per frame**, no decimation |
| distinct crc32 values, in order of first appearance | **165** (206 changes; some states recur) |
| **last change** | **frame 3381** |
| **presents after it, all identical** | **1 619** |

The gap histogram is not noise — it is structure:

| gap (frames) | occurrences | what |
|---|---|---|
| 1 | 129 | dense animation |
| **16** | **37** | the dense run's stride |
| **91 / 5** alternating | 5 + 5 | **the +96 periodic updater** |
| 96 | 2 | same thing, phase-shifted |
| 91…513 | 11 | scene transitions |

**The +96 signature is C-055's updater, seen from the picture side.** f1302, 1307,
1398, 1403, 1494, 1499, 1590, 1595, 1686, 1691 — a period of **exactly 96**,
five times, each event with a **5-frame visible tail**. C-055 measured the
**writer** at f1205, 1301, 1397, 1493, 1589, 1685 (`$03:D947 STA $0B5C,X` /
`$03:D94B STY $0B5B`, writes at **+96**, then nothing after f1685). **Two
instruments, two tiers of the same system, agreeing on the period to the frame
and on the end of it to within 2 frames.**

**Then the dense run:** f2637–f3259, gaps of 1, 2, 6, 7, 8 and 16. **It ends on
f3259** — which is exactly the frame C-052/C-053/C-058 measure as the one and
only write of the city-state block, by the new-city routine. The city's own
creation animation and the city-state initialiser **stop on the same frame.**

**Then:** a 106-frame gap, then **17 consecutive changes, f3365 → f3381**, then
**1 619 identical presents.**

## 2. This is a much better-posed C-006 than anything measured so far

**`clock-gate.sh` independently reports the date crop's last change at f3378**
(`1 distinct date images after f3600 (last change f3378 of 6000)`, DoD D2.3). That
lands **inside** the f3365–f3381 burst. **Two instruments, one full-framebuffer
crc32 per present and one date-crop hash sampled by the gate, agree on the same
boundary to within 3 frames.**

**So the freeze is a specific, frame-resolved, 17-present event — not a 3 000-frame
diff.** The city does something visible for 17 consecutive presents and then
stops, and the run is bit-identical for 1 619 presents afterwards. The next
measurement is therefore **17 pictures, not a 5 000-frame diff**: re-run
`SCREENSHOT_DIR` over f3360–f3390 and look at what the last act of the picture
is.

**Named as a lead for C-006 with no claim attached.** This says *when* the
picture stops and that the stopping is abrupt. **It does not say why**, and it is
**not** a cause.

## 3. A correction to T107's reasoning — the conclusion stands, one of its two reasons does not

T107 offered two reasons that the rendered date does not follow `$0B53`:

| T107's reason | status |
|---|---|
| the framebuffer is pixel-identical for 3 400+ frames | **DOES NOT SUPPORT THE CONCLUSION.** A display path reading `$0B53` *live* would also produce an unchanging picture, **because `$0B53` itself never changes** — C-068 measures 66 writes to `$0B51`–`$0B5F` in 14 000 frames and **none after f3259**. A frozen source and a frozen display are indistinguishable from the framebuffer alone |
| `$0B53`/`$0B54` read `$0FA0` continuously f4260→f4899 while the picture is unchanged | **SUPPORTS IT, decisively.** This is the poke, and it is a *change to the source with no change to the display* |

**So the conclusion is right on the second reason alone, and the first is void.**
The tree should not carry the first. **[Derived, Deck-native, from two
measurements already committed: T107's dump table and C-068's write census.]**
No new run was needed for this and none was taken.

## 4. Two discrepancies this run did NOT resolve, recorded rather than smoothed

- **f1459 vs f3381.** T107's md5 put the last picture change at **f1459**; this
  run puts it at **f3381**. **These are different routes** — T107 used
  `scripts/cheat_probe.script`, this run used `scripts/d_city.script` — and the
  two numbers **must not be compared.** Which route reaches the city sooner is
  **not measured**. *This session nearly made that comparison itself before
  reading which script each run used; CONF-13's lesson, learned a third time.*
- **1 877 presents over f0–f4890 does not reconcile with 5 000 presents over
  5 000 frames.** `SCREENSHOT_DIR`/`PRESENT_LOG` is **per present** and this run
  shows presents tracking frames **1:1**, so T107's run either presented fewer
  times than it simulated frames or covered a narrower range than its text says.
  **Not established, and not guessed.** OPEN.

## 5. Not claimed

- **Not a cause for C-006.** C-006 stays **OPEN**. This is a boundary and a
  structure, not a mechanism.
- **Not that f3381 is where the simulation stops.** It is where the *picture*
  stops. Bank `$03` already died at f3271 (C-039b, measured) and the city-state
  block stopped being written at f3259 (C-068, measured); whether those three
  boundaries are one boundary is **not measured**.
- **Not a re-measurement of `make clock`.** The gate was not run. It is red, and
  it must stay red — DoD Rule 0b, and a cheat or a poke must never make it pass.

## 6. ⚠️ OPEN — `check-retracted-claims.sh` was observed to flip once, and not reproduced

**Recorded because it happened, not because it is understood.**

Immediately after this file was written, one `make check-claims` printed:

```
  VIOLATION docs/RE_CITY_FREEZE.md:47  R-002 (refuted) asserted without a retraction marker
```

`RE_CITY_FREEZE.md:47` is item 2 of a four-item header block that reads *"The
city does not load" (entry (q)) is RETRACTED — see the retraction box in that
entry"*, i.e. the phrase appears **inside its own retraction marker**, and the
same line was judged clean in every other run. **A separate invocation in the
same session printed section 3's heading as `... disagrees with the ledger (42)`
where it now consistently prints `(34)`** — `(42)` is the total row count and
`(34)` the refuted count.

**Eight subsequent runs — five with the file untracked, one with it staged, and
two earlier — all printed `census: 42 rows = 34 refuted`, heading `(34)`, and
`RESULT: PASS`, with no violations.** So it does not reproduce, and **I did not
establish the cause and do not offer one.** The candidate — a partial or stale
read of the ledger or of a file in scope — is **UNVERIFIED**.

**Why it is filed rather than dismissed:** the guard that produced it is the one
whose absence let eleven retractions stand, and **its self-test covers seeded
phrases, not this**. A verdict that was observed to change once is a fact about
the guard, and the only honest state for it is OPEN. **Next session: run
`make check-claims` at least twice before believing a green, and if it ever fires
on a line that carries its own marker, keep the output.**
