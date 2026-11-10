# L1: bundle of test-run/all-2 (2026-11-10)

Wine test programs from the L1 bundle of run 42815100059 (`test-run/all-2`,
`04ae708`), run by the maintainer on Windows 10 (offline VM), x86_64 and
i386. Text results only, CR removed; files in this folder.

| Unit | x86_64 | i386 | For |
| --- | --- | --- | --- |
| `comctl32:toolbar` | 5309, 0 failures | 5309, 0 failures | toolbar setrows v2 (Wine's `test_setrows`), padding v5 |
| `msxml3:domdoc` | 36894, 0 failures, 3 skips | same | parseerror v2 |
| `urlmon:sec_mgr` | 692, 0 failures, 8 skips | same | urlmon res zone v2 (with the null-terminate test) |
| `kernel32:actctx` | 3863, 0 failures | same | ntdll actctx threading model v2 |
| `user32:mousemove` | 39, **1 failure** | 34, 0 failures | mouse move v3 |

The skips are in Wine's existing tests (`domdoc.c:129` classes not
registered, `sec_mgr.c:1929` and `:2244`), not in checks the patches
added. Exit codes: every unit 0 except `user32 mousemove x86_64=1`.

`user32:mousemove` x86_64: "show child under: got 6 WM_MOUSEMOVE,
expected 1", all six to the new child; every other scenario as measured,
including the hidden-window move (the `todo_wine` check, ordinary on
Windows). i386 passed completely. See
`patches/candidates/server-mousemove-on-show-hide-v4/README.md`: v4 (pump
fix from cross review) goes to the next VM visit and runs twice on both
architectures.

Result: L1 passed for setrows v2's existing test, parseerror v2, urlmon
v2 and actctx v2 (kernel32 test). Not yet for mouse move.
