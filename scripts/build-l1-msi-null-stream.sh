#!/usr/bin/env bash
# Original, AI-assisted (Codex), LGPL-2.1-or-later.
# Build the exact Wine candidate function with our native assertion harness.
set -euo pipefail
[[ $# == 1 ]] || { echo 'Usage: build-l1-msi-null-stream.sh NEW_OUTPUT' >&2; exit 2; }
here="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
python3 - "$here" <<'PY'
from pathlib import Path
import sys
root = Path(sys.argv[1])
patch = (root / 'patches/candidates/msi-transform-null-stream-v2/msi-transform-null-stream.patch').read_text()
added = '\n'.join(line[1:] for line in patch.splitlines() if line.startswith('+') and not line.startswith('+++'))
start = added.index('static BOOL null_transform_hr(HRESULT hr, const char *operation)\n')
end = added.index('\n}', added.index('static void test_transform_null_stream(void)\n')) + len('\n}')
body = (root / 'docs/wine-repro/msi-transform-null-stream/test-body.h').read_text()
if added[start:end] + '\n' != body:
    sys.exit('Candidate test and standalone test body differ')
PY
list="$(mktemp)"
trap 'rm -f -- "$list"' EXIT
printf '%s\n' 'msi-null-stream-l1 | docs/wine-repro/msi-transform-null-stream/l1.c | -Wextra -lmsi -lole32 -static' > "$list"
bash "$here/scripts/build-measure-bundle.sh" "$1" "$list"
cp "$here/docs/wine-repro/msi-transform-null-stream/"{l1.c,test-body.h} "$here/LICENSE" "$1/"
(cd "$1" && sha256sum ./*.exe ./*.c ./*.h LICENSE README.txt run-measure.bat > SHA256SUMS)
