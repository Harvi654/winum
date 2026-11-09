#!/usr/bin/env bash
# Original, AI-assisted (Codex), LGPL-2.1-or-later.
# Build the exact candidate test body, with our standalone assertion harness.
set -euo pipefail
[[ $# == 1 ]] || { echo 'Usage: build-l1-typedesc-offsets.sh NEW_OUTPUT' >&2; exit 2; }
here="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
python3 - "$here" <<'PY'
from pathlib import Path
import sys
root = Path(sys.argv[1])
patch = (root / 'patches/candidates/oleaut32-typedesc-z-offsets-v2/oleaut32-typedesc-z-offsets.patch').read_text()
added = '\n'.join(line[1:] for line in patch.splitlines() if line.startswith('+') and not line.startswith('+++'))
start = added.index('/* BEGIN winum record offset test:')
end = added.index('/* END winum record offset test */', start) + len('/* END winum record offset test */')
body = (root / 'docs/wine-repro/typedesc-offsets/test-body.h').read_text()
if added[start:end] + '\n' != body:
    sys.exit('Candidate test and standalone test body differ')
PY
list="$(mktemp)"
trap 'rm -f -- "$list"' EXIT
printf '%s\n' 'tdoffset-records-l1 | docs/wine-repro/typedesc-offsets/l1.c | -loleaut32 -lole32 -luuid -static' > "$list"
bash "$here/scripts/build-measure-bundle.sh" "$1" "$list"
cp "$here/docs/wine-repro/typedesc-offsets/"{l1.c,test-body.h} "$here/LICENSE" "$1/"
(cd "$1" && sha256sum ./*.exe ./*.c ./*.h LICENSE README.txt run-measure.bat measure.manifest > SHA256SUMS)
