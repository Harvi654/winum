# comctl32-toolbar-padding-height v5: test fix

Candidate only: `patches/series`, the production patch and its page marker
are unchanged. Substitute `comctl32-toolbar-padding-height.patch` from this
folder for the production file at its place in the series (entry 10).
Supersedes `../comctl32-toolbar-padding-height-v4/` (its README has the
change and the measurements; Codex approved v4's code:
`docs/wine-repro/cross-review-2026-11-10-comctl32-toolbar-padding-height-v4.md`).

## What changed from v4

Only the test: `test_padding_height` declared `HWND toolbar;` without a
value and passed it to `rebuild_toolbar()`, which first destroys the
window it is given. On i386 the stack held the test's parent window, so
every later toolbar of the unit failed (L1 of batch1-4 and batch1-5 on
i386, and Wine's own i386 run in the batch1-5 L2: the same failures,
starting in this test at `toolbar.c:595`). v5 initialises it to NULL.
The DLL change is byte-identical to v4.

## Validation of v5

- L0: 62/62 at `--fuzz=0` with the batch 1 candidates (2026-11-10).
- L1: x86_64 passed with v4's test body (5309 tests, run 42718217301);
  both architectures pending for v5 (`test-run/batch1-6`).
- L2: batch1-5's x86_64 toolbar result has only the known setrows v1
  failure; i386 pending.
- L4 (Inventor palette): pending. Cross review of the one-line test fix:
  pending (Codex).
