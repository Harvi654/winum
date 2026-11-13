# ole32-createfromfile-package.patch

<!-- winum-test: l1 -->

*Status page. Levels as in winum's AGENTS.md.*

- **DLL:** `ole32`
- **Origin:** Autodesk-Inventor-for-Linux issue #10 (Inventor 2027 Import CAD), 2026-11-11
- **In winum:** `patches/series` since 2026-11-12 (winum-11.20-2); contributed by @krokosik (winum PR #1), candidate `patches/candidates/inventor-reference-model/`

## Problem as the user saw it

Import CAD with Reference Model (and Convert Model with the report embedded) ended with "Could not Insert Object".

## Cause and fix

Inventor embeds its translation report (`.htm`) with `OleCreateFromFileEx`. Wine bound a file moniker to IDataObject, which fails for a file whose class is no embedding server. Windows wraps such a file in an OLE Package (`{0003000C-...}`, `\1CompObj` "OLE Package", `\1Ole10Native` with label, paths and contents). The patch does the same when the file is not a compound file and its CLSID has no `Insertable` key (a ProgID marked Insertable still gives a Package on Windows). Open failures and paths of MAX_PATH or more return STG_E_FILENOTFOUND, as measured; the contents are copied in chunks.

## Wine test

`test_OleCreateFromFile_package`, `_errors`, `_extent` and `_tmp_limit` in `dlls/ole32/tests/ole2.c` (the last two exercise the packager and need its patch).

## Validation

| Level | Result |
| --- | --- |
| L0 | applies after `patches/series` to Wine 11.20 with `--fuzz=0`; full WoW64 build in the Inventor repository's Nix package (2026-11-12) |
| L1 | Windows 10 22H2 x86_64 and i686, 2026-11-12: `ole32_test` storage32 19119, stg_prop 304, ole2 3271; `packager_test oleobj` 93; 0 failures, no skips; maintainer's offline Windows 10 VM (L1 bundle of winum run 43232971854, 2026-11-12), x86_64 and i686: `ole32:ole2` 3139 checks (1 skip: Wine's existing HKCR test without admin rights), `ole32:storage32` 19119, `ole32:stg_prop` 304, `packager:oleobj` 93, 0 failures |
| L2 | paired, x86_64, 2026-11-12: winum-11.20-1 with and without the candidate, every `ole32_test` suite and `packager_test oleobj`; with it 0 failures, without it failures only in the suites with the new checks; winum CI, paired vanilla against the series with the candidate: `test-run/inventor-reference-model-1` (run 43232971854, 516 units, 0 regressions) and `-2` (run 43253972090, ole32 and packager, 30 units, 0 regressions, packager oleobj 16 -> 93 checks) |
| L3 | MEASURE programs in [ole32-createfromfile-html](../wine-repro/ole32-createfromfile-html/README.md) (`createfromfile`, `stgerr`, `stglist`, `propnull`, `pkgprobe`) |
| L4 | Inventor 2027.1 on NixOS, v5 (2026-11-12): Import CAD Reference Model import, place, save, restart, reopen with the part resolved; Convert Model with the report embedded, saved with the Package inside the part. SOLIDWORKS: deferred (not worked on since 2026-11-09); maintainer, Arch, Inventor 2027.2, the Wine build of `topic/inventor-reference-model` (run 43233586580), 2026-11-12: Import CAD of a STEP file into an assembly with Reference and with Convert Model, save, restart, reopen; File -> Open of a STEP without "Could not Insert Object" |
| Cross review | Codex: v5 approved, 2026-11-12 (reviews of v2-v4 the same day) |

## Open points

What Windows does with a path whose ANSI form exceeds MAX_PATH (multibyte code page) is not measured. OLERENDER_FORMAT stores no cache on Windows either; the patch accepts it with a FIXME. Other render options (OLERENDER_DRAW, OLERENDER_ASIS) and an advise sink are not handled for a Package and report a FIXME (v6); what Windows does with them is not measured.

## Notes for a Wine bug report

The application side only: the call Inventor makes (above), what Wine
returns, what Windows 10 returns (MEASURE programs and the Wine test), and
the steps in the candidate's README. Nothing from inside Autodesk or
Microsoft binaries.

## History

- 2026-11-12: promoted into `patches/series` for winum-11.20-2 after the maintainer's L1, L2 and L4.

- v2-v5 2026-11-11/12: see the vN paragraphs in the patch header.
- v6 2026-11-13 (follow-up to the maintainer's review of winum PR #1): FIXMEs for the render options and the advise sink a Package leaves out, `WideCharToMultiByte` results checked in `create_package`. Tests unchanged; passed again on Windows 10 x86_64 and i686 (ole2 3271, storage32 19119, stg_prop 304, oleobj 93, 0 failures) and under Wine; the series still applies with `--fuzz=0`.
