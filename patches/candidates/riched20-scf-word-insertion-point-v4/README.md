# riched20-scf-word-insertion-point v4: msftedit's rule for RichEdit50W only

Candidate only: `patches/series`, the production patch and its page marker
are unchanged. Substitute `riched20-scf-word-insertion-point.patch` from
this folder for the production file at its place in the series (entry 23).
Supersedes `../riched20-scf-word-insertion-point-v3/`.

## What changed from v3

Inventor's text editors are RichEdit50W windows (maintainer's `+win`
trace in Inventor 2027: 56x RichEdit50W, 436x RICHEDIT50W, no RichEdit20W).
v3 applied msftedit's rule to both classes and so changed 8 measured
RichEdit20W points that vanilla had right; AGENTS.md forbids that. v4:

- `editstr.h`: a flag `msftedit` in `ME_TextEditor`;
- `txthost.c`, `create_windowed_editor`: set when the window's class is
  RichEdit50W (compared case-insensitively; Wine registers both classes
  with one window procedure). Windowless text services keep riched20's
  behaviour;
- `editor.c`: the msftedit rule (see v3's README) only with the flag; for
  RichEdit20W the empty-selection case is Wine's word movement, as in
  vanilla; for both classes `SCF_WORD` with a selection formats only the
  selection (measured for both);
- new: a point between a word and a paragraph mark formats the mark (both
  boundary points that v3 left `todo_wine`).

The same flag is used by `riched20-textfont-applytmp` v3 (next in the
series), which needs the class too.

## Check against the measurements (local Wine 11.20, x86_64)

| | vanilla | v4 | vanilla right, v4 wrong |
| --- | --- | --- | --- |
| msftedit, first run (174) | 113 | **174** | 0 |
| msftedit, boundary (50) | 3 | **50** | 0 |
| riched20 class, first run (174) | 121 | 127 | 0 |
| riched20 class, boundary (50) | 11 | 11 | 0 |

## Tests

- `dlls/msftedit/tests/editor.c` (new unit `msftedit:editor`): every
  measured msftedit point, no `todo_wine`. Vanilla 102 failures, v4 0.
- `dlls/riched20/tests/editor.c`, `test_EM_SETCHARFORMAT_word_point`: the
  same cases with RichEdit20W and riched20's measured values; the 81
  points where Wine's word movement differs from Windows riched20 are
  `todo_wine` (as in vanilla), every other point is an ordinary check, so
  a change of the RichEdit20W behaviour fails. Vanilla 6 failures (the
  selections), v4 0; the rest of the unit unchanged (11056 tests, 0
  failures).

## Open points

- RichEdit20W's own word rule (81 points) is not implemented.
- Windowless text services created through msftedit.dll get riched20's
  behaviour (Wine cannot tell them apart here).
- Other scripts, `SCF_WORD` without `SCF_SELECTION`.

## Validation of v4

- L0: the full series applies at `--fuzz=0` with the batch 1 candidates
  substituted, 62/62 (2026-11-09).
- Local run (2026-11-09): riched20 richole, editor, txtsrv and msftedit
  editor, richole: 0 failures.
- L1 (`msftedit:editor`, `riched20:editor`), L2 (`test-run/batch1-3`),
  L4 (Inventor: GD&T symbols survive repeated edits), cross review
  (Codex): pending.
