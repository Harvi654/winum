# riched20-caret-rewrap v2: Windows-derived test

Candidate only: `patches/series`, the production patch and its page marker
are unchanged. Substitute `riched20-caret-rewrap.patch` from this folder for
the production file at its place in the series (entry 22).

## What changed from v1

v1 rewrapped in `cursor_coords` instead of failing the assertion, but
kept the run and paragraph it had read from the cursor before wrapping;
wrapping splits runs, so the caret went to the first line (the new test
showed it: caret at 1,0, character 23 at 1,32). v2 takes the cursor's
character offset before wrapping, finds the cursor again from it
afterwards and reads run and paragraph then. It adds a Wine header and the
test `test_caret_after_settext` in
`dlls/riched20/tests/richole.c` (called last in `START_TEST`, away from
where `riched20-textfont-applytmp` v2 adds its test).

## Windows basis

MEASURE `docs/wine-repro/riched20-caret-rewrap/`, results
`windows-2026-11-09/`. A focused 120-pixel multi-line RichEdit20W with
"abc"; `ITextRange::SetText` of a long text over the whole text or at the
end; then `ITextRange::Select` (23,23), `EM_SETSEL`, `EM_EXSETSEL`, the
focus from another control, or `ITextRange::ScrollIntoView(tomStart)`.
Windows: no failure, S_OK / 23 returned, more than one line, the selection
23,23 on a line > 0, the caret at `EM_POSFROMCHAR` of the selection
(not checked after `ScrollIntoView`, which scrolls without moving the
caret).

Not asserted, Wine differs outside this patch: the line count right after
`SetText` (Windows wraps at once). The selection right after `SetText` is
`todo_wine` (Windows keeps 0,0; Wine restores a stale copy of the cursor
and reports 56,56 for the replace, 0,56 for the append); after the focus
the caret is compared only when the selection is empty.

## Expected results

- vanilla Wine 11.20: the test aborts at the first case (the assertion
  Inventor hit; in the MEASURE under Wine 9.0, 7 of the 12 TOM sequences
  per class aborted);
- with v1: no abort, but the caret checks fail (first line);
- with v2: all checks pass.

## Validation of v2

- L0: the full series applies at `--fuzz=0` on Wine 11.20 with this file
  substituted, 62/62 (2026-11-09).
- L1 (2026-11-09): **passed** - `riched20:richole` from the L1 bundle of `test-run/batch1-1`
  (run 42642179431) on Windows 10, x86_64 and i386: 183232 tests, 0 failures, exit 0
  (`docs/wine-repro/batch1-measure/l1-2026-11-09/`).
- L2, cross review (Codex): pending.

## Local run (2026-11-09)

Wine 11.20 built locally for x86_64 (Ubuntu, X11 under Xvfb), test units run
with the new tests applied, first on vanilla DLLs, then with all six batch 1
v2 candidates. Quick feedback only; L2 is the CI run.

| | vanilla | v1 code | v2 |
| --- | --- | --- | --- |
| riched20:richole | abort, `caret.c` line 232 | 4 failures (caret at the first line) | 181540 tests, 0 failures |

Round 2 of the fix passed. Open: `ITextRange::SetText` restores a stale
cursor (a separate fix, measured already: Windows keeps the selection).
