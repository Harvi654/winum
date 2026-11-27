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
drive_c=$(realpath -e -- "$INV_PREFIX/dosdevices/c:" 2>/dev/null || :)
if [[ -z "$drive_c" || ! -d "$drive_c" || ! -w "$drive_c" ]]; then
    printf 'The Wine prefix needs a writable C: drive.\n' >&2
    exit 1
fi
if [[ -n ${DOTNET_STARTUP_HOOKS:-} ]]; then
    printf 'Run from a shell without another .NET startup hook.\n' >&2
    exit 1
fi

work=$(mktemp -d -- "$drive_c/inventor-wpf-measure.XXXXXXXX")
cp -- "$probe_dll" "$work/InventorDetailsMeasure.dll"
printf -v windows_work 'C:\\%s' "${work##*/}"
export DOTNET_STARTUP_HOOKS="$windows_work\InventorDetailsMeasure.dll"
export INVENTOR_WPF_DETAILS_LOG="$windows_work\details.txt"
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
