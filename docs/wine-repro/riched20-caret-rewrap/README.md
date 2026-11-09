# Caret placed before a rewrap: Windows MEASURE

Original, AI-assisted (Claude), LGPL-2.1-or-later. Patch:
`patches/riched20-caret-rewrap.patch` (`cursor_coords` rewraps a paragraph
still marked `MEPF_REWRAP` instead of failing
`assert(~para->nFlags & MEPF_REWRAP)` in `caret.c`; Inventor aborted when a
dimension text was edited). Read before writing: the patch and Wine 11.20
`dlls/riched20/caret.c` (`cursor_coords`, `update_caret`), `editor.c`
(`set_selection`, `WM_SETFOCUS`), `paint.c` (`ME_UpdateRepaint`) and
`richole.c`.

How the state arises (from the Wine source): `ITextRange::SetText` inserts
text without `ME_UpdateRepaint`, so the paragraph stays marked for rewrap;
anything that then places the caret before the next repaint reaches
`cursor_coords`: `ITextRange::Select`, `EM_SETSEL` / `EM_EXSETSEL` (via
`set_selection`, which calls `update_caret` before `ME_Repaint`),
`WM_SETFOCUS` (`create_caret`) and `ITextRange::ScrollIntoView`.

Measured, `RichEdit20W` (riched20) and `RICHEDIT50W` (msftedit), a focused
multi-line control 120 pixels wide with "abc":

- change: `ITextRange::SetText` of a long text over the whole text
  (`tom_replace`), at the end (`tom_append`), or `EM_REPLACESEL` (a message
  path that rewraps, control);
- caret trigger: `ITextRange::Select` collapsed at character 23 (start of a
  wrapped line), `EM_SETSEL`, `EM_EXSETSEL`, focus moved back from another
  control, `ITextRange::ScrollIntoView(tomStart)`, or nothing.

Each case runs in a child process. `STATE` lines (initial, after the change,
after the trigger, after `UpdateWindow`): `EM_EXGETSEL`, text length,
`EM_GETLINECOUNT`, `EM_EXLINEFROMCHAR` and `EM_LINEINDEX` of the caret
position, `EM_POSFROMCHAR`, `GetCaretPos` and whether they match, focus.
The Wine test can then compare the caret with `EM_POSFROMCHAR` and the line
values rather than pixel values.

Local check (Wine 9.0, not an oracle): Inventor's assertion
(`caret.c, line 232`, exit 3) for `tom_replace` with every trigger and for
`tom_append` with focus and `ScrollIntoView`, in both classes.
