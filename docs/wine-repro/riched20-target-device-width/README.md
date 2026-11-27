# MEASURE: RichEdit target device line width and alignment

## Problem as the user saw it

In an Inventor 2027 drawing, the Format Text dialog of a datum identifier
symbol opens with an empty text box: the default letter ("A") is not
shown. Typing replaces it and the result appears in the drawing; the
text is only invisible in the dialog. The horizontal scroll bar has
nothing to scroll. Switching the paragraph to left alignment shows the
letter at once (maintainer, 2026-11-26, winum-11.22-3).

## What the application calls (Wine trace, `+richedit`)

The text box is a RichEdit window. Inventor sends
`EM_SETTARGETDEVICE` with a non-NULL DC and a line width of `0xa4e3`
(42211 twips, about 745 mm), inserts `"A"` with
`EM_SETTEXTEX(ST_SELECTION, 1200)` and sets the paragraph format three
times; the dialog shows centred alignment. Wine prints
`EM_SETTARGETDEVICE doesn't use non-NULL target devices`, uses the number
as the wrap width of the window and centres the letter in a line about
2800 pixels wide, so it lands about 1400 pixels to the right of a
630-pixel text box. `ME_UpdateScrollBar` reports a horizontal range of
4-5 pixels, the width of the text, not its position. Wine's code is the
`EM_SETTARGETDEVICE` case in `dlls/riched20/editor.c`.

## What is measured

`measure.c` creates a new 610 x 165 RichEdit for every case and records
where Windows puts character 0 (`EM_POSFROMCHAR`), the formatting
rectangle (`EM_GETRECT`), the horizontal scroll range (`GetScrollInfo`),
`EM_GETSCROLLPOS`, the line count and the return value of
`EM_SETTARGETDEVICE`:

- classes `RichEdit20W` (riched20.dll) and `RICHEDIT50W` (msftedit.dll);
- styles: plain multiline; with both scroll bars and
  `ES_DISABLENOSCROLL`; with both scroll bars and `ES_AUTOHSCROLL` /
  `ES_AUTOVSCROLL`;
- target device: none sent; NULL DC with 0, 1, 5000 and 42211; screen DC,
  memory DC, enhanced metafile DC and the "Microsoft Print to PDF" printer
  DC (if present; the default printer's name is never printed) with
  5000, 42211 and 100000 twips;
- left, centre and right alignment;
- order 0 as Inventor (target device, text, alignment) and order 1 (text,
  alignment, target device).

The DC Inventor passes is not known; the cases bracket it. One
tab-separated `case` line per case, header lines with the screen and
printer resolution, and `measure_complete cases=N failed=M` at the end.

## Build and run

CI (`.github/workflows/build-riched20-target-device-measure.yml`) builds
the Windows bundle with `scripts/build-measure-bundle.sh` and runs the
same program under winum-11.22-3 in Xvfb for comparison. Locally:

```bash
bash scripts/build-measure-bundle.sh /tmp/target-device-measure \
  docs/wine-repro/riched20-target-device-width/measure.list
(cd /tmp/target-device-measure && sha256sum -c SHA256SUMS)
```

Copy the verified folder to a fresh directory in the offline Windows 10
VM, run `run-measure.bat` from an interactive desktop without touching
mouse or keyboard, and return both `result-target-device-width-*.txt`
and `exitcodes.txt`.

## Results

Offline Windows 10 VM, 2026-11-26, both architectures: 612 cases, none
failed, exit 0. The centred 42211-twip rows of x86_64 are in
[results/windows-10/center-42211-x86_64.txt](results/windows-10/center-42211-x86_64.txt);
the Wine column is CI run 45861796621 (winum-11.22-3, Xvfb, 96 dpi).

| Centred, 42211 twips, DC not NULL | Windows 10 | winum-11.22-3 |
| --- | --- | --- |
| `EM_POSFROMCHAR(0)` x, RICHEDIT50W / RichEdit20W | 1404 / 1403 | 1404 / 1404 |
| Horizontal scroll range, both scroll bars, `ES_DISABLENOSCROLL` | 0..2814, page 585 (2815 in RichEdit20W) | 0..1, page 0 |
| Horizontal scroll range, both scroll bars, `ES_AUTOHSCROLL` | 0..2814, page 602 | 0..100 (order 0) or none (order 1) |
| `EM_GETSCROLLPOS` | 0,0 | 0,0 |

- Wine already lays the line out as Windows does: the target device's
  line width (42211 twips = 2814 pixels at 96 dpi) is the line, and the
  centred letter sits at its middle, outside the 610-pixel window, on
  Windows too. The printer DC at 600 dpi gives the same position, so the
  width is not converted through the DC's resolution.
- The difference is the horizontal scroll range. Windows covers the whole
  line (0..2814) so the letter can be scrolled to; Wine's range is the
  width of the text (`editor->nTotalWidth`, the widest paragraph), so the
  scroll bar has nothing to scroll.
- Side finding, not used by Inventor: with a NULL DC and 42211,
  RichEdit20W uses the width as well (x 1403), RICHEDIT50W centres in the
  window (x 298) but still reports the 0..2814 range; Wine centres in the
  window (x 301) for both.

Inventor's dialog on Windows (maintainer's offline VM, 2026-11-26) shows
the letter at the right edge of the text box with the scroll thumb in the
middle of the bar: the view is scrolled so that the caret after the letter
is visible. The Wine trace has no scroll message from Inventor
(`EM_SETSCROLLPOS`, `EM_SCROLLCARET`, `WM_HSCROLL`), so RichEdit itself
scrolls. Round 2 of the program keeps the caret after the letter, gives
the control the focus and reads the scroll position and the letter's
position again (`focus_*` columns) to measure that step.

### Round 2 (focus), 2026-11-26

Both architectures 612 cases, exit 0;
[centred 42211-twip rows](results/windows-10/round2-focus-center-42211-x86_64.txt).
With a scroll bar and a DC, giving the control the focus scrolls the view
so that the caret after the letter is at the right edge: scroll position
827 (`ES_DISABLENOSCROLL`, page 585; RichEdit20W 826) or 810
(`ES_AUTOHSCROLL`, page 602; RichEdit20W 809), the letter then at x 577
or 594. Before the focus the position is 0 on Windows. Without scroll
bars nothing scrolls. Under winum-11.22-3 the position stays 0 after the
focus, because the range ends at 1 (CI run 45863190609).
