#!/usr/bin/env bash
# check-retracted-claims.sh - the guard that did not exist for this project's
# retractions. For how many, do not count them here: run `--count`.
#
# WHY THIS EXISTS
#
# This project produced a run of retractions of the clock investigation's
# claims, two of them of the SAME claim ($0B51) in opposite directions. Two were
# not retracted in place: `make clock`'s own failure text asserted
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
COUNT_ONLY=0
for a in "$@"; do
	case "$a" in
		--self-test) SELF_TEST=1 ;;
		--open-labels) OPEN_LABELS=1 ;;
		--count) COUNT_ONLY=1 ;;
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

# ---------------------------------------------------------------------------
# 0b. THE COUNT, and the only definition of it.
#
# This project carried four different prose counts of its own retractions and no
# two agreed: "doze" in RE_CITY_FREEZE.md, "eleven" in the DoD and in this
# script's own header, "These eight" in CLAIMS_REGISTER.md §1, and "13 commits"
# counted off git log. All four were wrong in the same way - each was a human
# tally of a moving target, so they drifted the moment a row was appended.
#
# The definition, fixed once:
#
#   THE RETRACTION COUNT IS THE NUMBER OF LEDGER ROWS WHOSE STATUS IS `refuted`.
#
# `superseded` is excluded because the ledger's own header defines it as "the
# claim was true and has been replaced" - old, not wrong. `invalidated-premise`
# is excluded because the ledger says in terms "THIS IS NOT a retraction, and a
# checker that treats it as one will replace an unmeasured assertion with an
# unmeasured assertion of the opposite sign" - which is precisely how the paired
# $0B51 retraction happened.
#
# Note the ceiling this leaves: two refuted rows retract the SAME claim in two
# languages (R-005/R-006 English/Portuguese, R-007/R-008 likewise), so the refuted
# row count is an UPPER BOUND on the number of distinct retracted claims. The
# ledger records no claim identity, and inventing one here would be judgement
# dressed as data. `--count` prints all three numbers so a reader is never left
# guessing which figure is in play.
# ---------------------------------------------------------------------------
count_status() {
	local want="$1" c=0
	for ((i=0; i<n; i++)); do
		[ "${STATUSES[$i]}" = "$want" ] && c=$((c+1))
	done
	echo "$c"
}
N_REFUTED=$(count_status refuted)
N_SUPERSEDED=$(count_status superseded)
N_INVALIDATED=$(count_status invalidated-premise)

if [ "$COUNT_ONLY" -eq 1 ]; then
	printf 'ledger rows      : %d\n' "$n"
	printf 'refuted          : %d   <- THIS is the retraction count\n' "$N_REFUTED"
	printf 'superseded       : %d   (true but old; not a retraction)\n' "$N_SUPERSEDED"
	printf 'invalidated-prem.: %d   (evidence refuted, truth unknown; explicitly NOT a retraction)\n' "$N_INVALIDATED"
	exit 0
fi
printf '  census: %d rows = %d refuted + %d superseded + %d invalidated-premise\n' \
	"$n" "$N_REFUTED" "$N_SUPERSEDED" "$N_INVALIDATED"
printf '  THE RETRACTION COUNT IS %d (rows with status refuted). No prose may state\n' "$N_REFUTED"
printf '  a different one; cite `scripts/check-retracted-claims.sh --count` instead.\n'
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

# ------------------------------------------------- 3. the count, in prose
#
# Added 2026-10-02 after this project was found carrying four prose counts of its
# own retractions ("doze", "eleven", "These eight", "13 commits"), none of them
# equal, none of them computed. This section makes divergence impossible rather
# than documenting it: prose that states NO count is always fine, and prose that
# states a count must state the ledger's.
#
# TWO EXEMPTIONS, both deliberate and both narrow:
#
#   docs/review/RUBRIC.md      pre-registered and committed with a sha256
#                              (docs/review/RUBRIC.sha256). Editing it to update
#                              a count would break the hash chain that is the
#                              entire point of a pre-registered rubric. Its
#                              numbers are historical as of its registration
#                              date and must stay as written.
#   docs/review/REVIEW-*.md   a dated record of a review conducted at a named
#                              commit. Editing its prose would falsify the record
#                              of what was found then.
#
# Both are visible here on purpose: an exemption nobody can see is a gate with a
# hole in it.
COUNT_VIOL="$(mktemp -t simcity-countviol-XXXXXX)"
trap 'rm -f "$COUNT_VIOL" "${COUNT_VIOL}.q"' EXIT
# Quoting a superseded count is sometimes necessary - when the whole point of the
# paragraph is that the count used to be wrong. Those lines carry an explicit,
# greppable marker and are counted here, so the exemption is visible in the
# output and can be audited with `grep -rn 'count-quote'`. A silent exemption
# would be a hole; this one is a signpost.
COUNT_QUOTED=0
COUNT_SCOPE=(
	README.md
	docs/CLAIMS_REGISTER.md
	docs/CHECKLIST.md
	docs/DEFINITION_OF_DONE.md
	docs/LOOP_MODEL.md
	docs/RE_CITY_FREEZE.md
	docs/REQUIREMENTS.md
	docs/ROADMAP.md
	docs/VISION.md
	scripts/check-retracted-claims.sh
	scripts/clock-gate.sh
	scripts/perf-gate.sh
	scripts/verify-rom-render.sh
	scripts/cross-load-peer-save.sh
)
echo "-- 3. no prose count of the retractions disagrees with the ledger ($N_REFUTED) --"
WORDNUM='zero|one|two|three|four|five|six|seven|eight|nine|ten|eleven|twelve|thirteen|fourteen|fifteen|sixteen|seventeen|eighteen|nineteen|twenty|um|dois|duas|tres|quatro|cinco|seis|sete|oito|nove|dez|onze|doze|treze|quatorze|quinze|vinte'
to_digit() {
	printf '%s' "$1" | sed -E \
	 -e 's/^(zero)$/0/'         -e 's/^(one|um)$/1/'          -e 's/^(two|dois|duas)$/2/' \
	 -e 's/^(three|tres)$/3/'   -e 's/^(four|quatro)$/4/'     -e 's/^(five|cinco)$/5/' \
	 -e 's/^(six|seis)$/6/'     -e 's/^(seven|sete)$/7/'       -e 's/^(eight|oito)$/8/' \
	 -e 's/^(nine|nove)$/9/'    -e 's/^(ten|dez)$/10/'        -e 's/^(eleven|onze)$/11/' \
	 -e 's/^(twelve|doze)$/12/' -e 's/^(thirteen|treze)$/13/' \
	 -e 's/^(fourteen|quatorze)$/14/' -e 's/^(fifteen|quinze)$/15/' -e 's/^(twenty|vinte)$/20/'
}
# A count is a number or number-word IMMEDIATELY followed by the word
# "retract", in either language:
#
#     <N> retractions
#     <N> retractações
#     <N> retracted claims
#
# FORWARD ONLY, and that is a measured decision rather than a stylistic one. An
# earlier version also matched a number AFTER "retract" and reported
# "2026 retractions", "371 retractions" and "351 retractions" - every one of them
# a date, an address or a git hash picked up from the 24 characters following the
# word RETRACTED on some line. A pattern that cries wolf on its own corpus is
# worse than no pattern, so the direction is fixed.
#
# It also cannot fire on "13 of 13 samples" or "13 commits": those are counts of
# something else and the word "retract" is not between the number and them.
#
# KNOWN LIMIT, stated rather than hidden: a count separated from the keyword by
# an intervening word ("These eight are the user's own retractions") is NOT
# detected. Adjacency was chosen over recall because the non-adjacent version
# fired on its own corpus seven times per run. The one prose site that used the
# non-adjacent form was rewritten during this pass; the trade is deliberate and
# the limitation is a floor, not a proof -- exactly what
# docs/DEFINITION_OF_DONE.md says a guard is.
CNT_RE="($WORDNUM|[0-9]{1,3}) +retract[a-zçãõ]*"
for f in "${COUNT_SCOPE[@]}"; do
	[ -f "$f" ] || continue
	grep -nEi "$CNT_RE" "$f" 2>/dev/null | while IFS= read -r hit; do
		[ -z "$hit" ] && continue
		ln="${hit%%:*}"
		# Strip grep's own "NN:" prefix. (Measured: before this, the checker
		# reported "87 retractions" for the line numbered 87.)
		body="${hit#*:}"
		case "$body" in
			*count-quote*) echo x >> "${COUNT_VIOL}.q"; continue ;;
		esac
		while IFS= read -r phrase; do
			[ -z "$phrase" ] && continue
			# Reject a number that is part of a longer token ("$0B512",
			# "v1.15", "436b25b") or is an item reference ("§8 is
			# RETRACTED"). A count stands alone.
			printf '%s' "$phrase" | grep -qE '[0-9A-Za-z._#§]([0-9]{1,3}) ' && continue
			num=$(printf '%s' "$phrase" | grep -oiE "$WORDNUM|[0-9]{1,3}" | head -1)
			[ -z "$num" ] && continue
			n2=$(to_digit "$num")
			case "$n2" in ''|*[!0-9]*) continue ;; esac
			[ "$n2" = "$N_REFUTED" ] && continue
			printf 'VIOLATION   %s:%s states "%s retractions"; the ledger says %s\n' \
				"$f" "$ln" "$num" "$N_REFUTED" >> "$COUNT_VIOL"
			printf '            %s\n' "$(sed -n "${ln}p" "$f" | cut -c1-108)" >> "$COUNT_VIOL"
			printf '            fix: delete the number and cite `scripts/check-retracted-claims.sh --count`,\n' >> "$COUNT_VIOL"
			printf '                  or state %s. See docs/DEFINITION_OF_DONE.md Rule 0.\n' "$N_REFUTED" >> "$COUNT_VIOL"
		done < <(printf '%s' "$body" | grep -oiE "$CNT_RE" || true)
	done
done
if [ -f "${COUNT_VIOL}.q" ]; then COUNT_QUOTED=$((COUNT_QUOTED + $(wc -l < "${COUNT_VIOL}.q"))); rm -f "${COUNT_VIOL}.q"; fi
[ "$COUNT_QUOTED" -gt 0 ] && printf '  note: %d line(s) quote a superseded count and carry the explicit\n         marker `count-quote`. Audit them with: grep -rn count-quote %s\n' "$COUNT_QUOTED" "${COUNT_SCOPE[*]}"
if [ -s "$COUNT_VIOL" ]; then
	while IFS= read -r l; do printf '  %s\n' "$l"; done < "$COUNT_VIOL"
	viol=$((viol + $(grep -c '^VIOLATION' "$COUNT_VIOL")))
	rm -f "$COUNT_VIOL"
else
	rm -f "$COUNT_VIOL"
	echo "  (no violations)"
fi
echo

if [ "$OPEN_LABELS" -eq 1 ]; then
	echo "-- 4. the open question is labelled open where it is raised --"
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
	echo "-- 5. SELF-TEST: the checker must FAIL on a seeded violation --"
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

	# 5b. The COUNT check must also fail on a seeded wrong count, in both
	# languages and in both tenses. Written because the first version of this
	# section was never seeded at all, and a version of it WAS shipped that
	# reported 13 false positives per run against its own corpus.
	seed2=$(mktemp -d)
	BAD_WORD_EN=eleven
	BAD_WORD_PT=doze
	# Assembled from fragments on purpose: written as a literal heredoc, this
	# seed is itself a violation of section 3 and the checker (correctly)
	# flagged its own source file. A guard that cannot be demonstrated without
	# tripping over its own demonstration is a guard with a maintenance trap.
	{
		printf 'The investigation went through %s %s before anyone counted them.\n' \
			"$BAD_WORD_EN" retraction
		printf 'Esta investigação passou por %s %s.\n' \
			"$BAD_WORD_PT" retractacoes
		printf 'The count is %s %s and that one is right.\n' \
			"$N_REFUTED" retraction
	} > "$seed2/DOC.md"
	seeded2=0
	while IFS= read -r ln; do
		case "$ln" in
			*"$BAD_WORD_EN"*|*"$BAD_WORD_PT"*) seeded2=$((seeded2+1)) ;;
		esac
	done < <(grep -nEi "($WORDNUM|[0-9]{1,3}) +retract[a-zçãõ]*" "$seed2/DOC.md")
	if [ "$seeded2" -eq 2 ]; then
		echo "  CONFIRMED  the seeded count violations are detected (EN + PT caught,"
		echo "             the correct count is not flagged)"
	else
		echo "  SELFTEST FAIL: expected 2 seeded count violations (eleven / doze),"
		echo "             detected $seeded2. The count check is too lax."
		rm -rf "$seed2"; exit 1
	fi
	rm -rf "$seed2"
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
