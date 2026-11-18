# Measure the shell folder background context menu

## Observed symptom

In the Inventor Open dialog under winum 11.22, a right-click on the
background of a filesystem folder showed Line up Icons, Refresh, Paste,
Paste as Link and Properties. It did not offer a New submenu or a way
to create a folder from that menu.

In Wine 11.22, the shell view's background menu calls
`BackgroundMenu_QueryContextMenu` with a first command ID of zero. It
merges the `MENU_002` resource with an offset of `idCmdFirst - 0x7000`
and `MM_SUBMENUSHAVEIDS`. Its New popup has a raw ID of `0x42`. At
this offset the popup ID wraps past the last allowed ID, so the merge
can skip the popup before examining the New Folder command inside it.
View and Arrange Icons have popup IDs in the same range.

## What to measure

The program binds a normal filesystem folder and asks its
`IShellFolder::CreateViewObject` for an `IContextMenu`. It calls
`QueryContextMenu` with `CMF_NORMAL` for first IDs 0, 1 and 0x7000.
The last ID stays at 0x7fff. The 0x7000 run gives Wine a zero-offset
comparison. For each query the program records the HRESULT, IDs,
submenu structure, captions and enabled state. Only the initial
populated menu is inspected. A submenu may populate its children when
opened interactively. The program does not invoke any item.

A trace from Inventor will confirm the command range used by the dialog.
Windows expectations for the regression test will come from the offline
VM results below.

## Build

Use a new output directory and both MinGW-w64 cross compilers:

```bash
export MEASURE_OUT="$PWD/measure-shell32-background-menu"
bash scripts/build-measure-shell32-background-menu.sh "$MEASURE_OUT"
(cd "$MEASURE_OUT" && sha256sum -c SHA256SUMS)
```

## Offline Windows 10 run

Download and extract the `shell32-background-menu-measure` artifact
from the branch's Build shell32 background menu MEASURE workflow.
Verify `SHA256SUMS` on Linux. Copy `run-measure.bat` and both EXEs
into a fresh folder in the offline Windows 10 snapshot, then
double-click `run-measure.bat`.

The batch file creates an empty temporary folder, runs both
architectures and removes the empty sample folder. It saves the
Windows build and process exit codes to `exitcodes.txt` and each
program's measurement text to `result-x86_64.txt` and
`result-i686.txt`. Use a fresh copy for a repeat measurement.

Return only these three text files. Mask personal paths or user
names if present. Keep menu captions, IDs, depth and HRESULTs.

## Results

Recorded 2026-11-18 on offline Windows 10 build `10.0.19045.6466`.
Both programs exited with code 0 and produced identical measurements
apart from the architecture line. All three `CreateViewObject` calls
returned `S_OK`. Each `QueryContextMenu` returned `0x00000053` with
five top-level entries, including separators.

| First ID | Last ID | Enabled New popup ID | Initial New child ID |
| ---: | ---: | ---: | ---: |
| `0x0` | `0x7fff` | `0x11` | `0x14` |
| `0x1` | `0x7fff` | `0x12` | `0x15` |
| `0x7000` | `0x7fff` | `0x7011` | `0x7014` |

The top-level entries with captions were Give access to, New and
Properties. New was an enabled submenu in all three ranges. These
results cover the initial menu. They do not measure the entries
populated when New is opened or the result of invoking New Folder.
Inventor 2027 on winum-11.22-1, breadcrumb candidate `9cd7d88`,
confirmed the first range on right-click in the Open dialog. Process,
thread and pointer values are masked.

```text
<PID>:<TID>:trace:shell:BackgroundMenu_QueryContextMenu (<ptr>)->(hmenu=<ptr> indexmenu=0 cmdfirst=0 cmdlast=7fff flags=0 )
<PID>:<TID>:trace:shell:BackgroundMenu_QueryContextMenu (<ptr>)->returning 0x101
```
