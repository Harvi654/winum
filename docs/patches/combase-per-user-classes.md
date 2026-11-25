# combase-per-user-classes.patch

<!-- winum-test: l1 -->

- **DLL:** `combase` (`dlls/combase/combase.c`), test in `combase`
- **Status:** In the winum-11.22-3 series. [Candidate history](../../patches/candidates/combase-per-user-classes/README.md)
- **Target:** winum-11.22-3

## Symptom

Inventor Nastran 2027, installed into the Inventor 2027.2 prefix, showed "Could not start the user interface components ... 0x80040154" at Generate Mesh and at Run. Not on Windows 10. Nastran writes its NEWPFUI class registrations to `HKCU\Software\Classes` at its first start.

## Cause and fix

Wine's COM looked up InprocServer32, ProgID and TreatAs only in the machine classes. Non-elevated Windows 10 uses the per-user registration first, also when both name the same class, and resolves per-user ProgID and TreatAs keys; at High integrity it ignores HKCU-only classes (`REGDB_E_CLASSNOTREG` from `CoGetClassObject` and `CoCreateInstance`, both architectures, measured). The patch searches the current user's classes before the existing machine lookup for those reads, unless the effective token (thread token when impersonating, else the process token) is at High or above or its level cannot be read. Explicit registry views are kept; other HKCR callers are unchanged.

Needs the two token patches: without them every Wine process is High and the per-user lookup would never be used.

In plain words: COM now also finds classes a program registered for the current user only, and, like Windows, not for elevated programs.

## How it was found

Wine `+reg,err+ole` traces of Nastran's dialog; native COM activation matrix on offline Windows 10 (user only, machine only, both, normal and High integrity), `docs/wine-repro/combase-per-user-classes/`.

## Wine test

`dlls/combase/tests/per_user_classes.c` with a small in-process test server (`per_user_server.c`): user-only, machine-only and conflicting registrations, ProgID and TreatAs, `CoGetClassObject` and `CoCreateInstance`; at High only the machine registration. The six `todo_wine` marks on checks the patch passes were dropped when the test moved into the patch.

## Validation

| Level | Result |
| --- | --- |
| L0 | Applies with `--fuzz=0` after the full series on Wine 11.22; built x86_64 and i386 in the combined next-release build [45485575952](https://github.com/Harvi654/winum/actions/runs/45485575952) (2026-11-24) and by "Build Wine" on the release branch |
| L1 | v2 on offline Windows 10: 54 (x86_64) / 59 (i686) normal checks plus 12 High checks per architecture, all exit 0, no failures, todos or skips (2026-11-24) |
| L2 | Paired with the token patches on both sides [45462274569](https://github.com/Harvi654/winum/actions/runs/45462274569); [Run 45485567767](https://github.com/Harvi654/winum/actions/runs/45485567767): the released 11.22-2 series against it plus the seven 11.22-3 patches, 452 units in five groups, both architectures; no new failure (details in `docs/wine-repro/next-release-1/README.md`) - the test's six todo marks were reported as passing and are dropped |
| L3 | Native COM matrix reproduced on Wine with the token runtime: 51 x86_64 / 56 i686 targeted checks [45462274892](https://github.com/Harvi654/winum/actions/runs/45462274892) |
| L4 | With the combined next-release build (maintainer, 2026-11-24/25): Inventor 2027.2 + Nastran prefix copy - part, assembly, drawing, open/save/save-as and reopen, Stress Analysis, Frame Analysis, Frame Generator, Nastran Generate Mesh / Run / results without the 0x80040154 dialog; AutoCAD 2027.1.1 fresh install from its Create Installer image (33 steps, exit 0), sign-in, open, SAVE, SAVEAS, reopen. SOLIDWORKS deferred under AGENTS.md |
| L5 | Pending |
| Cross review | Written by Codex. v1 reviewed by Claude; v2 (integrity condition) read in full and approved by Claude Opus 5.5 on 2026-11-25, see the candidate README |

## Open

- A fresh Wine prefix has no HKCU Classes WOW64 tree, so an explicit 32-bit user registration can land in the 64-bit view (registry layout, not COM); CI models the tree.
- A per-user key that exists but cannot be read gives `REGDB_E_READREGDB` instead of falling back to the machine key (not measured).

## Notes for a Wine bug report

COM in Wine ignores `HKCU\Software\Classes` for in-process servers, ProgIDs and TreatAs (part of bug 17019). Windows uses it first for non-elevated callers. Public test: the combase test above.

## History

- v1 2026-11-23: per-user lookup.
- v2 2026-11-24: skip HKCU at High integrity (measured).
- v3 2026-11-25: no code change; test moved into the patch, satisfied todos dropped; promoted to the series.
