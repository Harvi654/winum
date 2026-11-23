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
| L4 | Pending maintainer run of the Nastran 2027 installer and mesh/run/results in a separate copy of the Inventor 2027.2 prefix and runtime. |
| Cross review | Pending Claude review. |

## Inventor Nastran 2027 L4

Close Inventor and wait for Wine to finish using the source prefix before
copying it. Use an Inventor 2027.2 prefix from before the Nastran install.
Use a test assembly copy for the run. Set `prefix`, `image` and
`inventor_repo` to your source prefix, Nastran image and Inventor project
checkout. Run the following from this Winum checkout. The new directory
needs space for the prefix and the Winum runtime.

~~~bash
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
INV_WINE_HOME="$trial" INV_WINE=bundled INV_YEAR=2027 \
    bash "$inventor_repo/scripts/launch-inventor.sh"
~~~

Report the Register_Addin result, whether Nastran appears in the Add-In
Manager, its loaded module path as observed externally, any 0x80040154
dialogs, and whether mesh, run and results work. Use selected installer
trace lines with identifiers removed. Do not include vendor script
contents or the full logs.
