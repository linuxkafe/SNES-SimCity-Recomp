#!/usr/bin/env bash
# Measure the clock in a LIVE city, without a savestate.
#
# The obvious way to do this - save a state, load it headlessly, diff - does not
# work here, and the reason is worth stating because it cost two rounds of
# measurement: loading a state taken from a running city produces a guest that
# is completely frozen and a garbled picture. One byte of WRAM moves in 3600
# frames where the live city animates. So the frozen state is our savestate, not
# the game, and every conclusion drawn from it was about the wrong thing.
#
# This measures the live game instead: run it, play into the city, and take two
# WRAM dumps of the real thing as it plays. No state, no load, nothing to trust
# except the game itself.
#
# The picture comes out too, so the dumps are self-documenting - if the run was
# not in a city when the dumps were taken, the screenshot says so.
set -euo pipefail

cd "$(dirname "$0")/.."

EXE=build/SimCitySNESRecomp
ROM="${1:-$PWD/SimCity (USA).sfc}"
EARLY="${2:-8000}"
LATE="${3:-14000}"
# states/ is gitignored and survives `rsync --delete` of build/, which is where the
# owner can leave the dumps for handoff.
# ABSOLUTE on purpose: the host chdir()s to the exe directory, so a relative
# output path lands in build/ - successfully, and in the wrong place, with no
# warning. Verified: SNESRECOMP_SCREENSHOT=relout.ppm writes build/relout.ppm.
DUMP="$PWD/states/live"

rm -rf "$DUMP"
mkdir -p "$DUMP"

cat <<EOF
== live clock probe ==

Play into a running city NOW and stay there until the game is on screen for
another ${LATE} frames or so - about $((LATE / 60 / 60)) minutes of play.
Quit with the window when you are done; the dumps are written along the way.

  dumps at frame ${EARLY} and ${LATE}

EOF

# The guest's battery-backed SRAM lives in <exe dir>/saves/save.srm, is
# per-machine, is gitignored, and is read by the game - so it changes WRAM and
# it changes it CONTEXTUALLY. Measured on a 3000-frame menu run: 44 of 131072
# WRAM bytes move with it, and an absent, an all-0x00 and an all-0xFF image give
# three different WRAM hashes. "Same ROM, same script, same frames" is therefore
# NOT sufficient for a byte-identical WRAM image, and the framebuffer is invariant
# to it, so no picture-based gate can see it. Pin it cold, or the diff is
# measuring this instead of the clock.
if [ -f build/saves/save.srm ]; then
  echo "== pinning the guest SRAM cold (it otherwise moves 44 bytes of WRAM) =="
  : > build/saves/save.srm
fi

# Mouse on: the route into a city ends in a click no script can make.
SNESRECOMP_MOUSE=1 \
SDL_VIDEODRIVER="${SDL_VIDEODRIVER:-}" \
SDL_AUDIODRIVER="${SDL_AUDIODRIVER:-}" \
SNESRECOMP_WRAM_DUMP="$DUMP/wram" \
SNESRECOMP_WRAM_DUMP_AT="$EARLY,$LATE" \
SNESRECOMP_SCREENSHOT="$DUMP/live.ppm" \
SNESRECOMP_SCREENSHOT_FRAME="$LATE" \
"$EXE" --config build/config.ini "$ROM" 2>&1 \
  | grep -iE "wramdump|screenshot|quicksave|error" || true

A="$DUMP/wram.f${EARLY}.bin"
B="$DUMP/wram.f${LATE}.bin"

python3 - "$DUMP/live.ppm" "$DUMP/live.png" <<'PYEOF' 2>/dev/null || true
import struct, sys, zlib
d = open(sys.argv[1], 'rb').read()
parts = d.split(b'\n', 3)
w, h = map(int, parts[1].split())
px = parts[3]
raw = b''.join(b'\x00' + px[y * w * 3:(y + 1) * w * 3] for y in range(h))
def chunk(tag, data):
    return (struct.pack('>I', len(data)) + tag + data +
            struct.pack('>I', zlib.crc32(tag + data) & 0xffffffff))
open(sys.argv[2], 'wb').write(
    b'\x89PNG\r\n\x1a\n'
    + chunk(b'IHDR', struct.pack('>IIBBBBB', w, h, 8, 2, 0, 0, 0))
    + chunk(b'IDAT', zlib.compress(raw, 6))
    + chunk(b'IEND', b''))
print("  screenshot: %s" % sys.argv[2])
PYEOF

if [ ! -f "$A" ] || [ ! -f "$B" ]; then
  cat >&2 <<EOF
expected $A and $B

The dumps are taken by frame number, so quitting before frame ${LATE} means
the second one never happened. Stay in the city until the game has been on
screen long enough, or pass smaller frame numbers as arguments 2 and 3.
EOF
  exit 1
fi

echo
echo "== what moved in the live city between frame ${EARLY} and ${LATE} =="
python3 scripts/wram-diff.py "$A" "$B" --frames $((LATE - EARLY)) --city
