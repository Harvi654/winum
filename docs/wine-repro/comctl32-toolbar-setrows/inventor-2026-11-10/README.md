# Inventor's palette calls (2026-11-10)

Summary of filtered `+toolbar` traces of the drawing-text symbol palette
in Inventor 2027 under Wine, taken by the maintainer. The trace lines
themselves are not kept in the repository (AGENTS.md: no logs); this
file replaced the first trace excerpt that had been committed here.

Palette: `CCS_VERT | TBSTYLE_WRAPABLE | TBSTYLE_FLAT | CCS_NORESIZE |
CCS_NODIVIDER` (style `0x54000acf`), in this order:

1. `TB_SETBITMAPSIZE` 16x15, `TB_SETBUTTONSIZE` 23x22
2. `TB_SETBITMAPSIZE` 32x16, `TB_SETBUTTONSIZE` 40x24
3. 47 buttons one by one: `TB_ADDBITMAP` (one bitmap handle) and
   `TB_ADDBUTTONS` (one button) each, separators between the groups
4. `TB_SETPADDING(4, 0)`, `TB_SETINDENT`
5. `TB_SETROWS(12, TRUE)`
6. three resizes of the toolbar

A first grep had missed step 1-2's `TB_SETBUTTONSIZE` (it looked for the
message name in capitals). Measured on Windows with exactly this order:
`../../comctl32-toolbar-padding-height/windows-2026-11-10-sequence2/`.
