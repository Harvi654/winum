# Windows 10 assertion L1 (2026-11-09)

Maintainer's offline Windows 10 VM, Wine test unit user32:attachment,
x86_64 and i386, returned after the L1 bundle of
[run 42695491325](https://github.com/Harvi654/winum/actions/runs/42695491325),
branch test-run/batch4-1 at 36b639e (`tests-series-g1/l1-bundle/`).
This is the Wine test executable, not the standalone l1.c wrapper.
The run's test-only patch has the same assertion body as the v2 candidate;
source parity was checked before the paired run was pushed.

| Architecture | Checks | Todo / flaky | Failures | Skipped | Exit |
| --- | --- | --- | --- | --- | --- |
| x86_64 | 10195 | 0 / 0 | 0 | 0 | 0 |
| i386 | 10195 | 0 / 0 | 0 | 0 | 0 |

The unit's final ordinary assertion requires all 44 scenarios to complete.
Its Wine-only todo branches run as ordinary assertions on native Windows.
These results establish a native pass of the candidate test, independently
of the earlier MEASURE program's successful exit. They do not establish
repeated-run stability, Wine L2 success or consumer L4.

Before committing, every byte of the three returned text files was checked
against the expected summary/exit-code grammar. They contain only a numeric
test process ID, the unit name, counts and architecture/exit status; no
user names, machine names, paths, e-mails or vendor text. Original CRLF
bytes are retained and SHA256SUMS covers the three files. The returned
summaries do not embed binary hashes or build identifiers.
