#!/usr/bin/env bash
# validate-findings-2026-10-02b.sh - validate the findings in REVIEW-2026-10-02b.md
# against this tree, by someone other than the author.
#
# Run it, record the output, and the review's findings become confirmed by the
# protocol's own standard. Until then they are the author's assertions - which is
# the same status every finding in this project has ever had.
#
#   docs/review/validate-findings-2026-10-02b.sh
#
# Each check is a COMMAND, not a judgement. A check that cannot fail is not a
# check, so every block below prints FAIL on the condition it exists to detect
# and this script exits non-zero if any of them does.

set -uo pipefail
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$ROOT" || exit 2
bad=0
say() { printf "  %-6s %s\n" "$1" "$2"; }
ok()  { say "ok" "$1"; }
no()  { say "FAIL" "$1"; bad=$((bad+1)); }

echo "== validate REVIEW-2026-10-02b.md =="
echo "  repo: $ROOT"
echo "  NOTE: a green run here means the findings are CONFIRMED AS PRESENT."
echo "        Findings that are present and confirmed still have to be FIXED."
echo

# --- the rubric must be unmodified, or nothing below is measured against it ---
echo "-- rubric integrity --"
if sha256sum -c docs/review/RUBRIC.sha256 >/dev/null 2>&1; then
  ok "docs/review/RUBRIC.md matches its committed sha256"
else
  no "the rubric does NOT match its hash - a review against a changed rubric is invalid"
fi
echo

# --- R-01: make test reports PASSED for a test that did not run ---
echo "-- R-01  make test reports PASSED with no ROM (C-04, C-05, G-01, E-05) --"
# It must be run from a directory with NO ROM. The reviewer's first version of this
# check ran it with the repository root as cwd - where a ROM IS present - so it
# never reached the skip path and reported a verdict for the wrong reason.
if [ -x tests/test_deterministic_replay ] || [ -f build/test_deterministic_replay ]; then
  bin=$(cd "$(dirname "$(ls build/test_deterministic_replay tests/test_deterministic_replay 2>/dev/null | head -1)")" && pwd)/$(basename "$(ls build/test_deterministic_replay tests/test_deterministic_replay 2>/dev/null | head -1)")
  tmp=$(mktemp -d); out=$(cd "$tmp" && env -u SIMCITY_ROM "$bin" 2>&1); rc=$?; rm -rf "$tmp"
  if printf '%s' "$out" | grep -q "SKIP: no ROM" && [ "$rc" -eq 0 ]; then
    no "CONFIRMED: with no ROM reachable the test prints SKIP and returns 0."
    say "" "ctest renders that as Passed, so 'make test' exits 0 having run nothing."
    say "" "closure: exit non-zero on skip, or set SKIP_RETURN_CODE in CMakeLists."
  elif printf '%s' "$out" | grep -q "SKIP: no ROM"; then
    ok "the test exits non-zero on a skip (rc=$rc)"
  else
    ok "no skip path reached (rc=$rc)"
  fi
  if grep -q SKIP_RETURN_CODE CMakeLists.txt; then
    ok "CMakeLists.txt sets SKIP_RETURN_CODE"
  else
    say "note" "CMakeLists.txt has no SKIP_RETURN_CODE, so a skip renders as Passed"
  fi
else
  say "skip" "no test binary built; run 'make build' first"
fi
echo

# --- R-02: README asserts an invalidated-premise inference as a conclusion ---
echo "-- R-02  README asserts 'INC.w \$0B51 executes zero times' (E-01, E-02) --"
hits=$(grep -nEi "executes zero times|never reached after the city loads" README.md || true)
if [ -n "$hits" ]; then
  marked=0
  while IFS= read -r h; do
    ln="${h%%:*}"
    lo=$(( ln>2 ? ln-2 : 1 )); hi=$(( ln+2 ))
    if sed -n "${lo},${hi}p" README.md | grep -qiE "retract|OPEN|invalidat|void premise|not established"; then
      marked=$((marked+1))
    fi
  done <<< "$hits"
  total=$(printf '%s\n' "$hits" | grep -c .)
  if [ "$marked" -eq "$total" ]; then
    ok "every occurrence carries a retraction/open marker ($marked/$total)"
  else
    no "CONFIRMED: $((total-marked)) of $total occurrences are asserted without a marker"
  fi
else
  ok "README no longer states the claim"
fi
echo

# --- R-03: README states the refuted bank-03 claim ---
echo "-- R-03  README says bank 03 does not run (E-01, E-02, G-03) --"
hits=$(grep -n "bank-03 tick" README.md || true)
if [ -n "$hits" ]; then
  marked=0; total=0
  while IFS= read -r h; do
    ln="${h%%:*}"; total=$((total+1))
    # Window is ONE line each side, and the marker must NAME this claim. The
    # reviewer's first version accepted the bare word "false" within 4 lines -
    # and README line 75 says "measured false" about the $0012 chain, forty
    # lines away. A window that catches a DIFFERENT retraction is a false
    # negative wearing a green tick.
    lo=$(( ln>1 ? ln-1 : 1 )); hi=$(( ln+1 ))
    sed -n "${lo},${hi}p" README.md | grep -qiE "retract|515,?043|f3301|does not run.*(false|wrong)|not \*\*does not run\*\*" && marked=$((marked+1))
  done <<< "$hits"
  [ "$marked" -eq "$total" ] && ok "occurrence(s) carry a marker" \
    || no "CONFIRMED: the bank-03 claim is asserted without a retraction marker"
else
  ok "README no longer asserts it"
fi
echo

# --- R-04: falsified prescription, unmarked, outside the guard's scope ---
echo "-- R-04  force_lle 0x009311 asserted present; file outside guard scope --"
if grep -rq "force_lle 0x009311" recomp/ 2>/dev/null; then
  no "recomp/ CONTAINS force_lle 0x009311 - the docs saying it was removed are now wrong"
else
  ok "recomp/ has no force_lle 0x009311, so the docs' 'removed' claim is correct"
fi
if grep -q "force_lle 0x009311" docs/RE_SCENARIO_NAV.md 2>/dev/null; then
  ln=$(grep -n "force_lle 0x009311" docs/RE_SCENARIO_NAV.md | head -1 | cut -d: -f1)
  lo=$(( ln>4 ? ln-4 : 1 )); hi=$(( ln+4 ))
  if sed -n "${lo},${hi}p" docs/RE_SCENARIO_NAV.md | grep -qiE "retract|removed|436b25b"; then
    ok "RE_SCENARIO_NAV.md carries a retraction marker"
  else
    no "CONFIRMED: RE_SCENARIO_NAV.md asserts it with no retraction marker (line $ln)"
  fi
else
  ok "RE_SCENARIO_NAV.md no longer mentions it"
fi
if grep -q "RE_SCENARIO_NAV" scripts/check-retracted-claims.sh; then
  ok "RE_SCENARIO_NAV.md is inside the guard's scope"
else
  no "CONFIRMED: RE_SCENARIO_NAV.md is NOT in the guard's SCOPE_FILES"
fi
echo

# --- R-05: the render gate's healthy figure ---
echo "-- R-05  verify-rom-render.sh states 206 where 257 was measured (E-04) --"
if grep -qn "206" scripts/verify-rom-render.sh; then
  no "CONFIRMED: the stale figure 206 is still in scripts/verify-rom-render.sh"
else
  ok "the stale figure is gone"
fi
echo

# --- R-06: make perf agrees with itself ---
echo "-- R-06  make perf straddles its own threshold (G-01) --"
say "note" "not checked here: it needs the ROM and ~5 runs. Recorded in the review"
say ""     "as 48.38 FAIL / 51.52 PASS / 46.99 FAIL against a threshold of 50."
echo

# --- R-07: README points at the files that carry the current position ---
echo "-- R-07  README links to the current-position files (E-02) --"
n=$(grep -cE "CAUSE_CLAIMS|CONFLICTS|measurements/" README.md || true)
if [ "${n:-0}" -ge 3 ]; then ok "README links $n times"; else no "only $n link(s); need >= 3"; fi
echo

# --- R-09: CLAIMS_REGISTER ticks a row its own section 14 calls false ---
echo "-- R-09  CLAIMS_REGISTER section 9 contradicts section 14 (E-04) --"
if grep -q "exclude_range 0x930D 0x9318" docs/CLAIMS_REGISTER.md; then
  s9=$(grep -n "exclude_range 0x930D 0x9318" docs/CLAIMS_REGISTER.md | head -1 | cut -d: -f1)
  lo=$(( s9>4 ? s9-4 : 1)); hi=$(( s9+6 ))
  if sed -n "${lo},${hi}p" docs/CLAIMS_REGISTER.md | grep -qiE "superseded|false|corrected|retracted|0x130D"; then
    ok "the row carries its correction inline"
  else
    no "CONFIRMED: the section 9 row is ticked with no correction near it (line $s9)"
  fi
else
  ok "the stale row is gone"
fi
echo

# --- R-10: cross-load-peer-save.sh still has no dead-end banner ---
echo "-- R-10 (RETRACTED finding)  the banner exists; the register is what is stale --"
if [ -f scripts/cross-load-peer-save.sh ]; then
  if sed -n '1,25p' scripts/cross-load-peer-save.sh | grep -qiE "impossible|dead end|do not run|cannot work|does not carry"; then
    ok "the first 20 lines DO state the disproof - review finding R-10 was wrong"
  else
    no "no dead-end banner in the first 20 lines (R-10 would stand)"
  fi
else
  ok "the script is deleted"
fi
if sed -n '1,40p' docs/CLAIMS_REGISTER.md | grep -qiE "no dead-end banner|no deprecation note"; then
  no "CONFIRMED: CLAIMS_REGISTER section 4 still says the script has no banner"
fi
echo

# R-12: the relative-ROM defect
echo "-- R-12  find_rom returns a relative path the emulator cannot open (C-04) --"
if [ -x build/test_deterministic_replay ]; then
  out=$(env -u SIMCITY_ROM ./build/test_deterministic_replay 2>&1); rc=$?
  if [ "$rc" -ne 0 ] && printf '%s' "$out" | grep -q "exited with code"; then
    no "CONFIRMED: invoked with the repo root as cwd the test fails (rc=$rc)"
    say "" "find_rom returned a relative ROM path and the host chdir'd away from it"
  else
    ok "no relative-path failure (rc=$rc)"
  fi
else
  say "skip" "no test binary built"
fi
echo

# --- always true, and that is the point ---
echo "-- permanent rules --"
[ "$(git ls-files | grep -ci 'sfc$')" -eq 0 ] && ok "no ROM tracked" || no "a ROM is tracked"
[ "$(git ls-files | grep -cE '^(aes|\.aes)/')" -eq 0 ] && ok "aes/ and .aes/ are untracked" \
  || no "something under aes/ is TRACKED - DoD D4.3 is a permanent rule"
echo

echo "== summary =="
if [ "$bad" -eq 0 ]; then
  echo "  Every finding checked here is either already closed or not present."
  echo "  THIS IS NOT A PASS FOR THE PROJECT. A confirmed finding is still a"
  echo "  defect; this script reports presence, and the verdict lives in"
  echo "  docs/review/REVIEW-2026-10-02b.md."
  echo
  echo "  RESULT: no findings reproduced"
  exit 0
fi
echo "  $bad finding(s) reproduced against the current tree."
echo
echo "  RESULT: findings present - the review's verdict stands"
exit 1
