# ITextFont::Reset(tomApplyTmp): Windows MEASURE

Original, AI-assisted (Claude), LGPL-2.1-or-later. Patch:
`patches/riched20-textfont-applytmp.patch` (`Reset(tomApplyTmp)` caches the
font and drops the changes made after it; the next `Reset(tomApplyNow)` does
not write stale cached properties into the text; Inventor's text turned
black after a symbol was inserted, the spell checker uses temporary
formatting for its wavy underline). Read before writing: the patch and Wine
11.20 `dlls/riched20/richole.c` (`TextFont_Reset`, `set_textfont_prop`,
`textfont_cache_range_props`, `textfont_apply_range_props`) and the Reset
tests in `tests/richole.c`.

Measured, `RichEdit20W` and `RICHEDIT50W`, text "abc def ghi" in green with
"def" red and bold, a font object of the range "def" (4-7). Sequences:

- `applytmp_now`: `Reset(tomApplyTmp)` (value 4), `SetUnderline(tomWave)`,
  `SetForeColor(blue)`, `SetBold(tomFalse)`, `Reset(tomApplyNow)`;
- `applytmp_stale`: the same, but the text's colour is changed with
  `EM_SETCHARFORMAT` after the font object was taken (stale cache);
- `applytmp_only`: no Reset after the changes;
- `applytmp_empty`: `Reset(tomApplyTmp)` then `Reset(tomApplyNow)`, nothing
  set;
- `applytmp_later`: `tomApplyTmp`, changes, `tomApplyLater`, `tomApplyNow`;
- `applylater_now` and `direct`: controls with the existing semantics;
- `detached`: `Reset(tomApplyTmp)` on a duplicate font (no range).

Printed: every HRESULT, what the font object returns (`FONT`: underline,
fore colour, bold) after each step and from a fresh font object at the end,
and what `EM_GETCHARFORMAT` shows per character (`TEXT`: underline type,
colour, `CFE_AUTOCOLOR`, bold).

Local check (Wine 9.0, not an oracle): `Reset(tomApplyTmp)` is ignored, the
changes go straight into the text, and `Reset(tomApplyNow)` writes the
cached colour back over a newer one (`applytmp_stale`).
