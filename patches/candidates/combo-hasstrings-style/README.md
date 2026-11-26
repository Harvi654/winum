# EXPERIMENT: CBS_HASSTRINGS in a combo box's window style

Not a candidate for `patches/series`; a quick check of a hypothesis for
Inventor's Content Center filter crash (Harvi654/Autodesk-Inventor-for-Linux#45).

`docs/wine-repro/user32-window-tree` printed the window tree of the same
Inventor 2027 state (Content Center editor open) on Windows 10 and on
winum-11.22-3. The trees match except for the combo boxes of the Content
Center dialogs: on Windows their `GWL_STYLE` has `CBS_HASSTRINGS` (0x200)
- `50010242`, `50010203`, `48010203` - and in Wine it does not
(`50010042`, `50010003`, `48010003`). Wine's `COMBO_NCCreate` (user32 and
comctl32_v6) adds the flag only to its own copy of the style.

The overlay sets the flag in the window style as well. If Inventor's
filter then works, a MEASURE program settles the exact rule on Windows
(which combo styles get the flag, when) and a proper patch with a test
follows; if not, the hypothesis is dropped.
