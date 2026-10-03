#!/usr/bin/env bash
# scripts/deck-instr-build.sh - configure and build the INTERPRETER tier on the Steam Deck.
#
# WHY THIS SCRIPT EXISTS
#
# docs/DECK_RUNBOOK.md carried this recipe as a TABLE ROW in prose:
#
#   build-instr | -DSNESRECOMP_INTERP_PROFILE=1 on BOTH -DCMAKE_C_FLAGS and
#                 -DCMAKE_CXX_FLAGS, plus -idirafter /home/deck/sysroot/usr/include
#
# **That line does not configure on the Deck.** It was tried on 2026-10-03
# (T112) and failed twice, in two unrelated places, so it reads like two
# separate problems:
#
#   1. CMake Error at build-instr/_deps/sdl3-src/cmake/macros.cmake:415:
#        SDL could not find X11 or Wayland development libraries on your system.
#      The Deck is console-only. Every working cache on it carries
#      SDL_UNIX_CONSOLE_BUILD=ON, SDL_X11=OFF, SDL_WAYLAND=OFF. The check at
#      sdl3-src/cmake/macros.cmake:413 is skipped when SDL_UNIX_CONSOLE_BUILD
#      is set.
#
#   2. CMake Error at .../FindPackageHandleStandardArgs.cmake:227:
#        Could NOT find OpenGL (missing: OPENGL_INCLUDE_DIR)
#      The Deck's rootfs is damaged (503 of 504 glibc headers absent while
#      pacman reports the package installed), so OpenGL's headers are not on
#      the default search path. They ARE present under the hand-assembled
#      prefix at /home/deck/sysroot/usr/include.
#
# THE TRAP, and the reason this is worth a script rather than a longer table
# cell: **`-idirafter /home/deck/sysroot/usr/include` on CMAKE_C_FLAGS does not
# satisfy `find_package(OpenGL)`.** That flag is a COMPILER include path; CMake
# resolves OPENGL_INCLUDE_DIR through its own search. The two must both be
# given, and they are given by two different flags. That is exactly the shape
# of CONF-9 (the trace tier broke because the prefix was on CMAKE_C_FLAGS and
# not CMAKE_CXX_FLAGS): the same header prefix, a different mechanism, and a
# build that fails with a message that does not mention it.
#
# This is the trace script's configure line minus the trace knobs. Nothing else
# in the repository calls it. It is DECK-SPECIFIC by construction: it hardcodes
# /home/deck/sysroot, which is a property of that machine's rootfs, not of this
# project.
#
# USAGE (on the Deck):
#   bash scripts/deck-instr-build.sh
#   env SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy SNESRECOMP_RUN_FRAMES=3400 \
#     SNESRECOMP_WLOG_ADDR="0010:001F:/dev/shm/w.log" \
#     SNESRECOMP_COUNT_PC=0x009311 SNESRECOMP_PHASE_MS=1 \
#     timeout 900 build-instr/SimCitySNESRecomp \
#       --script "$HOME/simcity/scripts/d_city.script" \
#       "/home/deck/rom/SimCity (USA).sfc" | grep 'pc watched'
#
# The self-test at the end runs 200 frames and refuses to call a build usable
# unless the interpreter actually emits. The trap it guards is the one in
# README.md: a knob that accepts its variable and prints nothing is a clean
# no-op, and SNESRECOMP_INTERP_PROFILE=1 that failed to reach the C compile is
# exactly that.
set -euo pipefail

SYSROOT=${SNESRECOMP_DECK_SYSROOT:-/home/deck/sysroot/usr/include}
BUILD=${SNESRECOMP_DECK_INSTR_BUILD:-build-instr}
ROM=${1:-/home/deck/rom/SimCity (USA).sfc}

if [ ! -d "$SYSROOT" ]; then
    echo "deck-instr-build: no header prefix at $SYSROOT" >&2
    echo "This script is for the Steam Deck only. It is not a portable build recipe." >&2
    exit 2
fi

# The prefix must be on BOTH flags. One of them is CONF-9.
# -DOPENGL_INCLUDE_DIR is a SEPARATE mechanism and is not optional.
FLAGS="-idirafter $SYSROOT -DSNESRECOMP_INTERP_PROFILE=1"

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
    -DCMAKE_C_FLAGS="$FLAGS" \
    -DCMAKE_CXX_FLAGS="$FLAGS"

cmake --build "$BUILD" -j"$(nproc)"

# Self-test: refuse to call a link success an instrument success.
n=$(env SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy \
      SNESRECOMP_RUN_FRAMES=200 \
      timeout 300 "$BUILD/SimCitySNESRecomp" --script "$PWD/scripts/d_city.script" "$ROM" \
      2>&1 | grep -c 'exit: RUN_FRAMES reached' || true)
echo "deck-instr-build: clean-completion lines in a 200-frame run = $n"
if [ "$n" -eq 0 ]; then
    echo "deck-instr-build: LINKED BUT DEAD. The run did not reach RUN_FRAMES;" >&2
    echo "a knob that accepts its variable and prints nothing is a clean no-op." >&2
    exit 1
fi
echo "deck-instr-build: OK, interpreter tier links and runs on this machine."