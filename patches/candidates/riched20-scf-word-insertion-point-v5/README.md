# riched20-scf-word-insertion-point v5: class flag initialised

Candidate only: `patches/series`, the production patch and its page marker
are unchanged. Substitute `riched20-scf-word-insertion-point.patch` from
this folder for the production file at its place in the series (entry 23).
Supersedes `../riched20-scf-word-insertion-point-v4/` (read its README for
the rule, the measurements and the per-class tables; they are unchanged);
review: `docs/wine-repro/cross-review-2026-11-09-riched20-scf-word-insertion-point-v4.md`.

## What changed from v4

- P1: `ME_MakeEditor` sets `msftedit = FALSE` for every editor (the
  editor is allocated with `malloc`); only `create_windowed_editor` sets
  it for RichEdit50W windows. Before, RichEdit20W, riched32 and windowless
  editors read an unset bit.
- P2: `test_scf_word_classes` in `dlls/msftedit/tests/editor.c` creates
  and destroys a RichEdit50W and a RichEdit20W window in turn, three
  rounds, in one process, at the measured distinguishing point `ab|␣`
  (msftedit: nothing; riched20: `ab␣`), ordinary assertions for both.
  With the flag forced to TRUE (simulating the unset bit) the test fails
  for RichEdit20W (3 failures); with v5 it passes.
- P3 (cursor lookups from the document start for each character) is not
  changed: the measured texts are short; noted as an open point.

## Local run (2026-11-09, Wine 11.20 x86_64, all six batch 1 candidates)

riched20 richole (181587), editor (11056), txtsrv (215), msftedit editor
(382), richole (117): 0 failures.

## Validation of v5

- L0: the full series applies at `--fuzz=0` with the batch 1 candidates
  substituted, 62/62 (2026-11-09).
- L1 (`msftedit:editor`, `riched20:editor`), L2 (`test-run/batch1-4`),
  L4 (Inventor GD&T), cross review (Codex): pending.

## L1 of test-run/batch1-4 (2026-11-09)

Run 42698489281, Windows 10, x86_64 and i386: the new test passed
(`docs/wine-repro/batch1-measure/l1-2026-11-09-batch1-4/`).

## L1 and L2 of test-run/batch1-6 (2026-11-10)

Run 42772681118 (`docs/wine-repro/batch1-measure/l1-2026-11-10-batch1-6/`).
L1: unchanged since batch1-4 (`msftedit:editor`, `riched20:editor` passed there); in this run as part of the series. L2 (paired, both architectures): 6 regressions, all known and
none from batch 1 (production setrows v1, mousemove v1, urlmon v1 and its
tests-first test; ntdll threadpool timing).
