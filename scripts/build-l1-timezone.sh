#!/usr/bin/env bash
# Original, AI-assisted (Codex), LGPL-2.1-or-later.
# Build the complete candidate timezone test body for the offline VM.
set -euo pipefail
[[ $# == 1 ]] || { echo 'Usage: build-l1-timezone.sh NEW_OUTPUT' >&2; exit 2; }
here="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
python3 - "$here" <<'PY'
from pathlib import Path
import sys
root = Path(sys.argv[1])
patch = (root / 'patches/candidates/wbemprox-timezone-v2/wbemprox-timezone.patch').read_text()
unit_diff = patch.split('diff --git a/dlls/wbemprox/tests/timezone.c b/dlls/wbemprox/tests/timezone.c\n', 1)[1]
unit = '\n'.join(line[1:] for line in unit_diff.splitlines() if line.startswith('+') and not line.startswith('+++')) + '\n'
body = (root / 'docs/wine-repro/wbemprox-timezone/test-body.h').read_text()
start = 'static BOOL timezone_hr(HRESULT hr, const char *operation)\n'
end = unit.index('\nSTART_TEST(timezone)\n')
if unit[unit.index(start):end] != body[body.index(start):]:
    sys.exit('Candidate timezone test and standalone body differ')
PY
list="$(mktemp)"
trap 'rm -f -- "$list"' EXIT
printf '%s\n' 'timezone-l1 | docs/wine-repro/wbemprox-timezone/l1.c | -Wextra -lwbemuuid -lole32 -loleaut32 -luuid -ladvapi32 -static' > "$list"
bash "$here/scripts/build-measure-bundle.sh" "$1" "$list"
cp "$here/docs/wine-repro/wbemprox-timezone/"{l1.c,test-body.h} "$here/LICENSE" "$1/"
(cd "$1" && sha256sum ./*.exe ./*.c ./*.h LICENSE README.txt run-measure.bat measure.manifest > SHA256SUMS)
