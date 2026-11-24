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
| L4 | 2026-11-23: joint registration completed and NEiInventor.dll loaded from the installed path. Nastran UI startup still reported 0x80040154 on per-user COM activation. |
| Cross review | Claude, 2026-11-24: approved with three changes required before the series (below). |

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

On 2026-11-23 the maintainer reported that Register_Addin completed with
both overlays and that NEiInventor.dll loaded from its installed path.
The UI startup still reported 0x80040154 for a class registered under
HKCU\Software\Classes. The next joint run depends on the combase
per-user class candidate. Mesh, solver and results remain open for that
run, along with the Inventor smoke test.

## Cross review (second model)

Claude Opus 5.5, 2026-11-24, on `topic/msado15-stream-charset-measure`
at 95d9eee. **Approved with three required changes** (1-3); the code
does what the Windows 10 measurement shows.

Read in full: `msado15-stream-charset.patch` against Wine 11.22's
`dlls/msado15/stream.c` (with `patches/series`; both patches apply with
`--fuzz=0`), `patches/tests-first/msado15-tests-stream-charset.patch`,
this README, `docs/wine-repro/msado15-stream-charset/README.md`, the
charset lines of `results/windows-10/result-x86_64.txt`. Skimmed:
`measure.c`, `stream.vbs`, the other result files, the four workflows,
`run-wine.sh`, `prepare-nastran-l4.sh`,
`scripts/build-l1-msado15-stream-charset.sh`.

Built and run locally (Wine 11.22 + series, x86_64 and i386, 2026-11-24):

| | `msado15` unit | `stream_charset` unit |
| --- | --- | --- |
| without the candidate | 1950 checks, 75 todo, 0 failures | 157 checks, 39 todo, 0 failures |
| with the candidate | 1950 checks, 75 todo, 0 failures | 335 checks, 63 failures - all "Test succeeded inside todo block" |

So the existing suite is unchanged and every new check passes with the
candidate; the failures are the todos (item 2).

### Required before the series

1. **The test does not link inside Wine's tree.** `stream_charset.c` uses
   `IID_NULL`, which needs `uuid`; `msado15.c` in the same test module
   already defines the GUIDs it uses (initguid), so adding `uuid` to
   `IMPORTS` gives multiple definitions instead. Both MinGW
   architectures fail with "undefined reference to `GUID_NULL`". The
   standalone L1 build links `-luuid`, which hid it. This is why the
   paired CI run 45270694368 failed on both sides without results (the
   README still says "running"). A local `static const GUID null_iid;`
   passed to `GetIDsOfNames`/`Invoke` fixes it (used for the local run
   above).
2. **todo_wine.** The fix does not touch the test, so with it 63 checks
   per architecture report "succeeded inside todo block". When the test
   moves into the fix's patch, drop `todo_wine` in `check_read` and the
   `todo` argument of every `operation()` call that now passes (all of
   them in this test), and record it in a v2 paragraph.
3. **TRACE strings.** `stream_CopyTo`, `stream_SaveToFile` and
   `stream_LoadFromFile` use `"\\n"` (a literal backslash and `n`, no
   newline) in their TRACE format; `+msado15` logs run those lines
   together. Use `"\n"` as the rest of the file does.

### Checked, no change needed

- `stream_codepage` maps only the measured names (both UTF-8 spellings
  through `wcsicmp`, us-ascii 20127, windows-1252, iso-8859-1 28591);
  every other charset keeps the old Unicode path. ADO knows many more
  MIME names; adding one needs its own measurement.
- `put_Charset` at a nonzero position fails with adErrIllegalOperation
  when either side is one of these code pages, as measured. Windows
  also refuses Unicode -> Unicode at a nonzero position (`change_at_zero`,
  charset Unicode, `charset_after_write` 0x800a0c93); the candidate keeps
  Wine's old behaviour there (allowed). A known remaining difference,
  not a regression.
- `read_encoded_text`: counts UTF-16 units and advances by the length of
  the returned units re-encoded. That reproduces the measured positions
  exactly, including the split surrogate (`ReadText(3)` -> 12, the next
  line read starts with U+FFFD and ends at 18); the BOM is skipped only
  at position 0; `adReadLine` returns the text without the separator and
  moves past it.
- `write_encoded_text`: BOM only on an empty stream, overwrite in place
  and grow to `max(end, size)`, lone surrogates become `?` for the
  single-byte code pages (measured "one question mark per surrogate
  code unit"); sizes are checked against `MAXLONG` before allocating.
- `LoadFromFile`: inserts EF BB BF for a UTF-8 text stream without one
  (measured: both fixtures become 18 bytes); size checked against
  `MAXLONG`; file opened with `FILE_SHARE_READ`; position 0.
  `SaveToFile` writes the whole buffer and resets the position, as
  measured; the two documented options only.
- `CopyTo`: text -> binary copies the raw bytes from the position
  (with BOM); binary -> text adErrIllegalOperation as measured; text ->
  text stays E_NOTIMPL (not measured). `dst == iface` is refused.
- `resize_buffer` no longer overflows when doubling past `MAXLONG / 2`.

### Notes

- `read_encoded_text` decodes the whole remainder of the stream on every
  call, so reading a large text stream line by line is quadratic. Fine
  for the add-in scripts that need this; worth a look if a program reads
  big files through `ADODB.Stream`.
- An invalid UTF-8 byte in the middle of a stream decodes to U+FFFD and
  re-encodes as three bytes, so the position can move past the end of a
  short invalid sequence; `min(consumed, remaining)` keeps it inside the
  stream. This matches the measured split-surrogate case; other invalid
  input is not measured.
- The test file's header has only the first paragraph of the LGPL
  notice; the other Wine test files carry the full one.
