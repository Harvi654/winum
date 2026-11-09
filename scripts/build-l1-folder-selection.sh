#!/usr/bin/env bash
# Original, AI-assisted (Codex), LGPL-2.1-or-later.
# Build the complete candidate shell folder-view test body for Windows.
set -euo pipefail
[[ $# == 1 || $# == 2 ]] || { echo 'Usage: build-l1-folder-selection.sh NEW_OUTPUT [BUNDLE_LIST]' >&2; exit 2; }
here="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
entry='folder-selection-l1 | docs/wine-repro/shell32-folderview-selection/l1.c | -Wextra -lshell32 -lole32 -luuid -static'
if [[ $# == 2 ]]; then
    list="$(realpath -e -- "$2")"
    grep -Fqx -- "$entry" "$list" || { echo 'Bundle list must contain the exact folder-selection L1 entry' >&2; exit 2; }
else
    list="$(mktemp)"
    trap 'rm -f -- "$list"' EXIT
    printf '%s\n' "$entry" > "$list"
fi
python3 - "$here" "$list" <<'PY'
from pathlib import Path
import sys
root = Path(sys.argv[1])
patch = (root / 'patches/candidates/shell32-folderview-selection-v2/shell32-folderview-selection.patch').read_text()
unit_diff = patch.split('diff --git a/dlls/shell32/tests/selection.c b/dlls/shell32/tests/selection.c\n', 1)[1]
unit = '\n'.join(line[1:] for line in unit_diff.splitlines() if line.startswith('+') and not line.startswith('+++')) + '\n'
body = (root / 'docs/wine-repro/shell32-folderview-selection/test-body.h').read_text()
start = 'struct selection_host '
end = unit.index('\nSTART_TEST(selection)\n')
if unit[unit.index(start):end] != body[body.index(start):]:
    sys.exit('Candidate folder-selection test and standalone body differ')
# The combined follow-up also rechecks the corrected offsets test's parity.
for line in Path(sys.argv[2]).read_text().splitlines():
    if line.split('|', 1)[0].strip() != 'tdoffset-records-l1':
        continue
    expected = 'tdoffset-records-l1 | docs/wine-repro/typedesc-offsets/l1.c | -loleaut32 -lole32 -luuid -static'
    if line.strip() != expected:
        sys.exit('Bundle list must use the exact offsets L1 entry')
    patch = (root / 'patches/candidates/oleaut32-typedesc-z-offsets-v2/oleaut32-typedesc-z-offsets.patch').read_text()
    added = '\n'.join(line[1:] for line in patch.splitlines() if line.startswith('+') and not line.startswith('+++'))
    start = added.index('/* BEGIN winum record offset test:')
    end = added.index('/* END winum record offset test */', start) + len('/* END winum record offset test */')
    if added[start:end] + '\n' != (root / 'docs/wine-repro/typedesc-offsets/test-body.h').read_text():
        sys.exit('Candidate offsets test and standalone body differ')
PY
bash "$here/scripts/build-measure-bundle.sh" "$1" "$list"
cp "$here/docs/wine-repro/shell32-folderview-selection/"{l1.c,test-body.h} "$here/LICENSE" "$1/"
if grep -q '^[[:space:]]*tdoffset-records-l1[[:space:]]*|' "$list"; then
    mkdir "$1/typedesc-offsets-source"
    cp "$here/docs/wine-repro/typedesc-offsets/"{l1.c,test-body.h} "$1/typedesc-offsets-source/"
fi
(cd "$1" && {
    bundle_files=(./*.exe ./*.c ./*.h LICENSE README.txt run-measure.bat measure.manifest)
    if [[ -d typedesc-offsets-source ]]; then
        bundle_files+=(typedesc-offsets-source/*.c typedesc-offsets-source/*.h)
    fi
    sha256sum "${bundle_files[@]}" > SHA256SUMS
})
