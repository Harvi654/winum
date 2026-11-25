# ntdll-token-integrity-level.patch

<!-- winum-test: l1 -->

- **DLL:** `ntdll` (`dlls/ntdll/unix/security.c`), test in `advapi32`
- **Status:** In the winum-11.22-3 series. [Candidate history](../../patches/candidates/server-limited-token-medium-integrity/README.md)
- **Target:** winum-11.22-3

## Symptom

As for [`server-token-integrity-level`](server-token-integrity-level.md): Inventor Nastran 2027's per-user .NET classes failed with 0x80040154.

## Cause and fix

`NtQueryInformationToken(TokenIntegrityLevel)` returned a constant S-1-16-12288 (High). It now asks wineserver for the token's level (previous patch) and builds the label SID from it, with the attributes Windows returns (`SE_GROUP_INTEGRITY | SE_GROUP_INTEGRITY_ENABLED`, 0x60). Size and layout of the result (28 bytes on x86_64, 20 on i686, as on Windows) do not change; the WoW64 thunk already converts it.

In plain words: ntdll now passes on the level the server keeps instead of always saying "High".

## How it was found

See [`server-token-integrity-level`](server-token-integrity-level.md).

## Wine test

`dlls/advapi32/tests/security.c`, `test_token_integrity_level`: process token, its pseudo handles, primary, impersonation, identification and restricted duplicates, an `ImpersonateSelf` thread token and both linked tokens, each against its own elevation type (Limited -> Medium, Full -> High, Default -> by `TokenElevation`). Fails 10 checks per architecture without the two patches.

## Validation

| Level | Result |
| --- | --- |
| L0 | Applies with `--fuzz=0` after the full series on Wine 11.22; built x86_64 and i386 in the combined next-release build [45485575952](https://github.com/Harvi654/winum/actions/runs/45485575952) (2026-11-24) and by "Build Wine" on the release branch |
| L1 | Passed on offline Windows 10, x86_64 and i686, normal and elevated console: 66 checks, 0 failures, todos or skips (maintainer, 2026-11-24) |
| L2 | As for the server patch; [Run 45485567767](https://github.com/Harvi654/winum/actions/runs/45485567767): the released 11.22-2 series against it plus the seven 11.22-3 patches, 452 units in five groups, both architectures; no new failure (details in `docs/wine-repro/next-release-1/README.md`) |
| L3 | As for the server patch |
| L4 | With the combined next-release build (maintainer, 2026-11-24/25): Inventor 2027.2 + Nastran prefix copy - part, assembly, drawing, open/save/save-as and reopen, Stress Analysis, Frame Analysis, Frame Generator, Nastran Generate Mesh / Run / results without the 0x80040154 dialog; AutoCAD 2027.1.1 fresh install from its Create Installer image (33 steps, exit 0), sign-in, open, SAVE, SAVEAS, reopen. SOLIDWORKS deferred under AGENTS.md |
| L5 | Pending |
| Cross review | Written by Claude; reviewed and approved by Codex (2026-11-24) |

## Open

As for the server patch.

## Notes for a Wine bug report

As for the server patch.

## History

- v1 2026-11-24: candidate.
- v2 2026-11-25: no code change; the advapi32 test moved from tests-first into the patch; promoted to the series.
