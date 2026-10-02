#!/usr/bin/env bash
# scripts/deck-trace-build.sh - configure and build the TRACE tier on the Steam Deck.
#
# WHY THIS SCRIPT EXISTS
#
# On 2026-10-02 commit e34ad12 recorded, as a measured cause, that the Deck
# "cannot build the trace tier" because its hand-assembled header prefix at
# /home/deck/sysroot "is a partial reconstruction: it satisfies the C translation
# units the rest of the runner needs and not a libstdc++ one" (README, entry (t);
# docs/measurements/2026-10-02-c041-bank03-pc-dump.md).
#
# That cause is REFUTED. Measured on the Deck, gcc 15.1.1:
#
#   $ printf '#include <cstdlib>\nint main(){return 0;}\n' > /tmp/t.cc
#   $ g++ -idirafter /home/deck/sysroot/usr/include -c /tmp/t.cc -o /tmp/t.o ; echo $?
#   0
#
# stdlib.h, stddef.h, features.h and friends are all present in the prefix
# (1483 files under ~/sysroot). Without -idirafter the same TU fails on
# features.h, i.e. one step earlier in the same chain. The prefix satisfies the
# C++ tier exactly as well as the C tier.
#
# THE REAL CAUSE, also measured: the build directory was configured with
# -DCMAKE_C_FLAGS carrying the sysroot prefix and -DCMAKE_CXX_FLAGS left EMPTY.
# Every .c file compiled; the first .cc file (debug_server.c) did not.
#
#   $ grep '^CMAKE_C_FLAGS:STRING='  build/CMakeCache.txt
#   -idirafter /home/deck/sysroot/usr/include
#   $ grep '^CMAKE_CXX_FLAGS:STRING=' build/CMakeCache.txt
#
# So: BOTH -DCMAKE_C_FLAGS and -DCMAKE_CXX_FLAGS need the prefix. Passing it to
# only one is what produced "the Deck cannot link the trace tier".
#
# WHAT THIS SCRIPT DOES
#
# It is the exact configure line that was measured to work, kept in the tree so
# the next session does not re-derive it and does not re-assert the retracted
# cause. It is DECK-SPECIFIC by construction: it hardcodes /home/deck/sysroot,
# which is a property of that machine's damaged rootfs, not of this project.
# Nothing else in the repository calls it.
#
# -DSDL_SHARED=OFF -DSDL_STATIC=ON was added because the Deck has no working
# SDL3 shared library; a run of the resulting binary is the proof that matters.
#
# USAGE (on the Deck):
#   bash scripts/deck-trace-build.sh
#   env SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy SNESRECOMP_RUN_FRAMES=200 \
#     SNESRECOMP_TRACE=1 SNESRECOMP_AOTBLK=1-50 \
#     build-tr/SimCitySNESRecomp --script "$PWD/scripts/d_city.script" \
#     "/home/deck/rom/SimCity (USA).sfc" | grep -c '^\[aotblk\]'
#
# The grep count MUST be non-zero. A zero here is trap 3 in README.md: the knob
# accepts its variable and prints nothing.
set -euo pipefail

SYSROOT=${SNESRECOMP_DECK_SYSROOT:-/home/deck/sysroot/usr/include}
BUILD=${SNESRECOMP_DECK_TRACE_BUILD:-build-tr}
ROM=${1:-/home/deck/rom/SimCity (USA).sfc}

if [ ! -d "$SYSROOT" ]; then
    echo "deck-trace-build: no header prefix at $SYSROOT" >&2
    echo "This script is for the Steam Deck only. It is not a portable build recipe." >&2
    exit 2
fi

# The prefix must be on BOTH flags. One of them is the bug this script exists for.
FLAGS="-idirafter $SYSROOT -DSNESRECOMP_INTERP_PROFILE=1 -DSNESRECOMP_TRACE=1"

cmake -S . -B "$BUILD" \
    -DCMAKE_BUILD_TYPE=Release \
    -DSDL_X11_XTEST=OFF \
    -DSDL_X11=OFF \
    -DSDL_WAYLAND=OFF \
    -DSDL_UNIX_CONSOLE_BUILD=ON \
    -DOPENGL_INCLUDE_DIR="$SYSROOT" \
    -DOpenGL_GL_PREFERENCE=LEGACY \
    -DSDL_SHARED=OFF \
    -DSDL_STATIC=ON \
    -DSNESRECOMP_TRACE_BUILD=ON \
    -DCMAKE_C_FLAGS="$FLAGS" \
    -DCMAKE_CXX_FLAGS="$FLAGS"

cmake --build "$BUILD" -j"$(nproc)"

# Self-test: refuse to call a link success a trace success.
n=$(env SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy \
      SNESRECOMP_RUN_FRAMES=200 SNESRECOMP_TRACE=1 SNESRECOMP_AOTBLK=1-50 \
      timeout 300 "$BUILD/SimCitySNESRecomp" --script "$PWD/scripts/d_city.script" "$ROM" \
      2>&1 | grep -c '^\[aotblk\]' || true)
echo "deck-trace-build: [aotblk] lines in f1-f50 = $n"
if [ "$n" -eq 0 ]; then
    echo "deck-trace-build: LINKED BUT MUTE. Trap 3: the knob accepts its variable" >&2
    echo "and prints nothing. cpu_trace_block() is a no-op without SNESRECOMP_TRACE=1" >&2
    echo "being defined at COMPILE time. This build is not usable." >&2
    exit 1
fi
echo "deck-trace-build: OK, trace tier links and emits on this machine."