#!/usr/bin/env bash
# check-cause-claims.sh - catch a CAUSE asserted with no measurement behind it
#
# WHY THIS EXISTS, AND WHY IT IS NOT check-retracted-claims.sh
#
# `check-retracted-claims.sh` is a LEXICAL guard: it holds a list of literal
# strings that were refuted, and it fails when one of them appears without a
# retraction marker nearby. That design has one structural weakness and this
# script is built around it:
#
#   IT ONLY SEES STRINGS SOMEONE ALREADY WROTE DOWN.
#
# A refuted claim that was never added to the ledger is invisible to it, forever.
# The ledger grew to 22 refuted rows over this project's life, one retraction at
# a time, and between those rows the guard was blind. Four of the five retracted
# claims this session found in tracked files (RE_CITY_FREEZE, README, two agent
# prompts) were caught by *reading*, not by running anything.
#
# So this guard inverts the question. Instead of "is this known-false string
# being asserted?", it asks:
#
#   DOES THIS LINE ASSERT A CAUSE? AND IF SO, DOES IT SAY WHERE THE CAUSE CAME
#   FROM?
#
# The second half is the enforceable one, and it is the part that could not have
# been faked by paraphrasing.
#
# WHAT IT MATCHES
#
# A line is a candidate if it contains a CAUSE cue ("the cause is", "root cause",
# "caused by", "it is because", "therefore", "which is why", "proves that") near a
# CLOCK noun (clock, city, simulat*, vblank, NMI, token, freeze, deadlock, frame,
# month, date, tick). The intersection is deliberate: "because SDL3 enables XTEST"
# is a claim about the build system and is not in scope.
#
# WHAT MAKES IT CATCH WHAT THE OTHER ONE MISSED
#
# 1. NORMALISATION. Before matching, the text is stripped of backslash escapes,
#    markdown emphasis, backticks, and Unicode quotes/dashes. The earlier guard
#    missed `printf "the gate is \$0012"` - a shell-escaped form - because the
#    literal in the ledger did not match the literal in the script. `\$0012`,
#    `$0012` and `` `$0012` `` now all normalise to `$0012`.
# 2. IT NEEDS NO LEDGER ROW. A brand-new false cause, never retracted because
#    nobody noticed it yet, is still caught. This is the whole point.
# 3. THE PROOF IS A MARKER, NOT A MATCH. "Measured", "OPEN", "NOT ESTABLISHED",
#    "hypothesis", "inferred", "RETRACT" all satisfy it. A cause claim with no
#    provenance in its own neighbourhood fails.
#
# WHAT IT DOES NOT DO, stated so it is not oversold
#
# It does not judge whether a cause is TRUE. It cannot; nothing lexical can. It
# checks that a causal assertion is labelled, which is the only part that is
# mechanically checkable. A confidently wrong cause that carries the word
# "measured" still passes, and that is a real limit, not a solved problem.
#
# EXIT 0 clean / 1 violation / 2 usage error.

set -uo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT" || exit 2

SELF_TEST=0
for a in "$@"; do
	case "$a" in
		--self-test) SELF_TEST=1 ;;
		-h|--help) sed -n '2,40p' "$0" | sed -n '/^# /p' | sed 's/^# //'; exit 0 ;;
		*) echo "unknown arg: $a" >&2; exit 2 ;;
	esac
done

# Every tracked markdown file and every user-facing gate script. Same derived
# scope as check-retracted-claims.sh, same two documented exclusions (a hashed
# rubric and dated review records must not be edited).
SCOPE=()
while IFS= read -r f; do
	case "$f" in docs/review/RUBRIC.md|docs/review/REVIEW-*.md) continue ;; esac
	SCOPE+=("$f")
done < <(git ls-files '*.md' 'scripts/*.sh' 2>/dev/null)

# Both patterns were tightened after they fired on real text. The failures are
# recorded because a guard that cries wolf is worse than no guard, and this one
# cried twice before it was right:
#   * CLOCK_NOUN matched "frame" inside "framework change"  -> word forms only
#   * "turns out to be" matched "if the containment turns out to be needed",
#     which asserts nothing about the clock at all                  -> cue removed
CAUSE_CUE='root cause|the cause is|cause of|caused by|it is because|the reason (is|it is)|therefore|which is why|because of|proves? (that|the)|is what (stops|blocks|prevents)|the fault is'
CLOCK_NOUN='clock|cit(y|ies)|simulat|vblank|nmi|token|freez|deadlock|frame (boundary|counter|rate)|per frame|each frame|month|date|tick|gate'
MARKER='RETRACT|retract|OPEN|open question|NOT ESTABLISHED|not established|INFERRED|inferred|MEASURED|measured|hypothesis|HYPOTHESIS|unverified|CLAIMED|superseded|SUPERSEDED|correlat|no cause|does not (establish|prove)|void'

viol=0
note() { printf "  %-9s %s\n" "$1" "$2"; }

echo "== check-cause-claims =="
echo "  repo  : $ROOT"
echo "  scope : ${#SCOPE[@]} tracked file(s)"
echo

# normalise: strip escapes, emphasis, backticks, unicode punctuation
norm() { sed -e 's/\\//g' -e 's/[*_`]//g' -e 's/[“”„‟]/"/g' -e 's/[‘’‚‛]/'"'"'/g' -e 's/[–—―]/-/g'; }

check_one() {  # $1 = file, $2 = label for messages
	local f="$1" tag="$2"
	[ -f "$f" ] || return 0
	local tmp; tmp=$(mktemp)
	norm < "$f" > "$tmp"
	local total; total=$(wc -l < "$tmp")
	local ln n
	while IFS= read -r ln; do
		[ -z "$ln" ] && continue
		n="${ln%%:*}"
		local body="${ln#*:}"
		# the line must pair a CAUSE cue with a CLOCK noun
		printf '%s' "$body" | grep -qiE "$CAUSE_CUE" || continue
		printf '%s' "$body" | grep -qiE "$CLOCK_NOUN" || continue
		# ...and carry provenance within 5 lines either side
		local lo=$(( n > 5 ? n - 5 : 1 )) hi=$(( n + 5 > total ? total : n + 5 ))
		if sed -n "${lo},${hi}p" "$tmp" | grep -qiE "$MARKER"; then
			continue
		fi
		viol=$((viol + 1))
		note VIOLATION "$tag:$n  causal assertion with no provenance marker within 5 lines"
		note "" "$(printf '%s' "$body" | cut -c1-96)"
	done < <(grep -nEi "$CAUSE_CUE" "$tmp" 2>/dev/null)
	rm -f "$tmp"
}

echo "-- 1. every causal assertion is labelled --"
for f in "${SCOPE[@]}"; do check_one "$f" "$f"; done
# Frozen here so the self-test can compare "the current tree" against "the seeded
# historical tree" without the seed's own hits contaminating the comparison. The
# first version of this self-test made exactly that mistake and reported its own
# seed as a failure of the current tree.
VIOL_CURRENT=$viol
if [ "$viol" -eq 0 ]; then note "" "(no violations)"; fi
echo

if [ "$SELF_TEST" -eq 1 ]; then
	echo "-- 2. SELF-TEST against the REAL tree this audit was handed --"
	echo
	echo "A synthetic seed proves a regex fires. It does not prove the guard fires"
	echo "on the thing it was written for. So this seeds from GIT HISTORY - the tree"
	echo "at 9624f0e, the commit immediately before this audit began - and asserts"
	echo "BOTH directions:"
	echo
	echo "   (a) the guard fires on that tree, and"
	echo "   (b) the guard is clean on the tree as it stands now."
	echo
	echo "Direction (b) matters as much as (a). A guard that fires on everything is"
	echo "not a guard, and one that was tuned until it passed the current tree"
	echo "without ever being shown the failing tree has proved nothing."
	echo
	seed=$(mktemp -d)
	seeded=0
	# Reset the counter so only the seeded hits are counted from here on.
	viol=0
	for f in README.md docs/RE_CITY_FREEZE.md docs/CLAIMS_REGISTER.md; do
		mkdir -p "$seed/$(dirname "$f")"
		if git show "9624f0e:$f" > "$seed/$f" 2>/dev/null; then
			seeded=$((seeded+1))
		else
			rm -f "$seed/$f"
		fi
	done
	if [ "$seeded" -eq 0 ]; then
		echo "  SELFTEST FAIL: cannot read the tree at 9624f0e from git history."
		echo "  The guard would be untested against its own target, which is exactly"
		echo "  the state the previous guard shipped in. Refusing to pass."
		rm -rf "$seed"; exit 1
	fi
	before=$viol
	for f in README.md docs/RE_CITY_FREEZE.md docs/CLAIMS_REGISTER.md; do
		[ -f "$seed/$f" ] && check_one "$seed/$f" "9624f0e:$f"
	done
	fired=$((viol - before))
	echo "  files seeded from 9624f0e          : $seeded"
	echo "  unlabelled causal assertions there  : $fired"
	if [ "$fired" -eq 0 ]; then
		echo
		echo "  SELFTEST FAIL: the guard does NOT fire on the tree this audit was"
		echo "  handed. It would have been useless for the failure it was written for,"
		echo "  and it must not be committed in that state."
		rm -rf "$seed"; exit 1
	fi
	if [ "$VIOL_CURRENT" -ne 0 ]; then
		echo
		echo "  SELFTEST FAIL: the guard fires on the CURRENT tree as well as the old"
		echo "  one. A guard that fires on everything is not a guard."
		rm -rf "$seed"; exit 1
	fi
	rm -rf "$seed"
	echo
	echo "  CONFIRMED  fires on the tree at 9624f0e ($fired assertions), clean on this"
	echo "             one. It has been seen to fail, which is the only way to know it"
	echo "             works."
	echo
	# The seed's hits must not decide this run's verdict. The summary below
	# reports the CURRENT tree only - otherwise `--self-test` would always exit
	# non-zero for the right reason and the wrong one.
	viol=$VIOL_CURRENT
fi

echo "== summary =="
if [ "$viol" -eq 0 ]; then
	echo "  Every causal assertion in scope carries provenance: a measurement, a"
	echo "  retraction, an OPEN label, or an explicit 'inferred'."
	echo
	echo "  What this does NOT prove: that any labelled cause is TRUE. A confident"
	echo "  wrong claim that says 'measured' passes this guard. It checks labelling,"
	echo "  not truth, and no lexical guard can do better. See the header."
	echo
	echo "  RESULT: PASS"
	exit 0
fi
echo "  $viol unlabelled causal assertion(s)."
echo
echo "  RESULT: FAIL"
exit 1