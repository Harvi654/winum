# shell32-label-edit-enter.patch

<!-- winum-test: l1 -->

- **DLL:** `shell32`
- **Status:** In the winum-11.22-2 series (merged into `main` 2026-11-22). [Candidate history](https://github.com/Harvi654/winum/blob/43693a2/patches/candidates/shell32-label-edit-enter/README.md)
- **Target:** winum-11.22-2 (Wine 11.22)

## Symptom

In Inventor Open, New Folder creates the directory and starts inline
name editing. Pressing Enter after typing a name opens the folder under
its default name. Clicking away from the editor commits the new name.

## Cause and fix

In the trace, the dialog offers Enter to
`IShellView::TranslateAccelerator` and then calls
`IExplorerBrowser::BrowseToIDList`. Wine's shell view implementation
returns `S_FALSE` without dispatching the key. In the offline Windows
10 measurement (build 19045.6466, x86_64 and i686), translating Enter
while the inline editor held focus returned `S_OK`, committed the
rename and closed the editor.

Dispatch the original key-down message only for Enter addressed to
the active, focused label editor of this view. The existing list-view
editor then performs the rename. The accelerator returns `S_OK` so
the host can treat the key as handled. Other keys and targets retain
the existing `S_FALSE` behavior.

## Wine test

`dlls/shell32/tests/ebrowser.c` hosts an ExplorerBrowser on a synthetic
filesystem folder, edits the name, and checks the return code, editor
and filesystem state. See the
[trace and measurement](../wine-repro/shell32-label-edit-enter/README.md).

## Validation

| Level | Result |
| --- | --- |
| L0 | Passed on Wine 11.22 x86_64 and i386 with `--fuzz=0` ([build 44359884019](https://github.com/Harvi654/winum/actions/runs/44359884019)) |
| L1 | Passed on offline Windows 10. The ebrowser unit completed 688 checks with zero failures or skips on both x86_64 and i386. |
| L2 | Passed on x86_64 and i386 ([run 44359884168](https://github.com/Harvi654/winum/actions/runs/44359884168)). The four new assertions are expected failures without the patch and pass with it. |
| L4 | Passed on candidate `c440f0e` (maintainer, 2026-11-18). Enter commits the name in the Open dialog. SOLIDWORKS and AutoCAD deferred under AGENTS.md. |
| L4 (release build) | `topic/next-release` at `3785565`, full series built in [run 45052921398](https://github.com/Harvi654/winum/actions/runs/45052921398) (winum-11.22-2 test build): Inventor 2027 on a copy of the maintainer's prefix - Open dialog breadcrumb names, New Folder and Enter in the name editor, typing a file name in Save As, start, part, assembly, drawing, save/reopen, restart: passed (maintainer, 2026-11-22). |
| L5 | Pending |
| Cross review | Approved by an independent Codex reviewer (2026-11-18). Both candidate diffs were read in full. |
| Cross review (second model) | Approved by Claude Opus 5.5 (2026-11-22), patch and test read in full. `TranslateAccelerator` now consumes only `WM_KEYDOWN VK_RETURN` addressed to the list view's focused label editor and dispatches it there, so comctl32's existing end-edit path commits the name; the caller then sends no `WM_CHAR` and Inventor no longer opens the folder. Every other key and target keeps returning `S_FALSE`, as before. Follow-up worth measuring (not blocking): what Windows returns for `VK_ESCAPE` in the same editor - if it consumes it too, Escape in Inventor's Open dialog would cancel the rename instead of reaching the dialog. |
