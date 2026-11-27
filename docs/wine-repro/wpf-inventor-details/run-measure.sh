#!/usr/bin/env bash
set -euo pipefail
umask 077

probe_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)
if [[ $# != 1 || -z ${INV_PREFIX:-} ]]; then
    printf 'Usage: INV_PREFIX=/path/to/prefix bash run-measure.sh /path/to/inventor-repo\n' >&2
    exit 1
fi
inventor_repo=$1
export INV_PREFIX
probe_dll=$probe_dir/InventorDetailsMeasure.dll

if [[ ! -f "$probe_dll" || ! -f "$inventor_repo/scripts/launch-inventor.sh" ]]; then
    printf 'Missing probe DLL or Inventor launcher.\n' >&2
    exit 1
fi
if [[ $(realpath -e -- "$INV_PREFIX/dosdevices/z:" 2>/dev/null) != / ]]; then
    printf 'This command needs the Wine prefix to map Z: to /.\n' >&2
    exit 1
fi
if [[ -n ${DOTNET_STARTUP_HOOKS:-} ]]; then
    printf 'Run from a shell without another .NET startup hook.\n' >&2
    exit 1
fi

work=$(mktemp -d ${TMPDIR:-/tmp}/inventor-wpf-measure.XXXXXXXX)
windows_path() {
    local slash=/
    local backslash=$'\\'
    printf 'Z:%s' "${1//"$slash"/"$backslash"}"
}
export DOTNET_STARTUP_HOOKS
DOTNET_STARTUP_HOOKS=$(windows_path "$probe_dll")
export INVENTOR_WPF_DETAILS_LOG
INVENTOR_WPF_DETAILS_LOG=$(windows_path "$work/details.txt")
export INV_LOGDIR="$work"
unset INV_WINE INV_VDESKTOP WINEDEBUG

printf 'Measurement: %s/details.txt\n' "$work"
cd -- "$inventor_repo"
status=0
bash scripts/launch-inventor.sh || status=$?
if [[ -s "$work/details.txt" ]]; then
    cat -- "$work/details.txt"
else
    printf 'No managed measurement. Launcher logs are in %s.\n' "$work"
fi
exit "$status"
