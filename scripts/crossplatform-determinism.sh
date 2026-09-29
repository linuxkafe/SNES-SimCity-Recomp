#!/usr/bin/env bash
# crossplatform-determinism.sh - run the determinism battery on two machines
# and compare the framebuffers byte for byte.
#
# WHY THIS EXISTS
#
# The guest is deterministic for a given script, and that was only ever checked
# on one machine. scripts/verify-rom-render.sh answers "does the picture move",
# and `ctest`'s test_deterministic_replay answers "is the core reproducible on
# this host". Neither of those can see a divergence that only exists across
# hosts, because a divergence that is invisible on the machine you ran it on is
# exactly what a single-machine gate cannot detect. Two hosts, one binary, one
# script, one frame count, identical bytes - that is a strictly stronger claim
# than either local gate, and it is the claim a recompiler actually has to make
# if the output is ever to be trusted on hardware it was not built on.
#
# The comparison is deliberately unforgiving: raw framebuffer pixels and a full
# 128 KB WRAM snapshot, sha256, both machines. Nothing is normalised, no
# tolerance, no "close enough" - a single differing byte fails the run. A false
# PASS here is worse than a false FAIL, because a false PASS is a licence to
# trust a build that is wrong.
#
# Presentation is checked separately from the guest. Window size is a host
# decision and is recorded and compared, but the pixels are what must match.
#
# A FOURTH INPUT: THE CARTRIDGE SRAM
#
# The host keeps the 65816's battery-backed SRAM in <exe dir>/saves/save.srm.
# It is gitignored, it is per-machine state that accumulates as soon as anyone
# plays the game, and the guest reads it - so it silently joins the ROM, the
# script and the frame count as an input to "what does the emulator produce".
# This was found the hard way: the first full run of this rig reported a
# divergence in case B, and the cause was not the two machines at all. The
# builder's rig is rebuilt from scratch on every run and so always starts cold
# (no save.srm), while the Deck's rig directory persists and was already warm.
# Same binary, same ROM, same script, different SRAM.
#
# Measured on case B at frame 800, 44 of 131072 WRAM bytes move with the SRAM:
#   absent  ==  all-0x00 save.srm  ->  fe9b176dd05c3114...
#   all-0xFF save.srm              ->  9c3397db5154e114...
#   populated save.srm             ->  6e90d610baab025b...
#
# The framebuffer is NOT affected - all three hash identically at frame 815 -
# so this is invisible to any picture-based check, including
# scripts/verify-rom-render.sh. The battery therefore pins the SRAM to a cold
# 32 KB zero image before every case, which states the precondition rather than
# weakening the test, and still compares the entire 128 KB WRAM snapshot and
# fails on a single differing byte.
#
# USAGE
#   scripts/crossplatform-determinism.sh [DECK] [BUILD_DIR]
#     DECK       ssh target for the second machine. Default deck@steamdeck
#     BUILD_DIR  build dir on this machine.          Default build
#
# ENV
#   ROM        path to the ROM on THIS machine. Default: ./SimCity (USA).sfc
#   DECK_ROM   path to the ROM on the DECK.         Default: probe a few paths
#   DECK_RIG   rig directory on the DECK.           Default: ~/simcity-testrig
#   WORK       local scratch dir for artefacts.      Default: mktemp -d
#   KEEP=1     do not delete the local scratch dir on exit
#
# The ROM is never copied off either machine and never committed: the battery
# runs each side against its own copy, and the two are sha256-checked against
# each other first, so "same ROM" is a verified fact and not an assumption.

set -euo pipefail

SCRIPT_PATH="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)/$(basename "${BASH_SOURCE[0]}")"
REPO="$(cd "$(dirname "$SCRIPT_PATH")/.." && pwd)"

# ---------------------------------------------------------------------------
# The case table. name|script|frames|screenshot_frame|wram_frame|config
#
# `config` is one of: - (use the config.ini that ships beside the binary),
# 4:3 or 16:9 (use a generated copy). A/D run the same attract script under
# both aspects so the aspect knob is measured on the same content.
#
# Frame counts: A is 3000 because the logo screen animates and a still frame
# would prove nothing. B's script only spans 425 frames (the host's script
# parser flushes a pending `wait` into the NEXT press, so a trailing `wait` is
# dropped); RUN_FRAMES is 825 to leave the map time to draw. C adds twelve
# B-presses to B: 1845 frames, which is as far as a script can go before the
# mouse is needed.
# ---------------------------------------------------------------------------
CASES=(
  "A_attract|a_attract.script|3000|2990|2000|-"
  "B_menu|b_menu.script|825|815|800|-"
  "C_naming|c_naming.script|1845|1835|1800|-"
  "D1_aspect4x3|a_attract.script|3000|2990|2000|4:3"
  "D2_aspect16x9|a_attract.script|3000|2990|2000|16:9"
)

# ---------------------------------------------------------------------------
# ROLE=battery
#
# The second half of this file. It is pushed to the far machine and run there,
# so both machines execute the identical case table rather than two
# hand-maintained copies of it that can drift. Not meant to be run by hand.
#
#   ROLE=battery BAT_DIR=... BAT_ROM=... BAT_LABEL=... "$0"
# ---------------------------------------------------------------------------
if [ "${ROLE:-}" = "battery" ]; then
  BAT_DIR="${BAT_DIR:?BAT_DIR required}"
  BAT_ROM="${BAT_ROM:?BAT_ROM required}"
  BAT_LABEL="${BAT_LABEL:-unknown}"
  # Resolve to an absolute path before anything derives a child path from it:
  # the rig dir arrives over ssh as whatever the caller typed (a bare
  # "simcity-testrig" by default), and the host chdirs, so a relative BAT_DIR
  # silently composes into a path that does not exist.
  mkdir -p "$BAT_DIR"
  BAT_DIR="$(cd "$BAT_DIR" && pwd)"
  EXE="$BAT_DIR/exe/SimCitySNESRecomp"
  OUT="$BAT_DIR/out"
  SCRIPTS="$BAT_DIR/scripts"
  mkdir -p "$OUT"
  cd "$BAT_DIR"

  [ -x "$EXE" ] || { echo "battery: no binary at $EXE" >&2; exit 2; }
  [ -f "$BAT_ROM" ] || { echo "battery: no ROM at $BAT_ROM" >&2; exit 2; }

  # A local copy of the shipped config, never edited in place: build/config.ini
  # is the user's file and the binary is what reads it, so touching it would
  # change what every other run in this build dir does.
  #
  # The filenames are spelled out rather than derived from the aspect string.
  # The obvious derivation - "${a/:/}" to strip the colon - produces config43.ini
  # while the case table asks for config4x3.ini, and the result is a --config
  # pointing at a file that does not exist. ParseConfigFile treats a missing
  # file as "no config", the host silently falls back to its built-in defaults
  # (new_renderer=0, scale=0, freq=32040), and the run produces a DIFFERENT
  # PICTURE from a different renderer while still looking like a clean pass.
  # The guard further down is what stops that from recurring quietly.
  sed 's/^\[Graphics\]/[Graphics]\nDisplayAspect = 4:3/' \
    "$BAT_DIR/exe/config.ini" > "$OUT/config4x3.ini"
  sed 's/^\[Graphics\]/[Graphics]\nDisplayAspect = 16:9/' \
    "$BAT_DIR/exe/config.ini" > "$OUT/config169.ini"
  for f in "$OUT/config4x3.ini" "$OUT/config169.ini"; do
    [ -s "$f" ] || { echo "battery: failed to generate $f" >&2; exit 1; }
    grep -q '^DisplayAspect = ' "$f" || {
      echo "battery: $f has no DisplayAspect key - the aspect case would" >&2
      echo "  silently run on the default aspect." >&2; exit 1; }
  done

  : > "$OUT/summary.txt"
  for row in "${CASES[@]}"; do
    IFS='|' read -r name script frames shot wram aspect <<<"$row"
    ppm="$OUT/$name.ppm"
    wbase="$OUT/$name.w"
    log="$OUT/$name.log"
    rm -f "$ppm" "$wbase".f*.bin "$log"

    # Pin the cartridge's battery-backed SRAM before EVERY case, not just once.
    #
    # This is not housekeeping, it is a required input. The host keeps the
    # 65816's persistent SRAM in <exe dir>/saves/save.srm. It is gitignored,
    # it is per-machine state, and it is not mentioned by the determinism
    # contract - and the guest READS it. Measured on case B, frame 800:
    #
    #   no save.srm          -> 44 bytes differ from a populated one
    #   save.srm all 0x00    -> identical to no save.srm (absence == zeros)
    #   save.srm all 0xFF    -> a third, different result
    #   save.srm as saved    -> a fourth, different result
    #
    # So "same ROM, same --script, same frame count" is NOT sufficient for a
    # byte-identical WRAM image; the saved SRAM is a fourth input. Left alone,
    # it makes this rig compare a freshly-cloned builder against a Deck that
    # has run the game, and report the difference as a determinism failure.
    # Zeroing it per case states the actual precondition instead of relaxing
    # the comparison: a cold cartridge, identical on both machines.
    #
    # Note what this does and does not affect: the 336x224 framebuffer is
    # invariant to the SRAM (all three variants above hash the same at frame
    # 815). The divergence is confined to ~44 bytes of low WRAM. The WRAM
    # comparison below is still run in full and is still fatal on mismatch.
    rm -rf "$BAT_DIR/exe/saves"
    mkdir -p "$BAT_DIR/exe/saves"
    dd if=/dev/zero of="$BAT_DIR/exe/saves/save.srm" bs=32768 count=1 status=none

    cfg=()
    case "$aspect" in
      -)     ;;                      # default config, read beside the binary
      4:3)   cfg=(--config "$OUT/config4x3.ini") ;;
      16:9)  cfg=(--config "$OUT/config169.ini") ;;
    esac

    # Absolute paths everywhere. The host chdir()s to the executable's own
    # directory, so a relative --script or output path is resolved against the
    # exe dir and silently lands somewhere else - or, for --script, exits 2.
    set +e
    SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy \
    SNESRECOMP_RUN_FRAMES="$frames" \
    SNESRECOMP_SCREENSHOT="$ppm" SNESRECOMP_SCREENSHOT_FRAME="$shot" \
    SNESRECOMP_WRAM_DUMP="$wbase" SNESRECOMP_WRAM_DUMP_AT="$wram" \
      "$EXE" "${cfg[@]}" --script "$SCRIPTS/$script" "$BAT_ROM" \
      >"$log" 2>&1
    rc=$?
    set -e

    wbin="$wbase.f$wram.bin"
    if [ $rc -ne 0 ] || [ ! -s "$ppm" ] || [ ! -s "$wbin" ]; then
      echo "battery: case $name FAILED (rc=$rc ppm=$([ -s "$ppm" ] && echo ok || echo MISSING) wram=$([ -s "$wbin" ] && echo ok || echo MISSING))" >&2
      grep -iE "error|script:|cannot|fail" "$log" | tail -5 >&2 || true
      exit 1
    fi

    # A --config the host could not read is not an error the host reports: the
    # config parser shrugs at a missing file, the built-in defaults take over,
    # and the run completes normally on a different renderer. So the one line
    # that says what was actually parsed is captured and required to agree
    # across every case. If the aspect cases came back on defaults, this fails
    # loudly instead of reporting a clean comparison between two different
    # renderers.
    parsed=$(grep -o 'config parsed:.*' "$log" | head -1)
    [ -n "$parsed" ] || {
      echo "battery: case $name logged no 'config parsed' line" >&2; exit 1; }
    if [ -z "${BAT_PARSED_REF:-}" ]; then
      BAT_PARSED_REF="$parsed"
    elif [ "$parsed" != "$BAT_PARSED_REF" ]; then
      echo "battery: case $name parsed a different config than the first case." >&2
      echo "  reference: $BAT_PARSED_REF" >&2
      echo "  $name: $parsed" >&2
      echo "  A --config that the host ignored looks exactly like this." >&2
      exit 1
    fi

    # Hash the pixel PAYLOAD, not the file: the P6 header carries the width, and
    # the width is legitimately different between the two aspects. Hashing the
    # whole file would conflate "the picture changed" with "the header changed".
    px=$(python3 - "$ppm" <<'PY'
import hashlib, sys
d = open(sys.argv[1], 'rb').read()
# P6\n<w> <h>\n<maxval>\n - skip four whitespace-delimited tokens then one byte.
i, toks = 0, []
while len(toks) < 4:
    while i < len(d) and d[i:i+1].isspace(): i += 1
    if d[i:i+1] == b'#':
        while i < len(d) and d[i:i+1] not in (b'\n', b''): i += 1
        continue
    j = i
    while j < len(d) and not d[j:j+1].isspace(): j += 1
    toks.append(d[i:j]); i = j
i += 1  # single whitespace byte after maxval
w, h = int(toks[1]), int(toks[2])
px = d[i:]
assert len(px) == w * h * 3, "payload %d != %d" % (len(px), w * h * 3)
print("%dx%d %s" % (w, h, hashlib.sha256(px).hexdigest()))
PY
)
    dim=${px%% *}; px_sha=${px##* }
    w_sha=$(sha256sum "$wbin" | cut -d' ' -f1)
    ppm_sha=$(sha256sum "$ppm" | cut -d' ' -f1)
    window=$(grep -o 'window created: [0-9]*x[0-9]*' "$log" | head -1 | sed 's/window created: //')
    wline=$(grep -o '\[wramdump\].*' "$log" | head -1 | sed "s#$wbase#$name.w#")
    sline=$(grep -o '\[host .*screenshot: wrote.*' "$log" | head -1 | sed 's/.*screenshot: /screenshot: /')

    printf '%s|%s|%s|%s|%s|%s|%s|%s|%s|%s\n' \
      "$name" "$frames" "$dim" "$px_sha" "$ppm_sha" "$w_sha" "$window" "$wline" "$sline" "$script" \
      >> "$OUT/summary.txt"
    echo "battery[$BAT_LABEL]: $name done (${dim}, $frames frames)"
  done
  echo "battery[$BAT_LABEL]: all ${#CASES[@]} cases complete"
  exit 0
fi

# ---------------------------------------------------------------------------
# ROLE=orchestrator (default) - run both machines and compare.
# ---------------------------------------------------------------------------
DECK="${1:-deck@steamdeck}"
BUILD_DIR="${2:-build}"
ROM="${ROM:-$REPO/SimCity (USA).sfc}"
DECK_RIG="${DECK_RIG:-\$HOME/simcity-testrig}"
DECK_ROM="${DECK_ROM:-}"

command -v ssh >/dev/null || { echo "ERROR: no ssh" >&2; exit 2; }
command -v scp >/dev/null || { echo "ERROR: no scp" >&2; exit 2; }
[ -x "$BUILD_DIR/SimCitySNESRecomp" ] || {
  echo "ERROR: no binary at $BUILD_DIR/SimCitySNESRecomp - run 'make build' first" >&2; exit 2; }
[ -f "$ROM" ] || { echo "ERROR: no ROM at $ROM" >&2; exit 2; }

# The script files are part of the battery, so they travel with the rig. They
# live in scripts/ and are committed, which is what makes the two machines run
# the same input rather than two copies of it.
SCRIPT_FILES=(a_attract.script b_menu.script c_naming.script)
for f in "${SCRIPT_FILES[@]}"; do
  [ -f "$REPO/scripts/$f" ] || {
    echo "ERROR: missing battery script $REPO/scripts/$f" >&2; exit 2; }
done

WORK="${WORK:-$(mktemp -d -t xpd-XXXXXX)}"
LOCAL_RIG="$WORK/builder"
cleanup() { [ "${KEEP:-0}" = 1 ] || rm -rf "$WORK"; }
trap cleanup EXIT
mkdir -p "$LOCAL_RIG/exe" "$LOCAL_RIG/scripts" "$LOCAL_RIG/out"

echo "== cross-platform determinism =="
echo "  builder : $REPO"
echo "  deck    : $DECK"
echo "  scratch : $WORK"
echo

# The identity of the thing under test. If these four do not match on the far
# side, the comparison below is meaningless and should be refused rather than
# reported.
b_bin=$(sha256sum "$BUILD_DIR/SimCitySNESRecomp" | cut -d' ' -f1)
b_rom=$(sha256sum "$ROM" | cut -d' ' -f1)
b_cfg=$(sha256sum "$BUILD_DIR/config.ini" | cut -d' ' -f1)
b_key=$(sha256sum "$BUILD_DIR/keybinds.ini" | cut -d' ' -f1)

# Stage a self-contained rig next to the scratch dir. The binary is COPIED, not
# referenced in place: the host chdir()s to the executable's own directory and
# reads config.ini from there, so running the build dir's own binary in place
# would have the battery depend on - and potentially write to - the user's real
# build dir. Nothing this script does ever touches $BUILD_DIR.
cp "$BUILD_DIR/SimCitySNESRecomp" "$BUILD_DIR/config.ini" "$BUILD_DIR/keybinds.ini" "$LOCAL_RIG/exe/"
cp "$REPO"/scripts/*.script "$LOCAL_RIG/scripts/"
cp "$REPO"/scripts/wram-diff.py "$REPO"/scripts/verify-rom-render.sh "$LOCAL_RIG/scripts/"
ROM_ABS="$(cd "$(dirname "$ROM")" && pwd)/$(basename "$ROM")"

# ---------------------------------------------------------------------------
# 1. push to the far machine and establish that the two sides are comparable
#
# This happens BEFORE any emulation on purpose. Identity checks that arrive
# after four minutes of headless frames are identity checks that cost four
# minutes to fail. If the two machines are not running the same binary over the
# same ROM, that is worth learning in the first ten seconds.
# ---------------------------------------------------------------------------
echo
echo "-- push to $DECK --"
# One round trip creates the rig and returns its ABSOLUTE path. Every later
# remote step then uses that resolved path instead of re-deriving it from a
# tilde or a relative name, which is where the quoting bugs live.
DECK_RIG_ABS=$(ssh -o BatchMode=yes "$DECK" \
  "mkdir -p \"$DECK_RIG/exe\" \"$DECK_RIG/scripts\" \"$DECK_RIG/out\" && cd \"$DECK_RIG\" && pwd")
[ -n "$DECK_RIG_ABS" ] || { echo "ERROR: could not create rig dir on $DECK" >&2; exit 2; }
echo "  rig    : $DECK_RIG_ABS"
tar -C "$LOCAL_RIG" -cf - exe scripts | ssh -o BatchMode=yes "$DECK" "tar -C '$DECK_RIG_ABS' -xf -"
scp -q "$SCRIPT_PATH" "$DECK:$DECK_RIG_ABS/crossplatform-determinism.sh"
ssh -o BatchMode=yes "$DECK" "chmod +x '$DECK_RIG_ABS/crossplatform-determinism.sh'"

# The ROM never moves. The far machine already has its own copy; find it and
# prove it is the same bytes as ours, because "same ROM" has to be a checked
# fact and not an assumption baked into a comparison.
#
# The candidates are paths in the REMOTE shell's namespace, not in this one's.
# They are tested and read over ssh, so the home directory has to survive the
# trip: each one is stored as a literal '$HOME/...' and wrapped in double quotes
# in the ssh command line, so it is the FAR machine that expands it. Expanding
# it here instead is the bug this shape avoids - it produced a path under this
# machine's home directory and then reported the ROM as missing.
if [ -z "$DECK_ROM" ]; then
  for cand in '$HOME/simcity-testrig/SimCity (USA).sfc' \
              '$HOME/aes-t058/SimCity (USA).sfc' \
              '$HOME/simcity/SimCity (USA).sfc' \
              '$HOME/SimCity (USA).sfc' \
              '$HOME/simcity.sfc'; do
    if ssh -o BatchMode=yes "$DECK" "test -f \"$cand\"" 2>/dev/null; then
      DECK_ROM="$cand"; break
    fi
  done
fi
[ -n "$DECK_ROM" ] || { echo "ERROR: no ROM found on $DECK (set DECK_ROM=...)" >&2; exit 2; }
# Resolve it on the FAR machine to a real absolute path, once. Every later use
# of this value is wrapped in single quotes inside an ssh command line, where a
# '$HOME/...' would be taken literally and the battery would report the ROM as
# missing. An absolute path has nothing left to expand.
DECK_ROM=$(ssh -o BatchMode=yes "$DECK" "readlink -f \"$DECK_ROM\"" 2>/dev/null || true)
ssh -o BatchMode=yes "$DECK" "test -f \"$DECK_ROM\"" 2>/dev/null || {
  echo "ERROR: DECK_ROM did not resolve to a readable file on $DECK" >&2; exit 2; }
d_rom=$(ssh -o BatchMode=yes "$DECK" "sha256sum \"$DECK_ROM\"" | cut -d' ' -f1)
if [ "$d_rom" != "$b_rom" ]; then
  echo "ERROR: ROM differs between machines - refusing to compare." >&2
  echo "  builder $b_rom" >&2
  echo "  deck    $d_rom  ($DECK_ROM)" >&2
  exit 2
fi
echo "  ROM sha256 matches on both machines: $b_rom"

d_bin=$(ssh -o BatchMode=yes "$DECK" "sha256sum '$DECK_RIG_ABS/exe/SimCitySNESRecomp'" | cut -d' ' -f1)
[ "$d_bin" = "$b_bin" ] || { echo "ERROR: binary transfer corrupt ($d_bin != $b_bin)" >&2; exit 2; }
echo "  binary sha256 matches on both machines: $b_bin"

# The config is an input to the run, not decoration. A config.ini that differs
# between the two machines would make any divergence a config artefact and any
# agreement meaningless, so it is checked rather than assumed. (keybinds.ini
# matters less - the battery is scripted, not key-driven - but it is part of
# what the binary loads, so it is checked too.)
d_cfg=$(ssh -o BatchMode=yes "$DECK" "sha256sum '$DECK_RIG_ABS/exe/config.ini'" | cut -d' ' -f1)
[ "$d_cfg" = "$b_cfg" ] || {
  echo "ERROR: config.ini differs between machines - the comparison is confounded." >&2
  echo "  builder $b_cfg" >&2; echo "  deck    $d_cfg" >&2; exit 2; }
d_key=$(ssh -o BatchMode=yes "$DECK" "sha256sum '$DECK_RIG_ABS/exe/keybinds.ini'" | cut -d' ' -f1)
[ "$d_key" = "$b_key" ] || {
  echo "ERROR: keybinds.ini differs between machines - the comparison is confounded." >&2
  echo "  builder $b_key" >&2; echo "  deck    $d_key" >&2; exit 2; }
echo "  config.ini and keybinds.ini sha256 match on both machines"

# ---------------------------------------------------------------------------
# 2. builder side
# ---------------------------------------------------------------------------
echo
echo "-- builder --"
ROLE=battery BAT_DIR="$LOCAL_RIG" BAT_ROM="$ROM_ABS" BAT_LABEL=builder "$SCRIPT_PATH"

# ---------------------------------------------------------------------------
# 3. deck side
# ---------------------------------------------------------------------------
echo
echo "-- deck --"
# Foreground over ssh on purpose: a process backgrounded out of an ssh session
# is killed when the session closes (it gets an SDL_QUIT), so the battery must
# be driven by a session that is still open. The log, not the return value, is
# what confirms it ran.
ssh -o BatchMode=yes "$DECK" \
  "ROLE=battery BAT_DIR='$DECK_RIG_ABS' BAT_ROM='$DECK_ROM' BAT_LABEL=deck '$DECK_RIG_ABS/crossplatform-determinism.sh'"

# ---------------------------------------------------------------------------
# 4. pull artefacts back and compare
# ---------------------------------------------------------------------------
echo
echo "-- pull artefacts --"
rm -rf "$WORK/deck"; mkdir -p "$WORK/deck"
scp -q -r "$DECK:$DECK_RIG_ABS/out/." "$WORK/deck/" 2>/dev/null || true
[ -s "$WORK/deck/summary.txt" ] || { echo "ERROR: no deck summary - the far side did not finish" >&2; exit 2; }

# Join the two summaries on case name.
python3 - "$LOCAL_RIG/out/summary.txt" "$WORK/deck/summary.txt" <<'PY'
import sys
def load(p):
    d = {}
    for line in open(p):
        f = line.rstrip("\n").split("|")
        if len(f) >= 10: d[f[0]] = f
    return d
b, k = load(sys.argv[1]), load(sys.argv[2])
names = list(b.keys()) + [n for n in k if n not in b]

def short(h): return h[:16] if h else "-"

rows, fails, missing = [], [], []
cols = ("case", "frames", "dim", "pixels", "wram", "window")
rows.append(cols)
# Every row is a 6-tuple. A row that is a single pre-formatted string has a
# length of ~78, and the renderer below indexes rows by column number - which
# raises IndexError on the first real result, after all the emulation is done.
rows.append(tuple("-" * len(c) for c in cols))
for n in names:
    if n not in b: missing.append("%s: missing on builder" % n); continue
    if n not in k: missing.append("%s: missing on deck" % n); continue
    B, K = b[n], k[n]
    # Summary field order, as written by the battery above:
    #   0 name  1 frames  2 dim  3 pixel_sha  4 ppm_sha  5 wram_sha
    #   6 window  7 wramlog  8 shotlog  9 script
    # The indices are named, not inlined, because getting one wrong does not
    # crash - it silently compares the wrong two strings. A window size is
    # identical on both machines even when the pixels are not, so an
    # off-by-one here would report PASS on a divergence.
    I_FRAME, I_DIM, I_PIX, I_WRAM, I_WIN = 1, 2, 3, 5, 6
    I_WLOG, I_SLOG, I_SCRIPT = 7, 8, 9
    ok_px   = B[I_PIX]  == K[I_PIX]
    ok_wram = B[I_WRAM] == K[I_WRAM]
    if not (ok_px and ok_wram): fails.append((n, B, K))
    rows.append((n, B[I_FRAME], B[I_DIM],
                 short(B[I_PIX])  + ("=" if ok_px else "!"),
                 short(B[I_WRAM]) + ("=" if ok_wram else "!"),
                 B[I_WIN] + " / " + K[I_WIN]))

w = [max(len(r[i]) for r in rows) for i in range(len(cols))]
def emit(cells): print("  " + "  ".join(str(cells[i]).ljust(w[i]) for i in range(len(cols))))
emit(cols)
emit(tuple("-" * w[i] for i in range(len(cols))))
for r in rows[2:]:
    emit(r)
print()
print("  'pixels' = sha256 of the raw framebuffer (P6 payload, header excluded)")
print("  'wram'   = sha256 of the 128 KB WRAM snapshot")
print("  'window' = the WxH each host logged for 'window created'")
print("  a trailing '=' means builder == deck; '!' means they differ")
print()

for m in missing: print("  MISSING: " + m)
if missing: print()

if fails:
    print("FAIL: %d of %d cases diverge between the two machines\n" % (len(fails), len(names)))
    for n, B, K in fails:
        print("  case %s" % n)
        if B[I_PIX] != K[I_PIX]:
            print("    pixels  builder %s (%s)" % (B[I_PIX], B[I_DIM]))
            print("            deck    %s (%s)" % (K[I_PIX], K[I_DIM]))
        if B[I_WRAM] != K[I_WRAM]:
            print("    wram    builder %s" % B[I_WRAM])
            print("            deck    %s" % K[I_WRAM])
        print("    window  builder %s / deck %s" % (B[I_WIN], K[I_WIN]))
        print("    wramlog %s" % B[I_WLOG])
        print("    shotlog %s" % B[I_SLOG])
        print("    script  %s (%s frames)" % (B[I_SCRIPT], B[I_FRAME]))
        print()
    sys.exit(1)

print("PASS: %d/%d cases byte-identical across both machines." % (len(names), len(names)))
print("  frames compared: " + ", ".join("%s=%s" % (b[n][0], b[n][1]) for n in names if n in b))
print("  total simulated frames per machine: %d" % sum(int(b[n][1]) for n in b))
PY
rc=$?

# ---------------------------------------------------------------------------
# 5. The aspect pair, reported separately.
#
# D1 and D2 run the SAME attract script and the SAME frame count under two
# DisplayAspect settings. The guest-visible framebuffer is NOT expected to be
# byte-identical between them, and the reason is in the source, not in luck:
# SimCityPrepareFrame() in src/main.c sets frame_w to 336 for the default
# aspect and 256 for 16:9 - 16:9 drops the 40px-per-side blank margins so the
# window is full-bleed. Different width, so different bytes, by design.
#
# What IS interesting, and is checked here, is whether the guest's own 256
# columns are the same picture in both - i.e. whether the aspect knob is purely
# a presentation choice and leaves the cartridge's output untouched.
# ---------------------------------------------------------------------------
a43="$LOCAL_RIG/out/D1_aspect4x3.ppm"
a169="$LOCAL_RIG/out/D2_aspect16x9.ppm"
if [ -s "$a43" ] && [ -s "$a169" ]; then
  echo
  echo "-- aspect check (builder) --"
  python3 - "$a43" "$a169" <<'PY'
import hashlib, sys
def read(p):
    d = open(p, 'rb').read(); i, t = 0, []
    while len(t) < 4:
        while d[i:i+1].isspace(): i += 1
        j = i
        while not d[j:j+1].isspace(): j += 1
        t.append(d[i:j]); i = j
    return int(t[1]), int(t[2]), d[i+1:]
w1, h1, p1 = read(sys.argv[1])
w2, h2, p2 = read(sys.argv[2])
print("  4:3  framebuffer %dx%d  sha256 %s" % (w1, h1, hashlib.sha256(p1).hexdigest()))
print("  16:9 framebuffer %dx%d  sha256 %s" % (w2, h2, hashlib.sha256(p2).hexdigest()))
print("  whole-framebuffer byte-identical: %s" % (p1 == p2))
# The margins: main.c says 336 = 256 + 2*40, so the guest's 256 columns sit
# between them. Compare that window only.
if w1 == 336 and w2 == 256 and h1 == h2:
    centre = b"".join(p1[y*w1*3+40*3 : y*w1*3+(40+256)*3] for y in range(h1))
    print("  guest 256 columns inside the 4:3 margins: sha256 %s" % hashlib.sha256(centre).hexdigest())
    print("  identical to the 16:9 framebuffer:        %s" % (centre == p2))
    marg = b"".join(p1[y*w1*3 : y*w1*3+40*3] for y in range(h1))
    print("  4:3 left margin is all-black: %s" % (marg == b"\x00" * len(marg)))
else:
    print("  (unexpected geometry %dx%d vs %dx%d - margin extraction skipped)" % (w1, h1, w2, h2))
PY
fi

echo
echo "artefacts: $WORK/builder/out  and  $WORK/deck"
[ "${KEEP:-0}" = 1 ] || true
exit $rc
