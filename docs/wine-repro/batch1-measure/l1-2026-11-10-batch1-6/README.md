# Batch 1 L1 and L2 of test-run/batch1-6 (2026-11-10)

Run 42772681118: series with toolbar v5, header v2, ownerdraw v2, caret
v2, SCF_WORD v5, textfont v6.

## L1 (offline Windows 10 VM, text results only)

| Unit | x86_64 | i386 |
| --- | --- | --- |
| comctl32:toolbar | 5309 tests, 0 failures | 5309, 0 |
| riched20:richole | 183279, 0 | 183279, 0 |
| msftedit:richole | 182, 0 | 182, 0 |

All exit 0. The toolbar i386 failure of batch1-4/batch1-5 (the test's
uninitialised handle) is gone with v5.

## L2 (paired with vanilla, both architectures; checked by the maintainer)

6 regressions, all known and none from batch 1: production toolbar
setrows v1, server mousemove v1, urlmon v1 with its tests-first test
(each fixed by its candidate), and the timing-dependent ntdll threadpool
timer check. Toolbar i386 no longer appears.
