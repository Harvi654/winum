# Batch 1 L1 and L2 of test-run/batch1-5 (2026-11-10)

Run 42718217301: series with toolbar v4, header v2, ownerdraw v2, caret
v2, SCF_WORD v5, textfont v5.

## L1 (offline Windows 10 VM, text results only)

| Unit | x86_64 | i386 |
| --- | --- | --- |
| comctl32:toolbar | 5309 tests, 0 failures | 2339 failures, exit 255 - **test bug, see below** |
| riched20:richole | 183279, 0 | 183279, 0 |
| msftedit:richole | 150, 0 | 150, 0 |

## L2 (paired with vanilla, both architectures)

420 units; 6 regressions, 2 fixed, 2 moved, 49 other changes (new checks).
The regressions:

- `comctl32.i386.toolbar`: the same failures as the i386 L1 - **ours**,
  the test bug below.
- `comctl32.x86_64.toolbar`, `toolbar.c:2134` ("Unexpected number of rows
  5 (expected 9)"): production toolbar setrows v1, fixed by the setrows v2
  candidate (known).
- `comctl32.x86_64.button`, `button.c:2631` x4 (WM_MOUSEMOVE instead of
  WM_PAINT): production server mousemove v1, fixed by mousemove v2 (known).
- `urlmon.*.sec_mgr`, `sec_mgr.c:2444`: production urlmon v1 with its
  tests-first test, fixed by the urlmon v2 candidate (known).
- `ntdll.x86_64.threadpool`, `threadpool.c:1645` "succeeded inside todo"
  (timer merging): timing; not in a DLL the batch touches.

## The toolbar i386 failure

Not the VM: Wine's own i386 run in L2 fails the same way. The first
failure is `toolbar.c:595` inside `test_padding_height` (all earlier
tests pass): the test passed an uninitialised `HWND` to
`rebuild_toolbar()`, which destroys the window the variable holds first.
On i386 the stack held the test's parent window, so it was destroyed and
every later toolbar (a child of it) failed. The same bug was in v3's test;
the batch1-4 README called it an environment failure, which was wrong
(corrected there). Fixed in toolbar v5 (`HWND toolbar = NULL`); the code
is unchanged.
