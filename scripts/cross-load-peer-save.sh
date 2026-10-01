#!/usr/bin/env bash
# Cross-load test: does OUR build simulate from the PEER's city?
#
# WHY
# ---
# We have a live city in saves/save1.sav. It renders, and nothing simulates:
# 29 WRAM bytes move in 3600 frames and the date never leaves 1900 JAN. The
# chain is documented in docs/RE_CITY_FREEZE.md and it is stalled at
# $C9 & #$9000 gating INC $14 inside the game's round-robin scheduler.
#
# The peer, driven through our own windowed frontend with a keyboard only,
# reaches a city and its clock advances. That is the reference we did not have.
#
# The two projects run the SAME ROM and both keep the 65816's battery-backed
# SRAM as a 32 KiB file. So a city that simulates under the peer should simulate
# under ours too, if our problem is execution and not state. And if it does NOT,
# then our problem is deeper than the route into the city, and that is worth
# knowing before anyone writes another line of scheduler code.
#
# The verdict is one of three, and they are not equally informative:
#
#   clock ticks    our state machine is fine, our EXECUTION of it is wrong.
#                  The bug is ours and local.
#   clock frozen   not the route and not the city. Something about how we run
#                  the guest breaks the scheduler outright.
#   no city at all the saves are not interchangeable, or our boot path differs.
#                  Establish that before reading anything else into it.
#
# NOTE ON WHAT THE SAVE CAN AND CANNOT TELL US
# The date lives in WRAM ($0B53/$0B55) and WRAM is not battery-backed. So a
# loaded save will show 1900 JAN even if it came from a city that had reached
# 1901 APR. That is expected and is not evidence of a frozen clock. What is
# evidence is whether the date ADVANCES from there.
set -euo pipefail

PEER_SAVE="${1:-}"
if [ -z "$PEER_SAVE" ] || [ ! -f "$PEER_SAVE" ]; then
  cat >&2 <<'EOF'
usage: cross-load-peer-save.sh <peer-save.srm>

<peer-save.srm> is the 32 KiB SRAM from a peer session that reached a running
city. In our windowed frontend that file is jj.srm next to the executable, and
it is only written on exit if the core reported the save dirty.

  SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy ... jjwin   # or just play it
  cp /tmp/opencode/peers/run/jj.srm somewhere safe

This script will not invent one. A save from the title screen, or from a city
that never started, tells us nothing and produces a confident wrong answer.
EOF
  exit 2
fi

EXPECT=32768
ACTUAL=$(stat -c %s "$PEER_SAVE" 2>/dev/null || stat -f %z "$PEER_SAVE")
if [ "$ACTUAL" != "$EXPECT" ]; then
  echo "that file is $ACTUAL bytes; a cartridge SRAM is $EXPECT" >&2
  exit 2
fi

cd "$(dirname "$0")/.."
EXE=build/SimCitySNESRecomp
ROM="$PWD/SimCity (USA).sfc"
[ -x "$EXE" ] || { echo "missing $EXE - run 'make build' first" >&2; exit 1; }

mkdir -p builds/saves
# Keep the player's own save. Overwriting it is how a test destroys the thing
# it is testing.
if [ -f builds/saves/save.srm ]; then
  cp builds/saves/save.srm builds/saves/save.srm.keep
  echo "kept your existing save as builds/saves/save.srm.keep"
fi
cp "$PEER_SAVE" builds/saves/save.srm
echo "loaded the peer's save: $(md5sum "$PEER_SAVE" | cut -d' ' -f1)"

DUMP=/tmp/crossload
rm -rf "$DUMP"; mkdir -p "$DUMP"

# Two dumps far enough apart that a month tick has room to appear. The peer
# moved month roughly every 1900 frames, so 4000 is generous but not slow.
EARLY=600
LATE=4600

cat > "$DUMP/run.script" <<EOF
wait 120
wait $((LATE - EARLY))
EOF

echo "== running $((LATE + 120)) frames from the peer's city =="
SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy \
  SNESRECOMP_RUN_FRAMES=$((LATE + 120)) \
  SNESRECOMP_WRAM_DUMP="$DUMP/wram" \
  SNESRECOMP_WRAM_DUMP_AT="$EARLY,$LATE" \
  SNESRECOMP_SCREENSHOT="$DUMP/end.ppm" \
  SNESRECOMP_SCREENSHOT_FRAME="$LATE" \
  "$EXE" --config build/config.ini --script "$DUMP/run.script" "$ROM" \
  2>&1 | grep -iE "sram|error|wramdump" | head -5

A="$DUMP/wram.f${EARLY}.bin"; B="$DUMP/wram.f${LATE}.bin"
[ -f "$A" ] && [ -f "$B" ] || { echo "no dumps written" >&2; exit 1; }

echo
python3 - "$A" "$B" "$EARLY" "$LATE" <<'PY'
import sys
a = open(sys.argv[1], 'rb').read()
b = open(sys.argv[2], 'rb').read()
e, l = int(sys.argv[3]), int(sys.argv[4])

def w(d, off): return d[off] | (d[off+1] << 8)

print("field          frame %-6s frame %-6s moved" % (e, l))
for name, off, width in [("$0B51 tick", 0x0B51, 2), ("$0B53 year", 0x0B53, 2),
                         ("$0B55 month", 0x0B55, 1), ("$0BA5 pop", 0x0BA5, 2),
                         ("$0B9D funds", 0x0B9D, 2), ("$0014 sched", 0x14, 2),
                         ("$0012 gate", 0x12, 1), ("$00C9 joypad", 0xC9, 2)]:
    va = w(a, off) if width == 2 else a[off]
    vb = w(b, off) if width == 2 else b[off]
    print("  %-12s  $%04X       $%04X      %s" % (name, va, vb, "YES" if va != vb else "no"))

changed = sum(1 for i in range(len(a)) if a[i] != b[i])
print()
print("WRAM bytes moved in %d frames: %d of %d" % (l - e, changed, len(a)))
if changed < 100:
    print("VERDICT: this is not simulating. The peer's city did not come alive here,")
    print("so our problem is NOT the route into the city.")
else:
    print("A lot moved - that is a live city, or at least a waking one.")
PY

echo
echo "screenshot: $DUMP/end.ppm  (convert it; do not trust the byte counts alone -"
echo "three different still screens all move very few bytes)"
