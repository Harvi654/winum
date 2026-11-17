#!/usr/bin/env bash
# Original, AI-assisted (Claude), LGPL-2.1-or-later.
# Builds the candidate GetAdaptersAddresses zone-indices test body for the
# offline VM, after checking that the test patch adds exactly test-body-zones.h.
set -euo pipefail
[[ $# == 1 ]] || { echo 'Usage: build-l1-iphlpapi-adapter-zones.sh NEW_OUTPUT' >&2; exit 2; }
here="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
python3 - "$here" <<'PY'
from pathlib import Path
import sys
root = Path(sys.argv[1])
patch = (root / 'patches/candidates/iphlpapi-adapter-zones/iphlpapi-tests-adapter-zones.patch').read_text()
added = [l[1:] for l in patch.splitlines() if l.startswith('+') and not l.startswith('+++')]
added = [l for l in added if l.strip() != 'test_GetAdaptersAddresses_zones();']
body = (root / 'docs/wine-repro/iphlpapi-adapters/test-body-zones.h').read_text().splitlines()
if [l for l in added if l.strip()] != [l for l in body if l.strip()]:
    sys.exit('Candidate test patch and test-body-zones.h differ')
PY
list="$(mktemp)"
trap 'rm -f -- "$list"' EXIT
printf '%s\n' 'iphlpapi-zones-l1 | docs/wine-repro/iphlpapi-adapters/l1-zones.c | -Wextra -liphlpapi -lws2_32 -static' > "$list"
bash "$here/scripts/build-measure-bundle.sh" "$1" "$list"
cp "$here/docs/wine-repro/iphlpapi-adapters/"{l1-zones.c,test-body-zones.h} "$here/LICENSE" "$1/"
(cd "$1" && sha256sum ./*.exe ./*.c ./*.h LICENSE README.txt run-measure.bat measure.manifest > SHA256SUMS)
