# L1 and L2: test-run/all-6 (2026-11-11)

`test-run/all-6` (`e9ba167`) = all-5 plus msxml3 parseerror v3. Run
42997560598.

## L1 (Windows 10, offline VM)

L1 bundle of run 42997560598, `msxml3:domdoc` only, one run.

| Unit | x86_64 | i386 |
| --- | --- | --- |
| `msxml3:domdoc` (parseerror v3) | 36900 checks, 0 failures, 3 skipped, exit 0 | 36900 checks, 0 failures, 3 skipped, exit 0 |

The three skips are DOMDocument40 / MXNamespaceManager40, which this
Windows has not registered (as in every earlier domdoc run). The new v3
check (after `loadXML("")`, `get_errorCode` returns S_OK and 0xc00ce558,
DOMDocument30 and DOMDocument60) passed on both architectures, together
with the v2 `get_filepos` checks. Result files are the text output of
the VM run.

## L2 (run 42997560598)

516 units. Regressions: 1. Fixed: 2. Moved: 2 (`user32.win`, as before).

- `ntdll.i386.time`, `time.c:460` x1 ("USD SystemTime / NtQuerySystemTime
  are out of order") - the same timing check as in all-1 and all-2. In
  this run the same line failed in **vanilla** x86_64 and passed in the
  series x86_64 (listed as "fixed"). No patch touches the time code. A
  timing-dependent check of vanilla Wine, not caused by the series.
- Fixed: `ntdll.x86_64.time` (above), `ntdll.x86_64.threadpool`
  `threadpool.c:1645` (timing, no patch touches the thread pool).
- `msxml3:domdoc` 42884 -> 42976 checks on both architectures without a
  failure (the v3 test).

Units only in the series, all finished without failures: msi
`ancestor`, `costing`, `maintenance`, `selection`; `user32:attachment`
(16 todo), `user32:mousemove` (33, 1 todo), `msftedit:editor`,
`mspatcha:signature`, `shell32:selection`, `wbemprox:timezone`. The
`user32.input` cursor-position check of all-2 and all-5 did not come back.
