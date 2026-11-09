# riched20-scf-word-insertion-point v2: only where Wine differs from Windows

**Superseded by `../riched20-scf-word-insertion-point-v3/` (2026-11-09):**
v2 was derived from the riched20 class only; the boundary measurement
contradicted its format-change rule there, and msftedit follows a
different, simpler rule. Kept as the record of round 2. Its L1 run
(`riched20:editor`, 2026-11-09) passed on Windows 10 for the measured
riched20 values.

Candidate only: `patches/series`, the production patch and its page marker
are unchanged. Substitute `riched20-scf-word-insertion-point.patch` from
this folder for the production file at its place in the series (entry 23).

## Why v1 is replaced

v1 formatted a word at an empty insertion point only with a word character
(`iswalnum` or `_`) on both sides. The Windows measurement
(`docs/wine-repro/riched20-scf-word-insertion-point/`, results
`windows-2026-11-09/`) shows a different rule: at a word end before a space
Windows formats the word in uniform text (`ab|␣` gives `ab␣` bold), which
vanilla Wine already did and v1 broke; inside a word only the word
characters are taken; `_ . -` and non-ASCII letters behave in their own way.
No single rule follows from the 50 points.

## The change

`dlls/riched20/editor.c`, `handle_EM_SETCHARFORMAT`: with `SCF_SELECTION |
SCF_WORD` and an empty selection, no word is formatted (only the insertion
point takes the format) when the point is

1. at the end of a paragraph,
2. where the character format changes (the runs before and after the point
   have different styles; equal formats share one style), or
3. at the start of the text or right after a space or tab, with an ASCII
   letter or digit after it.

Everywhere else Wine's word range is kept, as in vanilla.

## Check against the measurement

Measured with the batch 1 MEASURE program on a local Wine 11.20 (x86_64):

| | points matching Windows (of 174 lines: 50 insertion points, selections, typed text, both flags) |
| --- | --- |
| vanilla | 121 |
| v2 | 140 |

v2 changes 19 results, all 19 to the Windows value, and none of the 121
where vanilla matched. Of the 50 `SCF_SELECTION | SCF_WORD` insertion
points, vanilla differed at 42; v2 fixes 16 of them, among them Inventor's
point after "gaj" in another face. The other 26 differ as in vanilla.

## Test

`test_EM_SETCHARFORMAT_word_point` in `dlls/riched20/tests/editor.c`: the
measured format string (bold per character, or face per character) for
every insertion point of the two texts and of the face sequence, with the
26 points where Wine still differs as `todo_wine`, and the insertion point
bold. Local run: vanilla 16 failures (exactly the 16 points v2 changes),
v2 10844 tests in `riched20:editor`, 0 failures.

## Open points

- The 26 remaining points (inside words, next to `_ . - '`, `?!`, accented
  letters, `12,5`). A Windows-like word rule for them needs more data and
  is not needed by Inventor.
- Rules 1-3 are taken from the measured points; cases they did not cover
  (a paragraph start after a line break, the end of a paragraph inside the
  text, a format change inside a word or before punctuation, a change of
  colour only, a word starting with a digit or a non-ASCII letter) are
  measured by the follow-up program
  `docs/wine-repro/riched20-scf-word-insertion-point/measure-boundary.c`
  (in `docs/wine-repro/batch1-measure/followup.list`). If Windows differs
  there, the rules are narrowed before the series.
- SCF_WORD with a non-empty selection: Windows formats only the selection,
  Wine expands it to words (measured, not changed here).

## Validation of v2

- L0: the full series applies at `--fuzz=0` on Wine 11.20 with all six
  batch 1 v2 candidates substituted, 62/62 (2026-11-09).
- L1 of the new test, the follow-up MEASURE, L2, L4 (Inventor: GD&T symbols
  survive repeated edits), cross review (Codex): pending.
