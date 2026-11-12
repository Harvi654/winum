# ole32-propset-null-strings.patch

<!-- winum-test: l1 -->

*Status page. Levels as in winum's AGENTS.md.*

- **DLL:** `ole32`
- **Origin:** Autodesk-Inventor-for-Linux issue #10 (Inventor 2027 Import CAD), 2026-11-11
- **In winum:** candidate `patches/candidates/inventor-reference-model/` (not yet in `patches/series`)

## Problem as the user saw it

Saving a Reference Model part into an assembly crashed the save (access violation).

## Cause and fix

Inventor writes a VT_LPWSTR property whose pointer is NULL. Windows stores it with length 0 and reads it back as NULL, in Unicode and ANSI property sets alike. Wine asserted or dereferenced NULL. The patch changes the private `PropertyStorage_PropVariantCopy()` and the property-set writer and reader (a zero length reads back as NULL, for scalars and vector elements).

## Wine test

`test_null_strings` in `dlls/ole32/tests/stg_prop.c` (Unicode and ANSI sets, each also replacing existing values).

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

Zero-length vector elements share the reader change but have no own measurement.

## Notes for a Wine bug report

The application side only: the call Inventor makes (above), what Wine
returns, what Windows 10 returns (MEASURE programs and the Wine test), and
the steps in the candidate's README. Nothing from inside Autodesk or
Microsoft binaries.

## History

- v2-v5 2026-11-11/12: see the vN paragraphs in the patch header.
