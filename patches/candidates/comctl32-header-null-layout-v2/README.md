# comctl32-header-null-layout v2: NULL members too, and a Windows-derived test

Candidate only: `patches/series`, the production patch and its page marker
are unchanged. Substitute `comctl32-header-null-layout.patch` from this
folder for the production file at its place in the series (entry 42).

## What changed from v1

v1 returned FALSE only for a NULL `HDLAYOUT`. On Windows a NULL `prc` or
`pwpos` fails the same way, without writing the other member, so v2 checks
all three before writing (`HEADER_Layout`, one line). It adds the Wine test
`test_hdm_layout_null` in `dlls/comctl32/tests/header.c`, run in the v5
and the v6 part of `START_TEST`.

## Windows basis

MEASURE `docs/wine-repro/comctl32-header-null-layout/`, results in
`windows-2026-11-09/`: every NULL variant returns 0, keeps the last error
and leaves the given `RECT`/`WINDOWPOS` untouched, in v5 and v6, A and W,
with and without `HDS_HIDDEN`; a valid layout afterwards returns TRUE with
x, y, cx from the rectangle and cy = the header height (0 with
`HDS_HIDDEN`, and then the rectangle's top stays).

The test does not check the header height itself (font and version
dependent: 20 in v5, 24 in v6 here) nor the `WINDOWPOS` flags (Windows
0x14, Wine 0x4, a difference outside this patch).

## Expected results

- vanilla Wine 11.20: the test crashes at the first NULL `HDM_LAYOUT` (the
  SOLIDWORKS crash);
- with v1: the NULL-`lParam` checks pass, the test crashes at NULL `prc`;
- with v2: all checks pass.

## Validation of v2

- L0: the full series applies at `--fuzz=0` on Wine 11.20 with this file
  (and the toolbar-padding v2 candidate) substituted, 62/62 (2026-11-09).
- L1 (2026-11-09): **passed** - `comctl32:header` from the L1 bundle of `test-run/batch1-1`
  (run 42642179431) on Windows 10, x86_64 and i386: 1208 tests, 0 failures, exit 0
  (`docs/wine-repro/batch1-measure/l1-2026-11-09/`).
- L2, cross review (Codex): pending.

## Local run (2026-11-09)

Wine 11.20 built locally for x86_64 (Ubuntu, X11 under Xvfb), test units run
with the new tests applied, first on vanilla DLLs, then with all six batch 1
v2 candidates. Quick feedback only; L2 is the CI run.

| | vanilla | v2 |
| --- | --- | --- |
| comctl32:header | crash at the first NULL `HDM_LAYOUT` (exit 5) | 1129 tests, 0 failures (v5 and v6) |

SOLIDWORKS' L4 is deferred (AGENTS.md); v2 changes only the NULL cases,
which crash in vanilla Wine.
