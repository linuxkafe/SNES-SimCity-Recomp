#!/bin/sh
# Verify Implementation — checks acceptance criteria for a ticket
# Usage: scripts/verify-implementation.sh [--allow-unverified] [TICKET_ID]
# If no ticket ID given, reads current_ticket from aes/kanban.md
#
# CONF-8 (docs/CONFLICTS.md): this gate used to print "N passed" and exit 0 while
# having verified nothing — a ticked [x] short-circuited to pass() and an
# unverifiable criterion called skip(), which never incremented FAIL. A ticked box
# is a claim, not a measurement, so [x] is now verified on its own merits and an
# unverifiable criterion is reported as UNVERIFIED and turns the gate red.
# --allow-unverified restores the old lenient behaviour, deliberately and visibly.

set -e

case "${1:-}" in
  --help|-h)
    echo "Usage: $(basename "$0") [--allow-unverified] [TICKET_ID]"
    echo ""
    echo "Checks acceptance criteria for a ticket."
    echo "If no ticket ID given, reads current_ticket from aes/kanban.md"
    echo ""
    echo "Exit code:"
    echo "  0  every criterion was mechanically verified"
    echo "  1  a criterion failed, or a criterion could not be verified"
    echo "  2  the ticket states no acceptance criteria — nothing was checked"
    echo ""
    echo "--allow-unverified tolerates unverifiable criteria and reports 0 instead"
    echo "of 1. It does not turn exit 2 into a pass."
    exit 0
    ;;
esac

SCRIPT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
FAIL=0
COUNT=0

log() { printf "  %s\n" "$*"; }
pass() { COUNT=$((COUNT + 1)); log "✅ $1"; }
fail() { COUNT=$((COUNT + 1)); FAIL=$((FAIL + 1)); log "❌ $1"; }
skip() { COUNT=$((COUNT + 1)); log "⏭️  $1"; }
# An unverifiable criterion is NOT a passed criterion. CONF-8: this used to call
# skip(), which incremented COUNT but never FAIL, so the gate exited 0 while
# reporting "N passed" for criteria it had not checked. DoD Rule 0: a command
# exiting 0 is satisfaction; an unchecked box is not.
unverified() { COUNT=$((COUNT + 1)); UNVERIFIED=$((UNVERIFIED + 1)); log "⚠️  UNVERIFIED — not counted as passed: $1"; }
bail() { log "ERROR: $1"; exit 1; }

# Exit 0 requires every criterion to have been mechanically verified. Pass
# --allow-unverified to restore the old lenient behaviour deliberately.
ALLOW_UNVERIFIED=0
if [ "${1:-}" = "--allow-unverified" ]; then
  ALLOW_UNVERIFIED=1
  shift
fi
UNVERIFIED=0

# ── Resolve ticket ID ──────────────────────────────────────────────────────────
TICKET="${1:-}"
if [ -z "$TICKET" ]; then
  if [ -f "$SCRIPT_DIR/aes/kanban.md" ]; then
    TICKET=$(grep "^current_ticket:" "$SCRIPT_DIR/aes/kanban.md" | awk '{print $2}')
  fi
  if [ -z "$TICKET" ]; then
    bail "No ticket ID given and no current_ticket in aes/kanban.md"
  fi
fi

NAME_FILE=$(find "$SCRIPT_DIR/aes/tickets" -name "${TICKET}-*.md" 2>/dev/null | head -1)
if [ ! -f "$NAME_FILE" ]; then
  bail "Ticket file not found for $TICKET in aes/tickets/"
fi

echo ""
echo "══════════════════════════════════════════════════"
echo "  Verification Gate — $TICKET"
echo "  File: $(basename "$NAME_FILE")"
echo "══════════════════════════════════════════════════"
echo ""

# ── Extract acceptance criteria ───────────────────────────────────────────────
# Lines matching "- [ ] " or "- [x] " after "Acceptance Criteria" section
IN_SECTION=0
CRITERIA_FILE=$(mktemp)
trap 'rm -f "$CRITERIA_FILE"' EXIT

while IFS= read -r line; do
  case "$line" in
    *"Acceptance Criteria"*) IN_SECTION=1; continue ;;
    "##"*) [ "$IN_SECTION" = "1" ] && break ;;
  esac
  if [ "$IN_SECTION" = "1" ]; then
    case "$line" in
      *"- ["*"] "*) printf "%s\n" "$line" >> "$CRITERIA_FILE" ;;
    esac
  fi
done < "$NAME_FILE"

if [ ! -s "$CRITERIA_FILE" ]; then
  # 124 of 159 project tickets state no acceptance criteria at all, so this path is
  # the common case, not an edge case. It used to print "0 passed, 0 failed,
  # 0 total" and exit 0 — a green gate that had verified nothing. Exit 2 keeps the
  # three outcomes distinguishable: 0 = all verified, 1 = failed or unverified,
  # 2 = nothing to verify.
  skip "No acceptance criteria found in $(basename "$NAME_FILE")"
  echo ""
  echo "────────────────────────────────────────────────────"
  echo "  Result: NOTHING TO VERIFY — this ticket states no acceptance criteria."
  echo "  This is not a pass. It is the absence of anything to check."
  echo "────────────────────────────────────────────────────"
  exit 2
fi

# ── Verify each criterion ──────────────────────────────────────────────────────
while IFS= read -r line; do
  [ -z "$line" ] && continue
  checked=$(echo "$line" | sed -n 's/.*\[\(.\)\].*/\1/p')
  text=$(echo "$line" | sed 's/.*\] //')

  # CONF-8: a ticked box is a claim, not a measurement. It used to short-circuit
  # to pass("(already checked)") with no verification at all. Now the box only
  # annotates the result; the criterion is verified on its own merits below.
  WAS_TICKED=0
  case "$checked" in
    "x") WAS_TICKED=1 ;;
  esac

  result=""
  evidence=""

  # Extract backtick-quoted path (file or command)
  file=$(echo "$text" | sed -n 's/.*`\([^`]*\)`.*/\1/p')

  # 1) File existence check
  #    The trigger used to require the literal phrases "script exists"/"file exists"/
  #    "exists at", none of which occur in the ordinary criterion form
  #    "`some/path` exists". The result was that a MISSING file fell through to the
  #    generic fallback and was reported as unverifiable rather than failed — the
  #    gate could not tell "absent" from "not checkable". Match the form too.
  if echo "$text" | grep -iqE "script exists|file exists|hook exists|exists at|exists\`?$|\`.*\` +exists"; then
    if [ -n "$file" ] && [ -f "$SCRIPT_DIR/$file" ]; then
      result="pass"; evidence="exists at $file"
    elif [ -n "$file" ]; then
      result="fail"; evidence="NOT FOUND at $file"
    fi
  fi

  # 2) Executable check
  if [ -z "$result" ] && echo "$text" | grep -iq "executable\|is executable"; then
    if [ -n "$file" ] && [ -x "$SCRIPT_DIR/$file" ]; then
      result="pass"; evidence="$file is executable"
    elif [ -n "$file" ]; then
      result="fail"; evidence="$file NOT executable or not found"
    fi
  fi

  # 3) Command outputs usage with --help
  if [ -z "$result" ] && echo "$text" | grep -iq -- "--help\|shows usage"; then
    if [ -n "$file" ]; then
      if bash -c "$file --help" >/dev/null 2>&1; then
        result="pass"; evidence="'$file --help' exits 0"
      else
        result="fail"; evidence="'$file --help' FAILED"
      fi
    fi
  fi

  # 4) File content / grep based
  if [ -z "$result" ] && echo "$text" | grep -iq "contains\|prints\|outputs\|shows"; then
    grep_term=$(echo "$text" | sed -n "s/.*['\"]\([^'\"]*\)['\"].*/\1/p")
    if [ -n "$grep_term" ] && [ -n "$file" ]; then
      if grep -q "$grep_term" "$SCRIPT_DIR/$file" 2>/dev/null; then
        result="pass"; evidence="'$grep_term' found in $file"
      else
        result="fail"; evidence="'$grep_term' NOT found in $file"
      fi
    fi
  fi

  # 5) Makefile target exists
  if [ -z "$result" ] && echo "$text" | grep -iq "make.*target\|target exists"; then
    target=$(echo "$text" | sed 's/.*make \([a-zA-Z0-9_-]*\).*/\1/')
    if [ -n "$target" ] && grep -q "^${target}:" "$SCRIPT_DIR/Makefile" 2>/dev/null; then
      result="pass"; evidence="make $target target exists"
    elif [ -n "$target" ]; then
      result="fail"; evidence="make $target target NOT found"
    fi
  fi

  # 6) Command exits with 0
  if [ -z "$result" ] && echo "$text" | grep -iq "exits 0\|exits with\|passes\|all.*pass"; then
    if [ -n "$file" ]; then
      if bash -c "$file" >/dev/null 2>&1; then
        result="pass"; evidence="'$file' exits 0"
      else
        result="fail"; evidence="'$file' FAILED"
      fi
    fi
  fi

  # 7) Fallback: file path mentioned and it exists
  if [ -z "$result" ] && [ -n "$file" ] && [ -f "$SCRIPT_DIR/$file" ]; then
    if echo "$file" | grep -q '[.]'; then
      result="pass"; evidence="found: $file"
    fi
  fi

  case "$result" in
    pass) [ "$WAS_TICKED" = 1 ] && pass "[re-verified] $text — $evidence" || pass "$text — $evidence" ;;
    fail) [ "$WAS_TICKED" = 1 ] && fail "[ticked but NOT verified] $text — $evidence" || fail "$text — $evidence" ;;
    *)    unverified "$text" ;;
  esac
done < "$CRITERIA_FILE"

VERIFIED=$((COUNT - FAIL - UNVERIFIED))
echo ""
echo "────────────────────────────────────────────────────"
printf "  Result: %d verified, %d failed, %d unverified, %d total\n" \
  "$VERIFIED" "$FAIL" "$UNVERIFIED" "$COUNT"
echo "────────────────────────────────────────────────────"

if [ "$FAIL" -gt 0 ]; then
  exit 1
elif [ "$UNVERIFIED" -gt 0 ] && [ "$ALLOW_UNVERIFIED" -eq 0 ]; then
  log "Gate is RED: $UNVERIFIED criterion(s) could not be mechanically verified."
  log "An unverified criterion is not a satisfied criterion (DoD Rule 0)."
  log "Verify them by hand and re-run, or pass --allow-unverified to accept them."
  exit 1
fi
exit 0
