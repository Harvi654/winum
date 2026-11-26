# Window tree of a running program (Windows and Wine)

`wintree.c` prints every window of a running process (default
`Inventor.exe`): depth, handle, class, style, ex style, `GetParent`,
`GW_OWNER`, `GA_PARENT`, `GA_ROOT`, visible, enabled, control id and the
first 24 characters of the title (titles can name documents; mask them
before sharing). Public user32 calls only, from outside the program.
Build: `scripts/build-measure-bundle.sh OUT docs/wine-repro/user32-window-tree/measure.list`
or the workflow `build-user32-window-tree.yml`.

## Inventor 2027, Content Center editor open (2026-11-26)

Windows 10 (online snapshot with Inventor) and winum-11.22-3, same state,
before the filter is used (issue Harvi654/Autodesk-Inventor-for-Linux#45).
The trees match: "Place from Content Center" is a top-level window owned
by the main frame in both, with the same dialogs and controls, and the
documents sit under the `MDIClient`. Differences seen:

- combo boxes that are not owner-drawn have `CBS_HASSTRINGS` (0x200) in
  `GWL_STYLE` on Windows (`50010242`, `50010203`, `48010203`) and not in
  Wine (`50010042`, `50010003`, `48010003`); Wine keeps the implied flag
  only internally (`COMBO_NCCreate`). Exposing it did not change the
  crash (`patches/candidates/combo-hasstrings-style`, experiment);
- tooltips have `WS_BORDER` (0x00800000) in Wine, not on Windows;
- one `ComboLBox` has `WS_EX_LEFTSCROLLBAR` (0x4000) on Windows.

These are real differences worth a MEASURE each later, but none of them
is the cause of #45.
