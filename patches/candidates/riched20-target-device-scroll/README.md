# riched20 target device line width and the horizontal scroll range

Inventor 2027's Format Text dialog for a datum identifier symbol opens
with its default letter invisible. The dialog's RichEdit gets
`EM_SETTARGETDEVICE` with a 42211-twip line width and a centred
paragraph; Wine lays the letter out where Windows does, about 1400 pixels
to the right, but keeps the horizontal scroll range at the width of the
text, so it can never be scrolled into view. Windows covers the whole
line with the range and scrolls the caret into view when the control gets
the focus.

[MEASURE program, Windows results and the trace](../../../docs/wine-repro/riched20-target-device-width/README.md).

| Level | Evidence |
| --- | --- |
| MEASURE | Offline Windows 10, both architectures, 612 cases per round, two rounds (2026-11-26). |
| Test | `patches/tests-first/riched20-tests-target-device-scroll.patch`, `test_target_device_scroll` in `dlls/riched20/tests/editor.c`, both classes. |
| L0 | Pending (overlay workflow). |
| L1 | Pending (L1 workflow; run `run-tests.bat` in the offline VM at 96 dpi). |
| L2 | Pending (paired riched20 and msftedit suites; the new test runs on both sides and should fail only on the baseline). |
| L4 | Pending: Inventor datum identifier letter visible when the dialog opens; AutoCAD text editing (MText) unchanged. |
| Cross review | Pending: Codex (Claude wrote the patch). |

The fix keeps a second width, the target line width in pixels, next to
the text width and lets the scroll range cover the larger one. Nothing
else that uses the text width changes. Measured and left as they are:
the NULL-DC cases (Windows widens the range there too) and the moment of
the caret scroll (Windows waits for the focus, Wine scrolls when the text
or selection changes).

Build the overlay into a copy of the winum-11.22-3 runtime as for the
other riched20 candidates (`lib/wine/<arch>-windows/riched20.dll`, after
`sha256sum -c SHA256SUMS`).
