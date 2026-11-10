# Batch 1 L1 run of test-run/batch1-4 (2026-11-09)

L1 bundle of run 42698489281 (series with toolbar v3, header v2,
ownerdraw v2, caret v2, SCF_WORD v5, textfont v4), run by the maintainer
in the offline Windows 10 VM. Text results only.

| Unit | x86_64 | i386 |
| --- | --- | --- |
| comctl32:toolbar | 5043 tests, 0 failures | **2083 failures, exit 255 - a test bug, see below** |
| comctl32:header | 1208, 0 | 1208, 0 |
| comctl32:button | 14224, 0 | 14224, 0 |
| riched20:richole | 183279, 0 | 183279, 2 failures (existing `test_clipboard`, see below) |
| riched20:editor | 11059, 0 | 11059, 0 |
| msftedit:editor | 382, 0 | 382, 0 |
| msftedit:richole | 121, 0 | 121, 0 |

- toolbar i386: **corrected 2026-11-10 - not the environment.** The test
  passed an uninitialised `HWND` to `rebuild_toolbar()`, which destroyed
  the test's parent window on i386; every later toolbar failed. Wine's own
  i386 L2 run of batch1-5 shows the same. See
  `../l1-2026-11-10-batch1-5/README.md`; fixed in toolbar v5.
- riched20:richole i386: `richole.c:5139`, Wine's existing clipboard test,
  "Copy failed: 0x800401d0" (CLIPBRD_E_CANT_OPEN, the clipboard was busy);
  not a new test.

So the new tests of header v2, ownerdraw v2, caret v2, SCF_WORD v5 and
textfont v4 passed on both architectures; toolbar v3's passed on x86_64 and
failed on i386 because of the test bug.
