#!/usr/bin/env bash
# Original MEASURE runner, AI-assisted (Codex), LGPL-2.1-or-later.
set -euo pipefail

loader="$(realpath "$1")"
export WINESERVER="$(realpath "$2")"
bundle="$(realpath "$3")"
out="$(realpath -m "$4")"
mkdir -p "$out"
work="$(mktemp -d)"
export WINEPREFIX="$work/prefix" WINEDEBUG=-all WINEDLLOVERRIDES='mscoree,mshtml='
cleanup() {
    "$WINESERVER" -k
    "$WINESERVER" -w
    rm -rf -- "$work"
}
trap cleanup EXIT
"$loader" wineboot -i
"$WINESERVER" -w
cd "$bundle"
sha256sum -c SHA256SUMS
for arch in x86_64 i686; do
    status=0
    timeout 120 "$loader" "./combase-per-user-classes-$arch.exe" --direct-machine > "$work/result" 2>&1 || status=$?
    file="$out/result-$arch.txt"
    tr -d '\r' < "$work/result" |
        sed -n -E '/^(measure=|ole_init=|setup_error=|case=|register |hkcr |com |cleanup |machine |measure_complete=)/p' > "$file"
    printf 'probe_exit=%s\n' "$status" >> "$file"
    test "$status" -eq 0
    grep -q '^measure_complete=1 cleanup_errors=0$' "$file"
    grep -q '^machine conflict_measured=1$' "$file"
    test "$(grep -c '^case=' "$file")" -eq 5
    grep -q '^register scope=progid status=0$' "$file"
    grep -q '^register scope=treatas status=0$' "$file"
done
