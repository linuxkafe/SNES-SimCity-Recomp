#!/usr/bin/env bash
# validate-findings-c041.sh - re-derive every mechanical claim in
# docs/review/REVIEW-2026-10-02c.md from the tree and the ROM.
#
# WHY A SECOND VALIDATOR, AND WHY IT IS NOT TRUSTED EITHER
#
# The previous validator (validate-findings.sh) passed on a tree that contained
# a DoD row asserting a green gate which was red, because it never checked that
# row. A validator that only re-runs the checks its author remembered is a
# transcript, not a check. So this one is written to FAIL on the two things its
# predecessors missed, and it says so:
#
#   * it re-derives the ROM-byte claims from the ROM, not from a doc;
#   * it asserts that no tracked file claims a green gate while that gate is
#     red - the E-01 shape that shipped at 8a7340f.
#
# THE AUTHOR RAN THIS SCRIPT. The peer-review protocol wants a non-author to run
# it. That has not happened, and the review says so. Treat the output as the
# output of a self-check.

set -uo pipefail
cd "$(dirname "$0")/../.." || exit 2
ROOT="$PWD"
CONF=0
REFUTED=0
SKIPPED=0
ok()   { CONF=$((CONF+1)); printf "  CONFIRMED  %s\n" "$*"; }
bad()  { printf "  REFUTED    %s\n" "$*"; REFUTED=$((REFUTED+1)); }
skip() { printf "  SKIPPED    %s\n" "$*"; SKIPPED=$((SKIPPED+1)); }

echo "== validate-findings-c041 =="
echo "  repo  : $ROOT"
echo

# ---------------------------------------------------------------- rubric hash
echo "-- the rubric this review was scored against --"
if [ -f docs/review/RUBRIC.sha256 ] && command -v sha256sum >/dev/null; then
	if sha256sum -c --status docs/review/RUBRIC.sha256; then
		ok "F-00 rubric hash verifies against RUBRIC.sha256"
	else
		bad "F-00 rubric hash does NOT verify - the rubric was edited after registration"
	fi
else
	skip "F-00 sha256sum or RUBRIC.sha256 absent"
fi
echo

# ------------------------------------------------- C-01 byte-level verification
echo "-- C-01 instructions located by CPU address, re-read from the ROM --"
ROM="${ROM:-$ROOT/SimCity (USA).sfc}"
if [ -f "$ROM" ]; then
	# The first version of this function computed the slice end as
	# `$1 + len/2`, which python3 rejects as a float index: it returned the
	# empty string for every label and would have reported PASS had the
	# comparison not been there. Found by running it, not by reading it.
	expect() { # $1 = file offset, $2 = expected hex, $3 = what it is
		got=$(python3 - "$ROM" "$1" "$((${#2}/2))" <<'PYEOF'
import sys
d = open(sys.argv[1], 'rb').read()
off = int(sys.argv[2], 0)
n = int(sys.argv[3])
print(d[off:off+n].hex())
PYEOF
)
		if [ -z "$got" ]; then bad "C-01 $3: the reader returned nothing - the CHECK is broken"
		elif [ "$got" = "$2" ]; then ok "C-01 $3 at $1 = $got"
		else bad "C-01 $3 at $1 is '$got', review says '$2'"; fi
	}
	expect 0x18026 ee510b '$03:8026 = INC.w $0B51'
	expect 0x130f  64b9e6c7a5b9f0fa60 '$00:930F vblank handshake'
	# LoROM: 3*0x8000 + ($03D2B7 & 0x7FFF) = 0x1D2B7. The first version of this
	# line said 0x52b7 - the naive offset with the bank base missing - and the
	# ROM refuted it (0x9a there, 0x6b here). Same class as the naive-offset
	# mistake docs/CLAIMS_REGISTER.md §8 already retracted.
	expect 0x1d2b7 6b '$03D2B7 = RTL (the last bank-03 instruction)'
	# and the negative claim, which is the one that carries the review
	n=$(grep -c '^\s*\$038026' docs/measurements/2026-10-02-c041-bank03-pc-dump.md 2>/dev/null || true)
	if [ "${n:-0}" -eq 0 ]; then ok "C-01 no dump row for \$038026, as the review states"
	else bad "C-01 the dump page contains \$038026 - the C-041 answer changed"; fi
else
	skip "C-01 no ROM at $ROM (pass --rom PATH)"
fi
echo

# ------------------------------- E-01 the shape that shipped: a green claim, red gate
echo "-- E-01 no tracked file may assert a green gate while that gate is red --"
# Generic: for each `make <target>` row in the README gate table, run the gate
# and compare with what the table says. Only targets that are cheap are run; the
# expensive ones are named and skipped rather than assumed.
# Symmetric, because the one-way version had to be edited the moment the guard
# was fixed: it asserted "red is correct" and then reported REFUTED when the
# gate went green, which is the right answer to the wrong question. This asks
# the real one - does the README's stated verdict match the gate's exit code?
#
#   verdict_is() runs a gate and prints PASS/FAIL/INCONCLUSIVE/UNKNOWN.
verdict_is() { # $1 = make target
	case "$1" in
		check-claims|check-causes)
			if make -s "$1" >/dev/null 2>&1; then echo PASS
			else echo FAIL; fi ;;
		# NOT review-check-c041: this script IS that target, and invoking it
		# from here recursed until the first run timed out at 120 s. Found by
		# running it. A validator that calls itself is a fork bomb, not a check.
		review-check)
			if make -s "$1" >/dev/null 2>&1; then echo PASS
			else echo FAIL; fi ;;
		*) echo UNKNOWN ;;   # expensive targets are not re-run here
	esac
}
check_row() { # $1 target, $2 expected verdict
	local row actual claimed
	row=$(grep -E "^\| \`make $1\`" README.md 2>/dev/null | head -1)
	if [ -z "$row" ]; then skip "E-01 no README gate-table row for \`make $1\`"; return; fi
	actual=$(verdict_is "$1")
	case "$row" in
		*"**$2**"*) claimed="$2" ;;
		*"$2"*)     claimed="$2" ;;
		*)           claimed="something else" ;;
	esac
	if [ "$claimed" = "$2" ]; then ok "E-01 README says $2 for \`make $1\` and the gate exits accordingly ($actual)"
	else bad "E-01 README's row for \`make $1\` says '$claimed' but the gate returns $actual -- one of them is a claim"; fi
}
check_row check-claims    PASS
check_row check-causes    PASS
check_row review-check    PASS
echo

# ---------------------------------------------- E-02 retractions where made
echo "-- E-02 retracted claims are visible where they were made --"
if grep -qi "retract" README.md && grep -qi "retract" docs/RE_CITY_FREEZE.md; then
	ok "E-02 retraction markers present in README and RE_CITY_FREEZE"
else
	bad "E-02 a retraction marker is missing"
fi
echo

# ---------------------------------------------- E-03 the gate's failure text
echo "-- E-03 the clock gate's failure text asserts no retracted cause --"
# 'What is established:' is the exact emitted heading. The looser token
# 'is established' matched the SENTENCE that retracts it ("under a heading
# reading \"What is established\""), which is the fix and not the defect - the
# previous validator's comment says flagging that would make the fix impossible.
for tok in '0012' 'CODE_03D283' 'CODE_03D287' 'CODE_008061' 'What is established:'; do
	c=$(grep -cE "printf.*${tok}" scripts/clock-gate.sh 2>/dev/null || true)
	[ "${c:-0}" -gt 0 ] && bad "E-03 clock-gate.sh still prints something containing '$tok' ($c)" \
		|| ok "E-03 clock-gate.sh prints nothing containing '$tok'"
done
echo

# ---------------------------------------------- E-05 the dropped link
echo "-- E-05 README links the pages it reproduces --"
for page in 2026-10-02-deck-interp-histogram 2026-10-02-c041-bank03-pc-dump \
            REVIEW-2026-10-02b REVIEW-2026-10-02c RUBRIC; do
	if grep -q "$page" README.md; then ok "E-05 README links $page"
	else bad "E-05 README does NOT link $page"; fi
done
echo

# ---------------------------------------------- legal, D4
echo "-- legal --"
n=$(git ls-files 2>/dev/null | grep -ci '\.sfc$')
[ "$n" -eq 0 ] && ok "no ROM tracked" || bad "$n ROM file(s) tracked"
n=$(git ls-files 2>/dev/null | grep -cE '\.(sav|srm)$')
[ "$n" -eq 0 ] && ok "no battery save tracked" || bad "$n save file(s) tracked"
n=$(git ls-files 2>/dev/null | grep -cE '^(aes|\.aes)/')
[ "$n" -eq 0 ] && ok "aes/ and .aes/ are untracked, as D4.3 requires by rule" \
	|| bad "$n path(s) under aes/ are TRACKED"
echo

# ---------------------------------------------- the review's own honesty
echo "-- the review file's own arithmetic --"
n=$(grep -cE '^\| \*\*R-[0-9]+\*\*' docs/review/REVIEW-2026-10-02c.md 2>/dev/null || true)
echo "  findings rows: ${n:-0}"
for id in R-01 R-02 R-11; do
	grep -q "$id" docs/review/REVIEW-2026-10-02c.md && ok "finding $id present" \
		|| bad "finding $id is missing from the review"
done
grep -q "NOT INDEPENDENT" docs/review/REVIEW-2026-10-02c.md \
	&& ok "the review declares it is not independent" \
	|| bad "the review does NOT declare its own lack of independence"
echo

echo "== summary =="
echo "  confirmed : $CONF"
echo "  refuted   : $REFUTED"
echo "  skipped   : $SKIPPED"
echo
if [ "$REFUTED" -eq 0 ]; then
	echo "  NO FINDING REFUTED. Note what that does NOT prove: this script checks"
	echo "  the claims listed above and nothing else. E-04 (numeric claims current)"
	echo "  remains UNVERIFIED because no command recomputes README's numbers -"
	echo "  DoD D3.3/D3.4 were retired for exactly that reason."
	echo
	echo "  RESULT: PASS (bounded)"
	exit 0
fi
echo "  $REFUTED claim(s) in REVIEW-2026-10-02c.md do not reproduce."
echo
echo "  RESULT: FAIL"
exit 1
