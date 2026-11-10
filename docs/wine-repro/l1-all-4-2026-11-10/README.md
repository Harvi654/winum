# L1 and L2: test-run/all-4 (2026-11-10)

## L1 (Windows 10, offline VM, two runs)

Bundle of run 42865253227 (`test-run/all-4`, `ef76d8a`), x86_64 and i386,
run twice. Text results, CR removed.

| Unit | x86_64 run 1 | x86_64 run 2 | i386 run 1 | i386 run 2 |
| --- | --- | --- | --- | --- |
| `comctl32:toolbar` (setrows v4) | 10353, 0 failures | 10353, 0 | 10353, 0 | 10353, 0 |
| `user32:mousemove` (v5) | 40, 0 failures | 34, 0 | 34, 0 | 34, 0 |

`user32:mousemove` x86_64 run 1 has 40 checks: 7 moves arrived instead of
one in a scenario (the recipient checks of the extra moves), within v5's
bounds. Explanation from the maintainer: after double-clicking the batch
file the mouse is moved down to the Linux host's dock (the VM is shown
through Looking Glass). The batch file runs x86_64 first, so that host
movement falls into the x86_64 run - which is why only x86_64 ever saw
extra moves (all-2, all-3, all-4). Not a Windows behaviour to model;
v5's bounds tolerate it, the exact zero scenarios were unaffected.

## L2 (run 42865253227)

One regression, `ntdll.i386.time` (`time.c:460`), the timing check seen
in all-1 and all-2 that no patch touches. Moved: `user32.win` as
before. `comctl32:toolbar` 2199 -> 10353 checks, `user32:mousemove` 33
checks (1 todo), all other units as in all-3, no failures in units only
the series has.
