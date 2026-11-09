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

## Windows observations (2026-11-09)

Offline Windows 10 VM, `windows-2026-11-09/` (both architectures complete,
`api_errors=0`, identical apart from `bits=`). riched20 and msftedit agree
on the points below.

Text 0 `ab cd  e_f1 g.h i'j k-l`, all in one format, bold with
`SCF_SELECTION | SCF_WORD` at an empty insertion point (`|` = the point):

| Point | Windows formats | Patch v1 formats a word (word character on both sides) |
| --- | --- | --- |
| `|ab` (0), `|cd`, `|e_f1`, `|g`, `|i`, `|k`, end | nothing | no |
| `a|b`, `c|d` (inside) | `ab`, `cd` (no space) | yes (Wine's word range) |
| `ab|␣` (end of word, before a space) | `ab ` | **no** |
| `cd|␣␣`, `cd␣|␣` | `cd  ` | **no** |
| `e|_f1`, `e_|f1` | nothing | **yes** |
| `f|1` | `f1` | yes |
| `f1|␣` | `f1 ` | **no** |
| `g|.h`, `g.|h` | nothing | no |
| `h|␣` | `h ` | **no** |
| `i|'j` | `i'` | **no** |
| `i'|j` | nothing | no |
| `i'j|␣` | `i'j ` | **no** |
| `k|-l`, `k-|l` | nothing | no |

Bold rows: the patch's condition gives the other answer than Windows. The
range Wine formats when the condition holds (Wine's word movement, with or
without the trailing space) is still to be checked on Wine 11.20.

The insertion point itself becomes bold in
every case (`insert_bold=1`, the typed `X` is bold). `SCF_WORD` without
`SCF_SELECTION` changes nothing and does not make the point bold.
Selections format exactly the selection with either flag set.

Inventor's sequence (`FACE`, "ab gaj cd" in Arial, "gaj" in Courier New,
Times New Roman at the point):

| Point | Windows `SCF_SELECTION|SCF_WORD` |
| --- | --- |
| `ab|␣gaj` (2, same format on both sides) | `ab ` |
| `|gaj` (3) | nothing |
| `g|aj`, `ga|j` (4, 5) | `gaj` (no space) |
| `gaj|␣cd` (6, the format changes at the point) | nothing - Inventor's case |
| `|cd` (7) | nothing |

So Windows' rule is not the patch's "a word character on both sides":
the end of a word before a space is formatted in uniform text (point 2)
but not where the format changes (point 6), inside a word only the word
characters are taken, `_ . -` separate words while `'` joins `i'j` in one
direction only. The patch gets Inventor's case right and the word starts,
and differs elsewhere; at `ab|␣` vanilla Wine (9.0 local run) formatted
`ab ` like Windows, the patch formats nothing. No single rule is derived from these 46 points yet.

## Follow-up: boundary cases (`measure-boundary.c`)

Original, AI-assisted (Claude), LGPL-2.1-or-later. Written for the v2
candidate's three rules, to measure the cases the first run did not cover:
two paragraphs (end of the first, start of the second, end of the text),
an italic range ending inside a word, at a word end, covering the space and
the next word, before a dot, a colour-only change at a word end and before a
dot, word starts with an accented letter, a digit, a dot or `_` after a
space, two spaces, a tab, the text starting with a space, a dot or an
accented letter, the text ending in a space, single-letter words. Per case
a `SETUP` line and a `POINT` line per insertion point: return value, bold
per character, the marked range, the insertion point's bold state.
riched20 and msftedit. In `docs/wine-repro/batch1-measure/followup.list`
and the main list. Built with `-Wall -Wextra -Werror` for both
architectures; runs to completion under a local Wine 11.20 build.
