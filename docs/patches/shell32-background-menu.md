# shell32-background-menu.patch

- **DLL:** `shell32`
- **Status:** [candidate](../../patches/candidates/shell32-background-menu/README.md)
- **Target:** winum-11.22-2 (Wine 11.22), pending validation

## Symptom

The Open dialog in Inventor has no New submenu when right-clicking the
empty area of a filesystem folder.

## Cause and fix

Inventor's shell view queries its background menu with first command ID
0, last ID 0x7fff and `CMF_NORMAL`. Wine rebases command IDs by
subtracting 0x7000. The submenu ID 0x42 wraps out of range and Wine skips
the submenu before merging its children. Offline Windows 10 build
19045.6466 showed an enabled New submenu at the observed command range
on both x86_64 and i686.

Put the New popup ID in the resource's command ID range. Only show it
when the parent provides the folder creation helper. If folder creation
fails or produces no PIDL, skip selection and label editing. The
general merge algorithm and the other background popups are unchanged.

## Test

`dlls/shell32/tests/shlfolder.c` checks for an enabled submenu at each
measured first command ID. It checks the New caption in an English UI.
The [MEASURE results](../wine-repro/shell32-background-menu/README.md)
capture the initial menu structure. Actual folder creation is checked
in Inventor.

## Confirming the new folder name

In Inventor 2027 on candidate `5ef7578`, New Folder creates the folder
and opens the inline name editor. Pressing Enter after typing a new
name leaves the default name and opens the folder. Clicking the
background or scrolling commits the edited name. In the maintainer's
Windows Inventor UI check, Enter committed the name and stayed in the
parent folder. This was an application check.

The Wine list-view edit control handles Enter in `EditLblWndProcT`
and sends an end-label-edit notification. Enter delivered to the
list view instead sends `NM_RETURN`, which the shell view handles
as the default action. A trace will distinguish a key delivered to
the list from an edit accepted or canceled before navigation.

The first filtered trace recorded two `LVN_BEGINLABELEDITW` calls.
Only the mouse-confirmed case recorded an end-edit notification and
`IShellFolder_fnSetNameOf`. No Enter keystroke or `NM_RETURN` appeared
in that extract. Check the ExplorerBrowser input and navigation traces
before changing either the label edit control or the dialog handling.

## Validation

| Level | Result |
| --- | --- |
| L0 | Passed on Wine 11.22 x86_64 and i386 with `--fuzz=0` ([build 44339967795](https://github.com/Harvi654/winum/actions/runs/44339967795)) |
| L1 | Passed on offline Windows 10, x86_64 and i386. The shlfolder unit reported 3915 checks and 0 failures on x86_64, and 2128 checks and 3 failures on i386. The i386 failures were all in the existing file timestamp check at shlfolder.c:1976. |
| L2 | Passed, paired shell32 suites with and without the fix ([run 44339967832](https://github.com/Harvi654/winum/actions/runs/44339967832)) |
| L4 | Inventor 2027 can create a folder through New. Keyboard confirmation and the remaining smoke checks are pending. |
| L5 | Pending |
| Cross review | Pending |
