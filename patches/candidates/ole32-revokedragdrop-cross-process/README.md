# ole32 RevokeDragDrop across processes

Revit 2027.4 calls RevokeDragDrop on a WebView2 window in another process
while initializing Home. On winum-11.22-3, ole32 tries to release the
IDropTarget pointer stored by the other process and the caller crashes.

[MEASURE program, Windows instructions and bug report notes](../../../docs/wine-repro/ole32-revokedragdrop-cross-process/README.md).

| Level | Evidence |
| --- | --- |
| MEASURE | Both rounds agree across x86_64 and i686. The foreign call invalidates the registration, preserves LastError and leaves the owner references unchanged. |
| L0 | Both v2 ole32 architectures built after zero-fuzz application on the winum-11.22-3 series. [Build](https://github.com/Harvi654/winum/actions/runs/45843181838). |
| L1 | Offline Windows 10 passed 19 checks per architecture on 2026-11-26, seven in the caller and twelve in the owner, with zero failures, todos, flaky checks or skips. [Results](../../../docs/wine-repro/ole32-revokedragdrop-cross-process/results/l1-windows-10/summary.tsv). The test is unchanged in v2, so this L1 remains valid. [Verified L1 bundle](https://github.com/Harvi654/winum/actions/runs/45827199214/artifacts/15118964743). |
| L2 | v1 matched the owner's HRESULT but set LastError to 2 on both architectures. v2 restores the saved value for the same-process not-registered return. Paired ole32, combase and user32 v2 rerun (2026-11-26): 86 units, 0 regressions; `user32 win.c:10846-10847` passed on the candidate side only (a timing-dependent window test, not touched by the patch), 3 other changes without new failures. [v2 comparison](https://github.com/Harvi654/winum/actions/runs/45843181988). [v1 comparison](https://github.com/Harvi654/winum/actions/runs/45827199191) |
| L4 | 2026-11-26, v2 overlay ([run 45843181838](https://github.com/Harvi654/winum/actions/runs/45843181838)) on copies of the prefixes. **Revit 2027.4:** reaches Home (crashed there on plain winum-11.22-3); opening a model or template is not yet checked, because New project now stops in riched20 (`ITextDocument` default tab stop stubs, candidate `riched20-default-tab-stop`). **Inventor 2027:** passed - start and sign-in, part open/change/save/reopen, reordering and moving features into a folder in the part and assembly browsers, `.ipt` dropped from Dolphin opens, a part dropped into an open assembly is placed, Nastran mesh and von Mises result. **AutoCAD 2027:** passed - start and sign-in, DWG open/change/SAVE/SAVEAS/reopen, `.dwg` dropped from Dolphin. Logs: no `DragDrop`, `Unhandled` exception or fatal lines; only the known unregistered-class `err:ole` lines. A drop from Dolphin takes a few seconds with the cursor hidden; it is the same on plain winum-11.22-3 (X11 drag and drop, not this candidate). |
| Cross review | Claude, 2026-11-26: approved for L2/L4; two open points below. |

The fix invalidates both window registration properties when the owner
process differs. The target reference and map are process-local.
The foreign caller leaves them alone, so invalidation can leave the
owner-side registration resources alive. Native Windows retained the
observed owner reference. The candidate leaves the same-process path
and RegisterDragDrop behavior in place.

## Cross review (Claude, 2026-11-26)

Read in full: the fix patch, the test patch
(`patches/tests-first/ole32-tests-revokedragdrop-cross-process.patch`),
the three Windows 10 result tables and the L1 summary; `measure.c`, the
two workflows and `scripts/build-l1-ole32-revokedragdrop.sh` only skimmed.
Checked against Wine 11.22's `dlls/ole32/ole2.c`.

- The fix matches the measured cross-process rows: a registered foreign
  window gives `RPC_E_WRONG_THREAD` and loses its registration without a
  Release in the owner (`owner-first-references.tsv`: refs stay 2,
  releases 0 until the owner's own cleanup); an empty foreign window gives
  `DRAGDROP_E_NOTREGISTERED`; LastError is kept in both. `is_droptarget()`
  reads the window property, which works across processes, and the
  foreign path never dereferences the other process's pointer or touches
  its mapping handle - the crash Revit hit cannot happen there.
- The owner's view after the foreign call is right for Wine too: its
  `RevokeDragDrop` finds no registration (`DRAGDROP_E_NOTREGISTERED`,
  measured) and `RegisterDragDrop` succeeds again. The owner's mapping
  and target reference stay alive until the process ends - the same
  retained reference Windows shows.
- Nit: when `GetWindowThreadProcessId` fails the function returns before
  restoring LastError. It cannot fail after `IsWindow` succeeded, so this
  only matters for a window destroyed in between.

Open points (not blockers for this candidate, which is limited on
purpose):

1. **Same process, other thread.** Windows returns `RPC_E_WRONG_THREAD`
   there too and also drops the registration without a Release
   (`calls.tsv` and `owner-first-references.tsv`, case
   `same_process_other_thread`), so its rule looks thread-based, not
   process-based. Wine (vanilla and with this patch) still revokes with a
   Release and `S_OK` in that case. A follow-up could use the window's
   thread for the check; it needs its own test, as it changes behaviour a
   program in one process might rely on.
2. **`RegisterDragDrop` on a foreign window.** Windows returns `S_OK`
   (`caller_register_unregistered`) - a second process can register on
   another's window. Not measured in Wine and not needed by Revit;
   recorded for the bug report.

## v2 (2026-11-26)

The normal property lookup sets LastError to ERROR_FILE_NOT_FOUND
when the original owner calls RevokeDragDrop after the registration
has been removed. Restore the entry value on the same-process
DRAGDROP_E_NOTREGISTERED return. The regression test and L1
expectations are unchanged.

For L4, use the [ole32-revokedragdrop-cross-process-overlay](https://github.com/Harvi654/winum/actions/runs/45843181838/artifacts/15128672652)
from run 45843181838 with a test copy of the winum-11.22-3 runtime. Verify SHA256SUMS in the extracted artifact. Install each
ole32.dll into the matching x86_64-windows or i386-windows directory
under lib/wine. BUILDINFO identifies the release base and exact
candidate commit.

## Combined L4 (2026-11-26, evening)

Overlay with all four candidates built from one winum-11.22-3 tree
(tab stop -> paragraph navigation -> target device scroll, ole32 v2;
[run 45880891853](https://github.com/Harvi654/winum/actions/runs/45880891853/artifacts/15147073657),
SHA-256 of archive and DLLs verified), on copies of the prefixes:

- Inventor 2027: note text, dimension text, datum identifier editor
  (letter reachable by scrolling), drag and drop, open/save/reopen - ok.
- AutoCAD 2027: MTEXT (several lines, centred, reopened), TEXT,
  save/reopen - ok.
- Revit 2027.4, DirectX Software: New project opens "Podlaží 1". A wall
  drawn afterwards gives Revit's warning that none of the created
  elements is visible in the view - the drawing problem recorded in the
  Revit repository, not RichEdit or OLE.
