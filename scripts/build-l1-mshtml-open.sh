#!/usr/bin/env bash
# Original, AI-assisted (Codex), LGPL-2.1-or-later.
# Build the exact Wine candidate function with our native assertion harness.
set -euo pipefail
[[ $# == 1 ]] || { echo 'Usage: build-l1-mshtml-open.sh NEW_OUTPUT' >&2; exit 2; }
here="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
python3 - "$here" <<'PY'
from pathlib import Path
import sys
root = Path(sys.argv[1])
patch = (root / 'patches/candidates/mshtml-document-open-optional-result-v2/mshtml-document-open-optional-result.patch').read_text()
added = '\n'.join(line[1:] for line in patch.splitlines() if line.startswith('+') and not line.startswith('+++'))
start = added.index('static void test_doc_open_optional_result(void)\n')
end = added.index('\n}', start) + len('\n}')
body = (root / 'docs/wine-repro/mshtml-document-open-optional-result/test-body.h').read_text()
if added[start:end] + '\n' != body:
    sys.exit('Candidate test and standalone test body differ')
PY
list="$(mktemp)"
trap 'rm -f -- "$list"' EXIT
printf '%s\n' 'mshtml-open-l1 | docs/wine-repro/mshtml-document-open-optional-result/l1.c | -Wextra -lole32 -loleaut32 -luuid -static' > "$list"
bash "$here/scripts/build-measure-bundle.sh" "$1" "$list"
cp "$here/docs/wine-repro/mshtml-document-open-optional-result/"{l1.c,test-body.h} "$here/LICENSE" "$1/"
(cd "$1" && sha256sum ./*.exe ./*.c ./*.h LICENSE README.txt run-measure.bat > SHA256SUMS)
