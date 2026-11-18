# Measure Enter during shell view label editing

In the Inventor 2027 Open dialog on winum, pressing Enter with a new
folder name typed into the inline editor opens the folder under its
default name. Clicking off the editor commits the typed name. In an
Inventor application check under Windows 10, Enter committed the name
and left the dialog in the parent folder.

The filtered Wine trace shows this order. Process and thread IDs and
pointer arguments are masked.

```text
<PID>:<TID>:trace:shell:ShellView_OnNotify -- LVN_BEGINLABELEDITW <ptr>
<PID>:<TID>:trace:shell:IShellView_fnTranslateAccelerator -- key=0x04d
<PID>:<TID>:trace:shell:IExplorerBrowser_fnBrowseToIDList <ptr> (<ptr>, 0x0)
<PID>:<TID>:trace:shell:IExplorerBrowser_fnBrowseToIDList SBSP_ABSOLUTE
```

The Enter attempt did not show the end-label-edit notification that the
click attempt logged. Wine's shell view accelerator returns `S_FALSE`
without processing the key. The hypothesis is that a host delegates
Enter to the shell view first and falls back to opening the folder
when the view declines it.

The measurement hosts an ExplorerBrowser, starts label editing on a
synthetic folder, then passes a `WM_KEYDOWN` with `VK_RETURN` directly to
`IShellView::TranslateAccelerator` with the edit control as its target.
It reports the HRESULT, editor state and filesystem names before and
afterward. If translation returns `S_FALSE` and leaves editing active,
it also reports the effect of dispatching the message directly.

## Run on offline Windows 10

Download and extract the `shell32-label-edit-enter-measure` artifact and
verify `SHA256SUMS` on Linux. Copy both EXEs and `run-measure.bat` into a
new directory in the offline Windows 10 snapshot. Double-click
`run-measure.bat`. The harness opens a short-lived test window. Leave
its focus alone while it runs both architectures.

Return only `result-x86_64.txt`, `result-i686.txt` and `exitcodes.txt`.
The batch file records the Windows version and refuses to overwrite
existing results.

To build the same bundle on Linux with MinGW-w64:

```bash
export MEASURE_OUT="$PWD/measure-shell32-label-edit-enter"
bash scripts/build-measure-shell32-label-edit-enter.sh "$MEASURE_OUT"
(cd "$MEASURE_OUT" && sha256sum -c SHA256SUMS)
```

## Results

Pending from the offline Windows 10 VM.
