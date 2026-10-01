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
RUN="$ROOT/run"
OUT="${OUT:-$ROOT/out}"
FRAMES="${FRAMES:-5000}"
# Absolute path. The peer chdirs to its own executable directory, so a relative
# ROM path will not resolve.
ROM="${ROM:-$PWD/SimCity (USA).sfc}"
# Optional input script. Leave empty to let it sit on the title screen.
SCRIPT="${SCRIPT:-}"

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
if [ ! -f "$RUN/jjhead.c" ]; then
  echo "jjhead.c not found in $RUN - it is our harness, not the peer's" >&2
  exit 1
fi
say "building the headless driver"
gcc -O2 -o "$RUN/jjhead" "$RUN/jjhead.c" \
  -I"$BUILD" -I"$SRC/static-recomp/include" \
  "$LIB" -lstdc++ -lm -lpthread

# ------------------------------------------------------------------ 4. run
[ -f "$ROM" ] || { echo "ROM not found: $ROM" >&2; exit 1; }
mkdir -p "$OUT"
# A 32 KiB all-zero SRAM. The game refuses to start without one, and an absent
# path is reported as "srm_in bad" rather than being treated as cold boot.
SRM="$RUN/cold.srm"
[ -f "$SRM" ] || head -c 32768 /dev/zero > "$SRM"

say "running $FRAMES frames"
"$RUN/jjhead" "$ROM" "$SRM" "$RUN/srm.out" "$FRAMES" "${SCRIPT:--}" "$OUT"

# ---------------------------------------------------------------- 5. verdict
say "what to look at"
cat <<'EOF'
  $OUT/frame.f*.bgra   rendered frames, 256x210 BGRA (convert to PNG to view)
  $OUT/timeline.log    frame, master_clock, insns, $12, $B9, $C7, $0B51,
                       $0B53, $0B55, $0B4D, $0B55
  $OUT/wram.f*.bin     full 128 KiB WRAM snapshots

Read $0B53/$0B55 as the date. $0B53=0 / $0B55=0 is 1900 JAN - that is the
encoding, not "unset". A running city moves month to month roughly every 1900
frames.

If you got this far with no input, $0B55 staying 0 forever is CORRECT. The
naming screen is mouse-gated. See the header of this script.
EOF
