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
    set +e
    timeout 160 "$loader" "./wbemdisp-method-invoke-$arch.exe" > "$work/result" 2>&1
    status=$?
    set -e
    file="$out/result-$arch.txt"
    # Keep only structured probe observations. The interpreter's error
    # diagnostic can include the runner's path and is outside the comparison.
    tr -d '\r' < "$work/result" |
        sed -n -E '/^(measure|ole_init|scratch_|bind_|parse_moniker|cogetobject|invoke |cscript |vbs |measure_complete)/p' > "$file"
    printf 'probe_exit=%s\n' "$status" >> "$file"
    test "$status" -eq 0
    grep -q '^measure_complete dispatch=1$' "$file"
    test "$(grep -c '^invoke ' "$file")" -eq 26
    grep -q '^cscript kind=direct exit=' "$file"
    grep -q '^cscript kind=runtime-error exit=' "$file"
    grep -q '^vbs intentional_error_begin$' "$file"
    ! grep -q '^vbs intentional_error_unreachable$' "$file"
    grep -q '^scratch_cleanup=0$' "$file"
done
