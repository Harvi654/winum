#!/usr/bin/env bash
# Original, AI-assisted (Codex), LGPL-2.1-or-later.
set -euo pipefail
umask 077

if [[ $# == 2 && "$1" == --from-summary ]]; then
    summary="$2"
    export ACAD_EVENTPIPE_RUN="$(sed -nE 's/^eventpipe_run=(winum-eventpipe\.[[:alnum:]]{8})\r?$/\1/p' "$summary")"
    test -n "$ACAD_EVENTPIPE_RUN"
    set -- "$(sed -nE 's/^eventpipe_pid=0x([[:xdigit:]]+)\r?$/\1/p' "$summary")" \
        "$(sed -nE 's/^last_chance=.* tid=0x([[:xdigit:]]+) .*/\1/p' "$summary")" \
        "$(sed -nE 's/^observer_return_pc=0x([[:xdigit:]]+)\r?$/\1/p' "$summary")"
fi

if [[ $# -lt 2 || $# -gt 3 || ! "$1" =~ ^[[:xdigit:]]{1,8}$ || ! "$2" =~ ^[[:xdigit:]]{1,8}$ ||
      ( $# == 3 && ! "$3" =~ ^[[:xdigit:]]{1,16}$ ) ]]; then
    echo 'Usage: summarize-autocad-eventpipe.sh HEX_PROCESS_ID HEX_THREAD_ID [HEX_RETURN_PC]' >&2
    exit 2
fi

test "$(uname -m)" = x86_64
bundle="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
drive="$(realpath -e -- "$ACAD_AUTOCOMPLETE_WORK/base-prefix/drive_c")"
pid="$((16#$1))"
shopt -s nullglob
if [[ -n "${ACAD_EVENTPIPE_RUN:-}" ]]; then
    [[ "$ACAD_EVENTPIPE_RUN" =~ ^winum-eventpipe\.[[:alnum:]]{8}$ ]]
    traces=("$drive/$ACAD_EVENTPIPE_RUN/events-$pid.nettrace")
else
    traces=("$drive"/winum-eventpipe.*/events-"$pid".nettrace)
fi
if [[ "${#traces[@]}" != 1 ]]; then
    printf 'matching_eventpipe_files=%s\n' "${#traces[@]}"
    exit 2
fi
test -s "${traces[0]}"

scratch="$(mktemp -d "$bundle/runtime.XXXXXXXX")"
trap 'rm -rf -- "$scratch"' EXIT
cd "$scratch"
curl -fsSL --retry 2 --max-time 90 -o runtime.tar.gz \
    https://builds.dotnet.microsoft.com/dotnet/Runtime/10.0.0/dotnet-runtime-10.0.0-linux-x64.tar.gz
curl -fsSL --retry 2 --max-time 40 -o traceevent.nupkg \
    https://api.nuget.org/v3-flatcontainer/microsoft.diagnostics.tracing.traceevent/3.2.8/microsoft.diagnostics.tracing.traceevent.3.2.8.nupkg
sha256sum -c <<'EOF'
b453ea0657d89d474184f5056c95c3a68598d16f8e87dad4f1247170d25680d1  runtime.tar.gz
37284f885e75326bbd1858bf19f20c9da2818fec7a3bd49803209209be5fd38d  traceevent.nupkg
EOF
tar -xzf runtime.tar.gz
unzip -jq traceevent.nupkg 'lib/netstandard2.0/Microsoft.Diagnostics.Tracing.TraceEvent.dll' \
    'lib/netstandard2.0/Microsoft.Diagnostics.FastSerialization.dll'
cp -- "$bundle/eventpipe-summary.dll" "$bundle/eventpipe-summary.runtimeconfig.json" .
./dotnet ./eventpipe-summary.dll "${traces[0]}" "${@:2}"
