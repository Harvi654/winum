#!/usr/bin/env bash
# Original, AI-assisted (Codex), LGPL-2.1-or-later.
# Build the exact candidate attachment assertion body for native Windows.
set -euo pipefail
[[ $# == 1 ]] || { echo 'Usage: build-l1-input-components.sh NEW_OUTPUT' >&2; exit 2; }
here="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
python3 - "$here" <<'PY'
from pathlib import Path
import sys
root = Path(sys.argv[1])
patch = (root / 'patches/candidates/server-input-attachment-components-v2/server-input-attachment-components.patch').read_text()
unit = patch.split('diff --git a/dlls/user32/tests/attachment.c b/dlls/user32/tests/attachment.c\n', 1)[1]
unit = '\n'.join(line[1:] for line in unit.splitlines() if line.startswith('+') and not line.startswith('+++')) + '\n'
body = (root / 'docs/wine-repro/server-input-attachment-components/test-body.h').read_text()
start = '#define THREADS'
if unit[unit.index(start):unit.index('\nSTART_TEST(attachment)\n')] != body[body.index(start):]:
    sys.exit('Candidate attachment test and standalone assertion body differ')
PY
bash "$here/scripts/build-measure-bundle.sh" "$1" "$here/docs/wine-repro/batch4-measure/l1.list"
cp "$here/docs/wine-repro/server-input-attachment-components/"{l1.c,test-body.h} "$here/LICENSE" "$1/"
(cd "$1" && sha256sum ./*.exe ./*.c ./*.h LICENSE README.txt run-measure.bat measure.manifest > SHA256SUMS)
