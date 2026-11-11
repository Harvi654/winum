# L1, L2 and L4: test-run/all-5 (2026-11-10/11)

## L1 (Windows 10, offline VM)

Bundle of run 42889340446 (`test-run/all-5`, `f5b9397`), one run.

| Unit | x86_64 | i386 |
| --- | --- | --- |
| `comctl32:toolbar` (setrows v5) | 11108 checks, 1 failure | 11107 checks, 0 failures |

The x86_64 failure is in Wine's older `test_WM_PAINT`
(`toolbar.c:3495`, "WM_PAINT without TBSTYLE_TRANSPARENT"): two
`WM_GETICON` (0x7f) messages from outside the test entered its message
sequence. That test passed in all-3 and all-4 (four runs per
architecture); x86_64 runs first after the batch file is started, as with
the extra mouse moves of all-2 to all-4. Every check of the setrows tests
(`test_setrows_groups`, `test_setrows_palette`, `test_setrows_button_size`)
passed on both architectures.

## L2 (run 42889340446)

One regression: `user32.x86_64.input`, `input.c:6432` x1, "got pos
(49,51)" - the same `test_ClipCursor` check as in all-2 (`input.c:6398`
there). After `SetCursorPos(49, 51)`, `ClipCursor` and four relative
moves the cursor is back at the pre-clip position with the clip gone.
Local check (Wine 11.20 x86_64, Xvfb, `user32:input` six times each):
**vanilla** server 3 of 6 runs with the failure (1, 2 and 1 occurrence),
with the mouse move server patch 1 of 6. A timing-dependent check of
vanilla Wine, not caused by the series. Also moved: `ntdll.i386.exception`
and `user32.win` (same failures, other lines).

## L4 (Inventor 2027, build 42889351892)

The maintainer compared the drawing-text symbol palette with Inventor on
the Windows VM (screenshots, not committed; the VM had no GPU and
scales differently): same 5 columns, same groups (rows of 5, 5, 4 / 5, 5,
1 / 5, 5, 5, 3 / 1), separator lines across, "looks right". Pixel sizes
cannot be read from the two screenshots because of the different
scaling; the measurement of Inventor's exact call sequence on Windows
gives 40x24 buttons, which setrows v5 now reproduces.
