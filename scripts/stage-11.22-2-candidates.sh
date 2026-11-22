#!/usr/bin/env bash
# Build a private patch list for this test run.
set -euo pipefail
repo="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
out="${1:?New patch staging directory required}"
mkdir -- "$out"
cp -- "$repo"/patches/{series,*.patch} "$out/"
for candidate in ucrtbase-cxx-exception-size server-cross-thread-killtimer; do
    cp -- "$repo/patches/candidates/$candidate/$candidate.patch" "$out/"
    printf '%s.patch\n' "$candidate" >> "$out/series"
done
