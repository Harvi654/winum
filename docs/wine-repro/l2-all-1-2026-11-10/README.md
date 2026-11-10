# L2: test-run/all-1 (2026-11-10)

Run 42788810738 on branch `test-run/all-1` (`b687417`): the production
series with the current version of every candidate substituted (64
entries, list in the branch's commit message), paired with vanilla Wine
11.20, groups g1-g5, both architectures. Build for the Inventor smoke test
from the same branch: Build Wine run 42788837110 (success).

## Compare report

428 units. Regressions: 1. Fixed: 0. Moved: 2. Other changes: 70.

- Regression: `ntdll.i386.time`, `time.c:460` x1 ("USD SystemTime /
  NtQuerySystemTime are out of order"). No patch touches the time code;
  a timing check, to be re-run before it is recorded anywhere as a pass.
- Moved: `user32.win` (both architectures), the same two failures 157
  lines later (a patched test file).
- The known production regressions of earlier runs (toolbar setrows v1,
  server mousemove v1, urlmon v1 and its tests-first test) are gone:
  their candidates replace them here.

## Units only in the series

`compare-tests.py` did not check failures in units that exist only on
the series side (new test files), so the report above says nothing about
them. The maintainer read them from the series artifacts (summary JSON,
one line per unit):

| Unit (x86_64 and i386 identical) | checks | todo | failures |
| --- | --- | --- | --- |
| `msi:ancestor` | 1330 | 0 | 0 |
| `msi:costing` | 872 | 0 | 0 |
| `msi:maintenance` | 166 | 0 | 0 |
| `msi:selection` | 3308 | 0 | 0 |
| `user32:attachment` | 12039 | 16 | 0 |
| `msftedit:editor` | 382 | 0 | 0 |
| `mspatcha:signature` | 50 | 0 | 0 |
| `shell32:selection` | 357 | 0 | 0 |
| **`wbemprox:timezone`** | 84 | 0 | **1** (`timezone.c:186`) |

`msftedit:richole` (in both, 178 checks, 5 todo, 0 failures) was in the
same query. The wbemprox failure is the `win_skip` predicted in its cross
review: the container runs in UTC, which is not the measured standard-time
setup, and `win_skip` counts as a failure on Wine. All units finished.

Since this run, `compare-tests.py` reports a series-only unit that fails
or does not finish as a regression.

## Not covered by this run

- `mshtml` (disabled by `WINEDLLOVERRIDES` in `test-series.sh`, in no
  group): mshtml document open v2 has no L2 here.
- `kernel32` was in no group, so `kernel32:actctx` (ntdll actctx
  threading model v2) did not run; added to g3 after this run.
- The absolute inactive-state view of `user32:attachment` (prerequisite
  absent under Xvfb, reported by `skip`, as before).

## Result per candidate

L2 without a new failure for: toolbar padding v5, header null layout v2,
owner-draw capture v2, toolbar setrows v2, statusbar v2, caret rewrap v2,
SCF_WORD v5, textfont v6, msxml3 parseerror v2, typedesc pointee v3,
typedesc z-offsets v2 (`oleaut32:typelib` with its four size todos, no
failure), server mousemove v2, input attachment v5 (user32 and neighbours),
urlmon res zone v2, msi null stream v2, mspatcha v2, shell32 folder view
v2, wininet seek v2. ntdll actctx v2: `ntdll` clean, `kernel32` not run.
**Not passed:** wbemprox timezone v2 (its own test, above). **Not run:**
mshtml v2.
