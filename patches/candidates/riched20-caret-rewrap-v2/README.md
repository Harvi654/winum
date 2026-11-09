# riched20-caret-rewrap v2: Windows-derived test

Candidate only: `patches/series`, the production patch and its page marker
are unchanged. Substitute `riched20-caret-rewrap.patch` from this folder for
the production file at its place in the series (entry 22).

## What changed from v1

The code change is the same as v1 (`cursor_coords` rewraps a paragraph
still marked `MEPF_REWRAP` instead of failing the assertion). v2 adds a
Wine header and the test `test_caret_after_settext` in
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
`SetText` (Windows wraps at once), and the selection right after `SetText`
(Windows keeps 0, Wine 9 moved it to the end); with focus the test compares
the caret with whatever the selection is.

## Expected results

- vanilla Wine 11.20: the test aborts at the first case (the assertion
  Inventor hit; in the MEASURE under Wine 9.0, 7 of the 12 TOM sequences
  per class aborted);
- with v1 or v2: all checks pass.

## Validation of v2

- L0: the full series applies at `--fuzz=0` on Wine 11.20 with this file
  substituted, 62/62 (2026-11-09).
- L1 of the new test, L2, cross review (Codex): pending.
