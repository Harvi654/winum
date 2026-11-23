# Per-user in-process COM classes

Non-elevated Windows 10 uses the per-user in-process registration ahead
of the machine registration for the same CLSID. The candidate applies
that lookup to InprocServer32, ProgID and TreatAs reads in combase.
Direct registry calls retain the existing Wine HKCR implementation.

| Level | Status |
| --- | --- |
| L0 | Both DLL architectures compiled after zero-fuzz apply in [round 1](https://github.com/Harvi654/winum/actions/runs/45278159284). |
| L1 | 54 successful native checks on x86_64 and 59 on i686. [Results](../../../docs/wine-repro/combase-per-user-classes/results/l1-windows-10/) |
| L2 | Paired combase and ole32 suites running. [CI](https://github.com/Harvi654/winum/actions/runs/45278894160) |
| L3 | All native COM results reproduced with the WOW64 test subtree. Focused Wine unit passed 51 x86_64 and 56 i686 checks. [Results](../../../docs/wine-repro/combase-per-user-classes/results/candidate/) |
| L4 | Pending joint Nastran run with wbemdisp and msado15 and the Inventor application smoke test. |
| Cross review | Claude review pending. |

[Native and Winum measurement](../../../docs/wine-repro/combase-per-user-classes/README.md).

## WOW64 registry prerequisite

Round 1 found that a fresh Wine prefix can place an explicit 32-bit user
registration in the 64-bit path because its HKCU Classes WOW64 tree is absent.
The x86_64 runner then selected the wrong registration. Round 2 prepares an empty
WOW64 Classes subtree in the isolated CI prefixes to separate COM lookup from
registry key placement. Direct HKCR reads and creation or separation of user
registry views on an existing user prefix are outside this combase candidate.

The combined L4 trial uses a working Inventor prefix and keeps its original
registry layout. The Nastran registration under test is in the default
64-bit user view.

[combase overlay](https://github.com/Harvi654/winum/actions/runs/45278822590/artifacts/14868737293) ·
[wbemdisp overlay](https://github.com/Harvi654/winum/actions/runs/45260888480/artifacts/14860665980) ·
[msado15 overlay](https://github.com/Harvi654/winum/actions/runs/45271185530/artifacts/14866335284).

The combase artifact ZIP SHA-256 is
`71ef7efdff585f2f6347cf86660a0cb1d624eb277505d7df2671d2c1044885f2`.
The preparation helper checks the individual overlay manifests and the
base runtime archive before copying the prefix.

## Joint Nastran and Inventor L4

Close Inventor and wait for Wine to exit before copying the original
prefix. Start from a working Inventor 2027.2 prefix with Nastran absent.
Set `prefix`, `image` and `inventor_repo` to your 2027.2 prefix, Nastran
image and Inventor checkout. Run from the Winum checkout on
`topic/combase-per-user-classes-measure`.

~~~bash
(
    set -euo pipefail
    work=$(mktemp -d -- "$(dirname -- "$prefix")/nastran-l4.XXXXXXXX")
    gh run download 45260888480 --repo Harvi654/winum \
        --name wbemdisp-method-invoke-overlay --dir "$work/wmi"
    gh run download 45271185530 --repo Harvi654/winum \
        --name msado15-stream-charset-overlay --dir "$work/ado"
    gh run download 45278822590 --repo Harvi654/winum \
        --name combase-per-user-classes-overlay --dir "$work/com"
    gh release download winum-11.22-2 --repo Harvi654/winum \
        --pattern winum-11.22-2-x86_64.tar.xz --dir "$work"

    bash docs/wine-repro/combase-per-user-classes/prepare-nastran-l4.sh \
        "$prefix" "$work/winum-11.22-2-x86_64.tar.xz" \
        "$work/wmi" "$work/ado" "$work/com" "$work/test"

    trial="$work/test"
    runtime="$trial/winum-11.22-2-x86_64"
    export PATH="$runtime/bin:$PATH"
    export WINE="$runtime/bin/wine" WINESERVER="$runtime/bin/wineserver" WINELOADER="$runtime/bin/wine"
    export INV_PREFIX="$trial/prefix" INV_LOGDIR="$trial/logs"
    export WINEDLLOVERRIDES='wbemdisp,msado15,combase=b'

    WINEDEBUG=+msi,fixme+wbemdisp,err+wbemdisp,fixme+msado15,err+msado15,err+ole \
        python3 "$inventor_repo/scripts/install-from-image.py" "$image" --prefix "$INV_PREFIX"
    WINEDEBUG=+loaddll,err+ole INV_WINE_HOME="$trial" INV_WINE=bundled INV_YEAR=2027 \
        bash "$inventor_repo/scripts/launch-inventor.sh"
)
~~~

Record whether the add-in appears in the Add-In Manager and whether
either `0x80040154` or “Requested functionality is not available in the
current version” appears. Mesh, solve and display results with a test
model. For the Inventor smoke test, create a part, assembly and drawing,
save and reopen them, then restart Inventor in the same trial prefix.
If a step fails, share its exact dialog or filtered Wine diagnostics
with identifiers removed.

## Cross review (Claude Opus 5.5, 2026-11-23)

Read in full: the `dlls/combase/combase.c` part of
`combase-per-user-classes.patch` against Wine 11.22. Skimmed: the test, the
MEASURE program and results.

**Approved for L4**, with these notes:

1. Narrow and in the right place: only COM's own lookups read the user's
   classes first - `InprocServer32` during activation, `ProgID` and
   `TreatAs` of a CLSID, and `CLSIDFromProgID`. A key missing in HKCU falls
   back to the existing HKLM lookup; any other HKCU error is reported, not
   hidden. `open_key_for_clsid` keeps its old behaviour for the other
   callers, and the plain registry API's HKCR (bug 17019 proper) is left
   alone - right for a first patch.
2. **Watch at L4: the class here is served by `mscoree.dll`.** Combase will
   now find `InprocServer32 = mscoree.dll` in HKCU, but the .NET Framework
   shim then reads the class's `Assembly`/`CodeBase`/`RuntimeVersion` itself,
   through the ordinary registry API under HKEY_CLASSES_ROOT - which in
   Wine still shows only HKLM. With native `mscoree` (as in Inventor
   prefixes with .NET 4.8) that lookup may fail the same way
   (`CLASS_E_CLASSNOTAVAILABLE` or `0x80040154`). If L4 shows that, the
   next step is measured, not guessed: which HKCR reads the shim does
   (`+reg` around the activation), and whether a narrow HKCR fallback for
   `CLSID\{...}` reads is enough.
3. Not covered (by design, recorded): `LocalServer32`, `InprocHandler32`,
   `AppID` and per-user type libraries; WOW64 needs the HKCU
   `Software\Classes\Wow6432Node` branch that a clean Wine prefix may lack
   (measured and documented above).
4. combase is used by everything: the Inventor smoke test (part, assembly,
   drawing, save/reopen, restart) is part of this L4, as the README says.
