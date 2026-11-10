#!/usr/bin/env bash
# Checks that every patch in patches/series has a test that passed on
# Windows 10 (L1) or a written reason why it cannot have one (L3 only).
# Each docs/patches/<patch>.md carries one machine-readable line:
#   <!-- winum-test: l1 -->
#   <!-- winum-test: l3-only: <reason> -->
#   <!-- winum-test: known-deviation: <reason> -->  (maintainer's exception)
#   <!-- winum-test: test-not-on-windows -->   (test exists, L1 not run)
#   <!-- winum-test: missing -->
# Only the first three pass. Usage: check-series.sh [--report]
#   --report  print the table and the count, but exit 0 (branch builds)
# Original, AI-assisted (Claude), LGPL-2.1-or-later.
set -euo pipefail
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
report=0
[ "${1:-}" = --report ] && report=1
fail=0 ok=0
while IFS= read -r p; do
    case "$p" in ''|'#'*) continue ;; esac
    name="${p%.patch}"
    page="$HERE/docs/patches/$name.md"
    if [ ! -f "$page" ]; then
        state="no page"
    else
        state="$(sed -n 's/^<!-- winum-test: \(.*\) -->$/\1/p' "$page" | head -1)"
        [ -n "$state" ] || state="no marker"
    fi
    case "$state" in
        l1|l3-only:?*|known-deviation:?*) ok=$((ok + 1)); mark=ok ;;
        *) fail=$((fail + 1)); mark=MISSING ;;
    esac
    printf '%-8s %-48s %s\n' "$mark" "$name" "$state"
done < "$HERE/patches/series"
echo "series check: $ok with L1, a reason for L3 only or a known deviation, $fail without"
if [ "$fail" -gt 0 ] && [ "$report" = 0 ]; then
    echo "every patch in patches/series needs a Windows-tested Wine test, an 'l3-only' reason or a 'known-deviation' exception (AGENTS.md)" >&2
    exit 1
fi
exit 0
