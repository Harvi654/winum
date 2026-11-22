# Shell32 autocomplete responsiveness candidate

The maintainer approved keeping v2 with a known deviation on 2026-11-19.
The [patch page](../../../docs/patches/shell32-autocomplete-responsive.md)
records the decision, validation and open Windows compatibility follow-up.

AutoCAD 2027 can stop responding while typing a file name with
FILEDIA enabled. The trace found repeated drive queries while the
edit's autocomplete source was being enumerated. See the
[Windows 10 and winum measurements](../../../docs/wine-repro/shell32-autocomplete-timing/README.md).

The candidate lets the edit finish handling input before starting the
enumeration. A timer on the edit window processes a small batch of
Next calls, yields and resumes until the source reaches its end.
The existing sorting and matching run when the enumeration finishes.
Losing focus, replacing the text or closing the edit cancels pending
work. Changing the directory discards the old request and resets and
expands the source for the new path.

The candidate calls the source on the edit thread to keep it in the
same COM apartment. Windows 10 called the measured sources on another
thread. The thread assertion and two held-callback lifecycle checks
remain `todo_wine`. A single Reset, Expand or Next call can block the
edit thread. A long list can still take time to populate.

## Validation on 2026-11-19

| Check | Result |
| --- | --- |
| Patch applies with zero fuzz to Wine 11.22 | Passed |
| Targeted shell32 build | x86_64 and i386 compiled and linked |
| Windows 10 L1, existing test from 5d308e9 | Both architectures passed on Windows. Callback thread still differs in the candidate |
| Existing shell32 autocomplete unit, x86_64 | Zero failures and skips in both variants. The candidate made 14 fewer Next calls in this suite. The only checks that ran fewer times were the assertions inside that callback |
| Slow standard and agile MEASURE, x86_64 | Baseline about 1.57 s inside WM_CHAR and 21 probe timeouts per case. Candidate 0–1 ms inside WM_CHAR, zero inline Next calls and 0–1 probe timeouts. All 128 suggestions returned per case |
| Text replacement from inside the first Next call, x86_64 | The replacement remained in the edit and the candidate made no further Next calls for the canceled request |
| AutoCAD 2027 dialog with FILEDIA=1 | Paired check passed. Baseline froze during filename editing and required termination. With v2 the Cancel button returned to the drawing, editing could continue and AutoCAD exited normally |
| Paired L2 on winum-11.22-1 | All 21 shell32 units completed in each variant on x86_64 and i386. No new failure locations. The autocomplete unit ran 14 fewer callback assertions on each architecture. [Paired results](https://github.com/Harvi654/winum/actions/runs/44592013573) |
| Inventor 2027 L4 | Maintainer confirmed the Open, Place, Save As, reopen and restart checks using the 11.22-1 runtime with v2 DLLs from build `12294c2` |
| Inventor 2026 L4 | Maintainer confirmed the same checks in a separate 2026 prefix with the same runtime and v2 DLLs |
| Full L0 | [Full-series x86_64 and i386 build passed](https://github.com/Harvi654/winum/actions/runs/44616982879) on `test-run/shell32-autocomplete-responsive-2` at `48bc33e`. The source and binary jobs applied the candidate with `--fuzz=0`. |

The second revision includes two changes from independent review.
It releases a running request if a callback unwinds and checks the
edit state again after autoappend sends messages. The x86_64
autocomplete and MEASURE checks were repeated with the verified
v2 artifact. The maintainer reproduced the dialog freeze in the
baseline and confirmed normal cancellation and recovery with v2.

## AutoCAD A/B test

Use **shell32-autocomplete-candidate** from the
[v2 build](https://github.com/Harvi654/winum/actions/runs/44562084665)
at `12294c2`. Verify SHA256SUMS in the downloaded folder. The artifact
provides both DLLs for winum-11.22-1. The base and candidate use the same version and
other patches. If the AutoCAD test uses the 11.22-1 MSXML3 PR #4
runtime, keep that runtime in both trials and add these DLLs to its
candidate copy.

Start in the AutoCAD repo. Set ACAD_AUTOCOMPLETE_SOURCE to the AutoCAD
prefix copy you already use for testing. Set ACAD_AUTOCOMPLETE_BASE
to the corresponding unpacked winum 11.22-1 runtime and
ACAD_AUTOCOMPLETE_BUNDLE to the artifact directory before running these
commands. Close AutoCAD first and check the available disk space with
df -h "$HOME". The two test prefixes and runtime copy are created on
the home filesystem.

~~~bash
export ACAD_AUTOCOMPLETE_WORK="$(mktemp -d "$HOME/.acad-autocomplete.XXXXXXXX")"
(
    set -eu
    umask 077
    test -d "$ACAD_AUTOCOMPLETE_SOURCE/drive_c/Program Files/Autodesk/AutoCAD 2027"
    test -x "$ACAD_AUTOCOMPLETE_BASE/bin/wine"
    grep -Eq '^name:[[:space:]]+winum-11\.22-1-x86_64$' "$ACAD_AUTOCOMPLETE_BASE/BUILDINFO"
    (cd "$ACAD_AUTOCOMPLETE_BUNDLE" && sha256sum -c SHA256SUMS)

    cp -aT --reflink=auto "$ACAD_AUTOCOMPLETE_SOURCE" "$ACAD_AUTOCOMPLETE_WORK/base-prefix"
    cp -aT --reflink=auto "$ACAD_AUTOCOMPLETE_SOURCE" "$ACAD_AUTOCOMPLETE_WORK/candidate-prefix"
    cp -aT --reflink=auto "$ACAD_AUTOCOMPLETE_BASE" "$ACAD_AUTOCOMPLETE_WORK/wine"

    cp --remove-destination "$ACAD_AUTOCOMPLETE_BUNDLE/x86_64-windows/shell32.dll" \
        "$ACAD_AUTOCOMPLETE_WORK/wine/lib/wine/x86_64-windows/shell32.dll"
    cp --remove-destination "$ACAD_AUTOCOMPLETE_BUNDLE/i386-windows/shell32.dll" \
        "$ACAD_AUTOCOMPLETE_WORK/wine/lib/wine/i386-windows/shell32.dll"
    cp --remove-destination "$ACAD_AUTOCOMPLETE_BUNDLE/x86_64-windows/shell32.dll" \
        "$ACAD_AUTOCOMPLETE_WORK/candidate-prefix/drive_c/windows/system32/shell32.dll"
    cp --remove-destination "$ACAD_AUTOCOMPLETE_BUNDLE/i386-windows/shell32.dll" \
        "$ACAD_AUTOCOMPLETE_WORK/candidate-prefix/drive_c/windows/syswow64/shell32.dll"
    printf 'Test directory: %s\n' "$ACAD_AUTOCOMPLETE_WORK"
)
~~~

In the same terminal, define this launcher function and run each trial
separately. Close AutoCAD before starting the other one.

~~~bash
run_acad_autocomplete() (
    set -eu
    export ACAD_PREFIX="$1"
    export WINEPREFIX="$ACAD_PREFIX"
    export INV_PREFIX="$ACAD_PREFIX"
    export ACAD_WINE_BIN="$2/bin"
    export INV_WINE=system
    export PATH="$ACAD_WINE_BIN:$PATH"
    export WINE="$ACAD_WINE_BIN/wine"
    export WINESERVER="$ACAD_WINE_BIN/wineserver"
    export WINELOADER="$ACAD_WINE_BIN/wine"
    export ACAD_LOGDIR="$ACAD_AUTOCOMPLETE_WORK/log-$3"
    export WINEDEBUG=-all
    test -f scripts/launch-autocad.sh
    test -x "$WINE"
    test -d "$ACAD_PREFIX/drive_c"
    mkdir -p "$ACAD_LOGDIR"
    if ! bash -c '
        source scripts/common-env.sh || exit 1
        cd "$INVENTOR_DIR" || exit 1
        source scripts/lib/common.sh || exit 1
        test "$(realpath -e "$WINEPREFIX")" = "$(realpath -e "$1")" &&
        test "$(realpath -e "$(type -P wine)")" = "$(realpath -e "$2/wine")" &&
        test "$(realpath -e "$(type -P wineserver)")" = "$(realpath -e "$2/wineserver")"
    ' bash "$ACAD_PREFIX" "$ACAD_WINE_BIN" > "$ACAD_LOGDIR/preflight.log" 2>&1; then
        printf '%s\n' 'Launcher prefix or runtime check failed.' >&2
        exit 1
    fi
    bash scripts/launch-autocad.sh
)

run_acad_autocomplete "$ACAD_AUTOCOMPLETE_WORK/base-prefix" "$ACAD_AUTOCOMPLETE_BASE" baseline
# Close AutoCAD before the candidate run.
run_acad_autocomplete "$ACAD_AUTOCOMPLETE_WORK/candidate-prefix" "$ACAD_AUTOCOMPLETE_WORK/wine" candidate
~~~

In each run, set FILEDIA to 1. In a scratch drawing open **Save As**,
type the same filename in the same directory and press **Cancel**.
Record whether the text updates, the dialog responds to Cancel and
AutoCAD returns to the drawing. Turn off the optional fail-fast
observer and EventPipe for the pair.

The maintainer reproduced crashes on both Save As and Open with
FILEDIA set to 0 in both builds. The dialog-only comparison checks the
autocomplete change without accepting a filename. The CoreCLR
failure remains open in the
[AutoCAD follow-up](../../../docs/wine-repro/shell32-autocomplete-timing/README.md#autocad-2027-follow-up).
