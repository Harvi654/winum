# riched20-textfont-applytmp v4: exact class in the RichEdit20W test

Candidate only: `patches/series`, the production patch and its page marker
are unchanged. Substitute `riched20-textfont-applytmp.patch` from this
folder for the production file at its place in the series (entry 24).
Supersedes `../riched20-textfont-applytmp-v3/` (its README describes the
change and the measurements); review:
`docs/wine-repro/cross-review-2026-11-09-riched20-textfont-applytmp-v3.md`.
**Depends on `riched20-scf-word-insertion-point` v5** (entry 23), which
adds the class flag and now initialises it (review P1).

## What changed from v3

- P2: `test_textfont_applytmp_riched20` creates a `RichEdit20W` window
  (the class that was measured) through a small helper, instead of the
  suite's RichEdit20A factory. Then, after loading msftedit.dll, three
  rounds of a RichEdit50W and a RichEdit20W window in turn in one
  process: `Reset(tomApplyTmp)` S_OK and E_INVALIDARG, ordinary
  assertions. With the class flag forced to TRUE the RichEdit20W checks
  fail (8 failures); with SCF v5 + textfont v4 they pass.
- The code is unchanged from v3.
- P3 (what one font does after a full temporary cycle) stays open until
  `docs/wine-repro/riched20-textfont-applytmp/measure-followup.c`
  returns (bundle of run 42662108608).

## Local run (2026-11-09, Wine 11.20 x86_64)

riched20:richole 181587 tests, msftedit:richole 117 tests: 0 failures.

## Validation of v4

- L0: 62/62 at `--fuzz=0` with the batch 1 candidates (2026-11-09).
- L1 (`riched20:richole`, `msftedit:richole`), L2 (`test-run/batch1-4`),
  L4 (Inventor), cross review (Codex): pending.
