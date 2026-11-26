# ole32 RevokeDragDrop across processes

Revit 2027.4 calls RevokeDragDrop on a WebView2 window in another process
while initializing Home. On winum-11.22-3, ole32 tries to release the
IDropTarget pointer stored by the other process and the caller crashes.

[MEASURE program, Windows instructions and bug report notes](../../../docs/wine-repro/ole32-revokedragdrop-cross-process/README.md).

| Level | Evidence |
| --- | --- |
| MEASURE | Both Windows 10 architectures completed the first probe. Foreign revoke returned RPC_E_WRONG_THREAD and preserved LastError. Owner-first cleanup and the unregistered window observation are pending. |
| L0 | Pending. Apply the candidate after the winum-11.22-3 series with zero fuzz. |
| L1 | Pending. Add the measured expectations to the ole32 dragdrop test and run both architectures offline on Windows 10. |
| L2 | Pending. Pair ole32, combase and user32 against the same release series. |
| L4 | Pending. Verify Revit 2027.4 reaches Home and opens a model on the desktop PC. Smoke test Inventor and AutoCAD, including open, save, reopen and restart. |
| Cross review | Claude after test and fix. |

The native observation will determine how ole32 handles a foreign
window. The candidate will avoid touching the other process's local
pointer and preserve the same-process path.
