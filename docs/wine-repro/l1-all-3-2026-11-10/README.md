# L1 and L2: test-run/all-3 (2026-11-10)

## L1 (Windows 10, offline VM, two runs)

Bundle of run 42835130976 (`test-run/all-3`, `a946877`), run twice by the
maintainer (`_run1`, `_run2`), x86_64 and i386. Text results, CR removed.

| Unit | x86_64 run 1 | x86_64 run 2 | i386 run 1 | i386 run 2 |
| --- | --- | --- | --- | --- |
| `comctl32:toolbar` (setrows v3) | 9069, 0 failures | 9069, 0 | 9069, 0 | 9069, 0 |
| `user32:mousemove` (v4) | 35, **1 failure** | 34, 0 | 34, 0 | 34, 0 |

`user32:mousemove` x86_64 run 1: "five show/hide pairs: got 2
WM_MOUSEMOVE, expected 1", both to the measured window. With the all-2
run ("show child under: got 6"), x86_64 Windows sent more than one move
in two of three runs; i386 always one. The test is wrong there, not the
patch: mouse move v5 bounds the counts (at least one; a burst fewer than
one per change).

## L2 (run 42835130976)

No regression (compare step green). Moved: `user32.win` as before. The
ntdll time check, `urlmon.i386.protocol` timeout and `user32.x86_64.input`
cursor position check of all-2 did not come back. All units only in the
series finished without failures (`user32:mousemove` 33 checks, 1 todo;
`wbemprox:timezone` 83; the msi, msftedit, mspatcha, shell32 and
attachment units as in all-2). `comctl32:toolbar` 2199 -> 9069 checks.

## L4 (Inventor 2027, build 42835150574)

Mouse move: the 32 ms default works (recorded before). Setrows v3: the
drawing-text symbol palette **fell apart** (screenshot from the
maintainer, not committed: the columns split by a vertical line, the
separators inside rows). Cause: Inventor changes the palette's width
after `TB_SETROWS`; v3 dropped the rows and handed the toolbar to Wine's
width wrap. Setrows v4 does what Windows did in the measurement after a
width change (groups kept, buttons per row from the width).
