# comctl32-toolbar-setrows-wrapable v2 (candidate)

Replaces `patches/comctl32-toolbar-setrows-wrapable.patch` once reviewed
and validated. Written by Claude, 2026-11-08; v1 is the Inventor patch from
Autodesk-Inventor-for-Linux `eddb691` (wine-11.19-1).

## Why

The first full L2 runs of the series (`docs/wine-repro/l2-series-2026-11-08.md`)
showed `comctl32/tests/toolbar.c:2134` failing with the series in both
architectures and both runs, and passing on vanilla Wine. That is
`test_setrows`, whose table `tbrows_results` describes Windows: 9 buttons,
`TB_SETROWS` with 8 rows and `bLarger` TRUE must give 9 rows (one button per
row), with `bLarger` FALSE 5 rows. v1 ignored `bLarger` and always took the
next smaller number of rows (5).

## The change

After v1 has found the fewest buttons per row that fit into the requested
rows, v2 steps back one column when `bLarger` is set and the result has
fewer rows than requested, so the next larger achievable number of rows is
used. Everything else of v1 (groups between separators start a new row,
separators are not counted, the column count is kept on resize, separator
lines in vertical toolbars) is unchanged. A simulation of the table
(rows = ceil(9 / columns)) gives 1, 2, 3, 5, 9, 1 as expected.

Applies with `--fuzz=0` after `comctl32-toolbar-padding-height.patch`
(the order of `patches/series`).

## Open before it replaces v1

- cross review by Codex;
- L2: done 2026-11-08, run 42497038726 (`test-run/toolbar-v2`):
  `toolbar.c:2134` passes in both architectures, no new comctl32 failure
  (the remaining user32/comctl32 mouse-move failures belong to
  `server-mousemove-on-show-hide`);
- a Wine test for what Inventor needed - groups between separators in
  `TB_SETROWS` - measured on Windows first (MEASURE program); v1's
  separator handling was compared with Windows only visually;
- L4: Inventor's drawing text editor symbol palette still shows 5 columns
  and its groups as on Windows.

## L2: test-run/all-1 (2026-11-10)

[docs/wine-repro/l2-all-1-2026-11-10](../../../docs/wine-repro/l2-all-1-2026-11-10/README.md), run 42788810738, with every current candidate: no new failure in the touched DLLs, both architectures.

## MEASURE for a Wine test (2026-11-10)

The maintainer chose a measurement over `l1` with an open point:
`docs/wine-repro/comctl32-toolbar-setrows/` (separator groups, rows,
`bLarger`, wrapable styles, comctl32 5 and 6, Inventor's palette
sequence). From its Windows results: a Wine test for setrows v3, or a
confirmation that v2 matches.

## L1: bundle of test-run/all-2 (2026-11-10)

[docs/wine-repro/l1-all-2-2026-11-10](../../../docs/wine-repro/l1-all-2-2026-11-10/README.md), Windows 10: `comctl32:toolbar` 5309 checks, 0 failures, x86_64 and i386 - Wine's existing `test_setrows` only. The MEASURE of separator groups (first run, same visit) shows Windows placing groups differently from v2: see `docs/wine-repro/comctl32-toolbar-setrows/README.md`. A v3 follows the complete measurement.

**Superseded by `../comctl32-toolbar-setrows-v3/` (2026-11-10)** after
the complete measurement.
