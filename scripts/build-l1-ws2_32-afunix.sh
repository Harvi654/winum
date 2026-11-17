#!/usr/bin/env bash
# Original, AI-assisted (Claude), LGPL-2.1-or-later.
# Build the AF_UNIX candidate test (docs/wine-repro/ws2_32-afunix/test-body.h,
# the same code the candidate patch adds to dlls/ws2_32/tests/sock.c) as
# native executables for L1 in the offline Windows 10 VM.
set -euo pipefail
[[ $# == 1 ]] || { echo 'Usage: build-l1-ws2_32-afunix.sh NEW_OUTPUT' >&2; exit 2; }
here="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
python3 - "$here" <<'PY'
from pathlib import Path
import sys
root = Path(sys.argv[1])
patch = (root / 'patches/candidates/ws2_32-af-unix/ws2_32-tests-afunix-winum.patch').read_text()
added = [l[1:] for l in patch.splitlines() if l.startswith('+') and not l.startswith('+++')]
added = [l for l in added if l != '    test_afunix_winum();']
unit = '\n'.join(added).strip('\n') + '\n'
body = (root / 'docs/wine-repro/ws2_32-afunix/test-body.h').read_text()
if unit != body:
    sys.exit('Candidate test patch and test-body.h differ')
PY
list="$(mktemp)"
trap 'rm -f -- "$list"' EXIT
printf '%s\n' 'afunix-l1 | docs/wine-repro/ws2_32-afunix/l1.c | -Wextra -lws2_32 -lmswsock -ladvapi32 -static' > "$list"
bash "$here/scripts/build-measure-bundle.sh" "$1" "$list"
cp "$here/docs/wine-repro/ws2_32-afunix/"{l1.c,test-body.h} "$here/LICENSE" "$1/"
(cd "$1" && sha256sum ./*.exe ./*.c ./*.h LICENSE README.txt run-measure.bat measure.manifest > SHA256SUMS)
