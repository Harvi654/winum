# winum-11.20-2

Wine 11.20, `WINUM_BUILD=2`: winum-11.20-1 (62 patches) plus four patches
for Inventor 2027's Import CAD into an assembly ("Could not Insert Object",
Autodesk-Inventor-for-Linux issue #10), contributed by @krokosik as winum
PR #1 and promoted on 2026-11-12. Nothing else changed in the series.

| Patch | DLL | What it fixes |
| --- | --- | --- |
| `ole32-createfromfile-package` | ole32 | `OleCreateFromFileEx` wraps a file without an embedding server in an OLE Package, as Windows does (Inventor embeds its import report this way) |
| `ole32-storage-localdocs` | ole32 | `STG_E_INVALIDNAME` / `STG_E_PATHNOTFOUND` for invalid names and missing directories; a sub-storage opened with `STGM_SIMPLE` works (a Reference Model part stored in the assembly) |
| `ole32-propset-null-strings` | ole32 | a NULL `VT_LPSTR`/`VT_LPWSTR` property is stored with length 0 and read back as NULL instead of crashing the save |
| `packager-save-view-advise` | packager | the Package object gets class, user type, save, extent, advise and `IViewObject2` |

## Validation

| Level | Result |
| --- | --- |
| L0 | the series applies to Wine 11.20 with `--fuzz=0` and builds (Build Wine run 43233586580; the PR's own run 43223977293) |
| L1 | maintainer's offline Windows 10 VM, x86_64 and i686 (L1 bundle of run 43232971854): `ole32:ole2` 3139, `ole32:storage32` 19119, `ole32:stg_prop` 304, `packager:oleobj` 93 checks, 0 failures; also the contributor's Windows 10 22H2 host |
| L2 | `test-run/inventor-reference-model-1` (run 43232971854): 516 units, 0 regressions, the known `user32.win` shift; `-2` (run 43253972090): ole32 and packager, 0 regressions. `packager` now belongs to test group g3 |
| L4 | Arch, Inventor 2027.2 (maintainer): Reference and Convert Model into an assembly, save, restart, reopen; File -> Open of a STEP without the error. NixOS, Inventor 2027.1 (contributor) the same. SOLIDWORKS (ole32 is shared): deferred, not worked on since 2026-11-09 |
| Cross review | Codex, v5 approved (2026-11-12, contributor's side); read in full by Claude for the maintainer (2026-11-12) |

`scripts/check-series.sh`: 66 entries with L1, an L3-only reason or a
known deviation, 0 without.

## Open after the release

- Small follow-ups from the review on PR #1: a FIXME for render options
  other than NONE/FORMAT in the Package path, unchecked
  `WideCharToMultiByte` results, stale links in
  `docs/wine-repro/ole32-createfromfile-html/README.md`, a credit line in
  the patch headers.
- Autodesk-Inventor-for-Linux: pin winum-11.20-2 (`wine/version.env`,
  `wine/SHA256SUMS`, the `winum` flake input) in one commit after the
  release exists.
