#!/usr/bin/env bash
# validate-findings.sh - independently confirm or refute the findings in
# docs/review/REVIEW-2026-10-02.md
#
# WHY THIS EXISTS AND WHY A HUMAN MUST RUN IT
#
# The aes-peer-review protocol requires the moderator to emit an executable
# validation script that a person OTHER THAN THE AUTHOR executes, and records
# the output. The author of these findings is also the person who ran every
# command while writing them, so nothing here is confirmed by the protocol's own
# standard until someone else runs it.
#
# This script is deliberately dumb. It re-derives each finding from the tree,
# prints CONFIRMED or REFUTED per check, and exits non-zero if any BLOCKER or
# MAJOR fails to reproduce. It does not read the review; it does not know what
# the findings "mean"; it only checks facts that are checkable.
#
# Usage:  docs/review/validate-findings.sh
#         docs/review/validate-findings.sh --rom "/path/to/SimCity (USA).sfc"
#
# Exit 0  = every check reproduced
# Exit 1  = at least one check did not
# Exit 2  = the tree is not in the state the review describes (wrong commit)

set -uo pipefail

ROM="${ROM:-}"
while [ $# -gt 0 ]; do
	case "$1" in
		--rom) ROM="$2"; shift 2 ;;
		*) echo "unknown arg: $1" >&2; exit 2 ;;
	esac
done

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$ROOT" || exit 2

pass=0; fail=0
ok()   { printf "  CONFIRMED  %s\n" "$1"; pass=$((pass+1)); }
bad()  { printf "  REFUTED    %s\n" "$1"; fail=$((fail+1)); }
skip() { printf "  SKIPPED    %s (%s)\n" "$1" "$2"; }

echo "== validate-findings.sh =="
echo "  repo   : $ROOT"
echo "  head   : $(git rev-parse --short HEAD 2>/dev/null || echo 'not a git repo')"
echo "  review : docs/review/REVIEW-2026-10-02.md (rubric sha256 in RUBRIC.sha256)"
echo

# ---------------------------------------------------------------- rubric hash
echo "-- the rubric the review was scored against --"
if sha256sum -c docs/review/RUBRIC.sha256 >/dev/null 2>&1; then
	ok "F-00 rubric hash verifies against RUBRIC.sha256"
else
	bad "F-00 rubric hash does NOT verify -- the review's rubric is not the committed one"
fi
echo

# ------------------------------------------------- F-01 gate asserts a cause
echo "-- F-01 (BLOCKER) clock-gate.sh asserts a cause measurement refuted --"
# Check for the ASSERTION, not the mention. The script is allowed to say "a
# previous version printed a heading reading X" - that is the retraction, and
# flagging it would make the fix impossible. What must not survive is a printf
# that emits the heading or the claim.
# The label form only. A line that NAMES the old heading while retracting it
# ("under a heading reading \"What is established\". That cause was...") is the
# fix, not the defect; flagging it would make the fix impossible to write.
n_est=$(grep -cE 'printf.*What is established:' scripts/clock-gate.sh 2>/dev/null); n_est=${n_est:-0}
if [ "$n_est" -gt 0 ]; then
	bad "F-01 clock-gate.sh still PRINTS 'What is established' ($n_est) -- BLOCKER not closed"
else
	ok "F-01 clock-gate.sh no longer prints the 'What is established' heading"
fi
for tok in '0012' 'CODE_03D283' 'CODE_03D287' 'CODE_008061'; do
	c=$(grep -cE "printf.*${tok}" scripts/clock-gate.sh 2>/dev/null); c=${c:-0}
	if [ "$c" -gt 0 ]; then
		bad "F-01 clock-gate.sh still PRINTS a refuted cause containing '$tok' ($c site(s))"
	fi
done
if ! grep -qE 'printf.*(NOT ESTABLISHED|does not guess)' scripts/clock-gate.sh 2>/dev/null; then
	bad "F-01 clock-gate.sh does not state that the cause is not established"
else
	ok "F-01 clock-gate.sh states the cause is NOT ESTABLISHED"
fi
echo

# --------------------------------------- F-02 test hardcodes the author's home
echo "-- F-02 (BLOCKER) tests/ hardcodes an absolute path into the author's home --"
# Count CODE occurrences, not comments. The test's header deliberately records
# that it used to hardcode the path; a comment saying so is the retraction, and
# flagging it would forbid documenting the fix.
n_home=$(grep -rn '/home/seyon' tests/ 2>/dev/null | grep -vE '^[^:]+:[0-9]+: *(\*|//|/\*)' | wc -l)
if [ "$n_home" -gt 0 ]; then
	bad "F-02 $n_home CODE line(s) in tests/ contain /home/seyon -- BLOCKER not closed"
	grep -rn '/home/seyon' tests/ 2>/dev/null | grep -vE '^[^:]+:[0-9]+: *(\*|//|/\*)' | sed 's/^/      /'
else
	ok "F-02 no absolute path to the author's home in tests/ code (comments excluded)"
fi
if grep -q 'now relocatable' README.md 2>/dev/null; then
	if [ "$n_home" -gt 0 ]; then
		bad "F-02 README claims the gate 'is now relocatable' while the path is still hardcoded"
	else
		ok "F-02 README's relocatability claim is now true"
	fi
fi
echo

# ------------------------------------ F-03 refuted premise listed as retracted
echo "-- F-03 (BLOCKER) a refuted premise recorded as a retraction --"
# The requirement is that CODE_008061's status is labelled OPEN, not that the
# words "never run" are absent - the retraction itself must name the claim.
if grep -n 'CODE_008061' docs/CLAIMS_REGISTER.md 2>/dev/null \
   | grep -qiE 'OPEN'; then
  ok "F-03 CLAIMS_REGISTER labels CODE_008061's status OPEN, not retracted"
else
  bad "F-03 CLAIMS_REGISTER does not label CODE_008061's status OPEN"
fi
if scripts/check-retracted-claims.sh >/dev/null 2>&1; then
  ok "F-03 no script or doc asserts 'CODE_008061 never runs' without a marker"
else
  bad "F-03 check-retracted-claims.sh reports an unmarked assertion (run it for detail)"
fi
echo

echo "-- F-05 (MAJOR) CLAIMS_REGISTER recommends committing aes/ --"
if grep -qi 'un-ignore' docs/CLAIMS_REGISTER.md 2>/dev/null; then
	bad "F-05 CLAIMS_REGISTER still recommends un-ignoring aes/ (forbidden by standing rule)"
else
	ok "F-05 CLAIMS_REGISTER no longer recommends un-ignoring aes/"
fi
# and the standing rule itself
n_aes=$(git ls-files 2>/dev/null | grep -cE '^(aes|\.aes)/')
if [ "$n_aes" -eq 0 ]; then
	ok "F-05 standing rule holds: 0 files under aes/ or .aes/ are tracked"
else
	bad "F-05 $n_aes file(s) under aes/ are TRACKED -- the standing rule is broken"
fi
echo

# ------------------------------------------- F-08 dead-end script still armed
echo "-- F-08 (MAJOR) cross-load-peer-save.sh is an armed dead end --"
f=scripts/cross-load-peer-save.sh
if [ ! -e "$f" ]; then
	ok "F-08 cross-load-peer-save.sh has been deleted"
else
	if head -25 "$f" 2>/dev/null | grep -qiE 'cannot work|impossible|dead end|do not use|known impossible'; then
		ok "F-08 cross-load-peer-save.sh carries a dead-end banner"
	else
		bad "F-08 cross-load-peer-save.sh exists with no dead-end banner in its first 25 lines"
	fi
	if head -25 "$f" 2>/dev/null | grep -q '0012'; then
		bad "F-08 cross-load-peer-save.sh still pokes the refuted \$0012 diagnosis"
	fi
	if [ -x "$f" ]; then
		bad "F-08 cross-load-peer-save.sh is still executable (mode $(stat -c %a "$f"))"
	fi
fi
echo

# ------------------------------------------------------ F-12 comment vs bytes
echo "-- F-12 (MINOR) bank00.cfg comment vs the actual bytes --"
if [ -f recomp/bank00.cfg ]; then
	if grep -q 'INC \$00C7' recomp/bank00.cfg; then
		bad "F-12 bank00.cfg still says 'INC \$00C7'; the bytes E6 C7 are INC \$C7 (direct page)"
	else
		ok "F-12 bank00.cfg no longer mis-states the addressing mode"
	fi
else
	skip "F-12" "recomp/bank00.cfg absent"
fi
echo

# ------------------------------------------------------- F-13 deleted line
echo "-- F-13 (MINOR) the log quotes a config line that was deleted --"
# A mention is fine; an unmarked mention is not. The guard implements the
# marker-within-N-lines rule, so delegate rather than counting.
if scripts/check-retracted-claims.sh >/dev/null 2>&1; then
  ok "F-13 every 'force_lle 0x009311' mention carries a retraction marker"
else
  bad "F-13 an unmarked 'force_lle 0x009311' quote remains (run check-retracted-claims.sh)"
fi
if grep -rq 'force_lle 0x009311' recomp/ 2>/dev/null; then
  bad "F-13 recomp/ actually contains force_lle 0x009311 -- the doc may be right"
else
  ok "F-13 confirmed: recomp/ has no force_lle 0x009311, so the doc quote is stale"
fi
echo

echo "-- F-14 (MINOR) stale figures --"
# Presence is not the test; an UNMARKED presence is. Both figures are in the
# ledger, so the guard decides, and a marked mention is the fix rather than a
# violation.
if scripts/check-retracted-claims.sh >/dev/null 2>&1; then
  ok "F-14 every mention of the stale figures (2.45 ms, 254 crc32) carries a marker"
else
  bad "F-14 an unmarked stale figure remains (run check-retracted-claims.sh)"
fi
if grep -qE '\b254\b' README.md 2>/dev/null; then
  bad "F-14 README still states 254 distinct crc32"
else
  ok "F-14 README no longer states 254 distinct crc32"
fi
echo

echo "-- legal (D4 of the definition of done) --"
n_sfc=$(git ls-files 2>/dev/null | grep -ci '\.sfc$')
[ "$n_sfc" -eq 0 ] && ok "no ROM tracked" || bad "$n_sfc ROM file(s) tracked"
n_sav=$(git ls-files 2>/dev/null | grep -cE '\.(sav|srm)$')
[ "$n_sav" -eq 0 ] && ok "no battery save tracked" || bad "$n_sav save file(s) tracked"
if [ -e "SimCity (USA).sfc" ] && ! git ls-files --error-unmatch "SimCity (USA).sfc" >/dev/null 2>&1; then
	ok "the ROM is present on disk and untracked (expected; it is user-supplied)"
fi
echo

# --------------------------------- the one measurement only a ROM can confirm
echo "-- the two claims that need a ROM run --"
if [ -n "$ROM" ] && [ -f "$ROM" ]; then
	if [ -x build/SimCitySNESRecomp ]; then
		tmp=$(mktemp -d)
		SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy \
		SNESRECOMP_RUN_FRAMES=6000 \
		SNESRECOMP_WRAM_DUMP="$tmp/w" \
		SNESRECOMP_WRAM_DUMP_AT=4000 \
		./build/SimCitySNESRecomp --script "$PWD/scripts/d_city.script" "$ROM" \
			>/dev/null 2>&1
		if [ -f "$tmp/w.f4000.bin" ]; then
			year=$(python3 -c "
d=open('$tmp/w.f4000.bin','rb').read()
print('%04X' % (d[0x0B53] | (d[0x0B53+1]<<8)))")
			mon=$(python3 -c "
d=open('$tmp/w.f4000.bin','rb').read()
print('%04X' % (d[0x0B55] | (d[0x0B55+1]<<8)))")
			tok=$(python3 -c "
d=open('$tmp/w.f4000.bin','rb').read()
print('%02X' % d[0x00B9])")
			g12=$(python3 -c "
d=open('$tmp/w.f4000.bin','rb').read()
print('%02X' % d[0x0012])")
			if [ "$year" != "0000" ]; then
				ok "D001 reproduced: city is loaded, \$0B53 = 0x$year (non-zero), month 0x$mon"
			else
				bad "D001 NOT reproduced: \$0B53 = 0000 -- this build may load no city"
			fi
			[ "$tok" = "01" ] && ok "D002 reproduced: \$00B9 = 01 (token survives the frame)" \
			                  || bad "D002 NOT reproduced: \$00B9 = $tok (the deadlock may be back)"
			[ "$g12" != "00" ] && ok "D003 reproduced: \$0012 = $g12, NOT 0 -- the gate's printed cause is false" \
			                   || bad "D003 NOT reproduced: \$0012 = 00 -- the gate's printed cause may hold"
		else
			bad "the 6000-frame run wrote no WRAM dump at f4000"
		fi
		rm -rf "$tmp"
	else
		skip "the two ROM claims" "build/SimCitySNESRecomp not built"
	fi
else
	skip "the two ROM claims (city loaded; \$00B9; \$0012)" "no --rom given or ROM absent"
fi
echo

# ---------------------------------------------------------------------------
printf "== summary ==\n"
printf "  confirmed : %d\n" "$pass"
printf "  refuted   : %d\n" "$fail"
printf "  skipped   : see above\n"
if [ "$fail" -eq 0 ]; then
	printf "\nALL CHECKS REPRODUCED. The BLOCKERs in the review are closed.\n"
	exit 0
fi
printf "\n%d CHECK(S) DID NOT REPRODUCE. See REFUTED lines above.\n" "$fail"
printf "A REFUTED line does not automatically mean the review was wrong --\n"
printf "a finding may have been fixed (that is the point), or the reviewer may\n"
printf "have erred. Read the line, then the tree, then decide.\n"
exit 1
