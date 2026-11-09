# riched20-textfont-applytmp v5: no temporary values after the cycle

Candidate only: `patches/series`, the production patch and its page marker
are unchanged. Substitute `riched20-textfont-applytmp.patch` from this
folder for the production file at its place in the series (entry 24).
Supersedes `../riched20-textfont-applytmp-v4/` (READMEs of v3 and v4
describe the class split and the tests). **Depends on
`riched20-scf-word-insertion-point` v5.**

## What the follow-up measurement showed (review of v3, P3)

`docs/wine-repro/riched20-textfont-applytmp/windows-2026-11-09-followup/`,
msftedit: a second tomApplyNow after a temporary cycle writes nothing; v4
wrote the temporary values still in the font's cache. A change without a
Reset after the cycle applies at once (v4: the same). After a second
tomApplyTmp cycle on the same font object the change ends up in the text
on Windows; v4 and v5 drop it.

## The change (v5)

`TextFont_Reset(tomApplyNow)` with the temporary state: drop it and cache
the text's current properties, so a later tomApplyNow writes the text's
own values (nothing changes). Everything else as v4.

## Tests

`dlls/msftedit/tests/richole.c`: in the first sequence a second
tomApplyNow leaves red and bold; `test_textfont_after_applytmp`: the
measured "change after the cycle" (ordinary) and "second cycle" (the bold
change, `todo_wine`, the measured Windows value). Local run: msftedit
richole 146 tests, 0 failures (vanilla 13 failures); riched20 richole
181587, 0 failures. With v4's code the second-tomApplyNow check fails.

## Open points

- The second temporary cycle: why its change reaches the text on Windows
  is not known; `measure-followup2.c` (`followup3.list`) reads every step
  of it, on the same and on a new font object, and a cycle whose change is
  bold. Until then the case stays `todo_wine`; Inventor's spell checker
  may run several cycles on one font, so this matters for L4.
- RichEdit20W's stale tomApplyNow write (todo_wine, as vanilla).

## Validation of v5

- L0: 62/62 at `--fuzz=0` with the batch 1 candidates (2026-11-09).
- L1, L2 (`test-run/batch1-5`), L4 (Inventor), cross review (Codex):
  pending.
