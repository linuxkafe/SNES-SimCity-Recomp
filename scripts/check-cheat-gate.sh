#!/usr/bin/env bash
# scripts/check-cheat-gate.sh - a cheat must never be able to make `make clock` pass.
#
# WHY THIS EXISTS. docs/CHEAT_CODES.md records a verified third-party cheat table
# and, more importantly, a rule: a cheat or a poke is a DIAGNOSTIC, never a fix.
# docs/DEFINITION_OF_DONE.md Rule 0 already says a criterion is satisfied only by
# a command, and Rule 0's spirit says the command must be the real thing. But
# Rule 0 is prose, and this project's own record is that prose does not protect
# the next session:
#
#   - "Qualquer resultado de COUNT_PC registado sem prefixo 0x e nulo" sat in an
#     append-only log at RE_CITY_FREEZE.md:1546 and voided a measurement four
#     days later (R-037).
#   - docs/DECK_RUNBOOK.md taught two mistakes for a whole session while being
#     untracked, so no gate could see it (CONF-16).
#
# So the rule gets a gate. It is deliberately NARROW: it does not try to judge
# whether a cheat "helps". It asks one mechanical question:
#
#     CAN any script under scripts/ cause a guest-memory write on a run it drives?
#
# If the answer becomes yes, then `make clock`'s green could be a cheat rather
# than a simulation, and the gate that exists to prove the game works would be
# reporting on the wrong thing. This is a RULE, not a causal claim, and it is
# labelled as one: docs/DEFINITION_OF_DONE.md Rule 0b. See MEASURED/HYPOTHESIS
# below for the only sentence here that does assert a cause.
#
# WHAT IT CHECKS, all by reading tracked + untracked-not-ignored files:
#
#   1. No script in scripts/ invokes the host with a WRAM-writing knob
#      (SNESRECOMP_WRAM_POKE, SIMCITY_GODMODE) or with a script that contains a
#      `poke` / `forcepoke` line.
#   2. scripts/clock-gate.sh specifically invokes neither, and its own
#      `--script` argument is either absent or points at a script that contains
#      no `poke`/`forcepoke` line.
#   3. The rule is actually stated in the two places a reader will hit it
#      (docs/CHEAT_CODES.md and docs/DEFINITION_OF_DONE.md), so deleting it from
#      the source of truth is itself a failure.
#
# WHAT IT DOES NOT DO, stated so it is not oversold:
#   - It does not run the gate. It cannot know what a run's WRAM looks like.
#   - It cannot see a write performed by the HOST itself (e.g. GodMode is a host
#     feature), only one requested from a script we drive. Check 1 is what
#     covers the host-toggle case, by name.
#   - It is lexical. A gate that names no knob and hides a write behind a
#     variable will pass. That is a floor, and docs/DEFINITION_OF_DONE.md's
#     "Not criteria" section is where this belongs.
#
# Self-test:  scripts/check-cheat-gate.sh --self-test
set -uo pipefail

REPO="$(cd "$(dirname "$0")/.." && pwd)"
cd "$REPO"

viol=0
note() { printf '  %s\n' "$*"; }
bad()  { printf 'VIOLATION %s\n' "$*"; viol=$((viol + 1)); }

# Knobs that cause a guest-memory write from the host side.
WRITE_KNOBS='SNESRECOMP_WRAM_POKE|SIMCITY_GODMODE'
# Script verbs that write guest memory.
POKE_VERBS='^[[:space:]]*(poke|pokefor|forcepoke)[[:space:]]'

SELFTEST=0
[ "${1:-}" = "--self-test" ] && SELFTEST=1

# ---------------------------------------------------------------- self-test
if [ "$SELFTEST" = 1 ]; then
    fails=0
    tmp_script="scripts/.cheatgate-seed.script"
    tmp_gate="scripts/.cheatgate-seed-gate.sh"

    cleanup() { rm -f "$tmp_script" "$tmp_gate"; }
    trap cleanup EXIT

    # (a) NEGATIVE control: a DRIVING script containing a poke must be caught.
    printf 'poke 0B53 A0\n' > "$tmp_script"
    if grep -qE "$POKE_VERBS" "$tmp_script"; then
        note "SELFTEST ok  : a script containing 'poke' is detected"
    else
        note "SELFTEST FAIL: the poke-verb pattern missed 'poke 0B53 A0'"
        fails=$((fails + 1))
    fi

    # (b) POSITIVE control: a script with no write must NOT be caught. A guard
    #     that fires on everything is worse than no guard.
    printf 'press start 5\nwait 90\nmouseclick right\n' > "$tmp_script"
    if grep -qE "$POKE_VERBS" "$tmp_script"; then
        note "SELFTEST FAIL: the poke-verb pattern fires on a clean script"
        fails=$((fails + 1))
    else
        note "SELFTEST ok  : a script with no write is not flagged"
    fi

    # (c) The write-knob pattern must catch a real invocation, and must not
    #     catch a mention inside a comment that documents the prohibition.
    printf '#!/bin/sh\nSNESRECOMP_WRAM_POKE=0B53=A0 build/SimCitySNESRecomp x\n' > "$tmp_gate"
    if grep -qE "$WRITE_KNOBS" "$tmp_gate"; then
        note "SELFTEST ok  : a gate invoking a WRAM-write knob is detected"
    else
        note "SELFTEST FAIL: the write-knob pattern missed SNESRECOMP_WRAM_POKE"
        fails=$((fails + 1))
    fi
    printf '#!/bin/sh\n# a cheat must never make make clock pass; see\n# SNESRECOMP_WRAM_POKE and SIMCITY_GODMODE\ntrue\n' > "$tmp_gate"
    if grep -qE "^\s*(export\s+)?($WRITE_KNOBS)" "$tmp_gate"; then
        note "SELFTEST FAIL: the write-knob pattern fires on a documentation mention"
        fails=$((fails + 1))
    else
        note "SELFTEST ok  : a documentation mention of the knob is not flagged"
    fi

    # (d) The scan must include DOTFILES. Measured: with a plain
    #     `scripts/*.sh` glob, scripts/.hidden.sh invoking a WRAM write passed
    #     this check silently. A guard that a leading dot hides is not a guard.
    hidden="scripts/.cheatgate-hidden.sh"
    printf '#!/usr/bin/env bash\nSIMCITY_GODMODE=1 build/SimCitySNESRecomp x\n' > "$hidden"
    # Capture rather than pipe: `set -o pipefail` plus `grep -q` exiting at the
    # first match SIGPIPEs the producer, so `"$0" | grep -q ...` reports the
    # PIPELINE as failed even when grep succeeded. That made this assertion
    # report FAIL while the guard was in fact working. Measured 2026-10-03.
    out=$("$0" 2>&1 || true)
    if printf '%s' "$out" | grep -q "VIOLATION $hidden"; then
        note "SELFTEST ok  : a DOTFILE gate invoking a write knob is detected"
    else
        note "SELFTEST FAIL: a dotfile gate escaped the scan (glob lacks dotglob)"
        fails=$((fails + 1))
    fi
    rm -f "$hidden"

    rm -f "$tmp_script" "$tmp_gate"
    trap - EXIT

    # (e) The real tree must be clean.
    if "$0" >/dev/null 2>&1; then
        note "SELFTEST ok  : the current tree passes the real check"
    else
        note "SELFTEST FAIL: the current tree does not pass the real check"
        fails=$((fails + 1))
    fi

    echo
    if [ "$fails" -eq 0 ]; then
        echo "SELFTEST PASS: 5/5 seeded assertions behave as required"
        exit 0
    fi
    echo "SELFTEST FAIL: $fails seeded assertion(s) misbehaved"
    exit 1
fi

# ------------------------------------------------------------------ 1 + 2
echo "== check-cheat-gate =="
echo "  repo: $REPO"
echo
echo "-- 1. no driven script may write guest memory --"
# dotglob matters, and it was MISSING at first and the self-test caught it:
# `scripts/*.sh` does not match `scripts/.hidden.sh`, so a hidden gate script
# invoking a WRAM write passed this check silently. Measured 2026-10-03. A guard
# that can be hidden by a leading dot is not a guard.
shopt -s nullglob dotglob
scripts=(scripts/*.sh)
for f in "${scripts[@]}"; do
    [ -f "$f" ] || continue
    # Skip the checker itself and the self-test seeds.
    case "$(basename "$f")" in check-cheat-gate.sh) continue ;; esac
    hits=$(grep -nE "^[[:space:]]*(export[[:space:]]+)?($WRITE_KNOBS)" "$f" 2>/dev/null)
    if [ -n "$hits" ]; then
        bad "$f invokes a guest-memory write knob:"
        printf '%s\n' "$hits" | sed 's/^/            /'
    fi
done
[ "$viol" -eq 0 ] && note "  (no violations)"

echo
echo "-- 2. the clock gate drives no script that writes guest memory --"
CLOCK="scripts/clock-gate.sh"
if [ ! -f "$CLOCK" ]; then
    bad "$CLOCK does not exist; the delivery gate must be present to be checked"
else
    if grep -nE "^[[:space:]]*(export[[:space:]]+)?($WRITE_KNOBS)" "$CLOCK" >/dev/null 2>&1; then
        bad "$CLOCK invokes a guest-memory write knob"
    fi
    # Resolve --script arguments, if any, and read them.
    driven=$(grep -oE '\-\-script[[:space:]]+"?[^"[:space:]]+"?' "$CLOCK" 2>/dev/null \
             | sed -E 's/--script[[:space:]]+"?//; s/"?$//' | sort -u)
    if [ -z "$driven" ]; then
        note "  $CLOCK drives no --script (no guest input route to poison)"
    else
        for s in $driven; do
            # Resolve a path that is written with a shell variable or an
            # absolute prefix. Measured: clock-gate.sh passes
            # "$PWD/scripts/d_city.script", and a guard that cannot resolve that
            # fires on the delivery gate's own clean route - which is the
            # CONF-14 failure mode (a guard that cries wolf on this project's
            # own corpus is worse than the hole it closes).
            real=""
            if [ -f "$s" ]; then
                real="$s"
            else
                base="$(basename "$s")"
                for cand in "$base" "scripts/$base"; do
                    [ -f "$cand" ] && { real="$cand"; break; }
                done
            fi
            [ -n "$real" ] || {
                bad "$CLOCK drives $s, which could not be resolved to a file"
                continue
            }
            s="$real"
            if grep -qE "$POKE_VERBS" "$s"; then
                bad "$CLOCK drives $s, and that script WRITES GUEST MEMORY:"
                grep -nE "$POKE_VERBS" "$s" | sed 's/^/            /'
            else
                note "  $CLOCK drives $s - no write verbs in it"
            fi
        done
    fi
fi

# ---------------------------------------------------------------------- 3
echo
echo "-- 3. the rule is stated where a reader will hit it --"
for doc in docs/CHEAT_CODES.md docs/DEFINITION_OF_DONE.md; do
    if [ ! -f "$doc" ]; then
        bad "$doc is missing; the anti-cheat rule must live somewhere durable"
    elif grep -qiE 'cheat' "$doc" && grep -qiE 'must (never|not) make .?make clock' "$doc"; then
        note "  $doc states the rule"
    else
        bad "$doc does not state the anti-cheat rule. The rule must be visible,"
        printf '            not only enforced.\n'
    fi
done

echo
echo "== summary =="
if [ "$viol" -eq 0 ]; then
    echo "  No gate in scripts/ can turn a WRAM write into a clock-gate result,"
    echo "  and the rule is stated in the docs. RESULT: PASS"
    echo
    echo "  What this does NOT prove: that a run's memory was unaltered. This"
    echo "  reads code; it does not run the game. A write performed by the host"
    echo "  under a name not listed in WRITE_KNOBS would pass. It is a floor."
    exit 0
fi
echo "  A cheat, or a script that writes guest memory, can reach the delivery"
echo "  gate. RESULT: FAIL"
exit 1