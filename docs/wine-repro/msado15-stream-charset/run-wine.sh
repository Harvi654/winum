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
    timeout 160 "$loader" "./msado15-stream-charset-$arch.exe" > "$work/result" 2>&1
    status=$?
    set -e
    file="$out/result-$arch.txt"
    # Keep only structured probe observations. The interpreter's error
    # diagnostic can include the runner's path and is outside the comparison.
    tr -d '\r' < "$work/result" |
        sed -n -E '/^(measure|ole_init|scratch_|fixtures_ready|c |cscript |vbs |file )/p' > "$file"
    printf 'probe_exit=%s\n' "$status" >> "$file"
    test "$status" -eq 0
    grep -q '^measure_complete=1$' "$file"
    grep -q '^scratch_cleanup=1$' "$file"
    grep -q '^cscript exit=0$' "$file"
    grep -q '^vbs complete=1$' "$file"
    grep -q '^c create type=2 hr=0x00000000$' "$file"
    grep -q '^vbs create type=2 err=0x00000000$' "$file"
    for engine in c vbs; do
        test "$(grep -c "^$engine case=write " "$file")" -eq 6
        test "$(grep -c "^$engine case=change_at_zero " "$file")" -eq 6
        test "$(grep -c "^$engine case=load " "$file")" -eq 4
        grep -q "^$engine step=write_line_lf " "$file"
        grep -q "^$engine step=copy_binary_to_text " "$file"
    done
done
