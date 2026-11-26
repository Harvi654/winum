# riched20 target device line width and the horizontal scroll range

Inventor 2027's Format Text dialog for a datum identifier symbol opens
with its default letter invisible. The dialog's RichEdit gets
`EM_SETTARGETDEVICE` with a 42211-twip line width and a centred
paragraph; Wine lays the letter out where Windows does, about 1400 pixels
to the right, but keeps the horizontal scroll range at the width of the
text, so it can never be scrolled into view. Windows covers the whole
line with the range and scrolls the caret into view when the control gets
the focus.

[MEASURE program, Windows results and the trace](https://github.com/Harvi654/winum/tree/9707b4d3e35c4cbf1b8b33204cea17985ead84e5/docs/wine-repro/riched20-target-device-width).

| Level | Evidence |
| --- | --- |
| MEASURE | Offline Windows 10, both architectures, 612 cases per round, two rounds (2026-11-26). |
| Test | `patches/tests-first/riched20-tests-target-device-scroll.patch`, `test_target_device_scroll` in `dlls/riched20/tests/editor.c`, both classes. |
| L0 | Both riched20.dll architectures built after zero-fuzz application to the winum-11.22-3 series (2026-11-26). [Overlay](https://github.com/Harvi654/winum/actions/runs/45865073202/artifacts/15139632984). |
| L1 | Offline Windows 10, 96 dpi, 2026-11-26: 83 tests per architecture, 0 failures, todos or skips, exit 0. [L1 bundle](https://github.com/Harvi654/winum/actions/runs/45865705834/artifacts/15140801437). |
| L2 | Paired riched20 and msftedit suites, 2026-11-26: 10 units, 0 regressions; the new test fails on the baseline and passes with the candidate (`editor.c` nMax and visibility checks, both architectures). [Run 45865705690](https://github.com/Harvi654/winum/actions/runs/45865705690). |
| L4 | Inventor 2027, 2026-11-26: the dialog opens scrolled with the thumb in the middle as on Windows, and the letter can now be scrolled to. It sits just past the right edge of the box (Windows shows it inside, at the edge); the smallest scroll shows it. Maintainer: acceptable, the open point below. AutoCAD 2027, 2026-11-26: MTEXT (several lines, centred, reopened), TEXT and dimension text edit as before. |
| Cross review | Codex, 2026-11-26. No blocker found for the measured scope. See review below. |

The fix keeps a second width, the target line width in pixels, next to
the text width and lets the scroll range cover the larger one. Nothing
else that uses the text width changes. Measured and left as they are:
the NULL-DC cases (Windows widens the range there too) and the moment of
the caret scroll (Windows waits for the focus, Wine scrolls when the text
or selection changes).

Build the overlay into a copy of the winum-11.22-3 runtime as for the
other riched20 candidates (`lib/wine/<arch>-windows/riched20.dll`, after
`sha256sum -c SHA256SUMS`).

## Open point

After the focus scroll Inventor's letter ends a few pixels past the right
edge, where Windows shows it inside the box. Not yet traced. Likely
causes: Wine scrolls only until the caret position is in view, not the
character after it; the caret is before the letter when Wine scrolls; or
the visible width was taken before the vertical scroll bar appeared. The
L1 test's visibility check passes on Windows; the paired L2 shows whether
it passes under Wine in the plain case.


## Cross review (Codex, 2026-11-26)

Reviewed the entire runtime patch, tests-first patch and candidate
README at 38e4b23. Read the MEASURE README, source, manifest list, Wine
runner and both saved native extracts in full at 9707b4d. Read the
relevant Wine 11.22 source paths for target-device setup, rewrap, scroll,
drawing and sizing. The rest of the Wine tree and unrelated CI were
outside this review.

- At 96 DPI, MulDiv rounds 42211 twips to 2814 pixels and 100000 twips
  to 6667. Adding one pixel for RichEdit20W reproduces 2815 and 6668.
  The saved native extracts show 42211 across reference DC kinds and
  both scrollbar styles. The 100000 expectations appear in the passing
  Windows L1. The formula uses the layout context DPI rather than the
  reference DC passed in the message. The zoomed range is unmeasured.
- Only the non-NULL EM_SETTARGETDEVICE path sets a nonzero nAvailWidth.
  The NULL path resets it and rewraps, clearing target_scroll_width.
  The Windows NULL-DC behavior is a known compatibility difference.
- The candidate uses the extra extent only for horizontal scrollbar
  range and visibility. EN_REQUESTRESIZE, TxGetNaturalSize and border
  painting continue to read nTotalWidth.
- The remaining Inventor clipping is open. The test checks the start
  position of the letter against the client width. It does not check
  the entire glyph against the formatting rectangle or reproduce
  Inventor's timing. The caret position and the formatting rectangle
  before and after focus are needed to distinguish the hypotheses.

## Combined L4 (2026-11-26, evening)

Overlay with all four candidates built from one winum-11.22-3 tree
(tab stop -> paragraph navigation -> target device scroll, ole32 v2;
[run 45880891853](https://github.com/Harvi654/winum/actions/runs/45880891853/artifacts/15147073657),
SHA-256 of archive and DLLs verified), on copies of the prefixes:

- Inventor 2027: note text, dimension text, datum identifier editor
  (letter reachable by scrolling), drag and drop, open/save/reopen - ok.
- AutoCAD 2027: MTEXT (several lines, centred, reopened), TEXT,
  save/reopen - ok.
- Revit 2027.4, DirectX Software: New project opens "Podlaží 1". A wall
  drawn afterwards gives Revit's warning that none of the created
  elements is visible in the view - the drawing problem recorded in the
  Revit repository, not RichEdit or OLE.
