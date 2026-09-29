#!/usr/bin/env bash
# Verify the ROM actually renders — the gate T057 was missing.
#
# src/gen/ is generated from the ROM and is gitignored, so a regeneration can
# change the recompiled code from 0 AOT functions to 216 without leaving a trace
# in any commit. On 2026-09-28 it did exactly that, and the screen went black
# while `ctest` stayed green: the one registered test runs 30 frames of a
# deterministic replay and cannot see a picture. A broken renderer and a healthy
# one both pass it.
#
# What this measures: the emulator's own presented-frame fingerprints. The host
# writes one crc32 per present (SNESRECOMP_PRESENT_LOG), so "the picture is
# moving" reduces to "the crc32 column has many distinct values". Measured on this
# ROM: 206 distinct crc32 over frames 200-800 when healthy, 1 when the field is
# black. The threshold below is 10, which keeps a wide margin on both sides.
#
# It cannot be a ctest: the ROM is never committed, and a test that needs it
# would break for every developer without one. Hence a make target, not add_test.
#
# Usage: scripts/verify-rom-render.sh [--build DIR] [--rom PATH] [--frames N]
# Env:   ROM=... BUILD_DIR=...

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$SCRIPT_DIR"

BUILD_DIR="${BUILD_DIR:-build}"
ROM="${ROM:-}"
FRAMES=800
# Frames below this are the boot window: forced-blank $2100=$8F until the title
# screen appears, so the picture legitimately barely moves there (T039).
SKIP=200
MIN_DISTINCT=10

while [ $# -gt 0 ]; do
  case "$1" in
    --build) BUILD_DIR=$2; shift 2 ;;
    --rom) ROM=$2; shift 2 ;;
    --frames) FRAMES=$2; shift 2 ;;
    --help|-h) sed -n '2,/^set -euo/p' "$0" | sed -n '/^# /p' | sed 's/^# //'; exit 0 ;;
    *) echo "verify-rom-render: unknown flag: $1" >&2; exit 2 ;;
  esac
done

bail() { printf "ERROR: %s\n" "$*" >&2; exit 1; }

[ -x "$BUILD_DIR/SimCitySNESRecomp" ] || bail "no binary at $BUILD_DIR/SimCitySNESRecomp — run 'make build' first"
if [ -z "$ROM" ]; then
  for cand in "SimCity (USA).sfc" simcity.sfc simcity.smc; do
    [ -f "$cand" ] && { ROM="$cand"; break; }
  done
fi
[ -n "$ROM" ] && [ -f "$ROM" ] || bail "no ROM found. Pass --rom <path> or set ROM= (you must legally own a copy of SimCity (USA).)"

# The host chdirs to the executable's own directory before it reads the ROM, so
# a relative path is resolved against the build dir and fails to open. Absolute.
ROM="$(cd "$(dirname "$ROM")" && pwd)/$(basename "$ROM")"

CSV="$(mktemp -t simcity-render-XXXXXX.csv)"
trap 'rm -f "$CSV"' EXIT

printf "== ROM render gate ==\n"
printf "  binary : %s\n" "$BUILD_DIR/SimCitySNESRecomp"
printf "  rom    : %s\n" "$ROM"
printf "  window : frames %d-%d, need >= %d distinct presented crc32\n\n" "$SKIP" "$FRAMES" "$MIN_DISTINCT"

set +e
SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy \
SNESRECOMP_RUN_FRAMES="$FRAMES" \
SNESRECOMP_PRESENT_LOG="$CSV" \
  timeout 600 "$BUILD_DIR/SimCitySNESRecomp" "$ROM" >/dev/null 2>&1
rc=$?
set -e

[ "$rc" -eq 0 ] || bail "emulator exited $rc before frame $FRAMES (a timeout here is also a failure: the game is not reaching the frame budget)"

[ -s "$CSV" ] || bail "no presents.csv written at $CSV"

total=$(awk -F, 'NR>1' "$CSV" | wc -l)
distinct=$(awk -F, -v a="$SKIP" -v b="$FRAMES" 'NR>1 && $2>=a && $2<=b {print $4}' "$CSV" | sort -u | wc -l)
luma_max=$(awk -F, -v a="$SKIP" -v b="$FRAMES" 'NR>1 && $2>=a && $2<=b {if ($5+0 > m) m = $5+0} END {printf "%.3f", m}' "$CSV")

printf "  frames presented : %d\n" "$total"
printf "  distinct crc32   : %d\n" "$distinct"
printf "  peak luma        : %s\n" "$luma_max"
printf "\n"

if [ "$distinct" -lt "$MIN_DISTINCT" ]; then
  cat >&2 <<EOF
FAIL: the emulated picture does not move.

  ${distinct} distinct presented crc32 over frames ${SKIP}-${FRAMES}, need >= ${MIN_DISTINCT}.

  A healthy run of this ROM gives ~206 over the same window. One distinct
  value means every presented frame is byte-identical, which is what a black
  field looks like. This is the failure T057 shipped for a day and a half with
  ctest green, so do not "fix" it in the renderer before checking the other
  cause it usually is:

    1. src/gen/ was regenerated with a different toolchain or flags. It is
       gitignored, so compare against a build you know worked:
         nm <builddir>/CMakeFiles/SimCitySNESRecomp.dir/src/gen/bank00*.o
       A healthy regen here has ~187 AOT functions; 0 means the regen ran
       without --cfg-roots, 216+ means it seeded roots.
    2. A regenerator AOT'd a function whose native code does not draw. T057's
       was func VBlank_Wait 0x804D reaching bank_00_8D65_M1; recomp/bank00.cfg
       pins it with force_lle 0x00804D.
EOF
  exit 1
fi

echo "PASS: the emulated picture moves."
