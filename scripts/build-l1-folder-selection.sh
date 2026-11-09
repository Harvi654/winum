#!/usr/bin/env bash
# Original, AI-assisted (Codex), LGPL-2.1-or-later.
# Build the complete candidate shell folder-view test body for Windows.
set -euo pipefail
[[ $# == 1 ]] || { echo 'Usage: build-l1-folder-selection.sh NEW_OUTPUT' >&2; exit 2; }
here="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
python3 - "$here" <<'PY'
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
PY
list="$(mktemp)"
trap 'rm -f -- "$list"' EXIT
printf '%s\n' 'folder-selection-l1 | docs/wine-repro/shell32-folderview-selection/l1.c | -Wextra -lshell32 -lole32 -luuid -static' > "$list"
bash "$here/scripts/build-measure-bundle.sh" "$1" "$list"
cp "$here/docs/wine-repro/shell32-folderview-selection/"{l1.c,test-body.h} "$here/LICENSE" "$1/"
(cd "$1" && sha256sum ./*.exe ./*.c ./*.h LICENSE README.txt run-measure.bat measure.manifest > SHA256SUMS)
