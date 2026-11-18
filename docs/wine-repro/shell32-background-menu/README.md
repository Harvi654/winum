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

Copy only the two test executables into the offline Windows 10
snapshot. From their directory in `cmd.exe`:

```bat
mkdir C:\winum-measure-background-menu
ver
measure-x86_64.exe C:\winum-measure-background-menu > result-x86_64.txt
measure-i686.exe C:\winum-measure-background-menu > result-i686.txt
findstr /B /C:"arch=" /C:"SHParseDisplayName " /C:"SHGetDesktopFolder " /C:"BindToObject " /C:"range " /C:"CreateViewObject " /C:"QueryContextMenu " /C:"item depth=0 " /C:"item depth=1 " result-x86_64.txt result-i686.txt
```

Return the Windows build and the filtered measurement text. Mask
personal paths or user names if present. Keep menu captions, IDs, depth
and HRESULTs.

## Results

Inventor trace and offline Windows 10 measurements pending.
