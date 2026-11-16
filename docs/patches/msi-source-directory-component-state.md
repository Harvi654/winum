# msi-source-directory-component-state.patch

<!-- winum-test: l1 -->

*Status page. Levels as in winum's AGENTS.md. "Not recorded" means no result was written down, not that the check failed.*

- **DLL:** `msi`
- **Origin:** Solidworks-for-Linux (private for now), written by Codex; position 17 of the MSI set
- **In winum:** `patches/series` from winum-11.20-1

## Problem as the user saw it

SOLIDWORKS Installation Manager: features were missing or installed wrongly (part of the 17-patch MSI feature-selection set).

## Cause and fix

Registers a Source component with a directory key path. Windows values come from the set's own conformance fixture, measured on Windows 10.

## Wine test

`dlls/msi/tests/selection.c (shared fixture with its own CA DLL)`

## Validation

| Level | Result |
| --- | --- |
| L0 | SOLIDWORKS candidate builds (msi-selection profile, G0); with the full winum series: pending CI; full 62-patch series built in winum CI, both architectures, no fuzz (run 42298819890, 2026-11-06) |
| L1 | Windows 10 x64/i686 Optional 2315/0, mixed 233/0 (2026-10-30) |
| L2 | paired x64 and i686, `custom.c:1488` on both engines (2026-11-05/06, `scripts/run-msi-l2.py`) |
| L3 | selection/boundary fixtures of the conformance suite; separate L3 per patch not recorded |
| L4 | SOLIDWORKS installation (clean-F2 2026-10-28, profile -8 2026-11-05) and Inventor installation on the msi-selection runtime (2026-11-05) |
| L5 | not started |
| Cross review | Claude, 2026-11-05 (written by Codex); P2-1 fixed in `785fc53` |

### winum-11.22-1 (Wine 11.22)

| Level | Result | Date |
| --- | --- | --- |
| L0 | the whole series on the official Wine 11.22 archive, both architectures, `--fuzz=0` ([run 43703211594](https://github.com/Harvi654/winum/actions/runs/43703211594)) | 2026-11-15 |
| L1 | unchanged from winum-11.20-2: the patch and its test are the same, and Windows did not change | - |
| L2 | [run 43703213332](https://github.com/Harvi654/winum/actions/runs/43703213332), group g3 re-run [43739605851](https://github.com/Harvi654/winum/actions/runs/43739605851), paired against vanilla Wine 11.22: only the known flaky failures (ntdll threadpool, ntdll time, urlmon `url.c:3535` not repeated in the re-run); the workflow shows "failure" because of these | 2026-11-15 |
| L4 | Inventor 2027 on a copy of the maintainer's prefix: import, parts, assemblies (maintainer) - a smoke test of the build, not a retest of this patch's own feature. SOLIDWORKS and AutoCAD: deferred (not being worked on since 2026-11-09, AGENTS.md) | 2026-11-15 |

## Open points

Maintenance (Modify) behaves as vanilla Wine, which differs from Windows; measured separately, later task.

## Evidence

Solidworks-for-Linux `docs/wine-repro/msi-candidates-evidence.md` (table and sources), `docs/wine-repro/cross-review-2026-11-05-msi.md`, Windows maintenance results `docs/wine-repro/msi-maintenance/windows-2026-11-05/`.

