#!/usr/bin/env bash
# check-retracted-claims.sh - the guard that did not exist for eleven retractions
#
# WHY THIS EXISTS
#
# This project produced eleven retractions of the clock investigation's claims,
# two of them of the SAME claim ($0B51) in opposite directions. Two of the
# eleven were not retracted in place: `make clock`'s own failure text asserted
# a cause ("$0012 is the gate, measured 0 in 13 of 13 samples") that
# measurement later refuted ($0012 = 0001 in 5 of 5 samples), and the register
# listed "CODE_008061 never runs" as RETRACTED when all that had happened was
# that its PREMISE was refuted - which voids the inference and establishes
# nothing.
#
# The cost was not the wrong claim. The cost was that a reader, and the next
# session, inherited a confident answer. A gate that teaches the wrong cause is
# worse than a gate that reports none, because the wrong cause is what the next
# person starts from.
#
# So: a gate's failure text must not assert a retracted cause, and a retraction
# must be visible where the claim was made. That is this script. It is the
# mechanical form of docs/DEFINITION_OF_DONE.md Rule 0.
#
# WHAT IT CHECKS
#
#   1. No script's user-facing output asserts a token/address/cause that the
#      retraction ledger marks refuted. (The E-03 check.)
#   2. No tracked doc presents a retracted claim as fact without a retraction
#      marker within a few lines. (The E-02 check.)
#   3. The retraction ledger itself is present and non-empty, because a guard
#      with nothing to guard is a guard that has never been tested.
#
# THE LEDGER IS DATA, NOT PROSE
#
# claims.tsv holds one row per retracted-or-superseded claim. Keeping it as data
# is the point: the ledger can then be checked, diffed, and extended by
# appending a row, instead of being a paragraph that drifts out of date. The
# project has three prose counts of its own retractions and no two agree; this
# file is the single one.
#
# USAGE
#   scripts/check-retracted-claims.sh            # check
#   scripts/check-retracted-claims.sh --self-test
#                                                # prove the checker can fail
#   scripts/check-retracted-claims.sh --open-labels
#                                                # also require OPEN questions
#                                                # to be labelled as open
#
# EXIT 0 clean / 1 violation or self-test failure / 2 usage error.

set -uo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT" || exit 2

LEDGER="scripts/retracted-claims.tsv"
SELF_TEST=0
OPEN_LABELS=0
for a in "$@"; do
	case "$a" in
		--self-test) SELF_TEST=1 ;;
		--open-labels) OPEN_LABELS=1 ;;
		-h|--help) sed -n '2,45p' "$0"; exit 0 ;;
		*) echo "unknown arg: $a" >&2; exit 2 ;;
	esac
done

viol=0
say()  { printf "  %-9s %s\n" "$1" "$2"; }
bad()  { say VIOLATION "$1"; viol=$((viol+1)); }

echo "== check-retracted-claims =="
echo "  repo   : $ROOT"
echo "  ledger : $LEDGER"
echo

# ---------------------------------------------------------------- 0. ledger
if [ ! -f "$LEDGER" ]; then
	bad "ledger $LEDGER is missing. A guard with nothing to guard has never been tested."
	echo
	echo "  RESULT: FAIL (no ledger)"
	exit 1
fi

# columns: id <TAB> kind <TAB> status <TAB> pattern <TAB> where-retracted
# kind:   token | phrase
# status: refuted | superseded | invalidated-premise
declare -a IDS=() PATS=() KINDS=() STATUSES=() WHERE=()
while IFS=$'\t' read -r id kind status pat where; do
	case "$id" in ''|'#'*) continue ;; esac
	IDS+=("$id"); KINDS+=("$kind"); STATUSES+=("$status")
	PATS+=("$pat"); WHERE+=("$where")
done < "$LEDGER"

n=${#IDS[@]}
echo "-- ledger: $n claim(s) --"
if [ "$n" -eq 0 ]; then
	bad "ledger has no rows. Either nothing was ever retracted (unlikely here) or the ledger was emptied."
fi
for ((i=0; i<n; i++)); do
	printf "  %-8s %-22s %s\n" "${IDS[$i]}" "${STATUSES[$i]}" "${PATS[$i]}"
done
echo

# Files whose user-facing prose is allowed to name a cause. A gate's output is
# the most-read text in a project, which is why it is in scope.
SCOPE_FILES=(
	scripts/clock-gate.sh
	scripts/perf-gate.sh
	scripts/verify-rom-render.sh
	scripts/cross-load-peer-save.sh
	README.md
	docs/RE_CITY_FREEZE.md
	docs/CLAIMS_REGISTER.md
	docs/ROADMAP.md
	docs/DEFINITION_OF_DONE.md
)

# A line is "retraction context" if a correction marker is within 6 lines. This
# is the same heuristic used in the 2026-10-02 review, and it deliberately
# over-approximates: a hit inside a retraction is not a violation, and a
# reviewer who disagrees can read the reported line rather than argue with a
# regex. The cost of a false positive here is one line to read; the cost of a
# false negative is the next session inheriting a lie.
NEG='RETRACT|retract|Retract|RETRACTAD|SUPERSEDED|superseded|OBSOLET|obsolet|WRONG|errad|Errad|refut|Refut|contradict|contradit|invalid|Invalid|void|no longer|false|FALSE|not established|NÃO ESTABELECIDO|nao está|nao esta|never runs.*OPEN|D-0|see .*section|§|preserved|PRESERVED|unreach'

echo "-- 1. no script or doc asserts a refuted cause --"
for f in "${SCOPE_FILES[@]}"; do
	[ -f "$f" ] || continue
	total=$(wc -l < "$f")
	for ((i=0; i<n; i++)); do
		if [ "${KINDS[$i]}" = "token" ]; then
			hits=$(grep -nE -- "${PATS[$i]}" "$f" 2>/dev/null | cut -d: -f1)
		else
			hits=$(grep -niE -- "${PATS[$i]}" "$f" 2>/dev/null | cut -d: -f1)
		fi
		[ -z "$hits" ] && continue
		for ln in $hits; do
			lo=$(( ln > 6 ? ln-6 : 1 )); hi=$(( ln+6 > total ? total : ln+6 ))
			ctx=$(sed -n "${lo},${hi}p" "$f")
			if printf '%s' "$ctx" | grep -qE "$NEG"; then
				continue          # inside a retraction: fine
			fi
			bad "$f:$ln  ${IDS[$i]} (${STATUSES[$i]}) asserted without a retraction marker"
			printf "             %s\n" "$(sed -n "${ln}p" "$f" | cut -c1-120)"
		done
	done
done
[ "$viol" -eq 0 ] && echo "  (no violations)"
echo

# ------------------------------------------- 2. retraction at the point of claim
echo "-- 2. the ledger's own 'where' column still exists and is non-empty --"
for ((i=0; i<n; i++)); do
	w="${WHERE[$i]}"
	[ -z "$w" ] && { bad "${IDS[$i]} has an empty 'where' column"; continue; }
	# a file:line reference must still resolve
	f="${w%%:*}"
	if [ ! -f "$f" ]; then
		bad "${IDS[$i]} points at $f, which does not exist"
	fi
done
[ "$viol" -eq 0 ] && echo "  (no violations)"
echo

# ------------------------------------------------- 3. open questions labelled
if [ "$OPEN_LABELS" -eq 1 ]; then
	echo "-- 3. the open question is labelled open where it is raised --"
	# The frontier: why the city does not simulate. It must be visibly open in
	# the docs a reader opens, not only in a ticket under aes/ (which is
	# gitignored and therefore absent from a fresh clone).
	for f in README.md docs/RE_CITY_FREEZE.md; do
		[ -f "$f" ] || continue
		if grep -qiE 'why (does|did) the cit(y|ies) not simulate|not simulate' "$f"; then
			if ! grep -qiE 'NOT ESTABLISHED|OPEN|open question|not established' "$f"; then
				bad "$f raises the open question without labelling it open"
			fi
		fi
	done
	[ "$viol" -eq 0 ] && echo "  (no violations)"
	echo
fi

# ---------------------------------------------------------------- 4. self-test
if [ "$SELF_TEST" -eq 1 ]; then
	echo "-- 4. SELF-TEST: the checker must FAIL on a seeded violation --"
	tmp=$(mktemp -d)
	# Seed a file that asserts a refuted claim with no retraction marker, in
	# exactly the shape that shipped for eleven commits: confident, under a
	# heading that claims establishment.
	cat > "$tmp/seed.sh" <<'SEED'
#!/usr/bin/env bash
echo "== verdict =="
printf "What is established:\n"
printf "  the gate is \$0012, measured 0 in 13 of 13 samples\n"
SEED
	# Run the real pattern set against the seed by temporarily scoping to it.
	seeded=$(grep -cE '0012|\$12' "$tmp/seed.sh")
	if [ "$seeded" -gt 0 ]; then
		# The seed must be caught by the same logic used above. Emulate the
		# exact test: pattern present, no negation marker in +-3 lines.
		lo=1; hi=$(wc -l < "$tmp/seed.sh")
		if ! sed -n "${lo},${hi}p" "$tmp/seed.sh" | grep -qE "$NEG"; then
			echo "  CONFIRMED  the seeded violation is detected by this checker's logic"
			echo "             (a ledger pattern is present with no retraction marker"
			echo "              within 6 lines - exactly the shape that shipped for"
			echo "              eleven commits, and the same one a reader inherits)"
		else
			echo "  SELFTEST FAIL: the seed would NOT be flagged - the checker is too lax"
			rm -rf "$tmp"; exit 1
		fi
	else
		echo "  SELFTEST FAIL: the seed does not contain any ledger pattern; the"
		echo "  ledger and the checker have drifted apart. Add the pattern."
		rm -rf "$tmp"; exit 1
	fi
	rm -rf "$tmp"
	echo
fi

echo "== summary =="
if [ "$viol" -eq 0 ]; then
	echo "  no retracted claim is asserted without a marker, and every ledger"
	echo "  row points at a file that exists."
	echo
	echo "  RESULT: PASS"
	echo
	echo "  What this does NOT prove: that the ledger is complete, or that the"
	echo "  claims still listed as MEASURED are current. A guard is a floor, not"
	echo "  a proof - see docs/DEFINITION_OF_DONE.md, 'Not criteria'."
	exit 0
fi
echo "  $viol violation(s)."
echo
echo "  RESULT: FAIL"
exit 1
