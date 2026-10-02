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
# WHY THE THRESHOLD IS 50 AND NOT 58
#
# Measured variance, same binary, same ROM, 600 frames, 10 consecutive runs on
# an i5-8500T with the machine otherwise idle:
#
#   57.36  58.49  59.46  59.96  60.02  60.03  60.05  60.06  60.06  60.06
#
#   min 57.36   mean 59.51   max 60.06
#
# A first attempt at this measurement produced 53.41-58.39 and was WRONG: it was
# taken while another process was running a 200 us SIGPROF sampling timer over
# the same binary, with a load average of 8.9 on 8 cores. The spread was the
# measurement apparatus, not the program. It is written down here because the
# failure mode is the likely one for whoever tunes this next: a performance
# number taken on a busy machine describes the machine.
#
# The threshold is 50 rather than something near 58 because the honest reason to
# be cautious is that this figure is host-dependent by construction (see above)
# and because a gate that cries wolf is worse than the bug it was meant to
# catch - the first person it fools is whoever runs it next. 50 sits far enough
# below the observed floor that ordinary noise cannot trip it.
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
#   Also: the Deck cannot build this project. SteamOS has an immutable rootfs
#   with no glibc headers, so `make build` there fails at configure. Every Deck
#   number in this repo comes from a binary built on the dev host and copied
#   over. Say so when reporting one.
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
#   PERF_MIN_FPS   threshold in fps. Default 50. See the note above before
#                  changing it: if you change it, say so and say why.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$SCRIPT_DIR"

BUILD_DIR="${BUILD_DIR:-build}"
ROM="${ROM:-}"
FRAMES=600
RUNS=5
MIN_FPS="${PERF_MIN_FPS:-50}"

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
printf "  runs   : %d x %d frames, need every run >= %s fps\n" "$RUNS" "$FRAMES" "$MIN_FPS"
printf "  budget : %.2f ms/frame at 60 fps\n\n" "$(awk "BEGIN{printf \"%.2f\", 1000/60}")"

worst=""
fail=0
: > "$TMP/fps"

for i in $(seq 1 "$RUNS"); do
  set +e
  out=$(SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy \
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

  # Deliberately a printed verdict rather than an awk exit code. `exit expr`
  # exits 0 when expr is 0, so a false condition "succeeds" - which is how the
  # first two versions of this line passed exactly the runs they should fail.
  # A performance gate that cannot fail is the one bug it cannot be allowed to
  # have, so the comparison states its result in words and nothing infers
  # meaning from a return value.
  verdict=$(awk "BEGIN{print (($fps) < ($MIN_FPS)) ? \"BELOW\" : \"OK\"}")
  if [ "$verdict" = "BELOW" ]; then
    fail=1
    printf "         ^ below the %s fps threshold\n" "$MIN_FPS"
  fi
done

worst=$(sort -n "$TMP/fps" | head -1)
printf "\n  worst run: %s fps (threshold %s)\n" "$worst" "$MIN_FPS"

if [ "$fail" -ne 0 ]; then
  printf "PERF: FAIL - a regression this large is not scheduler noise.\n"
  printf "  Before blaming the recompiler, read the per-stage split in\n"
  printf "  %s/last_run_report.json (breadcrumbs.events, 'video profile:').\n" "$BUILD_DIR"
  printf "  'guest' is the emulated 65816; 'upload-present' is the host present\n"
  printf "  path, which on this hardware already costs more than the guest.\n"
  exit 1
fi

printf "PERF: PASS\n"
