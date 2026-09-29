#!/usr/bin/env bash
# Is SimCity's clock ticking inside a running city?
#
# The city starts, and the date stays at 1900 JAN. Those are two different
# bugs and the entry one is closed, so this measures the second with the city
# already running rather than trying to get there again.
#
# It needs a savestate taken from inside a live city, because the only route
# into one ends in a mouse click no script can reproduce yet:
#
#   1. play to a running city by hand
#   2. press F11, save to slot 1
#   3. put the file at build/saves/save1.sav
#
# Then this loads it, lets it run, and reports which WRAM bytes move slowly.
# Nobody has mapped the date, population or treasury in WRAM; guessing
# addresses in 128 KB by hand is hopeless, but a city that runs and a clock
# that does not tick have a signature - the slow state is what changes every
# few dozen frames rather than every frame.
set -euo pipefail

cd "$(dirname "$0")/.."

EXE=build/SimCitySNESRecomp
ROM="${1:-$PWD/SimCity (USA).sfc}"
SLOT="${2:-1}"
STATE="build/saves/save${SLOT}.sav"
DUMP=/tmp/clockprobe
# Two dumps far enough apart that a month tick shows up in one and animation
# shows up in both. 3600 frames is a minute of play at 60 Hz.
EARLY=200
LATE=3800
TAIL=$((LATE + 100))

if [ ! -x "$EXE" ]; then
  echo "missing $EXE - run 'make build' first" >&2
  exit 1
fi
if [ ! -f "$STATE" ]; then
  cat >&2 <<EOF
missing $STATE

The clock can only be measured with a city already running, and the route
into one ends in a mouse click that no script reproduces yet. So take the
state by hand:

  1. SNESRECOMP_MOUSE=1 $EXE "$ROM"
  2. play to a running city (START -> START NEW CITY -> B x12 ->
     move the mouse -> right-click a letter -> click ENT)
  3. press F11, save to slot $SLOT

That writes build/saves/save${SLOT}.sav. Re-run this script.
EOF
  exit 1
fi

rm -rf "$DUMP"
mkdir -p "$DUMP"

printf 'wait 30\nloadstate %s\nwait %s\n' "$SLOT" "$((TAIL - EARLY))" \
  > "$DUMP/probe.script"

# Two dumps in one run, not a per-frame framedump: the framedump writes every
# frame in its range, and a 3600-frame range is 2.4 GB of WRAM to then diff two
# files out of.
echo "== loading build/saves/save${SLOT}.sav and running to frame ${TAIL} =="
SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy \
  SNESRECOMP_RUN_FRAMES="$TAIL" \
  SNESRECOMP_WRAM_DUMP="$DUMP/wram" \
  SNESRECOMP_WRAM_DUMP_AT="$EARLY,$LATE" \
  SNESRECOMP_SCREENSHOT="$DUMP/loaded.ppm" \
  SNESRECOMP_SCREENSHOT_FRAME="$LATE" \
  "$EXE" --config build/config.ini --script "$DUMP/probe.script" "$ROM" \
  2>&1 | grep -iE "savestate|slot|wramdump|error" | head -10

A="$DUMP/wram.f${EARLY}.bin"
B="$DUMP/wram.f${LATE}.bin"
if [ ! -f "$A" ] || [ ! -f "$B" ]; then
  echo "expected $A and $B to be written" >&2
  exit 1
fi

echo
echo "== what moved in the city between frame ${EARLY} and ${LATE} =="
python3 scripts/wram-diff.py "$A" "$B" --frames $((LATE - EARLY)) --city
echo
echo "== what the loaded state actually shows =="
# The screenshot is the real check. Everything above is arithmetic on a state
# that may not contain a city, and a still screen is indistinguishable from a
# dead clock by byte counts alone - three different still screens all move 27
# bytes in 3600 frames. Convert it so there is nothing to do but open it.
python3 - "$DUMP/loaded.ppm" "$DUMP/loaded.png" <<'PYEOF' 2>/dev/null || echo "  (screenshot em $DUMP/loaded.ppm)"
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
echo
echo "A field that changes once in the window, and not every frame, is a"
echo "candidate for the month tick. Re-run with a longer window to tell a"
echo "per-second counter from a per-month one."
