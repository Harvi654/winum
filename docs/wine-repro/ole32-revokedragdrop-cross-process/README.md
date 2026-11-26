# RevokeDragDrop on a window owned elsewhere

Base: winum-11.22-3 (Wine 11.22). On 2026-11-26 the maintainer observed an
access violation inside RevokeDragDrop during Revit 2027.4's WebView2 Home
initialization. The caller was Revit and the target window belonged to
the WebView2 process. See the maintainer's Revit notes in
Autodesk-Revit-on-Linux, docs/revit-2027-4-czech-desktop.md.

The probe creates its own windows and IDropTarget objects. It needs
neither Revit nor WebView2. All participating threads initialize OLE and
pump messages while they wait.

| Case | Sequence |
| --- | --- |
| cross_process | A helper process creates two windows, registers its IDropTarget on one and signals the caller. The caller revokes that registration and separately tries to register its own target on the helper's unregistered window. If the latter succeeds, the caller tries to undo it before the helper destroys the window. |
| same_process_other_thread | A second thread creates and registers a window. The main thread calls RevokeDragDrop on it. |
| Owner follow-up in both cases | The owner records its reference and Release counters before and after the caller's operations. It then tries RegisterDragDrop again on the original window, calls RevokeDragDrop there and records the final counters. |

Each API result records the HRESULT and GetLastError captured immediately
after the call. The probe seeds LastError with 0xdeadbeef before every
call. Compare Release counts at before_caller and after_caller to
exclude calls made during the owner's follow-up. The owner and caller
are verified to be different threads in the requested processes.

## Offline Windows 10

Check SHA256SUMS in the extracted MEASURE bundle on Linux. Transfer the
bundle to a short path in the offline Windows 10 snapshot and run
run-measure.bat in a normal console. Return these three text files:

- result-revoke-drag-drop-x86_64.txt
- result-revoke-drag-drop-i686.txt
- exitcodes.txt

A completed measurement has measure_complete=1 and exit 0 for both
architectures. Record any setup error or missing result line before
interpreting the API results. Expected HRESULTs, LastError values and
reference effects will be written after this run.

## Build

Use MinGW for both x86_64 and i686.

~~~bash
bash scripts/build-measure-bundle.sh out/revoke-drag-drop \
    docs/wine-repro/ole32-revokedragdrop-cross-process/measure.list
(cd out/revoke-drag-drop && sha256sum -c SHA256SUMS)
~~~

## Notes for a Wine bug report

The application symptom is an access violation during RevokeDragDrop
when a different process owns the registered window. The self-contained
probe supplies the sequence and a same-process, other-thread control.
Include the exact Windows and Wine build identifiers, caller HRESULT
and LastError, the owner's follow-up and counter deltas. Add the
reference results once measured. On the Wine side, ole32 stores a
process-local IDropTarget in the window property and RevokeDragDrop
tries to release that pointer from the caller. The report can point to
this synthetic program without needing any application files.
