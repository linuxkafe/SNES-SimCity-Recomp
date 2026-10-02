#!/usr/bin/env bash
# perf-gate.sh - fail when the frame rate regresses.
#
# WHY THIS IS NOT A CTEST
#
# Two reasons, and the second is the one that matters. It needs the ROM, which is
# never committed, exactly like scripts/verify-rom-render.sh. But it also needs
# a threshold that is a property of the machine it runs on: this gate compares
# against a number measured on some reference host, and a slower machine fails it
# for having a slower CPU rather than for having regressed. Putting that in
# ctest would mean a red build that means nothing, which is the fastest way to
# teach people to ignore a gate. So this is a make target you run deliberately,
# and PERF_MIN_FPS is how you tell it what your machine is capable of.
#
# WHY THE THRESHOLD IS 50, AND WHAT WAS ACTUALLY WRONG WITH THIS GATE
#
# Rewritten 2026-10-02 after the gate was measured straddling its own threshold
# on a single unchanged binary: 48.38 FAIL / 51.52 PASS / 46.99 FAIL against a
# threshold of 50. Three different verdicts, one binary, one ROM, one commit, one
# session. A gate that flaps on its own threshold is not a gate.
#
# The question was which of three things is wrong: the threshold, the measurement
# method, or the headless video driver. The answer, and it is not the convenient
# one:
#
#   THE MEASUREMENT METHOD IS WRONG. That is the whole defect.
#
# The old gate took the WORST of five runs and compared it to a threshold. Worst-
# of-N is the right estimator for a quantity with a real worst case and the wrong
# one for a quantity whose noise is symmetric: it converts ordinary run-to-run
# variance directly into failures. With a measured spread of ~4.5 fps on this
# host, min-of-5 sits ~2 fps below the mean by construction, which is the entire
# distance between PASS and FAIL here.
#
# What was NOT wrong, and what was therefore NOT changed:
#
#   The threshold stays at 50. Widening it until the flap stops would be the
#   convenient fix and it is not an honest one: it makes the symptom disappear
#   while leaving the estimator that produced the symptom in place. If the
#   estimator is wrong, fix the estimator. The threshold is still a legitimate
#   gross-regression floor; it is the DECISION RULE that was broken.
#
#   The headless video driver stays. SDL_VIDEODRIVER=dummy is what makes the run
#   headless at all, and the Deck's own instrumented numbers contradict the claim
#   that it distorts the result into irrelevance: on the Deck, guest 4.502
#   ms/frame against upload-present 1.007 ms/frame. The "upload-present costs
#   6.8x the guest" figure was an artifact of the dummy driver ON THE DEV HOST,
#   and that is a reason to REPORT the per-stage split, not to remove the driver.
#
# The old threshold's *justification* was also wrong, and this is worth recording
# because it is the kind of error that survives for years: 50 was derived from
# 57.36-60.06 fps measured on a windowed, vsync-capped i5-8500T. This gate
# measures a headless, unvsynced, whole-process rate. Those are different
# quantities and the threshold was carried from one to the other without
# noticing. It is retained as a floor, and it is now labelled as a floor rather
# than as a headroom figure.
#
# WHAT THIS GATE NOW DOES
#
#   1. Runs N times and takes the MEDIAN, not the worst.
#   2. Measures the spread. If (max-min)/max exceeds PERF_MAX_SPREAD (default
#      10%), the gate exits 2 with INCONCLUSIVE and says so. **It refuses to
#      report green on a measurement it does not trust**, which is the property
#      the old gate lacked and the reason it flapped.
#   3. Prints `guest` ms/frame when SNESRECOMP_HOST_PROFILE=1 is available, since
#      that - not fps - is the figure that shows headroom. This gate only ever
#      answered "does the game still hold its rate".
#
# EXIT 0 = PASS, 1 = FAIL, 2 = INCONCLUSIVE (the measurement was too noisy to
# judge). Exit 2 is deliberately distinct from 1: "the machine was busy" and "the
# build regressed" are different facts and must not share an exit code.
#
# NOTE WHAT IS NOT BEING MEASURED HERE: the game is vsync-capped at 60 fps, so
# a healthy Release build reports ~60 and has no headroom visible in this
# number at all. Faster hardware, or a lighter renderer, would not move it. The
# figure that shows headroom is `guest` ms/frame in last_run_report.json. The
# ~2.45 ms figure that used to be quoted here for the Deck was RETRACTED on
# 2026-10-02: the number behind it was never re-measured, and the register's own
# re-measurement of the same stage was 4.511 ms - 27% of the budget, not 15%.
# "The 65816 is not the bottleneck" is therefore UNPROVEN and is not asserted
# here. If you want per-stage cost, read last_run_report.json; if you want to
# know whether the emulated CPU has slack, take that measurement yourself and
# write the number down next to the machine and the date. This gate only answers
# "does the game still hold 60".
#
# THIS GATE IS MACHINE-DEPENDENT, AND MEASURED TO BE FRAME-LOCKED ON THE DECK
#
# Measured 2026-10-02 at afceeec, same binary, same ROM, same commit:
#
#   dev host seyon    FAIL, exit 2, worst run 48.38 fps (20.67 ms/frame)
#   Deck steamdeck    PASS, exit 0, 56.88 fps
#
# Two consequences, both measured:
#
#   1. A red gate here is not a recompiler regression. The same binary passes
#      on the Deck. The threshold is a property of the machine, as the Makefile
#      says. Always report which machine a number came from.
#   2. On the Deck all five runs finished in EXACTLY 10.549 s - to the
#      millisecond, five times. Pacing there is frame-locked, not CPU-bound, so
#      this gate CANNOT detect guest performance regression on the Deck at all.
#      It is a "does it still run" check. Do not use Deck fps to argue about
#      whether the emulated CPU is or is not the bottleneck.
#
# THE DECK NOW COMPILES THIS PROJECT. Corrected 2026-10-02; the previous
# version of this header said the opposite and was wrong.
#
# It does NOT follow that Deck numbers are clean. The Deck's SteamOS rootfs is
# DAMAGED in a way pacman does not report: 503 of 504 glibc headers under
# /usr/include are absent from disk while base-devel reports installed, and
# `echo '#include <stdio.h>' | gcc -E -` gives "No such file or directory". There
# is no sudo and no glibc in /var/cache/pacman/pkg, so it cannot be repaired.
# The build therefore resolves libc headers from a hand-assembled prefix at
# /home/deck/sysroot (headers from archive.archlinux.org) with **-idirafter**,
# deliberately not -isystem, which sorts before /usr/include and breaks
# libstdc++'s #include_next <stdlib.h>.
#
# ENVIRONMENT-FIDELITY CAVEAT, and it travels with every Deck number cited
# anywhere in this repository: the binary was compiled on the Deck, and it was
# compiled against a reconstructed header prefix because the rootfs is damaged.
# A performance figure measured under those conditions describes those
# conditions.
#
# WHAT THIS DOES AND DOES NOT MEASURE
#
# It measures wall-clock frames per second of the whole process, headless. That
# is a host figure, not a guest figure, and on this hardware the host's present
# path costs more than the emulated 65816 - so a regression here can be a
# renderer change with nothing wrong in the recompiler. The per-stage split is
# in exe/last_run_report.json under breadcrumbs.events as "video profile:
# stage=..."; read it before blaming the guest. In particular, keep `guest` and
# `upload-present` apart: they are different costs with different owners, and
# conflating them has already produced a wrong conclusion in this project once.
#
# It also cannot see a game that runs at 60 fps and has stopped simulating. A
# frozen city moves about four times per 1000 frames and passes this gate. That
# is what verify-rom-render.sh and the clock probe are for.
#
# USAGE
#   scripts/perf-gate.sh [--build DIR] [--rom PATH] [--frames N] [--runs N]
#
# ENV
#   PERF_MIN_FPS   median-fps floor. Default 50. If you change it, say so and
#                  say why. See the note above: it was NOT changed in the
#                  2026-10-02 rewrite, because the estimator was the defect.
#   PERF_MAX_SPREAD  percent. Default 10. Above this the gate exits 2
#                  INCONCLUSIVE instead of guessing.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$SCRIPT_DIR"

BUILD_DIR="${BUILD_DIR:-build}"
ROM="${ROM:-}"
FRAMES=600
RUNS=5
MIN_FPS="${PERF_MIN_FPS:-50}"
# Above this relative spread across runs, the gate declines to give a verdict.
# 10% is chosen from the measured behaviour, not from taste: the healthy dev
# host spreads ~7% across five 600-frame runs, and the flapping session spread
# ~10%. Raise PERF_MAX_SPREAD on a noisier machine rather than lowering MIN_FPS.
MAX_SPREAD="${PERF_MAX_SPREAD:-10}"

while [ $# -gt 0 ]; do
  case "$1" in
    --build) BUILD_DIR="$2"; shift 2 ;;
    --rom) ROM="$2"; shift 2 ;;
    --frames) FRAMES="$2"; shift 2 ;;
    --runs) RUNS="$2"; shift 2 ;;
    --help|-h) sed -n '2,/^set -euo/p' "$0" | sed -n '/^# /p' | sed 's/^# //'; exit 0 ;;
    *) echo "perf-gate: unknown flag: $1" >&2; exit 2 ;;
  esac
done

bail() { printf "ERROR: %s\n" "$*" >&2; exit 1; }

[ -x "$BUILD_DIR/SimCitySNESRecomp" ] || bail "no binary at $BUILD_DIR/SimCitySNESRecomp - run 'make build' first"
if [ -z "$ROM" ]; then
  for cand in "SimCity (USA).sfc" simcity.sfc simcity.smc; do
    [ -f "$cand" ] && { ROM="$cand"; break; }
  done
fi
[ -n "$ROM" ] && [ -f "$ROM" ] || bail "no ROM found. Pass --rom <path> or set ROM= (you must legally own a copy of SimCity (USA).)"

# The host chdir()s to the executable directory before it reads the ROM, so a
# bare relative filename resolves there and not in our shell. Absolute.
ROM="$(cd "$(dirname "$ROM")" && pwd)/$(basename "$ROM")"

TMP="$(mktemp -d -t simcity-perf-XXXXXX)"
trap 'rm -rf "$TMP"' EXIT

printf "== Performance gate ==\n"
printf "  binary : %s\n" "$BUILD_DIR/SimCitySNESRecomp"
printf "  rom    : %s\n" "$ROM"
printf "  runs   : %d x %d frames, need the MEDIAN >= %s fps\n" "$RUNS" "$FRAMES" "$MIN_FPS"
printf "  spread : if (max-min)/max > %s%%, verdict is INCONCLUSIVE (exit 2), not PASS\n" "$MAX_SPREAD"
printf "  budget : %.2f ms/frame at 60 fps\n\n" "$(awk "BEGIN{printf \"%.2f\", 1000/60}")"

worst=""
fail=0
: > "$TMP/fps"

for i in $(seq 1 "$RUNS"); do
  set +e
  out=$(SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy \
       SNESRECOMP_HOST_PROFILE=1 \
       SNESRECOMP_RUN_FRAMES="$FRAMES" \
       timeout 600 "$BUILD_DIR/SimCitySNESRecomp" "$ROM" 2>&1)
  rc=$?
  set -e

  # A timeout is a failure, not a slow pass: reaching the frame budget at all is
  # part of what is being claimed.
  if [ "$rc" -ne 0 ]; then
    bail "run $i exited $rc before frame $FRAMES - the game is not reaching the frame budget"
  fi

  totals=$(printf '%s\n' "$out" | grep -o 'presentations=[0-9]* seconds=[0-9.]*' | tail -1) \
    || bail "run $i produced no 'video totals' line; cannot measure"
  presents=$(printf '%s\n' "$totals" | sed -E 's/.*presentations=([0-9]+).*/\1/')
  seconds=$(printf '%s\n' "$totals" | sed -E 's/.*seconds=([0-9.]+).*/\1/')

  fps=$(awk "BEGIN{printf \"%.2f\", $presents / $seconds}")
  ms=$(awk "BEGIN{printf \"%.2f\", $seconds * 1000 / $presents}")
  printf "  run %-2s  %s presents in %ss  = %s fps  (%s ms/frame)\n" \
    "$i" "$presents" "$seconds" "$fps" "$ms"
  echo "$fps" >> "$TMP/fps"

  # `guest` is the emulated 65816 and is the only figure here that shows
  # headroom; fps on a vsync-capped build does not move at all. Reported, never
  # thresholded - the old gate cited guest ms in its comments and then decided
  # on fps, which is how "the 65816 is not the bottleneck" survived a
  # re-measurement that contradicted it.
  # `|| true` is load-bearing. Under `set -euo pipefail`, a grep that matches
  # nothing exits 1, the command substitution inherits that status, and the
  # assignment fails - so the gate died silently after run 1 with no verdict.
  # It did that three times before it was found, which is the honest reason the
  # rule "run the gate you just wrote, do not read it" is in this repository's
  # rubric rather than in a style guide.
  g=$(printf '%s\n' "$out" | grep -o 'stage=guest count=[0-9]* total_ms=[0-9.]* mean_ms=[0-9.]*' \
      | tail -1 | sed -E 's/.*mean_ms=([0-9.]*).*/\1/' || true)
  # `if`, not `[ ... ] &&`: under `set -e` a false test as the last statement of
  # a loop body exits the script, which is how the first run of the rewritten
  # gate died after run 1 with no output. Found by running it, not by reading it.
  if [ -n "$g" ]; then
    printf "         guest %s ms/frame (informational; budget is 16.67)\n" "$g"
  fi
done

sort -n "$TMP/fps" > "$TMP/fps.sorted"
n=$(wc -l < "$TMP/fps.sorted")
worst=$(head -1 "$TMP/fps.sorted")
best=$(tail -1 "$TMP/fps.sorted")
# Median of an even count is the mean of the two middle values.
mid_lo=$(( (n+1)/2 )); mid_hi=$(( (n+2)/2 ))
median=$(awk -v a="$(sed -n "${mid_lo}p" "$TMP/fps.sorted")" -v b="$(sed -n "${mid_hi}p" "$TMP/fps.sorted")" \
        'BEGIN{printf "%.2f", (a+b)/2}')
spread=$(awk -v lo="$worst" -v hi="$best" 'BEGIN{ if (hi>0) printf "%.1f", 100*(hi-lo)/hi; else print "0" }')

printf "\n  runs    : %s ... %s fps   median %s   spread %s%% (limit %s%%)\n" \
       "$worst" "$best" "$median" "$spread" "$MAX_SPREAD"
printf "  median   : %s fps (threshold %s)\n" "$median" "$MIN_FPS"

# The property the old gate lacked: refuse to report green on a measurement
# this noisy to trust. Compared with awk in floating point rather than as
# strings, and stated in words because nothing here infers meaning from a
# return value - `exit expr` exits 0 when expr is 0, which is how the first two
# versions of this line passed exactly the runs they should have failed.
if awk "BEGIN{exit !($spread > $MAX_SPREAD)}"; then
  printf "PERF: INCONCLUSIVE - the measurement spread is too wide to judge.\n\n"
  printf "  Five runs of one unchanged binary spanned %s%%, above the %s%% limit.\n" "$spread" "$MAX_SPREAD"
  printf "  That is the machine, not the build: a verdict from this sample would be\n"
  printf "  a guess, and a gate that guesses is the defect this rewrite removed.\n\n"
  printf "  Close other work, or raise PERF_MAX_SPREAD deliberately and say why in\n"
  printf "  the commit that does it. Do NOT lower PERF_MIN_FPS to make this go away:\n"
  printf "  the threshold is not what is broken here.\n"
  exit 2
fi

if awk "BEGIN{exit !(($median) < ($MIN_FPS))}"; then
  fail=1
fi

if [ "$fail" -ne 0 ]; then
  printf "PERF: FAIL - the MEDIAN of %d runs is below %s fps, and the spread (%s%%) was\n" "$RUNS" "$MIN_FPS" "$spread"
  printf "  within the %s%% limit, so this is not noise.\n" "$MAX_SPREAD"
  printf "  Before blaming the recompiler, read the per-stage split in\n"
  printf "  %s/last_run_report.json (breadcrumbs.events, 'video profile:').\n" "$BUILD_DIR"
  printf "  'guest' is the emulated 65816; 'upload-present' is the host present\n"
  printf "  path, which on this hardware already costs more than the guest.\n"
  exit 1
fi

printf "PERF: PASS - median %s fps over %d runs, spread %s%% (limit %s%%).\n" \
  "$median" "$RUNS" "$spread" "$MAX_SPREAD"
printf "  This is a gross-regression floor and a \"does it still run\" check. It is\n"
printf "  NOT a headroom figure: on a vsync-capped build the fps number cannot move.\n"
printf "  The figure that shows headroom is \`guest\` ms/frame, printed above.\n"
