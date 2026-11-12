# packager-save-view-advise.patch

<!-- winum-test: l1 -->

*Status page. Levels as in winum's AGENTS.md.*

- **DLL:** `packager`
- **Origin:** Autodesk-Inventor-for-Linux issue #10 (Inventor 2027 Import CAD), 2026-11-11
- **In winum:** candidate `patches/candidates/inventor-reference-model/` (not yet in `patches/series`)

## Problem as the user saw it

Same as `ole32-createfromfile-package`: Inventor dropped the embedded Package and the import failed.

## Cause and fix

Wine's Package object had no class, user type, save, extent, advise or IViewObject2; Inventor asks for each. The patch implements them as measured: Save writes the kept `\1Ole10Native` (kept in a temporary docfile), the class and `\1CompObj`; the extent is the same for every aspect and follows the icon and label (5/4 of the wider of icon and label + 1, height icon + 11/10 label height + 5.5 px, icon title font; a rule inferred from the measured cases, asserted exactly only in those configurations); Draw keeps the caller's mapping; EnumAdvise is E_FAIL before Advise and E_NOTIMPL after; no notification for ADVF_PRIMEFIRST. Loading checks that the temporary copy's name fits (and a numbered name only on a collision), E_FAIL otherwise, as on Windows.

## Wine test

`test_save` and `test_view_advise` in `dlls/packager/tests/oleobj.c`; extent, drawing and the temporary path limit in ole32's `ole2.c`.

## Validation

| Level | Result |
| --- | --- |
| L0 | applies after `patches/series` to Wine 11.20 with `--fuzz=0`; full WoW64 build in the Inventor repository's Nix package (2026-11-12) |
| L1 | Windows 10 22H2 x86_64 and i686, 2026-11-12: `ole32_test` storage32 19119, stg_prop 304, ole2 3271; `packager_test oleobj` 93; 0 failures, no skips |
| L2 | paired, x86_64, 2026-11-12: winum-11.20-1 with and without the candidate, every `ole32_test` suite and `packager_test oleobj`; with it 0 failures, without it failures only in the suites with the new checks |
| L3 | MEASURE programs in [ole32-createfromfile-html](../wine-repro/ole32-createfromfile-html/README.md) (`createfromfile`, `stgerr`, `stglist`, `propnull`, `pkgprobe`) |
| L4 | Inventor 2027.1 on NixOS, v5 (2026-11-12): Import CAD Reference Model import, place, save, restart, reopen with the part resolved; Convert Model with the report embedded, saved with the Package inside the part. SOLIDWORKS: deferred (not worked on since 2026-11-09) |
| Cross review | Codex: v5 approved, 2026-11-12 (reviews of v2-v4 the same day) |

## Open points

The extent's height rule rests on two icon title font sizes; Draw's exact pixel placement inside the bounds differs from Windows; no IDataObject (Windows answers GetData with metafile pictures).

## Notes for a Wine bug report

The application side only: the call Inventor makes (above), what Wine
returns, what Windows 10 returns (MEASURE programs and the Wine test), and
the steps in the candidate's README. Nothing from inside Autodesk or
Microsoft binaries.

## History

- v2-v5 2026-11-11/12: see the vN paragraphs in the patch header.
