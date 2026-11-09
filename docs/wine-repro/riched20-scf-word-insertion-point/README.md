# EM_SETCHARFORMAT SCF_WORD at an insertion point: Windows MEASURE

Original, AI-assisted (Claude), LGPL-2.1-or-later. Patch:
`patches/riched20-scf-word-insertion-point.patch` (`SCF_SELECTION | SCF_WORD`
with an empty selection formats the word only when the insertion point is
inside a word, a word character on both sides; Inventor's GD&T symbols were
turned into the normal font). Read before writing: the patch and Wine 11.20
`dlls/riched20/editor.c` (`handle_EM_SETCHARFORMAT`) and the `SCF_WORD`
uses in `tests/editor.c`.

Measured, `RichEdit20W` and `RICHEDIT50W`:

- `WORD` lines: two own texts with letters, two spaces, `_`, digits, `.`,
  `'`, `-`, accented letters, `,` between digits, a tab and `?!`; at every
  insertion point (and three selections) `EM_SETCHARFORMAT` bold with
  `SCF_SELECTION | SCF_WORD`, `SCF_WORD` alone and `SCF_SELECTION` alone.
  Printed: the return value, the selection afterwards, the bold state of
  every character (`B`/`.`), the insertion point's format read at once
  (`insert_bold`, -1 = mixed), and the text after typing one `X` there
  (`typed`).
- `FACE` / `FACE_TYPED` lines: the Inventor sequence on "ab gaj cd" in Arial
  with "gaj" in Courier New, then `SCF_SELECTION | SCF_WORD` (and
  `SCF_SELECTION` for comparison) with Times New Roman at insertion points 2
  to 7; faces of every character, of the insertion point, and after typing.

Local check (Wine 9.0, not an oracle): at the point right after "gaj"
(point 6) `SCF_SELECTION | SCF_WORD` reformats "gaj" - the reported bug.
