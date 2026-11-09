# riched20-textfont-applytmp v2: Windows-derived test, tomApplyLater fix

**Superseded by `../riched20-textfont-applytmp-v3/` (2026-11-09)** after
cross review (P1: RichEdit20W's direct setters must stay; P2: a test for
RichEdit20W). Kept as the record.

Candidate only: `patches/series`, the production patch and its page marker
are unchanged. Substitute `riched20-textfont-applytmp.patch` from this
folder for the production file at its place in the series (entry 24).

## What changed from v1

- `TextFont_Reset(tomApplyLater)` no longer clears the temporary state. In
  v1 the sequence tomApplyTmp, changes, tomApplyLater, tomApplyNow wrote
  the temporary changes into the text; Windows (msftedit) writes nothing.
- A Wine header, and the test `test_textfont_applytmp` in
  `dlls/msftedit/tests/richole.c` (msftedit, where Windows supports
  tomApplyTmp; riched20's RichEdit20W returns E_INVALIDARG on Windows).

## Windows basis

MEASURE `docs/wine-repro/riched20-textfont-applytmp/`, results
`windows-2026-11-09/`. "abc def ghi" green, "def" red and bold, a font
object of "def":

| Sequence | Windows msftedit | vanilla Wine | v1 | v2 |
| --- | --- | --- | --- | --- |
| tomApplyTmp, set blue / not bold, tomApplyNow | S_OK; getters and text stay red, bold; nothing written | changes applied at once | as Windows | as Windows |
| same, text made purple after the font was taken | purple stays | cached red written back | as Windows | as Windows |
| tomApplyTmp, set, tomApplyLater, tomApplyNow | nothing written | applied | **written** | as Windows |
| tomApplyTmp, tomApplyNow, nothing set | nothing changes | cached values written (same here) | as Windows | as Windows |

`todo_wine` in the test, differences outside this patch: Wine's
`SetUnderline` rejects `tomWave` (Windows S_OK; with the patch it would be
dropped anyway), and `Reset(tomApplyTmp)` on a detached duplicate returns
E_NOTIMPL (Windows S_OK).

## Open points

- RichEdit20W: Windows rejects tomApplyTmp (E_INVALIDARG) and applies the
  changes; Wine follows msftedit for both classes, since riched20 does not
  know which class created the editor (only riched32's 1.0 emulation is
  distinguished). Inventor's text editor works with the msftedit behaviour
  (L4 of v1).
- Temporary formatting is not displayed (no spell-check underline).
- tomApplyLater on a fresh font object: Windows neither shows nor writes
  the changes, Wine applies them - not part of this patch.

## Validation of v2

- L0: the full series applies at `--fuzz=0` on Wine 11.20 with this file
  (and the caret-rewrap v2 candidate) substituted, 62/62 (2026-11-09).
- L1 (2026-11-09): **passed** - `msftedit:richole` from the L1 bundle of `test-run/batch1-1`
  (run 42642179431) on Windows 10, x86_64 and i386: 121 tests, 0 failures, exit 0
  (`docs/wine-repro/batch1-measure/l1-2026-11-09/`).
- L2, L4 (Inventor: inserting a symbol, text colours,
  spell check), cross review (Codex): pending.

## Local run (2026-11-09)

Wine 11.20 built locally for x86_64 (Ubuntu, X11 under Xvfb), test units run
with the new tests applied, first on vanilla DLLs, then with all six batch 1
v2 candidates. Quick feedback only; L2 is the CI run.

| | vanilla | v2 |
| --- | --- | --- |
| msftedit:richole | 13 failures, all in `test_textfont_applytmp` (changes applied, stale red written back) | 117 tests, 0 failures |
| riched20:richole | (with the caret v2 test) | 0 failures |

`msftedit` was in no L2 test group; it is added to group g5.

## L2 (2026-11-09)

`test-run/batch1-1`, run 42642179431 (both architectures, paired with
vanilla): 5 regressions in the report, all old and known - toolbar setrows
v1, server mousemove v1, urlmon v1 and its tests-first test - which the
`candidates-4` candidates fix. None comes from the batch 1 candidates.
