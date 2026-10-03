#!/usr/bin/env bash
# count-aot-symbols.sh - derive the README's "how much is native" numbers, and
# print the METHOD next to them.
#
# WHY THIS EXISTS
# ---------------
# README.md's "How much of it is actually native" table carried the figure
# "distinct AOT symbols emitted into src/gen/*.c | 186" with **no recorded
# derivation**, and three people re-derived three different numbers from it:
# 186, 187 and 559. A number nobody can reproduce is not a measurement, it is a
# rumour with a decimal point. This script is the method, so the next person
# runs it instead of guessing a grep.
#
# It needs src/gen/, which is derived from your own ROM and is never committed.
# Run tools/regen.sh first, or this reports 0 and says so.
#
# WHAT THE FOUR NUMBERS ARE, AND WHY THEY DIFFER
# ----------------------------------------------
#   239  every definition symbol in src/gen/*.c whose name carries the
#        register-state suffix _MxX. THIS is "the distinct AOT symbols emitted
#        into src/gen/*.c", and it is the number the README's row should say.
#        It agrees, independently, with the count of `aot_eligible` dispositions
#        in src/gen/program_manifest.json - two different files, same number.
#
#   186  the subset of those 239 whose name was auto-derived from the program
#        counter, i.e. matching bank_NN_PCCC_MxX. It EXCLUDES 52 symbols named
#        from recomp/*.cfg func declarations (City_Update_M1X1, MainLoop_M1X1,
#        PPU_Bitpack_8EA9_M0X0, ...) and 1 that is PC-derived but lacks the
#        bank_NN_ prefix (CODE_00987B_M0X0). Quoting 186 under a label that says
#        "distinct AOT symbols" is a labelling error, not an off-by-one.
#
#   187  186 plus CODE_00987B_M0X0 - the same PC-derived subset counted with a
#        pattern that admits either prefix. **This is the off-by-one a reviewer
#        reported, and it is fully accounted for.**
#
#   559  every distinct definition symbol in src/gen/*.c, including the 202
#        named and runtime helpers that are not recompiled SNES code at all
#        (SPC_*, LC_LZ5_*, RLE_Decompress, Scenario_Decompress, ...). This is
#        what a naive "grep all function signatures" returns, and it is not a
#        count of AOT symbols.
#
# USAGE
#   scripts/count-aot-symbols.sh
#
# Exit 0 always, unless src/gen/ is missing, in which case it says so and exits 1
# rather than printing zeros that look like a measurement.

set -euo pipefail
cd "$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

if [ ! -d src/gen ] || [ -z "$(ls -A src/gen 2>/dev/null)" ]; then
  echo "ERROR: src/gen/ is empty." >&2
  echo "  It is derived from your own copy of the ROM and is never committed." >&2
  echo "  Run:  bash tools/regen.sh \"SimCity (USA).sfc\"" >&2
  echo "  Printing zeros here would look like a measurement. It is not one." >&2
  exit 1
fi

# A DEFINITION is a line that opens a body: "<type> name(args) {".
# A DECLARATION ends in ';' and is a forward declaration, not a symbol emitted.
#
# NOTE, because it cost this script its first run: use explicit [0-9] / [0-9A-F]
# character classes, NOT POSIX classes with an interval. On this grep,
# [[:digit:]]{2} matched 5 of 186 names while [0-9]{2} matched all 186 - a
# pattern that silently under-matches is worse than one that fails to compile.
gen_defs() {
  grep -hoE '^[[:alnum:]_]+[[:space:]]+\**[[:alnum:]_]+\([^;]*\)[[:space:]]*\{[[:space:]]*$' src/gen/*.c \
    | sed -E 's/^[[:alnum:]_]+[[:space:]]+\**//; s/\([^;]*\)[[:space:]]*\{[[:space:]]*$//' \
    | sort -u
}

ALL=$(gen_defs)
MX=$(printf '%s\n' "$ALL" | grep -E '_M[0-9]X[0-9]$' || true)
BANK_MX=$(printf '%s\n' "$MX" | grep -E '^bank_[0-9]{2}_[0-9A-F]{4}_M[0-9]X[0-9]$' || true)
CODE_MX=$(printf '%s\n' "$MX" | grep -E '^CODE_' || true)
CFG_MX=$(printf '%s\n' "$MX" | grep -vE '^bank_|^CODE_' || true)

printf "== distinct AOT symbols emitted into src/gen/*.c ==\n"
printf "  A. every _MxX definition                        : %s   <- the README's figure\n" "$(printf '%s\n' "$MX" | grep -c . || true)"
printf "  B. of those, bank_NN_PCCC_MxX (PC auto-named)   : %s\n" "$(printf '%s\n' "$BANK_MX" | grep -c . || true)"
printf "  C. of those, PC-derived but not bank_-prefixed  : %s   %s\n" \
  "$(printf '%s\n' "$CODE_MX" | grep -c . || true)" "$(printf '%s\n' "$CODE_MX" | tr '\n' ' ')"
printf "  D. of those, named from recomp/*.cfg            : %s\n" "$(printf '%s\n' "$CFG_MX" | grep -c . || true)"
printf "  E. every distinct definition symbol (incl. %s non-AOT helpers): %s\n" \
  "$(( $(printf '%s\n' "$ALL" | grep -c . || true) - $(printf '%s\n' "$MX" | grep -c . || true) ))" \
  "$(printf '%s\n' "$ALL" | grep -c . || true)"
printf "\n"
printf "  B + C = %s, which is what a reviewer counting 'the PC-derived ones'\n" \
  "$(( $(printf '%s\n' "$BANK_MX" | grep -c . || true) + $(printf '%s\n' "$CODE_MX" | grep -c . || true) ))"
printf "  with either prefix gets. That is the reported off-by-one.\n"
printf "\n"

# Cross-check against the manifest, which is a different file entirely.
if [ -f src/gen/program_manifest.json ]; then
  MANIFEST=$(python3 -c "
import json
m = json.load(open('src/gen/program_manifest.json'))
print(sum(1 for v in m['nodes'].values() if v['disposition'] == 'aot_eligible'))
")
  printf "== cross-check, from src/gen/program_manifest.json (a different file) ==\n"
  printf "  nodes with disposition aot_eligible             : %s\n" "$MANIFEST"
  printf "  A (from src/gen/*.c)                            : %s\n" "$(printf '%s\n' "$MX" | grep -c . || true)"
  if [ "$MANIFEST" = "$(printf '%s\n' "$MX" | grep -c . || true)" ]; then
    printf "  AGREE. Two files, one number, derived two ways.\n"
  else
    printf "  DISAGREE. Do not quote either until this is explained.\n"
    exit 1
  fi
fi
