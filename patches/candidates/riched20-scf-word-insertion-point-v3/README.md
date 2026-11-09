# riched20-scf-word-insertion-point v3: msftedit's word rule

Candidate only: `patches/series`, the production patch and its page marker
are unchanged. Substitute `riched20-scf-word-insertion-point.patch` from
this folder for the production file at its place in the series (entry 23).
Supersedes `../riched20-scf-word-insertion-point-v2/`.

## Why a third version

Both Windows measurements (`docs/wine-repro/riched20-scf-word-insertion-point/`,
`windows-2026-11-09/` and `windows-2026-11-09-boundary/`) cover riched20's
RichEdit20W and msftedit's RICHEDIT50W, and the two classes differ: at a
word end before a space riched20 formats the word and the space, msftedit
nothing. v2 was derived from the riched20 rows only (the first summary
wrongly said both classes agree) and the boundary run contradicted its
format-change rule for riched20. msftedit follows one rule throughout, and
Inventor most likely uses msftedit (its spell checker uses
`tomApplyTmp`, which RichEdit20W rejects on Windows) - to be confirmed.
This is the third fix round; per AGENTS.md the state is written down here.

## The rule (msftedit, measured)

At an empty insertion point, `SCF_SELECTION | SCF_WORD` formats characters
only when the characters before and after the point are of the same
class, and then the run of that class around the point:

| class | characters | run formatted |
| --- | --- | --- |
| word | `iswalnum`, and an apostrophe between two of them | the word (`a|b` gives `ab`, `i|'j` gives `i'j`, `1|2,5` gives `12`) |
| punctuation | anything else that is not a space or a break | the punctuation run (`?|!` gives `?!`) |
| space | space, tab | the word before them and the spaces (`cd␣|␣` gives `cd␣␣`) |
| break | paragraph marks, objects | nothing |

So nothing changes at a word start or end, next to `_ , . -`, at the start
or end of the text - whatever the character formats (Inventor's point after
"gaj" in another face is a word end). With a selection, `SCF_WORD` formats
only the selection (both classes).

## Check against the measurements (local Wine 11.20, x86_64)

| | msftedit, first run (174) | msftedit, boundary (50) | riched20 class, first run (174) |
| --- | --- | --- | --- |
| vanilla matches | 113 | 18 (v2: 18) | 121 |
| v3 matches | **174** | **48** | 157 |
| vanilla right, v3 wrong | 0 | 0 | **8** |

The two boundary points still different: a point right before a paragraph
mark makes that mark bold on Windows (both classes); Wine formats no mark
(`todo_wine`). The 8 riched20-class points are word ends before a space,
where riched20 formats the word and the space and msftedit does not; Wine
cannot tell the classes apart (one window procedure for both), so v3
follows msftedit for both.

## Test

`dlls/msftedit/tests/editor.c` (new unit `msftedit:editor`, in the
Makefile): every measured msftedit point of both runs (insertion points,
the three selections per text, the face sequence, the boundary cases),
with the two paragraph-mark points `todo_wine`; the insertion point itself
bold. It lives in msftedit's tests because the expected values are
msftedit's; the code is riched20's. Local run: vanilla 102 failures, v3
358 tests, 0 failures; riched20's existing `editor` unit (with its older
`SCF_WORD` checks) 0 failures with both.

## Open points

- Which class Inventor's dimension text editor uses (RichEdit20W or
  RICHEDIT50W). A filtered trace answers it, e.g.
  `WINEDEBUG=+win` while opening the text editor, then
  `grep -m5 -i "richedit" <log>`. If it is RichEdit20W, v3 is wrong for
  Inventor and a class-aware variant (txthost.c telling the editor its
  class) is the next step.
- Paragraph marks before the point (above).
- Scripts other than Latin, and `SCF_WORD` without `SCF_SELECTION`, not
  measured beyond the first run.

## Validation of v3

- L0: the full series applies at `--fuzz=0` with this file and the other
  five batch 1 v2 candidates substituted, 62/62 (2026-11-09).
- L1 of `msftedit:editor`, L2 (test-run), L4 (Inventor: GD&T symbols
  survive repeated edits), cross review (Codex): pending.
