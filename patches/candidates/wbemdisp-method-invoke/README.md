# Direct WMI method calls in wbemdisp

The [offline Windows reference and L1](../../../docs/wine-repro/wbemdisp-method-invoke/README.md)
show that both method flag combinations dispatch the method, pass
positional inputs in signature order, copy outputs back by reference,
and return the provider status as a separate value.

The patch routes only dynamic method members to the WMI method
implementation. It preserves the type-library and ordinary property
paths. The dispatcher uses signature IDs when the provider supplies them
and signature enumeration order otherwise. Named arguments are outside
the measured scope.

Apply the candidate after the Winum 11.22-2 release series with
zero fuzz. The release series does not select it.

| Level | Status |
| --- | --- |
| L0 | The runtime patch applied with zero fuzz after the release series. Both candidate DLLs built. The full WoW64 build is in the L2 job. |
| L1 | Offline Windows 10. 135 checks per architecture, zero failures, todos and skips. |
| L2 | The paired wbemdisp, wbemprox and vbscript jobs are running. [L2](https://github.com/Harvi654/winum/actions/runs/45261162354) |
| L3 | The overlay passed 135 Wine checks per architecture. All 30 C calls and the VBScript output and process statuses matched the offline Windows reference on each architecture. [CI](https://github.com/Harvi654/winum/actions/runs/45260888480) · [Results](https://github.com/Harvi654/winum/actions/runs/45260888480/artifacts/14860436546) |
| L4 | 2026-11-23: the maintainer observed StdRegProv dispatch pass. Registration then stopped at ADODB.Stream WriteText with charset UTF-8. Full L4 is pending a fresh-prefix run with the msado15 candidate as well. |
| Cross review | Pending Claude review. |

## Inventor Nastran 2027 L4

Close Inventor and wait for Wine to finish using the source prefix before
copying it. Use an Inventor 2027.2 prefix from before the Nastran install.
Use a test assembly copy for the run. Set `prefix`, `image` and
`inventor_repo` to your source prefix, Nastran image and Inventor project
checkout. Run the following from this Winum checkout. The new directory
needs space for the prefix and the Winum runtime.

~~~bash
(
    set -euo pipefail
    work=$(mktemp -d -- "$(dirname -- "$prefix")/wbemdisp-l4.XXXXXXXX")
    gh run download 45260888480 --repo Harvi654/winum \
        --name wbemdisp-method-invoke-overlay --dir "$work/overlay"
    gh release download winum-11.22-2 --repo Harvi654/winum \
        --pattern winum-11.22-2-x86_64.tar.xz --dir "$work"
    bash docs/wine-repro/wbemdisp-method-invoke/prepare-inventor-l4.sh \
        "$prefix" "$work/winum-11.22-2-x86_64.tar.xz" "$work/overlay" "$work/test"

    trial="$work/test"
    runtime="$trial/winum-11.22-2-x86_64"
    export PATH="$runtime/bin:$PATH"
    export WINE="$runtime/bin/wine" WINESERVER="$runtime/bin/wineserver" WINELOADER="$runtime/bin/wine"
    export INV_PREFIX="$trial/prefix" INV_LOGDIR="$trial/logs"
    export WINEDLLOVERRIDES=wbemdisp=b

    WINEDEBUG=+msi,+wbemdisp python3 "$inventor_repo/scripts/install-from-image.py" \
        "$image" --prefix "$INV_PREFIX"
    WINEDEBUG=+loaddll INV_WINE_HOME="$trial" INV_WINE=bundled INV_YEAR=2027 \
        bash "$inventor_repo/scripts/launch-inventor.sh"
)
~~~

Report the Register_Addin result, whether Nastran appears in the Add-In
Manager, its loaded module path as observed externally, any 0x80040154
dialogs, and whether mesh, run and results work. Use selected installer
trace lines with identifiers removed. Do not include vendor script
contents or the full logs.

On 2026-11-23 the maintainer reported that StdRegProv dispatch proceeded
without the unsupported Invoke diagnostic. The next observed failure
was `msado15:stream_WriteText` rejecting the UTF-8 charset. The add-in
assembly entry still lacked its installation path. The joint follow-up
covers registration, loading, meshing, solving and results using both
candidates.

## Cross review (Claude Opus 5.5, 2026-11-23)

Read in full: the `dlls/wbemdisp/locator.c` part of
`wbemdisp-method-invoke.patch` against Wine 11.22. Skimmed: the test hunk,
the MEASURE program and its result tables.

**Approved for L4**, with these notes:

1. The route is right and narrow: only method members, called with
   `DISPATCH_METHOD` (alone or with `PROPERTYGET`), go through the method
   signature and `IWbemServices::ExecMethod`; properties and the type
   library members keep their paths. Positional arguments map to the input
   parameters in signature order (by `ID` qualifier when every parameter
   has a distinct one, else in the provider's order - inputs before
   outputs, as Wine's built-in signatures enumerate); missing trailing
   inputs take the signature's default (StdRegProv's `hDefKey`), and a
   missing required one raises `WBEM_E_INVALID_PARAMETER` through
   `DISP_E_EXCEPTION`. By-reference outputs are written back, the provider
   status comes back as the result - all as measured.
2. **One part is in the wrong DLL:** the mapping of `ERROR_UNSUPPORTED_TYPE`
   to `WBEM_E_TYPE_MISMATCH` for `StdRegProv.GetStringValue` sits in
   `wbemdisp`, but the value comes from the provider (`wbemprox`, the
   StdRegProv implementation). Through `ExecMethod_()` or a native
   `IWbemServices::ExecMethod` caller Wine would still return the old code.
   If Windows returns `WBEM_E_TYPE_MISMATCH` there for every caller (measure
   it with `ExecMethod` directly), it belongs in a separate `wbemprox`
   patch; if only this path matters for the measured case, keep it out of
   this patch and record it as open. Nastran's script does not need it for
   L4.
3. `VT_I4` hive values are accepted for `uint32` parameters by retyping
   (no value change) - right for the predefined `HKEY` constants VBScript
   passes as negative `Long`.
4. On promotion: the test moves into the patch, a page in `docs/patches/`
   with the `l1` marker, the open point from note 2, and the bug-report
   note (Wine bug 39463: a VBScript custom action calling
   `StdRegProv.GetStringValue` directly).
5. L4 by the maintainer as described above (fresh copy of a 2027.2 prefix
   without Nastran; `Register_Addin`, the Add-In Manager, no `0x80040154`,
   mesh/run/results).
