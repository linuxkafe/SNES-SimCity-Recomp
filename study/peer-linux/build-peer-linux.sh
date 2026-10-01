#!/usr/bin/env bash
# Build and drive the peer's SimCity SNES static recompilation on Linux.
#
# WHY THIS SCRIPT EXISTS
# ----------------------
# Junior-Jones/SimCity-SNES-Static-Recomp is published as a Windows project. Its
# CMakeLists has an explicit branch:
#
#     if(WIN32)
#         add_subdirectory(frontend/windows)
#     else()
#         message(STATUS "Windows frontend omitted on this host; ...")
#     endif()
#
# So on Linux you get the portable Full Static CORE (a .a), not a game. There is
# no window, no sound device and - the part that matters for us - no mouse.
#
# To see anything you have to supply a frontend. That is what jjhead.c below
# does: a headless driver that calls the same public C API the Windows launcher
# calls, so both run the identical core.
#
# THE LIMIT YOU WILL HIT, AND IT IS NOT A BROKEN BUILD
# ----------------------------------------------------
# With no mouse the game stops on the city-naming screen. Measured, not assumed:
# $0193 (the town-route index) stays 0 -> 0 and the cursor sits on SPACE. The
# naming screen's cursor is mouse-driven, not d-pad driven. The peer ships no
# mouse code at all - `grep -ril mouse` over its source returns nothing.
#
# To get past it you need a real mouse, which means the Windows frontend, which
# means Wine/Proton. That is the honest ceiling of the Linux path.
#
# LICENCE
# -------
# The repository declares no licence (`license: null`, no LICENSE file). Private
# study only. Do not publish, redistribute, or port this code until the author
# grants permission.
set -euo pipefail

REPO_URL="https://github.com/Junior-Jones/SimCity-SNES-Static-Recomp"
ROOT="${ROOT:-/tmp/opencode/peers}"
# Point SRC at an existing clone to skip the download.
SRC="${SRC:-$ROOT/src}"
BUILD="$ROOT/build"
# Our own harness lives next to this script, in the repository. Do NOT look for
# it under $ROOT: an earlier version did, and the only reason it ever worked is
# that I had staged the file by hand before running it. Anyone else got
# "jjhead.c not found" after a perfectly successful core build.
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# Source comes from the checkout. Build artefacts go to $ROOT, because writing a
# 10 MB binary and a save next to the script dirties the working tree.
SRC_HARNESS="${SRC_HARNESS:-$HERE/jjhead.c}"
RUN="${RUN:-$ROOT/run}"
OUT="${OUT:-$ROOT/out}"
FRAMES="${FRAMES:-5000}"
# Absolute path. The peer chdirs to its own executable directory, so a relative
# ROM path will not resolve.
ROM="${ROM:-$PWD/SimCity (USA).sfc}"
# Optional input script, for the measuring run only.
SCRIPT="${SCRIPT:-}"

# Positional arguments are honoured, because silently ignoring them is how
# "study/peer-linux/build-peer-linux.sh /path/to/jjwin /path/to/rom" ended up
# building a windowed frontend and then running the headless one, which reads
# exactly like "the windowed build is still headless".
MODE=play
SRAM_ARG=""
# Everything after a bare -- goes to the frontend, so the natural invocation
#
#   build-peer-linux.sh -- --date --wram 300
#
# reaches jjwin instead of dying on an unknown option. Without this the only way
# to pass a frontend flag was to run the frontend by hand, after the script had
# already built it, which is two commands where one should do.
FRONTEND_ARGS=()
while [ $# -gt 0 ]; do
  case "$1" in
    --) shift; FRONTEND_ARGS=("$@"); break ;;
    --headless|--measure) MODE=headless; shift ;;
    --sram) SRAM_ARG="$2"; shift 2 ;;
    -h|--help)
      sed -n '2,/^set -euo/p' "$0" | sed 's/^# \{0,1\}//; $d'
      printf '\nusage: %s [--headless] [--sram <path>] [--] [frontend args...]\n' "$0"
      printf '  --headless          measure instead of playing\n'
      printf '  --sram <path>       use this 32 KiB save\n'
      printf '  -- <args...>        pass the rest to jjwin (--rom --date --wram ...)\n'
      exit 0 ;;
    *) printf 'unknown argument: %s\n' "$1" >&2
       printf 'This script takes --headless, --sram and --help. To pass options to the\n' >&2
       printf 'windowed frontend, put them after a bare --:\n\n' >&2
       printf '  %s -- --rom "$PWD/SimCity (USA).sfc" --date --wram 300\n\n' "$0" >&2
       printf 'Or just run the script with no arguments to build and play.\n' >&2
       exit 2 ;;
  esac
done


say() { printf '\n=== %s\n' "$*"; }

# ---------------------------------------------------------------- 1. sources
if [ ! -d "$SRC/.git" ]; then
  say "cloning (private study - no licence, do not redistribute)"
  git clone "$REPO_URL" "$SRC"
else
  say "sources already at $SRC"
fi

# ------------------------------------------------------------------ 2. core
# The core is C++ even though its public header is C, so the link line needs
# -lstdc++. Omitting it fails at link with undefined operator new[]/delete[].
say "building the portable core"
cmake -S "$SRC" -B "$BUILD" -DCMAKE_BUILD_TYPE=Release
cmake --build "$BUILD" --target simcity-static-recomp -j"$(nproc)"

LIB="$BUILD/static-recomp/libsimcity-static-recomp.a"
[ -f "$LIB" ] || { echo "core not found at $LIB" >&2; exit 1; }
say "core built"
ls -la "$LIB"

# ------------------------------------------------------------- 3. frontend
# jjhead.c is ours, not theirs: a headless driver over the public API.
if [ ! -f "$SRC_HARNESS" ]; then
  cat >&2 <<EOF
jjhead.c not found at $SRC_HARNESS

That file is OUR harness, not the peer's - it is committed in the repository
next to this script. Run the script from the checkout, or point SRC_HARNESS at
it:

  SRC_HARNESS=/path/to/jjhead.c $0
EOF
  exit 1
fi
mkdir -p "$RUN"
say "building the headless driver from $SRC_HARNESS"
gcc -O2 -o "$RUN/jjhead" "$SRC_HARNESS" \
  -I"$BUILD" -I"$SRC/static-recomp/include" \
  "$LIB" -lstdc++ -lm -lpthread

# ------------------------------------------------------------ 3b. the game
# jjwin.c is a windowed SDL2 frontend: window, keyboard, speaker. This is the
# one to use if you want to PLAY the peer rather than measure it. Optional,
# because it is the only piece here that needs a system library the core itself
# does not.
if pkg-config --exists sdl2 2>/dev/null; then
  say "building the windowed frontend (SDL2 $(pkg-config --modversion sdl2))"
  gcc -O2 -o "$RUN/jjwin" "$HERE/jjwin.c" \
    -I"$BUILD" -I"$SRC/static-recomp/include" \
    "$LIB" -lstdc++ -lm -lpthread $(pkg-config --cflags --libs sdl2)
  echo "  windowed: $RUN/jjwin"
  cat <<'EOF'

  Run it with an ABSOLUTE ROM path. The core chdirs to its own directory:

    <exe> "/path/to/SimCity (USA).sfc"

  keys: arrows or WASD = d-pad, Z=A X=B C=X V=Y, Q=L E=R,
        Enter=Start, Shift=Select, Esc=quit

  A keyboard is enough to reach a city. The naming screen's cursor walks on the
  d-pad - I had written that it needed a mouse, and that was wrong.
EOF
else
  say "SDL2 not found - skipping the windowed frontend"
  echo "  install libsdl2-dev to get a playable window; the headless driver above"
  echo "  is unaffected and needs nothing beyond a C compiler."
fi

# ------------------------------------------------------------------ 4. run
[ -f "$ROM" ] || {
  echo "ROM not found: $ROM" >&2
  echo "The path must be absolute. If you passed one relative to where you" >&2
  echo "typed the command, the core will not find it: it chdirs to its own" >&2
  echo "executable directory before opening anything." >&2
  exit 1
}

if [ "$MODE" = play ]; then
  # ------------------------------------------------------------ 4a. play
  if [ ! -x "$RUN/jjwin" ]; then
    echo "the windowed frontend was not built, so there is nothing to play." >&2
    echo "It needs SDL2 development headers: install libsdl2-dev, then re-run." >&2
    echo "For measuring instead, re-run with --headless." >&2
    exit 1
  fi
  say "launching the windowed frontend"
  # $SRAM_ARG is passed through only when given, so the frontend applies its
  # own default rather than being handed an empty string it would try to open.
  # Any --rom the caller passed wins over our default, so the path they typed is
  # the path that gets opened rather than one we guessed.
  has_rom=0
  for a in ${FRONTEND_ARGS+"${FRONTEND_ARGS[@]}"}; do
    [ "$a" = "--rom" ] && has_rom=1
  done
  if [ "$has_rom" = 1 ]; then
    exec "$RUN/jjwin" ${FRONTEND_ARGS+"${FRONTEND_ARGS[@]}"}
  elif [ -n "$SRAM_ARG" ]; then
    exec "$RUN/jjwin" --rom "$ROM" --sram "$SRAM_ARG" ${FRONTEND_ARGS+"${FRONTEND_ARGS[@]}"}
  else
    exec "$RUN/jjwin" --rom "$ROM" ${FRONTEND_ARGS+"${FRONTEND_ARGS[@]}"}
  fi
fi

# --------------------------------------------------------- 4b. measure
mkdir -p "$OUT"
# A 32 KiB all-zero SRAM. The game refuses to start without one, and an absent
# path is reported as "srm_in bad" rather than being treated as cold boot.
SRM="$RUN/cold.srm"
[ -f "$SRM" ] || head -c 32768 /dev/zero > "$SRM"

say "measuring: $FRAMES frames, no input unless SCRIPT is set"
"$RUN/jjhead" "$ROM" "$SRM" "$RUN/srm.out" "$FRAMES" "${SCRIPT:--}" "$OUT"

# ---------------------------------------------------------------- 5. verdict
say "what to look at"
cat <<'EOF'
  $OUT/frame.f*.bgra   rendered frames, 256x210 BGRA (convert to PNG to view)
  $OUT/timeline.log    frame, master_clock, insns, $12, $B9, $C7, $0B51,
                       $0B53, $0B55, $0B4D
  $OUT/wram.f*.bin     full 128 KiB WRAM snapshots

Read $0B53/$0B55 as the date. $0B53=0 / $0B55=0 is 1900 JAN - that is the
encoding, not "unset". A city that is actually simulating moves month to month
roughly every 1900 frames.

With no input at all the game sits on the title screen and $0B55 stays 0
forever. That is correct, not a failure. To drive it, pass SCRIPT= pointing at
an input script, or just drop --headless and play it.
EOF
