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
# AND WHY THE DETECTOR ASKS WHETHER THE DATE MOVED RATHER THAN WHETHER THE
# PIXELS MOVED - see the block above the detector for the measurement. Short
# version: this gate used to hash raw RGB, $2100 INIDISP is a global multiply,
# the guest ramps it, and a fade alone satisfied the hash on all 16 of its
# frames. It was red only because the fade ended before the threshold.
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
#                         [--runs N] [--self-test] [--self-test-glyph]
#
#   --self-test   prove the detector works on a known-animated window, AND
#                 prove it is invariant to a global brightness change, then
#                 exit. Run this after changing anything in here.
#   --self-test-glyph
#                 the other half: prove a real glyph change still registers.
#                 Needs --frames greater than the city frame.
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
GLYPH_SELF_TEST=0
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
    --self-test-glyph) GLYPH_SELF_TEST=1; shift ;;
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

[ "$SELF_TEST" -eq 1 ] || [ "$GLYPH_SELF_TEST" -eq 1 ] || [ "$FRAMES" -gt "$CITY_FRAME" ] \
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
printf "  frame numbers below are TRUE FRAMES, joined from presents.csv. The\n"
printf "  capture filenames are present INDICES and are one less (CONF-24, closed).\n"
printf "  need    : >= %s distinct date images after f%d\n\n" "$MIN_ADVANCE" "$CITY_FRAME"

# ---------------------------------------------------------------------------
# The detector. Crops the HUD date and asks whether the DATE moved, which is not
# the same question as whether the PIXELS moved.
#
# WHY NOT A HASH OF THE CROP, which is what this used to be
# ----------------------------------------------------------
# It hashed raw RGB. $2100 (INIDISP) is a global multiply on every pixel, and
# the guest ramps it: MEASURED, f3363-f3378, $0300 -> $030F, one step per frame,
# never written again. So a brightness fade alone changes that hash on every
# frame of the ramp, and the gate was one moved fade away from calling a dead
# city alive. It was red only because the fade happened to END before the
# gate's own threshold. Measured over the 3600-present Deck dump: the OLD
# detector reported 16 distinct "date images" across f3360-f3390, of which 29
# of the 31 frames are BYTE-EXACT brightness levels of ONE constant crop.
#
# WHY NOT A PALETTE-INDEX OR LUMA-NORMALISED HASH, which are the obvious fixes
# --------------------------------------------------------------------------
# Both were measured and both fail, for one reason: at low brightness the
# renderer's integer divide is MANY-TO-ONE. At INIDISP=1 the bright tan
# #b59473 and #bdad7b both come out as the same grey #0e0e0e, and four dark
# entries all come out (0,0,0). The palette index is destroyed before any
# post-hoc normalisation can see it, so no per-frame canonical form can recover
# it - and a luma normalisation cannot undo a collapse. See
# docs/measurements/2026-10-03-t110-clock-gate-false-green.md section 1a.
#
# WHAT THIS ASKS INSTEAD
# ----------------------
# A RELATION BETWEEN TWO FRAMES, which needs no palette, no threshold, no
# brightness value and no tolerance:
#
#   F shows the same date image as R  iff  F is a pixel-consistent
#   NON-DECREASING recolouring of R
#
# i.e. every colour class of R maps to exactly one colour in F, and those
# colours are non-decreasing in R's colour order.
#
#   * INIDISP is a per-channel multiply, hence non-decreasing, hence it CANNOT
#     open a new state. That is the false-green vector, closed by construction
#     rather than by tuning.
#   * A glyph change splits or inverts a colour class, so it ALWAYS opens one.
#   * A uniform (blank) frame is the constant map, which is non-decreasing and
#     pixel-consistent, so it is consistent with every state and can never open
#     one. No special case is needed for it.
#
# It is EXACT, not tolerant: there is no epsilon to tune and no mask whose
# threshold has to be argued about.
# ---------------------------------------------------------------------------
cat > "$TMP/datehash.py" <<'PYEOF'
import hashlib, sys, os, glob

X0, X1, Y0, Y1 = 55, 125, 2, 21

def read_crop(path):
    raw = open(path, "rb").read()
    parts = raw.split(b"\n", 3)
    if len(parts) < 4:
        return None
    w, h = map(int, parts[1].split())
    px = parts[3]
    rows = []
    for y in range(Y0, Y1):
        base = y * w
        row = []
        for x in range(X0, X1):
            o = (base + x) * 3
            row.append((px[o], px[o + 1], px[o + 2]))
        rows.append(row)
    return rows

def flat(g):
    return [p for row in g for p in row]

def raw_hash(g):
    """The detector this replaced: sha256 over RAW RGB. Kept and REPORTED on
    every run, because it is the red side of the falsification below. A raw
    hash is a brightness meter and must never be the thing that decides."""
    return hashlib.sha256(bytes(v for p in flat(g) for v in p)).hexdigest()

def same_date(F, R):
    m = {}
    for pr, pf in zip(flat(R), flat(F)):
        if pr in m:
            if m[pr] != pf:
                return False
        else:
            m[pr] = pf
    v = [m[k] for k in sorted(m)]
    for i in range(len(v) - 1):
        if v[i] > v[i + 1]:
            return False
    return True

def inidisp(g, b):
    """The transform the PPU actually applies, forward.
    snesrecomp/runner/src/snes/ppu_legacy.c:134
        out = ((c << 3) | (c >> 2)) * PPU_brightness / 15"""
    out = []
    for row in g:
        o = []
        for p in row:
            o.append(tuple((((c >> 3) << 3 | (c >> 3) >> 2) * b // 15) for c in p))
        out.append(o)
    return out

def load(d):
    """Read every captured crop, labelled with its TRUE FRAME.

    CONF-24, closed. SNESRECOMP_SCREENSHOT_FROM=0 makes the host name files
    present_NNNNNN.ppm where NNNNNN is a counter over CAPTURED PRESENTS, not a
    frame. presents.csv, written into the same directory by the same run, carries
    the true frame in its second column. This used to parse the filename and
    call the result a frame, which made every number the gate printed one too
    small - LAST_CHANGE was frame-1, always - while the csv that would have said
    so sat unread in the same directory.

    The join is on the present index and it is CHECKED, in both directions:
    a capture with no csv row, a non-increasing frame, or no csv at all is a
    hard error. A silent fallback to the old derivation is the one thing that
    would make this look fixed while still being wrong, so there isn't one.
    """
    frame_of = {}
    csv_path = os.path.join(d, "presents.csv")
    if not os.path.exists(csv_path):
        sys.stderr.write(
            "datehash: no presents.csv in %s\n"
            "          Refusing to take the frame number from the filename: that is\n"
            "          a PRESENT INDEX and it is one less than the frame (CONF-24).\n" % d)
        sys.exit(3)
    with open(csv_path) as fh:
        fh.readline()
        for line in fh:
            c = line.strip().split(",")
            if len(c) >= 2 and c[0].strip().isdigit() and c[1].strip().isdigit():
                frame_of[int(c[0])] = int(c[1])
    seen = []
    last = 0
    for f in sorted(glob.glob(os.path.join(d, "*.ppm"))):
        g = read_crop(f)
        if g is None:
            continue
        idx = int(os.path.basename(f).split("_")[-1].split(".")[0])
        if idx not in frame_of:
            sys.stderr.write("datehash: present %d has no row in presents.csv\n" % idx)
            sys.exit(3)
        frame = frame_of[idx]
        if frame <= last:
            sys.stderr.write("datehash: frames not strictly increasing at %d\n" % frame)
            sys.exit(3)
        last = frame
        seen.append((frame, g))
    if not seen:
        sys.stderr.write("datehash: no captures in %s\n" % d)
        sys.exit(3)
    return seen

def count_states(seen):
    """Greedy sequential classifier, no chaining: a frame joins the current
    state unless it is a monotone recolouring of that state's representative,
    in which case it becomes the representative if it is the more informative.
    Chaining is deliberately not used - it could merge two genuinely different
    date images through a chain of photometric intermediates."""
    n = 0
    rep = None
    for _, g in seen:
        if rep is None or not same_date(g, rep):
            n += 1
            rep = g
    return n

def count_raw(seen):
    return len(set(raw_hash(g) for _, g in seen))

def analyse(d, win):
    seen = load(d)
    last = 0
    rep = None
    for frame, g in seen:
        if rep is None or not same_date(g, rep):
            last = frame
            rep = g
    print("DISTINCT_AFTER=%d" % count_states([s for s in seen if s[0] >= int(win)]))
    print("DISTINCT_BEFORE=%d" % count_states([s for s in seen if s[0] < int(win)]))
    print("DISTINCT_ALL=%d" % count_states(seen))
    print("RAW_DISTINCT_ALL=%d" % count_raw(seen))
    print("LAST_CHANGE=%d" % last)
    print("FRAMES_CAPTURED=%d" % len(seen))

def palette_depth(g):
    """Distinct 5-bit palette entries behind the crop.

    This, not the count of distinct RGB values, is what decides whether a crop
    can demonstrate anything. Measured failure of the obvious criterion: on the
    1200-frame menu window the busiest crop has THREE distinct RGB values and
    every one of them is below 8 in every channel, so all three reconstruct to
    palette entry (0,0,0) and the whole INIDISP ramp hashes identically. A crop
    is only useful here if the renderer left a non-zero palette index behind."""
    return len({(r >> 3, gg >> 3, b >> 3) for (r, gg, b) in flat(g)})

def pick_informative(d):
    """The captured crop carrying the most recoverable palette content."""
    best = None
    for frame, g in load(d):
        n = palette_depth(g)
        if best is None or n > best[0]:
            best = (n, frame, g)
    return best

def invariance(d):
    """FALSIFIER 1, negative control. Take a REAL crop from this run, push it
    through the PPU's own brightness transform at 13 levels, and require that
    the detector reports ONE date image. The raw hash must report more than
    one: if it does not, this check cannot fire and says so."""
    n, frame, g = pick_informative(d)
    print("INV_SRC_FRAME=%d" % frame)
    print("INV_SRC_PALETTE=%d" % n)
    if n < 2:
        print("INV_SKIPPED=no captured crop has a recoverable palette entry "
              "above (0,0,0), so there is nothing here to dim. This needs a "
              "screen with a picture on it: run make clock-glyph-self-test.")
        return
    ramp = [(frame, inidisp(g, b)) for b in (15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3)]
    print("INV_STATES=%d" % count_states(ramp))
    print("INV_RAW=%d" % count_raw(ramp))

def compose_advance(g):
    """FALSIFIER 2, positive control. A REAL date change, composed from the
    crop's OWN glyph cells: "1900 JAN" -> "1901 JAN", by copying the 8-px
    character cell that already holds the leading "1" over the cell that holds
    the last "0". Nothing is drawn by hand - it is the game's own bitmap moved
    to where the "1" goes. Geometry measured from the bright-ink column
    profile of the real city frame: text row y12-y18, 8-px cells at
    x = 58 + 8i. It is COMPOSED, not a screenshot of a real roll, and it is
    labelled that way wherever it is reported: no build in this project, ours
    or the reference's, renders a date change on screen."""
    CELL_X0, CELL_W, ROW_Y0, ROW_H, PITCH, DST = 58, 8, 11, 9, 8, 3
    out = [row[:] for row in g]
    for y in range(ROW_Y0 - Y0, ROW_Y0 - Y0 + ROW_H):
        for x in range(CELL_W):
            out[y][(CELL_X0 + PITCH * DST + x) - X0] = g[y][(CELL_X0 + x) - X0]
    return out

def glyph(d):
    n, frame, g = pick_informative(d)
    print("GLYPH_SRC_FRAME=%d" % frame)
    print("GLYPH_SRC_PALETTE=%d" % n)
    if n < 2:
        print("GLYPH_SKIPPED=no captured crop has a recoverable palette entry "
              "above (0,0,0); there is no glyph on screen to move")
        return
    adv = compose_advance(g)
    changed = sum(1 for a, b in zip(flat(g), flat(adv)) if a != b)
    print("GLYPH_PIXELS=%d" % changed)
    if changed == 0:
        print("GLYPH_SKIPPED=the composed cell is identical to the one it "
              "replaced, so there is no date change to detect")
        return
    # brightness held CONSTANT (both frames are the same captured frame), and
    # then deliberately NOT held constant, because a detector that only works
    # at one brightness is not a detector of the date.
    print("GLYPH_STATES_SAME_B=%d" % count_states([(frame, g), (frame, adv)]))
    print("GLYPH_RAW_SAME_B=%d" % count_raw([(frame, g), (frame, adv)]))
    for b in (11, 7, 3):
        pair = [(frame, g), (frame, inidisp(adv, b))]
        print("GLYPH_STATES_B%d=%d" % (b, count_states(pair)))
        print("GLYPH_RAW_B%d=%d" % (b, count_raw(pair)))

mode = sys.argv[1]
if mode == "analyse":
    analyse(sys.argv[2], sys.argv[3])
elif mode == "invariance":
    invariance(sys.argv[2])
elif mode == "glyph":
    glyph(sys.argv[2])
else:
    sys.stderr.write("datehash: unknown mode %s\n" % mode)
    sys.exit(2)
PYEOF

analyse() {  # $1 = dir, $2 = window start
  python3 "$TMP/datehash.py" analyse "$1" "$2" | tr '\n' ' '
}

if [ "$SELF_TEST" -eq 1 ]; then
  printf "== self-test 1/2: can the detector see a screen that IS alive? ==\n"
  printf "  counting every frame of this run (0-%d)\n\n" "$((FRAMES - 1))"
  run_once "$TMP/selftest" >/dev/null 2>&1 || bail "the run failed; fix that before trusting the self-test"
  read -r out <<< "$(analyse "$TMP/selftest" 0)"
  eval "$out"
  [ "$FRAMES_CAPTURED" -gt 0 ] || bail "no screenshots were written; the gate would pass on an empty directory"
  printf "  frames captured                        : %s\n" "$FRAMES_CAPTURED"
  printf "  distinct DATE images, whole run        : %s\n" "$DISTINCT_ALL"
  printf "  (raw-RGB hashes, the old detector)     : %s\n" "$RAW_DISTINCT_ALL"
  printf "  last frame the date changed           : %s\n" "$LAST_CHANGE"
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

  # ---------------------------------------------------------------------
  # The falsifiers live in --self-test-glyph, and the reason is measured, not
  # stylistic: both of them need a crop with recoverable palette content, and
  # on the 1200-frame menu window there is none. Every captured crop there is
  # either blank or carries dither noise below 8 in every channel, so all of it
  # reconstructs to palette entry (0,0,0) and the whole INIDISP ramp hashes
  # identically. A check that cannot fire must not live where it always fails;
  # `make clock-glyph-self-test` runs a route long enough to reach a city and
  # runs all three checks there.
  # ---------------------------------------------------------------------
  printf "\nSELF-TEST: PASS - the detector sees CONTENT where content changes.\n"
  printf "  %s raw-RGB hashes became %s date images on this window, and every\n" \
    "$RAW_DISTINCT_ALL" "$DISTINCT_ALL"
  printf "  one of the states it dropped was a brightness fade. Before this\n"
  printf "  change those fades WERE the self-test's evidence: they are the whole\n"
  printf "  reason the raw count is %s and the date count is %s.\n" \
    "$RAW_DISTINCT_ALL" "$DISTINCT_ALL"
  printf "\n  This is only half a result - a detector that ignored everything\n"
  printf "  would also pass it. The other half is that a real glyph change\n"
  printf "  still registers, and that a brightness fade does not. Both need a\n"
  printf "  city on screen:\n\n"
  printf "      make clock-glyph-self-test\n"
  exit 0
fi

# ---------------------------------------------------------------------------
# AND WHY IT ALSO RUNS ITS OWN POSITIVE CONTROL (--self-test-glyph)
#
# Falsifier 1 proves the detector ignores brightness. On its own that is half a
# result: a detector that ignored everything would pass it. Falsifier 2 is the
# other half - a REAL glyph change must still register - and it needs a screen
# with a glyph on it, which means a run long enough to reach a city.
#
# The control is COMPOSED and labelled as such. No build in this project, ours
# or the reference's, renders a date change on screen: our city is frozen from
# f3381, and the reference build's headless framebuffer does not render the game
# at all (R-044, and the measurement behind it). So the control is built by
# moving the game's OWN 8x9 bitmap for a "1" from where the leading "1" is to
# where the last digit goes, turning "1900 JAN" into "1901 JAN". It is a real
# glyph-cell change of exactly the class the detector has to catch, and it is
# not a screenshot of a real roll.
#
# It is checked at one brightness and then at three others, because a detector
# that only sees the date at full brightness is not a detector of the date.
# ---------------------------------------------------------------------------
if [ "$GLYPH_SELF_TEST" -eq 1 ]; then
  [ "$FRAMES" -gt "$CITY_FRAME" ] \
    || bail "--self-test-glyph needs --frames greater than $CITY_FRAME so a city is on screen"
  printf "== glyph positive control: does a real date change still register? ==\n"
  printf "  running %d frames so the city is on screen\n\n" "$FRAMES"
  run_once "$TMP/glyph" >/dev/null 2>&1 || bail "the run failed; fix that before trusting this check"

  # ---------------------------------------------------------------------
  # FALSIFIER 1 (negative control), first, because it is the one that closes
  # the false-green vector this gate had. A global brightness change must not
  # read as a date change; the raw-RGB hash this gate used to be must.
  # ---------------------------------------------------------------------
  printf "== FALSIFIER 1 (negative control): a brightness fade is not a date ==\n"
  printf "  the PPU's own INIDISP transform at 13 levels, applied to a real crop\n"
  printf "  from this run.\n\n"
  read -r inv <<< "$(python3 "$TMP/datehash.py" invariance "$TMP/glyph" | tr '\n' ' ')"
  eval "$inv"
  if [ -n "${INV_SKIPPED:-}" ]; then
    printf "SKIPPED: %s\n" "$INV_SKIPPED" >&2
    printf "This is a gap in the check, not a pass.\n" >&2
    exit 1
  fi
  printf "  source crop              : present %s, %s palette entries\n" \
    "$INV_SRC_FRAME" "$INV_SRC_PALETTE"
  printf "  distinct DATE images     : %s   <- must be 1\n" "$INV_STATES"
  printf "  raw-RGB hashes (old gate): %s   <- the red side; must be > 1\n" "$INV_RAW"
  if [ "$INV_STATES" -ne 1 ]; then
    cat >&2 <<EOF
GLYPH CONTROL: FAIL - the detector counted $INV_STATES date images across 13 brightness
levels of ONE unchanged crop. It is not invariant to INIDISP, so a fade in the
guest can satisfy the real check and this gate can go green on a dead city.

Do not raise CLOCK_MIN_ADVANCE to compensate. Fix the detector.
EOF
    exit 1
  fi
  if [ "$INV_RAW" -lt 2 ]; then
    cat >&2 <<EOF
GLYPH CONTROL: FAIL - the raw-RGB hash reported $INV_RAW image(s) across the same 13
brightness levels, so this falsifier can no longer demonstrate the false green it
exists to demonstrate. A check that cannot fail is not a check.
EOF
    exit 1
  fi

  printf "\n== FALSIFIER 2 (positive control): a real date change registers ==\n"
  read -r gl <<< "$(python3 "$TMP/datehash.py" glyph "$TMP/glyph" | tr '\n' ' ')"
  eval "$gl"
  if [ -n "${GLYPH_SKIPPED:-}" ]; then
    printf "SKIPPED: %s\n" "$GLYPH_SKIPPED" >&2
    printf "This is a gap in the check, not a pass.\n" >&2
    exit 1
  fi
  printf "  source crop                : present %s, %s palette entries\n" \
    "$GLYPH_SRC_FRAME" "$GLYPH_SRC_PALETTE"
  printf "  composed 1900 JAN -> 1901 JAN: %s pixels changed, one character cell\n" \
    "$GLYPH_PIXELS"
  printf "\n  %-42s %-8s %s\n" "case" "raw(old)" "date(new)"
  printf "  %-42s %-8s %s\n" "brightness HELD CONSTANT" \
    "$GLYPH_RAW_SAME_B" "$GLYPH_STATES_SAME_B"
  for b in 11 7 3; do
    eval "raw=\${GLYPH_RAW_B$b}"; eval "st=\${GLYPH_STATES_B$b}"
    printf "  %-42s %-8s %s\n" "date advanced, brightness $b vs 15" "$raw" "$st"
  done
  rc=0
  [ "$GLYPH_STATES_SAME_B" -ge 2 ] || {
    printf "\nGLYPH CONTROL: FAIL - %s pixels of a real glyph-cell change were read as\n" "$GLYPH_PIXELS"
    printf "the SAME date. The detector is invariant to brightness AND to the date,\n"
    printf "which means it cannot fail and therefore cannot certify anything.\n" >&2
    rc=1; }
  for b in 11 7 3; do
    eval "st=\${GLYPH_STATES_B$b}"
    [ "$st" -ge 2 ] || {
      printf "\nGLYPH CONTROL: FAIL - at brightness %s the date change was invisible.\n" "$b" >&2
      printf "A detector that only sees the date at one brightness is not a\n" >&2
      printf "detector of the date.\n" >&2
      rc=1; }
  done
  [ "$GLYPH_RAW_SAME_B" -ge 2 ] || {
    printf "\nGLYPH CONTROL: FAIL - the raw-RGB hash also failed to see the change, so\n" >&2
    printf "this control is no longer demonstrating anything.\n" >&2
    rc=1; }
  [ "$rc" -eq 0 ] || exit 1
  printf "\nGLYPH CONTROL: PASS - a real glyph change registers at every brightness\n"
  printf "  tested, and a pure brightness change registers as nothing.\n"
  printf "  Together with make clock-self-test that is the whole argument: the\n"
  printf "  detector sees content, and does not see illumination.\n"
  printf "  This control is composed from the game's own glyph cells and is NOT a\n"
  printf "  screenshot of a real month roll. See\n"
  printf "  docs/measurements/2026-10-03-t110-clock-gate-false-green.md section 2.\n"
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
  printf "  The criterion is CALIBRATED, not assumed. The configured floor is\n"
  printf "  %s distinct date images (default 2, overridable with\n" "$MIN_ADVANCE"
  printf "  CLOCK_MIN_ADVANCE); the reference build was MEASURED at 29 of them in\n"
  printf "  30000 frames, Deck-native, against a cold SRAM and a real\n"
  printf "  save (city at f3000, month rolls every ~780 frames, year turns at\n"
  printf "  f13080 and f24600, 1902 MAY at f30000). We produce 1. The floor sits\n"
  printf "  far below the reference's measured behaviour, so this gate is not\n"
  printf "  asking for something the original game does not do.\n"
  printf "  See docs/measurements/2026-10-02-t101-reference-simulates.md.\n\n"
  printf "  MEASURED, and measured NOT:\n"
  printf "  * \$03:8026 (INC.w \$0B51) executes ZERO times here, on both the\n"
  printf "    interpreter and the AOT tier, on two machines, over f0-f14000 -\n"
  printf "    14 000 frames, not the f0-f3700 this text used to quote (T102,\n"
  printf "    C-041). Bank \$03's last execution is f3271, not f3300.\n"
  printf "  * In the reference build that same instruction is what moves the\n"
  printf "    clock: \$0B51 = 4 x (months elapsed) + quarter, 113 individual\n"
  printf "    write events, 0 violations, month rolls at exactly the events\n"
  printf "    where it reaches a multiple of 4.\n"
  printf "  * NOT ESTABLISHED: why \$0B51 is not incremented here. The\n"
  printf "    mechanical thread is that \$00:804D reads \$0012 and is never\n"
  printf "    executed again after f3271, and \$03:D2AA sets \$0012 = 1 one\n"
  printf "    instruction before bank \$03's final RTL. What is supposed to\n"
  printf "    execute \$00:804D, and why it stops, is NOT ESTABLISHED and this\n"
  printf "    gate does not guess.\n"
  printf "  * ALSO NOT ESTABLISHED: whether the rendered date reads \$0B51 at\n"
  printf "    all. Nothing redraws the date glyphs after the city is built -\n"
  printf "    VRAM is filled by HDMA on channels 0 and 1 and the CPU never\n"
  printf "    writes \$2119 in 3 400 frames - so there is no live path from\n"
  printf "    \$0B51 to the screen for a poke to travel. Until that is measured,\n"
  printf "    \"the counter does not tick\" and \"the date would not move if it\n"
  printf "    did\" are both open, and fixing the counter alone may not be\n"
  printf "    enough.\n"
  printf "  See docs/measurements/2026-10-03-t102-tick-across-f13080.md and\n"
  printf "  docs/measurements/2026-10-03-t109-seventeen-pictures.md section 6.\n"
  exit 1
fi

printf "CLOCK: PASS - the date advanced in a live city.\n"
printf "  city-loaded proof: guest year word \$0B53 = %s (non-zero), so a city\n" "$year_word"
printf "  object existed at frame %d. The date advanced on top of that.\n" "$CITY_FRAME"
printf "  Both halves are required: a date that moves without a city is not a\n"
printf "  passing clock, and a city without a moving date is the current FAIL.\n"
