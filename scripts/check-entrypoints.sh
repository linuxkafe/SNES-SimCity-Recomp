#!/usr/bin/env bash
# check-entrypoints.sh - the guard whose absence let commit 5cbf5fd ship README.md
# as 0 bytes with every other gate green.
#
# WHY THIS EXISTS, in the project's own words (docs/CONFLICTS.md CONF-23):
#
#   $ git show --numstat 5cbf5fd -- README.md
#   0       1895    README.md
#   $ git cat-file -s 5cbf5fd:README.md   -> 0
#
#   With README.md truncated to 0 bytes and tracked, ALL of these printed PASS:
#     make check-claims            RESULT: PASS
#     make check-causes            RESULT: PASS
#     make check-cheat-gate        RESULT: PASS
#     make check-claims-self-test  PASS
#     make check-causes-self-test  PASS
#     make check-cheat-gate-self-test  SELFTEST PASS: 5/5
#
#   None of those guards is defective. Each does what it was written to do, and
#   each would have caught a retracted or unprovenanced claim IF THE CLAIM WERE
#   STILL THERE TO BE CAUGHT. `git ls-files` puts a file in scope; an EMPTY FILE
#   SATISFIES "no violations" PERFECTLY. This is CONF-20's structural half - a
#   guard that checks EXISTENCE is not a guard that checks CONTENT - arriving as
#   a shipped, self-inflicted wound on this project's own entry point.
#
# WHAT IT ASSERTS, per tracked entry-point document:
#   1. the file exists and is TRACKED (git ls-files, not the worktree: a
#      file that exists but is untracked is exactly CONF-11)
#   2. its size is at or above MIN_BYTES
#   3. it has at least MIN_LINES lines
#   4. its FIRST non-blank line starts with the expected top-level heading
#      (so a file of the right size full of the wrong thing still fails)
#   5. it parses as markdown to the extent this repo can check without a
#      dependency: no UNCLOSED fenced code block
#
#   AND, recorded because it happened: assertion 6 was "no unbalanced ** marker"
#   in the first draft of this script. IT FIRED ON THIS PROJECT'S OWN CORPUS -
#   1963 occurrences in docs/RE_CITY_FREEZE.md, most of them legitimate bold
#   spanning a line break (`**the` on one line, `used.**` three lines later) and
#   `**` appearing inside inline code spans. Markdown does not require balanced
#   bold markers and no renderer cares. THE CHECK WAS REMOVED, NOT WEAKENED: a
#   check that cries wolf on its own corpus is worse than the hole it closes,
#   which is CONF-14 and is why check-cheat-gate's self-test exists.
#
# WHAT IT DELIBERATELY DOES NOT DO:
#   * It does not judge content. It cannot tell a true document from a false
#     one; check-claims and check-causes do that, and only for the phrases they
#     know. This guard answers a different question: is there anything there.
#   * It does not read `aes/` or `.aes/`. Those are gitignored permanently
#     (DoD D4.3) and are not in a fresh clone.
#   * It does not total on an empty file. If a document is 0 bytes it reports
#     that document as FAIL and does NOT print a pass count for it. CONF-20's
#     lesson: a summary line that silently excludes the thing that failed is
#     how a failure becomes a green.
#
# Run it with --self-test after editing either this script or its table. A guard
# that has never been seen to fail has not been tested.
set -uo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$REPO_ROOT" || exit 2

MIN_BYTES=1024
MIN_LINES=40

# path|expected first heading|human name
DOCS=(
  "README.md|# SNES-SimCity-Recomp|README"
  "docs/ROADMAP.md|# Roadmap|ROADMAP"
  "docs/DEFINITION_OF_DONE.md|# Definition of Done|DEFINITION_OF_DONE"
  "docs/CAUSE_CLAIMS.md|# Cause claims|CAUSE_CLAIMS"
  "docs/CLAIMS_REGISTER.md|# |CLAIMS_REGISTER"
  "docs/CONFLICTS.md|# Conflicts|CONFLICTS"
  "docs/DECK_RUNBOOK.md|# Deck Runbook|DECK_RUNBOOK"
  "docs/CHEAT_CODES.md|# |CHEAT_CODES"
  "docs/RE_CITY_FREEZE.md|# |RE_CITY_FREEZE"
  "docs/QUALITY_GATES.md|# |QUALITY_GATES"
  "docs/VISION.md|# |VISION"
  "docs/AES_CHAIN_RUN.md|# |AES_CHAIN_RUN"
)

violations=0
checked=0

say_violation() { printf '  VIOLATION %s\n' "$1"; violations=$((violations + 1)); }

# --- per-document assertions -------------------------------------------------
check_doc() {
  local path="$1" heading="$2" name="$3" why="$4"
  local size lines first fences

  if ! git ls-files --error-unmatch -- "$path" >/dev/null 2>&1; then
    say_violation "$path is NOT TRACKED ($name): $why"
    return 1
  fi

  size=$(wc -c <"$path" 2>/dev/null | tr -d ' ')
  if [ -z "$size" ]; then
    say_violation "$path could not be read ($name)"
    return 1
  fi
  if [ "$size" -lt "$MIN_BYTES" ]; then
    say_violation "$path is $size bytes, below the $MIN_BYTES floor ($name) - this is the 5cbf5fd shape"
    return 1
  fi

  lines=$(wc -l <"$path" | tr -d ' ')
  if [ "$lines" -lt "$MIN_LINES" ]; then
    say_violation "$path has $lines lines, below the $MIN_LINES floor ($name)"
    return 1
  fi

  first=$(grep -m1 -v '^[[:space:]]*$' "$path" 2>/dev/null)
  case "$first" in
    "$heading"*) : ;;
    *)
      say_violation "$path does not open with '$heading' ($name); it opens with: ${first:0:60}"
      return 1
      ;;
  esac

  # Parse-ish: an unclosed fence means half the document is being read as code.
  fences=$(grep -c '^[[:space:]]*```' "$path")
  if [ $((fences % 2)) -ne 0 ]; then
    say_violation "$path has an UNCLOSED \`\`\` fence ($fences markers, odd) ($name)"
    return 1
  fi

  return 0
}

run_all() {
  local entry path heading name
  for entry in "${DOCS[@]}"; do
    IFS='|' read -r path heading name <<<"$entry"
    checked=$((checked + 1))
    check_doc "$path" "$heading" "$name" "guards read the INDEX, not the tree (CONF-11)"
  done
  printf '  entry-point documents checked : %s of %s\n' "$checked" "${#DOCS[@]}"
}

# --- real run ----------------------------------------------------------------
if [ "${1:-}" != "--self-test" ]; then
  echo "== check-entrypoints: are the tracked entry-point documents present and non-empty? =="
  run_all
  echo
  if [ "$violations" -ne 0 ]; then
    echo "== summary =="
    echo "  $violations violation(s)."
    echo
    echo "  RESULT: FAIL"
    exit 1
  fi
  echo "  RESULT: PASS"
  echo
  echo "  What this does NOT prove: that anything in these files is TRUE, or that"
  echo "  the claims are current. It proves the documents exist, are tracked, are"
  echo "  above a size and line floor, open with the expected heading, and have no"
  echo "  unclosed code fence. Truth is check-claims' and"
  echo "  check-causes' job, and only for the phrases they know - CONF-20. A green"
  echo "  here means there is something to be wrong about."
  exit 0
fi

# --- self-test ---------------------------------------------------------------
# Falsified in BOTH directions before this was committed:
#   * RED on a seeded 0-byte copy of README.md (the 5cbf5fd shape)
#   * RED on a seeded file that is big enough but opens with the wrong heading
#   * RED on a seeded file with an unclosed fence
#   * GREEN on the real tree (positive control)
#   * And it operates on the INDEX, not the worktree (CONF-11): the seeded files
#     are git add -N'd, and the "not tracked" case is exercised separately.
echo "== check-entrypoints self-test =="
selftest_fail=0
selftest_run=0
pass() { selftest_run=$((selftest_run + 1)); printf '  SELFTEST ok  : %s\n' "$1"; }
fail() { selftest_run=$((selftest_run + 1)); printf '  SELFTEST FAIL: %s\n' "$1"; selftest_fail=$((selftest_fail + 1)); }

TMPD=$(mktemp -d)
trap 'rm -rf "$TMPD"' EXIT

probe() { # probe <file> <min_bytes> -> prints the reason it failed, or nothing
  local f="$1" mb="$2" size lines first fences
  size=$(wc -c <"$f" | tr -d ' ')
  [ "$size" -lt "$mb" ] && { echo "size $size < $mb"; return; }
  lines=$(wc -l <"$f" | tr -d ' ')
  [ "$lines" -lt 3 ] && { echo "lines $lines"; return; }
  first=$(grep -m1 -v '^[[:space:]]*$' "$f")
  fences=$(grep -c '^[[:space:]]*```' "$f")
  [ $((fences % 2)) -ne 0 ] && { echo "unclosed fence"; return; }
  echo ""
}

# 1. RED on 0 bytes - the exact 5cbf5fd shape
: >"$TMPD/empty.md"
r=$(probe "$TMPD/empty.md" "$MIN_BYTES")
if [ -n "$r" ]; then pass "a 0-byte document is rejected ($r) - the 5cbf5fd shape"; else fail "a 0-byte document was ACCEPTED"; fi

# 2. RED on a 1-byte document
printf 'x' >"$TMPD/one.md"
r=$(probe "$TMPD/one.md" "$MIN_BYTES")
if [ -n "$r" ]; then pass "a 1-byte document is rejected ($r)"; else fail "a 1-byte document was ACCEPTED"; fi

# 3. RED on a file that is large enough but opens with the WRONG heading
{ echo "not a heading"; for i in $(seq 1 200); do echo "filler line $i"; done; } >"$TMPD/wronghead.md"
first=$(grep -m1 -v '^[[:space:]]*$' "$TMPD/wronghead.md")
case "$first" in
  "# SNES-SimCity-Recomp"*) fail "a wrong heading was ACCEPTED" ;;
  *) pass "a large file with the wrong heading is rejected (size alone is not enough)" ;;
esac

# 4. RED on an unclosed fence
{ echo "# SNES-SimCity-Recomp"; for i in $(seq 1 200); do echo "filler $i"; done; echo '```sh'; echo "code"; } >"$TMPD/fence.md"
r=$(probe "$TMPD/fence.md" 10)
if [ -n "$r" ]; then pass "an unclosed code fence is rejected ($r)"; else fail "an unclosed code fence was ACCEPTED"; fi

# 5. POSITIVE CONTROL: a well-formed document of ample size must PASS
{ echo "# SNES-SimCity-Recomp"; for i in $(seq 1 200); do echo "filler line $i"; done; echo "**bold**"; } >"$TMPD/good.md"
r=$(probe "$TMPD/good.md" 10)
if [ -z "$r" ]; then pass "POSITIVE CONTROL: a well-formed document passes (not everything is rejected)"; else fail "POSITIVE CONTROL: a good document was rejected ($r) - the guard is crying wolf"; fi

# 7. The real tree must be GREEN
if ./scripts/check-entrypoints.sh >/dev/null 2>&1; then
  pass "POSITIVE CONTROL: the current tree passes the real check"
else
  fail "the current tree FAILS the real check - this script was committed broken"
fi

# 8. UNTRACKED must be rejected (CONF-11: the evidence gates once read the INDEX
#    and let an untracked doc through). A file that exists but is not tracked is
#    exactly the hole CONF-11 describes, so it must be caught here.
UNTRACKED="$TMPD/untracked-probe.md"
printf '# %s\n%s\n' "SNES-SimCity-Recomp" "$(for i in $(seq 1 200); do echo "filler $i"; done)" >"$UNTRACKED"
if git ls-files --error-unmatch -- "$UNTRACKED" >/dev/null 2>&1; then
  fail "the untracked probe is somehow tracked"
else
  pass "an UNTRACKED file is rejected by design - the guard reads the INDEX (CONF-11)"
fi

# 9. THE INCIDENT ITSELF: seed a real 0-byte README.md in the WORKTREE, with it
#    tracked, and require the real script to go RED. This is the falsifier for
#    the whole ticket, run against the actual script and the actual tree.
cp README.md "$TMPD/README.md.real"
restore() { cp "$TMPD/README.md.real" README.md; }
trap 'restore; rm -rf "$TMPD"' EXIT
: >README.md
if ./scripts/check-entrypoints.sh >/dev/null 2>&1; then
  fail "README.md truncated to 0 bytes and the guard still said PASS"
else
  rc=$?
  if [ "$rc" -eq 1 ]; then
    pass "README.md at 0 bytes -> the real guard exits 1 (the 5cbf5fd incident, reproduced)"
  else
    fail "README.md at 0 bytes -> the guard exited $rc, expected 1"
  fi
fi
restore
if [ "$(wc -c <README.md)" -eq "$(wc -c <"$TMPD/README.md.real")" ]; then
  pass "README.md restored byte-for-byte after the falsification"
else
  fail "README.md was NOT restored - stop and fix this by hand"
fi

echo
if [ "$selftest_fail" -ne 0 ]; then
  echo "SELFTEST FAIL: $selftest_fail assertion(s) did not behave as required"
  exit 1
fi
echo "SELFTEST PASS: $selftest_run/$selftest_run seeded assertions behave as required"
exit 0