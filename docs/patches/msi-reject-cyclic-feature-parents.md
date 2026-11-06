# msi-reject-cyclic-feature-parents.patch

*Status page. Levels as in winum's AGENTS.md. "Not recorded" means no result was written down, not that the check failed.*

- **DLL:** `msi`
- **Origin:** Solidworks-for-Linux (private for now), written by Codex; position 13 of the MSI set
- **In winum:** `patches/series` from winum-11.20-1

## Problem as the user saw it

SOLIDWORKS Installation Manager: features were missing or installed wrongly (part of the 17-patch MSI feature-selection set).

## Cause and fix

Rejects cyclic Feature_Parent in CostInitialize. Windows values come from the set's own conformance fixture, measured on Windows 10.

## Wine test

`dlls/msi/tests/selection.c (shared fixture with its own CA DLL)`

## Validation

| Level | Result |
| --- | --- |
| L0 | SOLIDWORKS candidate builds (msi-selection profile, G0); with the full winum series: pending CI; full 62-patch series built in winum CI, both architectures, no fuzz (run 42298819890, 2026-11-06) |
| L1 | Windows 10 x64/i686 v14 16749/0 (2026-10-30) |
| L2 | paired x64 and i686, `custom.c:1488` on both engines (2026-11-05/06, `scripts/run-msi-l2.py`) |
| L3 | selection/boundary fixtures of the conformance suite; separate L3 per patch not recorded |
| L4 | SOLIDWORKS installation (clean-F2 2026-10-28, profile -8 2026-11-05) and Inventor installation on the msi-selection runtime (2026-11-05) |
| L5 | not started |
| Cross review | Claude, 2026-11-05 (written by Codex); P2-1 fixed in `785fc53` |

## Open points

Maintenance (Modify) behaves as vanilla Wine, which differs from Windows; measured separately, later task.

## Evidence

Solidworks-for-Linux `docs/wine-repro/msi-candidates-evidence.md` (table and sources), `docs/wine-repro/cross-review-2026-11-05-msi.md`, Windows maintenance results `docs/wine-repro/msi-maintenance/windows-2026-11-05/`.

