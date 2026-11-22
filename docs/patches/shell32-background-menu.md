# shell32-background-menu.patch

<!-- winum-test: l1 -->

- **DLL:** `shell32`
- **Status:** In the winum-11.22-2 series on `topic/next-release`. [v2 candidate history](https://github.com/Harvi654/winum/blob/43693a2/patches/candidates/shell32-background-menu-v2/README.md)
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
when the parent provides the folder creation helper and resolves to a filesystem path. If folder creation
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
as the default action. The [separate Enter fix](shell32-label-edit-enter.md) handles the measured accelerator path.

The first filtered trace recorded two `LVN_BEGINLABELEDITW` calls.
Only the mouse-confirmed case recorded an end-edit notification and
`IShellFolder_fnSetNameOf`. No Enter keystroke or `NM_RETURN` appeared
in that extract. Check the ExplorerBrowser input and navigation traces
before changing either the label edit control or the dialog handling.

## v2 cross review follow-up

Independent review found that Wine's Recycle Bin exposes the creation
helper with stub methods. v2 requires a filesystem path as well as
the helper. It checks the Recycle Bin behavior on Wine and skips the
English-caption assertion in other UI languages. The validation table
below refers to v1. The independent Codex reviewer approved both v2
diffs on 2026-11-18.

### v2 validation

| Level | Result |
| --- | --- |
| L0 | Passed on Wine 11.22 x86_64 and i386 with `--fuzz=0` ([build 44375516021](https://github.com/Harvi654/winum/actions/runs/44375516021)) |
| L1 | Passed the New submenu checks on offline Windows 10 x86_64 and i386. The shlfolder unit reported 3915 checks and 0 failures on x86_64, and 2128 checks and 3 failures on i386. The three i386 failures are from the existing file timestamp assertion at shlfolder.c:1976. |
| L2 | Passed both architectures without new failures ([run 44375516288](https://github.com/Harvi654/winum/actions/runs/44375516288)). The two expected baseline menu failures cleared with v2. |
| L3 | No separate run. The Wine-specific Recycle Bin check runs in L2. |
| L4 | Passed in Inventor 2027 on v2 build `aa3632d` (maintainer, 2026-11-18). The maintainer confirmed the New Folder and Enter flow. SOLIDWORKS and AutoCAD deferred under AGENTS.md. |
| L5 | Pending a week of ordinary use |
| Cross review | Independent review approved v2 on 2026-11-18 |

## v1 validation

| Level | Result |
| --- | --- |
| L0 | Passed on Wine 11.22 x86_64 and i386 with `--fuzz=0` ([build 44339967795](https://github.com/Harvi654/winum/actions/runs/44339967795)) |
| L1 | Passed on offline Windows 10, x86_64 and i386. The shlfolder unit reported 3915 checks and 0 failures on x86_64, and 2128 checks and 3 failures on i386. The i386 failures were all in the existing file timestamp check at shlfolder.c:1976. |
| L2 | Passed, paired shell32 suites with and without the fix ([run 44339967832](https://github.com/Harvi654/winum/actions/runs/44339967832)) |
| L4 | Passed in Inventor on combined candidate `c440f0e` (maintainer, 2026-11-18). Folder creation and name confirmation work. SOLIDWORKS and AutoCAD deferred under AGENTS.md. |
| L5 | Pending |
| Cross review | Requested the filesystem restriction implemented in v2 |
| Cross review (second model) | Approved by Claude Opus 5.5 (2026-11-22), patch v2 and tests read in full. `IDM_BACKGROUND_NEW` (0x7054) sits next to `FCIDM_SHVIEW_NEWFOLDER` (0x7053) and collides with no `FCIDM_SHVIEW_*` ID, so `Shell_MergeMenus` keeps the popup in range for first IDs 0, 1 and 0x7000; the other popups are untouched. The filesystem-path requirement removes New where creation cannot work (This PC, Recycle Bin), and `DoNewFolder` no longer passes an uninitialised PIDL to `SelectItem` when `AddFolder` fails - a crash fix on its own. Notes: the Recycle Bin check runs on Wine only (a regression guard, not a Windows description - fine as such); the caption check is English-only by design. |
