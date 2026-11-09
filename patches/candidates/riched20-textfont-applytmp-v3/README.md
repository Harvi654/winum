# riched20-textfont-applytmp v3: temporary formatting only in msftedit's class

**Superseded by `../riched20-textfont-applytmp-v4/` (2026-11-09)** after
cross review (P1: dependency on the uninitialised flag; P2: the test used
RichEdit20A). Kept as the record.

Candidate only: `patches/series`, the production patch and its page marker
are unchanged. Substitute `riched20-textfont-applytmp.patch` from this
folder for the production file at its place in the series (entry 24).
Supersedes `../riched20-textfont-applytmp-v2/`; review:
`docs/wine-repro/cross-review-2026-11-09-riched20-textfont-applytmp-v2.md`.
**Depends on `riched20-scf-word-insertion-point` v4** (entry 23), which adds
the editor's class flag.

## What changed from v2

Review P1: v2 dropped the changes after tomApplyTmp in both classes, but
RichEdit20W on Windows rejects tomApplyTmp (E_INVALIDARG) and applies the
following setters at once - which vanilla Wine already did. v3:
`TextFont_Reset(tomApplyTmp)` returns E_INVALIDARG unless the editor is a
RichEdit50W window; the temporary state is set only there. Everything
else as v2.

Review P2: `test_textfont_applytmp_riched20` in
`dlls/riched20/tests/richole.c`, RichEdit20W, ordinary assertions for the
measured E_INVALIDARG and the direct colour and bold changes (getters and
text). `todo_wine`: `SetUnderline(tomWave)` (Wine rejects tomWave),
`tomApplyNow` afterwards writing the font's cached values back (Windows
writes nothing; vanilla does the same as v3), and the detached duplicate
(E_INVALIDARG on Windows, E_NOTIMPL in Wine).

Review P3: the claim is limited to the measured sequences (one temporary
cycle). The follow-up `docs/wine-repro/riched20-textfont-applytmp/measure-followup.c`
measures a second tomApplyNow, later changes, a second cycle and
tomCacheParms afterwards, for both classes.

## Local run (2026-11-09, Wine 11.20 x86_64)

| | vanilla | v3 |
| --- | --- | --- |
| msftedit:richole | 13 failures (the msftedit test) | 117 tests, 0 failures |
| riched20:richole | 1 failure (E_INVALIDARG; the setters already direct) | 181556 tests, 0 failures |

## Validation of v3

- L0: the full series applies at `--fuzz=0` with the batch 1 candidates
  substituted, 62/62 (2026-11-09).
- L1 (`msftedit:richole`, `riched20:richole`), L2 (`test-run/batch1-3`),
  L4 (Inventor: symbol insertion, text colours, spell check), cross review
  (Codex): pending.
