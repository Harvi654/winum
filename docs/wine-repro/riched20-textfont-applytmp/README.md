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

## Windows observations (2026-11-09)

Offline Windows 10 VM, `windows-2026-11-09/` (both architectures complete,
`api_errors=0`, identical apart from `bits=`). The two classes differ:

msftedit (`RICHEDIT50W`):
- `Reset(tomApplyTmp)` returns S_OK (also on a detached duplicate).
- The changes after it return S_OK (`SetUnderline(tomWave)` included) but
  are invisible: the font's getters and `EM_GETCHARFORMAT` keep the old
  values (red, bold, no underline).
- `Reset(tomApplyNow)` afterwards writes nothing - also after
  `Reset(tomApplyLater)` (`applytmp_later`) and with nothing set.
- `applytmp_stale`: the font object tracks the text (purple after the
  `EM_SETCHARFORMAT`), and the purple stays after `tomApplyNow`: no stale
  write.

riched20 (`RichEdit20W`):
- `Reset(tomApplyTmp)` returns E_INVALIDARG (also detached); the changes
  that follow apply at once, as without a Reset (blue, not bold, wave
  underline in the text and in the getters); `tomApplyNow` changes nothing
  further and writes no stale values.

Both: `applylater_now` (`tomApplyLater`, changes, `tomApplyNow`, on a
fresh font object) - the changes are neither visible nor written. This
differs from what Wine does (it applies them) and is outside the patch;
Wine's own `test_ITextFont` uses `tomApplyLater` after `tomCacheParms`,
a different state.

## Follow-up after cross review (`measure-followup.c`)

Original, AI-assisted (Claude), LGPL-2.1-or-later. The review's P3: what
one font object does after a temporary cycle (tomApplyTmp, set blue,
tomApplyNow), then: a second tomApplyNow; a change without a new Reset;
another tomApplyTmp cycle; tomCacheParms and tomTrackParms. Per step the
HRESULTs, the font's colour and bold and the text's CHARFORMAT, for
RichEdit20W and RICHEDIT50W. In `docs/wine-repro/batch1-measure/followup2.list`
and the main list; completes under a local Wine 11.20 build. Until its
results are in, the candidate does not claim anything for these
sequences.
