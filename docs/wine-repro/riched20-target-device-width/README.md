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

Pending.
