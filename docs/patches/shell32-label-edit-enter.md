# shell32-label-edit-enter.patch

- **DLL:** `shell32`
- **Status:** [candidate](../../patches/candidates/shell32-label-edit-enter/README.md)
- **Target:** winum-11.22-2, pending validation

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
| L0 | Candidate build on `test-run/shell32-label-edit-enter-1` pending |
| L1 | MEASURE passed on x86_64 and i686. Wine test on Windows pending. |
| L2 | Paired shell32 suites pending |
| L4 | Inventor checks pending |
| L5 | Pending |
| Cross review | Pending |
