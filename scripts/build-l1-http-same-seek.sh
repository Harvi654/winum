#!/usr/bin/env bash
# Original, AI-assisted (Codex), LGPL-2.1-or-later.
# Compile the exact candidate HTTP helper/test block for native Windows.
set -euo pipefail
[[ $# == 1 ]] || { echo 'Usage: build-l1-http-same-seek.sh NEW_OUTPUT' >&2; exit 2; }
here="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
python3 - "$here" <<'PY'
from pathlib import Path
import sys
root = Path(sys.argv[1])
patch = (root / 'patches/candidates/wininet-same-position-seek-v2/wininet-http-same-position-seek.patch').read_text()
test_diff = patch.split('diff --git a/dlls/wininet/tests/http.c b/dlls/wininet/tests/http.c\n', 1)[1]
added = '\n'.join(line[1:] for line in test_diff.splitlines() if line.startswith('+') and not line.startswith('+++')) + '\n'
body = (root / 'docs/wine-repro/wininet-http-same-position-seek/test-body.h').read_text()
start = '#define SAMEPOS_BODY_SIZE 16384\n'
end = added.index('\n}', added.index('static void test_same_position_seek(void)\n')) + len('\n}')
if added[added.index(start):end] + '\n' != body[body.index(start):]:
    sys.exit('Candidate HTTP test and standalone body differ')
PY
list="$(mktemp)"
trap 'rm -f -- "$list"' EXIT
printf '%s\n' 'http-same-seek-l1 | docs/wine-repro/wininet-http-same-position-seek/l1.c | -Wextra -lwininet -lws2_32 -static' > "$list"
bash "$here/scripts/build-measure-bundle.sh" "$1" "$list"
cp "$here/docs/wine-repro/wininet-http-same-position-seek/"{l1.c,test-body.h} "$here/LICENSE" "$1/"
(cd "$1" && sha256sum ./*.exe ./*.c ./*.h LICENSE README.txt run-measure.bat measure.manifest > SHA256SUMS)
