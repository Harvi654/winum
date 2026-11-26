# ole32 RevokeDragDrop across processes

Revit 2027.4 calls RevokeDragDrop on a WebView2 window in another process
while initializing Home. On winum-11.22-3, ole32 tries to release the
IDropTarget pointer stored by the other process and the caller crashes.

[MEASURE program, Windows instructions and bug report notes](../../../docs/wine-repro/ole32-revokedragdrop-cross-process/README.md).

| Level | Evidence |
| --- | --- |
| MEASURE | Both rounds agree across x86_64 and i686. The foreign call invalidates the registration, preserves LastError and leaves the owner references unchanged. |
| L0 | Pending. Apply the candidate after the winum-11.22-3 series with zero fuzz. |
| L1 | Offline Windows 10 passed 19 checks per architecture on 2026-11-26, seven in the caller and twelve in the owner, with zero failures, todos, flaky checks or skips. [Results](../../../docs/wine-repro/ole32-revokedragdrop-cross-process/results/l1-windows-10/summary.tsv). [Verified L1 bundle](https://github.com/Harvi654/winum/actions/runs/45827199214/artifacts/15118964743). |
| L2 | CI pairs ole32, combase and user32 against the same release series. The new cross-process case runs on the patched side. [Paired L2](https://github.com/Harvi654/winum/actions/runs/45827199191). |
| L4 | Pending. Verify Revit 2027.4 reaches Home and opens a model on the desktop PC. Smoke test Inventor and AutoCAD, including open, save, reopen and restart. |
| Cross review | Claude after test and fix. |

The fix invalidates both window registration properties when the owner
process differs. The target reference and map are process-local.
The foreign caller leaves them alone, so invalidation can leave the
owner-side registration resources alive. Native Windows retained the
observed owner reference. The candidate leaves the same-process path
and RegisterDragDrop behavior in place.
