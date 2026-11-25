# kernelbase-hkcr-class-reads.patch

<!-- winum-test: l1 -->

- **DLL:** `kernelbase` (`dlls/kernelbase/registry.c`), test in `advapi32`
- **Status:** In the winum-11.22-3 series. [Candidate history](../../patches/candidates/kernelbase-hkcr-class-reads/README.md)
- **Target:** winum-11.22-3

## Symptom

Inventor Nastran 2027, installed into the Inventor 2027.2 prefix, showed "Could not start the user interface components ... 0x80040154" at Generate Mesh and at Run. Not on Windows 10. After the combase fix the native .NET shim opened `HKCR\CLSID\{...}\InprocServer32` of the per-user NEWPFUI class and failed.

## Cause and fix

Wine's HKEY_CLASSES_ROOT is the machine classes only. Windows 10 opens an HKCU-only CLSID through HKCR and relative to an HKCR\CLSID handle, in the matching view; in a collision the user value wins and a machine-only value stays readable; the same at High integrity (all 13 cases measured). The patch tags read handles opened through HKCR in the CLSID namespace, backs them with the machine key when present or the user key otherwise, and merges value reads (user first). Writes, enumeration, deletes and notifications keep their existing paths.

In plain words: reading a class registration through HKCR now also sees what is registered for the current user.

## How it was found

Wine `+reg` around the shim's failing open; native HKCR read probe `docs/wine-repro/kernelbase-hkcr-read/` (offline Windows 10, normal and High).

## Wine test

`dlls/advapi32/tests/registry.c`, `test_hkcr_class_reads` / `test_hkcr_class_collision` (the collision case needs a machine fixture and runs in the L1 runner); without the physical HKCU WOW64 CLSID tree the i686 user-only matrix is skipped.

## Validation

| Level | Result |
| --- | --- |
| L0 | Applies with `--fuzz=0` after the full series on Wine 11.22; built x86_64 and i386 in the combined next-release build [45485575952](https://github.com/Harvi654/winum/actions/runs/45485575952) (2026-11-24) and by "Build Wine" on the release branch |
| L1 | Offline Windows 10: 57 checks per architecture, no failures or skips; High integrity matched the normal reads in all 13 cases |
| L2 | Paired [45455927981](https://github.com/Harvi654/winum/actions/runs/45455927981); [Run 45485567767](https://github.com/Harvi654/winum/actions/runs/45485567767): the released 11.22-2 series against it plus the seven 11.22-3 patches, 452 units in five groups, both architectures; no new failure (details in `docs/wine-repro/next-release-1/README.md`) (`advapi32.x86_64.registry` 7268 -> 7306 checks, no failure) |
| L3 | 57 assertions per architecture with the modelled WOW64 subtree [45455927388](https://github.com/Harvi654/winum/actions/runs/45455927388) |
| L4 | With the combined next-release build (maintainer, 2026-11-24/25): Inventor 2027.2 + Nastran prefix copy - part, assembly, drawing, open/save/save-as and reopen, Stress Analysis, Frame Analysis, Frame Generator, Nastran Generate Mesh / Run / results without the 0x80040154 dialog; AutoCAD 2027.1.1 fresh install from its Create Installer image (33 steps, exit 0), sign-in, open, SAVE, SAVEAS, reopen. SOLIDWORKS deferred under AGENTS.md |
| L5 | Pending |
| Cross review | Written by Codex; read in full and approved by Claude Opus 5.5 (2026-11-24), `docs/wine-repro/cross-review-2026-11-24-kernelbase-hkcr-class-reads.md`; both required items done in v2 |

## Open

- HKCR writes, enumeration, `RegQueryInfoKey` counts and notifications are not merged (writes differ at High; separate work).
- Each value read through a tagged handle opens the user key once more (small cost).

## Notes for a Wine bug report

Reads through HKEY_CLASSES_ROOT do not see per-user CLSID registrations (bug 17019). Public test: the advapi32 registry test above.

## History

- v1 2026-11-24: candidate.
- v2 2026-11-24: test folded into the patch, todos dropped.
- v3 2026-11-25: no change; promoted to the series.
