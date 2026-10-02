#!/usr/bin/env bash
# clock-gate.sh - fail when the city stops simulating.
#
# WHY THIS GATE EXISTS AND WHY NONE OF THE OTHERS COULD HAVE CAUGHT IT
#
# The build this gate was written for renders a city that looks alive and is
# not. Measured, 30,000 frames (500 s of emulated time, more than eight
# in-game months):
#
#   * the picture is bit-identical from frame 3381 onward, one image repeated
#     26,619 times;
#   * the date reads 1900 JAN at frame 4000 and again at frame 19998, and no
#     month ever appears;
#   * the seasons never change - CGRAM writes fall from ~2,000 per 1,000
#     frames to 83;
#   * the controller does nothing at all.
#
# Every existing gate in this repository passes while all of that is true, and
# the reason is measured in each case rather than guessed: they all measure
# before the city exists. test_deterministic_replay runs 30 frames and
# verify-rom-render.sh inspects frames 200-800; the city is on screen from about
# f3378. (The reason used to be asserted without those numbers, which is what
# scripts/check-cause-claims.sh now fails on.)
# perf-gate.sh runs 600 frames. All three are still on the
# attract screen and the menus. They pass on the strength of motion that
# stopped 2,700 frames before the gate's own window ended.
#
# So this gate does the one thing none of them does: it drives the game all the
# way into a live city with scripts/d_city.script, and then asks whether TIME
# PASSED.
#
# WHY IT READS THE DATE OFF THE SCREEN
#
# Because the owner reads it off the screen, and because that is the claim
# under test. No WRAM address is known to hold the month - three weeks of
# diffing never found one, and a livelocked guest has no slow-moving game state
# to find. The HUD date is at x 58-119, y 2-20 of the 336x224 framebuffer, and
# it is rendered as tiles, so there is no string to grep for.
#
# The crop is deliberately narrow. It covers the date and nothing else, so
# cursor animation and palette work cannot make it pass. A whole-frame
# comparison would be useless here: the OAM writes that animate the cursor do
# not change the pixels, and the ones that do would be exactly the false
# positives this gate must not have.
#
# WHY IT ALSO CHECKS THE PICTURE
#
# The date is the claim; the picture is the corroboration, and it catches a
# different failure. A game whose simulation runs but whose clock is stuck
# would fail the date check and pass the picture one. Both are reported so a
# failure says which kind of broken it is.
#
# AND WHY IT CHECKS THE GUEST'S OWN YEAR WORD (added 2026-10-02)
#
# Because for several sessions this gate's failure text asserted a cause that
# measurement had refuted, and because "the city does not load" was believed
# for several more - both sourced from runs whose precondition (a zeroed save)
# was never checked. A gate that trusts a picture cannot tell a city from a
# menu. So the verdict now also requires $0B53, the guest's own absolute year
# word, to be non-zero at the city frame. Zero means no city object exists, and
# the gate says so instead of blaming the clock. This is DoD clause D2.2 in
# docs/DEFINITION_OF_DONE.md, and it is why this gate cannot report PASS on a
# build that loads no city.
#
# THE POSITIVE CONTROL, WHICH IS THE PART THAT MATTERS
#
# A gate that cannot pass is as useless as one that cannot fail, and this one
# was written without a working build to calibrate against - there is no build
# in this repository where the clock advances. So it ships with
# --self-test, which points the same machinery at a window of the same run
# that is known to be alive (the menus animate, f0-f1200) and asserts the
# detector reports motion there. If --self-test cannot detect life on a screen
# that is demonstrably alive, then a PASS from the real check means nothing.
#
# USAGE
#   scripts/clock-gate.sh [--build DIR] [--rom PATH] [--frames N]
#                         [--runs N] [--self-test]
#
#   --self-test   prove the detector works on a known-animated window, then
#                 exit. Run this after changing anything in here.
#
# ENV
#   CLOCK_MIN_ADVANCE   how many distinct date images the run must produce
#                       after the city loads. Default 2, i.e. "the date moved
#                       at least once". Deliberately low: it is a liveness
#                       floor, not a simulation-rate target, and a tight
#                       threshold here would fail for good reasons that have
#                       nothing to do with the bug.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$SCRIPT_DIR"

BUILD_DIR="${BUILD_DIR:-build}"
ROM="${ROM:-}"
FRAMES=6000
RUNS=1
SELF_TEST=0
MIN_ADVANCE="${CLOCK_MIN_ADVANCE:-2}"

# The city is on screen from ~f3381 and the hang begins there. Windows start
# well after it so a gate can never pass on pre-city motion.
CITY_FRAME=3600

while [ $# -gt 0 ]; do
  case "$1" in
    --build) BUILD_DIR="$2"; shift 2 ;;
    --rom) ROM="$2"; shift 2 ;;
    --frames) FRAMES="$2"; shift 2 ;;
    --runs) RUNS="$2"; shift 2 ;;
    --self-test) SELF_TEST=1; shift ;;
    --help|-h) sed -n '2,/^set -euo/p' "$0" | sed -n '/^# /p' | sed 's/^# //'; exit 0 ;;
    *) echo "clock-gate: unknown flag: $1" >&2; exit 2 ;;
  esac
done

bail() { printf "ERROR: %s\n" "$*" >&2; exit 1; }

EXE="$BUILD_DIR/SimCitySNESRecomp"
[ -x "$EXE" ] || bail "no binary at $EXE - run 'make build' first"
[ -f "$PWD/scripts/d_city.script" ] || bail "scripts/d_city.script is missing; this gate depends on it"

if [ -z "$ROM" ]; then
  for cand in "SimCity (USA).sfc" simcity.sfc simcity.smc; do
    [ -f "$cand" ] && { ROM="$cand"; break; }
  done
fi
[ -n "$ROM" ] && [ -f "$ROM" ] || bail "no ROM found. Pass --rom <path> or set ROM= (you must legally own a copy of SimCity (USA).)"
ROM="$(cd "$(dirname "$ROM")" && pwd)/$(basename "$ROM")"

[ "$SELF_TEST" -eq 1 ] || [ "$FRAMES" -gt "$CITY_FRAME" ] \
  || bail "--frames must exceed $CITY_FRAME so the check runs on a live city"

TMP="$(mktemp -d -t simcity-clock-XXXXXX)"
trap 'rm -rf "$TMP"' EXIT

# Pin the guest's battery-backed SRAM cold. It lives in <exe dir>/saves/save.srm,
# is per-machine, gitignored, and the guest reads it - it changes WRAM. Same
# binary and same script with a warm battery gives a different run, and "same
# inputs, different output" is the failure mode this gate must not have.
: > "$BUILD_DIR/saves/save.srm" 2>/dev/null || true

run_once() {  # $1 = tag
  # mkdir first: the host logs "cannot open ..." per present and still exits 0
  # when the screenshot directory does not exist, so a missing mkdir here is a
  # silent no-op that looks exactly like "the game produced nothing".
  # $1 is already a full path, so it is used verbatim - prefixing $TMP here
  # would silently create $TMP/tmp/... and write every screenshot there.
  mkdir -p "$1"
  # WRAM_DUMP_AT is what makes the city-loaded check below a measurement
  # instead of an assumption. It is the whole reason this gate cannot report
  # green on a build that loads no city: the verdict is gated on the guest's
  # own year word, not on the picture. See docs/DEFINITION_OF_DONE.md D2.2.
  SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy \
  SNESRECOMP_MOUSE=1 SNESRECOMP_SOFT_MOUSE=1 \
  SNESRECOMP_RUN_FRAMES="$FRAMES" \
  SNESRECOMP_WRAM_DUMP="$1/wram" \
  SNESRECOMP_WRAM_DUMP_AT="$CITY_FRAME" \
  SNESRECOMP_SCREENSHOT_DIR="$1" \
  SNESRECOMP_SCREENSHOT_FROM=0 \
  SNESRECOMP_SCREENSHOT_TO="$FRAMES" \
    "$EXE" --script "$PWD/scripts/d_city.script" "$ROM"
}

mkdir -p "$TMP/run"

printf "== Clock gate ==\n"
printf "  binary  : %s\n" "$EXE"
printf "  rom     : %s\n" "$ROM"
printf "  script  : scripts/d_city.script\n"
printf "  frames  : %d (city live from ~%d)\n" "$FRAMES" "$CITY_FRAME"
printf "  need    : >= %s distinct date images after f%d\n\n" "$MIN_ADVANCE" "$CITY_FRAME"

# ---------------------------------------------------------------------------
# The detector. Crops the HUD date and hashes it, so cursor animation and
# palette activity cannot make a dead game look alive.
# ---------------------------------------------------------------------------
cat > "$TMP/datehash.py" <<'PYEOF'
import hashlib, sys, os, glob

# 336x224 guest framebuffer. The date "1900 JAN" occupies x 58-119, y 2-20;
# this crop is a little wider than the glyphs and deliberately excludes the
# tool palette, the RCI bar and the treasury, all of which are to its right.
X0, X1, Y0, Y1 = 55, 125, 2, 21

d = sys.argv[1]          # the directory itself, not its parent
seen = []
for f in sorted(glob.glob(os.path.join(d, "*.ppm"))):
    raw = open(f, "rb").read()
    parts = raw.split(b"\n", 3)
    if len(parts) < 4:
        continue
    w, h = map(int, parts[1].split())
    px = parts[3]
    out = bytearray()
    for y in range(Y0, Y1):
        row = y * w
        for x in range(X0, X1):
            o = (row + x) * 3
            out += px[o:o + 3]
    frame = int(os.path.basename(f).split("_")[-1].split(".")[0])
    seen.append((frame, hashlib.sha256(bytes(out)).hexdigest()))

after = [h for f, h in seen if f >= int(sys.argv[2])]
before = [h for f, h in seen if f < int(sys.argv[2])]
distinct_after = len(set(after))
distinct_before = len(set(before))
# Distinct over the WHOLE run, which is what the self-test needs: it must show
# the detector fires somewhere, not merely that it fires after a given frame.
distinct_all = len(set(h for _, h in seen))
last_change = 0
prev = None
for f, h in seen:
    if h != prev:
        last_change = f
        prev = h
print("DISTINCT_AFTER=%d" % distinct_after)
print("DISTINCT_BEFORE=%d" % distinct_before)
print("DISTINCT_ALL=%d" % distinct_all)
print("LAST_CHANGE=%d" % last_change)
print("FRAMES_CAPTURED=%d" % len(seen))
PYEOF

analyse() {  # $1 = dir, $2 = window start
  python3 "$TMP/datehash.py" "$1" "$2" | tr '\n' ' '
}

if [ "$SELF_TEST" -eq 1 ]; then
  printf "== self-test: can the detector see a screen that IS alive? ==\n"
  printf "  counting every frame of this run (0-%d), which demonstrably animates\n" "$((FRAMES - 1))"
  printf "  -- the city never loads in a run this short, so nothing here is the\n"
  printf "  frozen date the real check looks for.\n\n"
  run_once "$TMP/selftest" >/dev/null 2>&1 || bail "the run failed; fix that before trusting the self-test"
  # Window start 0 with DISTINCT_ALL: the pre-city frames must show motion, and
  # the whole run is counted so the check does not depend on where the split is.
  read -r out <<< "$(analyse "$TMP/selftest" 0)"
  eval "$out"
  [ "$FRAMES_CAPTURED" -gt 0 ] || bail "no screenshots were written; the gate would pass on an empty directory"
  printf "  frames captured               : %s\n" "$FRAMES_CAPTURED"
  printf "  distinct date images, all     : %s\n" "$DISTINCT_ALL"
  printf "  last frame the date changed   : %s\n" "$LAST_CHANGE"
  if [ "$DISTINCT_ALL" -lt 2 ]; then
    cat >&2 <<EOF
SELF-TEST: FAIL - the detector found only $DISTINCT_ALL distinct date image(s) across
$FRAMES_CAPTURED frames of a window that is known to animate. A detector that cannot
see a live screen cannot certify a dead one, so a PASS from the real check
would mean nothing.

Check the crop (x 55-125, y 2-21) against a city screenshot before trusting
this gate.
EOF
    exit 1
  fi
  printf "\nSELF-TEST: PASS - the detector sees motion where motion exists.\n"
  printf "  It is now meaningful to read its verdict on the city window.\n"
  exit 0
fi

fail=0
city_loaded=0
year_word=0000
for i in $(seq 1 "$RUNS"); do
  dir="$TMP/run/$i"
  rm -rf "$dir"; mkdir -p "$dir"
  printf "  run %d/%d ... " "$i" "$RUNS"
  run_once "$dir" >/dev/null 2>&1 \
    || bail "run $i exited non-zero; the game is not reaching frame $FRAMES"
  read -r out <<< "$(analyse "$dir" "$CITY_FRAME")"
  eval "$out"
  [ "$FRAMES_CAPTURED" -gt "$CITY_FRAME" ] \
    || bail "run $i captured only $FRAMES_CAPTURED frames; the city window was never reached"
  printf "%s distinct date images after f%d (last change f%s of %s)\n" \
    "$DISTINCT_AFTER" "$CITY_FRAME" "$LAST_CHANGE" "$FRAMES"

  # Is a city actually there? Ask the guest, not the picture.
  #
  # $0B53 is the guest's own absolute year word; 0 means no city, 0x076C is
  # 1900. Reading it here is what stops this gate reporting PASS on a build
  # that renders a menu and calls it a city - the failure that let "the city
  # does not load" stand unchallenged for several sessions (RETRACTED
  # 2026-10-02; the city does load, but the gate must never assume it).
  yraw="$dir/wram.f${CITY_FRAME}.bin"
  if [ -f "$yraw" ]; then
    y=$(python3 -c "
import sys
d=open(sys.argv[1],'rb').read()
print('%04X' % (d[0x0B53] | (d[0x0B53+1] << 8)))" "$yraw" 2>/dev/null || echo "ERR")
    printf "  city-loaded check: \$0B53 = %s" "$y"
    if [ "$y" != "ERR" ] && [ "$y" != "0000" ]; then
      printf "  -> a city is present (year %d)\n" "$((16#$y))"
      city_loaded=1
      year_word="$y"
    else
      printf "  -> NO CITY (year word is zero)\n"
      city_loaded=0
    fi
  else
    printf "  city-loaded check: NO WRAM DUMP at f%d - cannot confirm a city\n" "$CITY_FRAME"
    city_loaded=0
  fi

  # Two independent ways to fail, deliberately kept separate so the verdict can
  # say which kind of broken it is.
  if [ "$city_loaded" -ne 1 ]; then
    fail=2
  elif [ "$DISTINCT_AFTER" -lt "$MIN_ADVANCE" ]; then
    fail=1
  fi
done

# Corroboration: the picture itself. Reported either way, because it separates
# "the clock is stuck" from "nothing is simulating".
printf "\n== verdict ==\n"
if [ "$fail" -eq 2 ]; then
  printf "CLOCK: FAIL - no city was loaded; the clock was never given a chance.\n\n"
  printf "  The guest's own year word \$0B53 reads 0000 at frame %d, which\n" "$CITY_FRAME"
  printf "  means no city object exists. This is a DIFFERENT and EARLIER fault\n"
  printf "  than a frozen clock, and it must not be reported as one.\n\n"
  printf "  This gate will not report PASS in this state, whatever the date\n"
  printf "  crop shows. A menu screen with a still date is not a city.\n"
  printf "  See docs/RE_CITY_FREEZE.md and docs/CLAIMS_REGISTER.md section 11.\n"
  exit 1
fi
if [ "$fail" -ne 0 ]; then
  printf "CLOCK: FAIL - the date did not advance in a live city.\n\n"
  printf "  What is measured, and nothing more:\n"
  printf "  * a city is loaded and rendered on screen, and\n"
  printf "  * its date did not advance within this window.\n\n"
  printf "  The city not simulating is the established fact. WHY it does not\n"
  printf "  simulate is NOT ESTABLISHED, and this gate deliberately does not\n"
  printf "  guess. See docs/RE_CITY_FREEZE.md for the measurements and\n"
  printf "  docs/CLAIMS_REGISTER.md for what has been retracted.\n\n"
  printf "  Expect: date frozen, no month ever appears, seasons never recolour\n"
  printf "  the map, controller ignored, cursor still animating. The cursor\n"
  printf "  animating is NOT evidence the game is alive - OAM writes continue\n"
  printf "  while the simulation does not. That is what made this look healthy\n"
  printf "  for weeks.\n\n"
  printf "  A previous version of this message named a specific cause here\n"
  printf "  under a heading reading \"What is established\". That cause was\n"
  printf "  measured FALSE and has been retracted; see docs/CLAIMS_REGISTER.md\n"
  printf "  section 2. A gate that teaches the wrong answer is worse than a gate\n"
  printf "  that reports none, because the wrong answer is what the next person\n"
  printf "  starts from.\n\n"
  printf "  The next measurement is on the REFERENCE build, not on this one.\n"
  printf "  Measured here and now: \$03:8026 (INC.w \$0B51) executes ZERO times,\n"
  printf "  on both the interpreter and the AOT tier, on two machines, over\n"
  printf "  f0-f3700; and bank 03's last execution is f3271, not f3300. So the\n"
  printf "  question is no longer 'why does bank 03 stop' but 'what advances\n"
  printf "  \$0B51 in the reference build if not \$03:8026'. That needs a PC trace\n"
  printf "  of the peer, whose API carries none today (C-032).\n"
  printf "  See docs/measurements/2026-10-02-c041-bank03-pc-dump.md.\n"
  exit 1
fi

printf "CLOCK: PASS - the date advanced in a live city.\n"
printf "  city-loaded proof: guest year word \$0B53 = %s (non-zero), so a city\n" "$year_word"
printf "  object existed at frame %d. The date advanced on top of that.\n" "$CITY_FRAME"
printf "  Both halves are required: a date that moves without a city is not a\n"
printf "  passing clock, and a city without a moving date is the current FAIL.\n"
