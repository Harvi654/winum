#!/usr/bin/env bash
# Original, AI-assisted (Codex), LGPL-2.1-or-later.
# Build the exact candidate signature test; PA19 decoder coverage is absent.
set -euo pipefail
[[ $# == 1 ]] || { echo 'Usage: build-l1-mspatcha-signature.sh NEW_OUTPUT' >&2; exit 2; }
here="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
python3 - "$here" <<'PY'
from pathlib import Path
import sys
root = Path(sys.argv[1])
patch = (root / 'patches/candidates/mspatcha-signature-v2/mspatcha-pa19-32bit-and-large-files.patch').read_text()
unit_diff = patch.split('diff --git a/dlls/mspatcha/tests/signature.c b/dlls/mspatcha/tests/signature.c\n', 1)[1]
unit = '\n'.join(line[1:] for line in unit_diff.splitlines() if line.startswith('+') and not line.startswith('+++')) + '\n'
body = (root / 'docs/wine-repro/mspatcha-pa19/test-body.h').read_text()
start = 'typedef BOOL (WINAPI *signature_buffer_fn)'
end = unit.index('\nSTART_TEST(signature)\n')
if unit[unit.index(start):end] != body[body.index(start):]:
    sys.exit('Candidate test and standalone test body differ')
PY
list="$(mktemp)"
trap 'rm -f -- "$list"' EXIT
printf '%s\n' 'mspatcha-signature-l1 | docs/wine-repro/mspatcha-pa19/l1.c | -Wextra -static' > "$list"
bash "$here/scripts/build-measure-bundle.sh" "$1" "$list"
cp "$here/docs/wine-repro/mspatcha-pa19/"{l1.c,test-body.h} "$here/LICENSE" "$1/"
(cd "$1" && sha256sum ./*.exe ./*.c ./*.h LICENSE README.txt run-measure.bat measure.manifest > SHA256SUMS)
