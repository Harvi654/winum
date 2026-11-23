# ADODB.Stream text charsets

The candidate adds UTF-8 in either case, US-ASCII, Windows-1252 and
ISO-8859-1 to msado15 text streams. It uses the Windows code-page APIs,
tracks byte positions, and incorporates the measured UTF-8 BOM and
surrogate handling. Existing Unicode ReadText and WriteText use their
original byte path.

The file methods support saving and loading byte buffers. UTF-8 text
loads normalize the measured input with and without a BOM. The measured
text-to-binary CopyTo path includes the BOM, and binary-to-text returns
the ADO illegal-operation status.

| Level | Status |
| --- | --- |
| L0 | Both candidate DLL architectures built after applying the release series and candidate with zero fuzz. Full WoW64 validation is in the paired run. |
| L1 | Offline Windows 10 passed 335 checks on x86_64 and i686 with zero failures, todos or skips. [Results](../../../docs/wine-repro/msado15-stream-charset/results/l1-windows-10/) |
| L2 | Paired msado15 suite is running. [CI](https://github.com/Harvi654/winum/actions/runs/45270694368) |
| L3 | The candidate passed 335 Wine checks per architecture. C and VBScript results match the offline reference for the changed charsets after normalizing the integer width of Size and Position. [Results](../../../docs/wine-repro/msado15-stream-charset/results/candidate/) |
| L4 | Pending Nastran 2027 with both wbemdisp and msado15 in a fresh Inventor 2027.2 prefix copy. |
| Cross review | Pending. |

[Measurement and exact reference bytes](../../../docs/wine-repro/msado15-stream-charset/README.md).

## Nastran 2027 joint L4

Close Inventor and wait for Wine to finish using the original prefix.
Start from a 2027.2 prefix with Inventor installed and Nastran absent.
Set `prefix`, `image` and `inventor_repo` to your source prefix, Nastran
image and Inventor checkout. Run from the Winum checkout of
`topic/msado15-stream-charset-measure`.
[msado15 overlay](https://github.com/Harvi654/winum/actions/runs/45271185530/artifacts/14866335284) ·
[wbemdisp overlay](https://github.com/Harvi654/winum/actions/runs/45260888480/artifacts/14860665980).

~~~bash
(
    set -euo pipefail
    work=$(mktemp -d -- "$(dirname -- "$prefix")/nastran-l4.XXXXXXXX")
    gh run download 45260888480 --repo Harvi654/winum \
        --name wbemdisp-method-invoke-overlay --dir "$work/wmi"
    gh run download 45271185530 --repo Harvi654/winum \
        --name msado15-stream-charset-overlay --dir "$work/ado"
    gh release download winum-11.22-2 --repo Harvi654/winum \
        --pattern winum-11.22-2-x86_64.tar.xz --dir "$work"
    bash docs/wine-repro/msado15-stream-charset/prepare-nastran-l4.sh \
        "$prefix" "$work/winum-11.22-2-x86_64.tar.xz" "$work/wmi" "$work/ado" "$work/test"

    trial="$work/test"
    runtime="$trial/winum-11.22-2-x86_64"
    export PATH="$runtime/bin:$PATH"
    export WINE="$runtime/bin/wine" WINESERVER="$runtime/bin/wineserver" WINELOADER="$runtime/bin/wine"
    export INV_PREFIX="$trial/prefix" INV_LOGDIR="$trial/logs"
    export WINEDLLOVERRIDES='wbemdisp,msado15=b'

    WINEDEBUG=+msi,fixme+wbemdisp,err+wbemdisp,fixme+msado15,err+msado15 \
        python3 "$inventor_repo/scripts/install-from-image.py" "$image" --prefix "$INV_PREFIX"
    WINEDEBUG=+loaddll INV_WINE_HOME="$trial" INV_WINE=bundled INV_YEAR=2027 \
        bash "$inventor_repo/scripts/launch-inventor.sh"
)
~~~

Report whether Register_Addin completes, the loaded Nastran module has
the installed assembly path, and Nastran appears in the Add-In Manager.
Use a test assembly to mesh, solve and display results. Include any
0x80040154 dialogs or the next observed Wine diagnostic if registration
stops again. Share only the relevant external observations with
identifiers removed.
