# Shell32 autocomplete responsiveness candidate

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
thread. The thread assertion therefore remains todo_wine. The
candidate remains blocked for the duration of any single Reset, Expand
or Next call. A long list can still take time to populate.

## Validation on 2026-11-19

| Check | Result |
| --- | --- |
| Patch applies with zero fuzz to Wine 11.22 | Passed |
| Targeted shell32 build | x86_64 and i386 compiled and linked |
| Windows 10 L1, existing test from 5d308e9 | Both architectures passed on Windows. Callback thread still differs in the candidate |
| Existing shell32 autocomplete unit, x86_64 | Zero failures and skips in both variants. The candidate made 14 fewer Next calls in this suite. The only checks that ran fewer times were the assertions inside that callback |
| Slow standard and agile MEASURE, x86_64 | Baseline about 1.57 s inside WM_CHAR and 21 probe timeouts per case. Candidate 0–1 ms inside WM_CHAR, zero inline Next calls and 0–1 probe timeouts. All 128 suggestions returned per case |
| Full L0, paired L2, AutoCAD and the other shell32 consumer (L4) | Open |

This patch remains a candidate. It targets the measured behavior that
held up the edit and still needs an AutoCAD test.

## AutoCAD A/B test

Use the artifact named **shell32-autocomplete-candidate** from the
**Build shell32 autocomplete candidate** workflow. Verify SHA256SUMS
in the downloaded folder. The artifact provides both DLLs for
winum-11.22-1. The base and candidate must use the same version and
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
(
    set -eu
    umask 077
    export ACAD_AUTOCOMPLETE_WORK="$(mktemp -d "$HOME/.acad-autocomplete.XXXXXXXX")"
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

Export ACAD_AUTOCOMPLETE_WORK with the resulting directory in the same
terminal. This launcher function uses the AutoCAD project's prefix
and Wine overrides. Run each trial separately. Close AutoCAD before
starting the other one.

~~~bash
run_acad_autocomplete() (
    set -eu
    export ACAD_PREFIX="$1"
    export WINEPREFIX="$ACAD_PREFIX"
    export INV_PREFIX="$ACAD_PREFIX"
    export ACAD_WINE_BIN="$2/bin"
    export PATH="$ACAD_WINE_BIN:$PATH"
    export WINE="$ACAD_WINE_BIN/wine"
    export WINESERVER="$ACAD_WINE_BIN/wineserver"
    export WINELOADER="$ACAD_WINE_BIN/wine"
    export ACAD_LOGDIR="$ACAD_AUTOCOMPLETE_WORK/log-$3"
    export WINEDEBUG=-all
    test -f scripts/launch-autocad.sh
    test -x "$WINE"
    test -d "$ACAD_PREFIX/drive_c"
    bash scripts/launch-autocad.sh
)

run_acad_autocomplete "$ACAD_AUTOCOMPLETE_WORK/base-prefix" "$ACAD_AUTOCOMPLETE_BASE" baseline
# Close AutoCAD before the candidate run.
run_acad_autocomplete "$ACAD_AUTOCOMPLETE_WORK/candidate-prefix" "$ACAD_AUTOCOMPLETE_WORK/wine" candidate
~~~

In each run, set FILEDIA to 1. In a scratch drawing, open **Save As**,
type a few characters in the file-name field, change the selection
and cancel. Try **Open** and change directory while typing. If the
candidate remains responsive, save a scratch DWG under a new name
and reopen it. Record whether typing, cancellation and save/reopen
worked for each build. Keep the test files away from original drawings
and remove the test prefix copies when done.
