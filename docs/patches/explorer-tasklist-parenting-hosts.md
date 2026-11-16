# explorer-tasklist-parenting-hosts.patch

<!-- winum-test: l3-only: Wine's own explorer.exe desktop/task list, no Windows counterpart; reproducer in docs/wine-repro -->

*Status page. Levels as in winum's AGENTS.md. "Not recorded" means no result was written down, not that the check failed.*

- **DLL:** `explorer`
- **Origin:** Solidworks-for-Linux (private for now), `patches/`
- **In winum:** `patches/series` from winum-11.20-1

## Problem as the user saw it

SOLIDWORKS helper windows appeared as separate taskbar entries.

## Cause and fix

Opt-in hidden parenting hosts in Explorer's task list.

## Wine test

None in the patch.

## Validation

| Level | Result |
| --- | --- |
| L0 | full builds full-1/full-2 |
| L1 | no Wine test (programs/explorer) |
| L2 | not recorded |
| L3 | opt-in off/on parenting and lifecycle checks (x64) |
| L4 | combined smoke |
| L5 | in use since 2026-10-28 (maintainer to confirm) |
| Cross review | not recorded (written before the cross-review rule) |

### winum-11.22-1 (Wine 11.22)

| Level | Result | Date |
| --- | --- | --- |
| L0 | the whole series on the official Wine 11.22 archive, both architectures, `--fuzz=0` ([run 43703211594](https://github.com/Harvi654/winum/actions/runs/43703211594)) | 2026-11-15 |
| L1 | no Windows L1 (`l3-only`, reason above); the patch is the same as in winum-11.20-2 | - |
| L2 | [run 43703213332](https://github.com/Harvi654/winum/actions/runs/43703213332), group g3 re-run [43739605851](https://github.com/Harvi654/winum/actions/runs/43739605851), paired against vanilla Wine 11.22: only the known flaky failures (ntdll threadpool, ntdll time, urlmon `url.c:3535` not repeated in the re-run); the workflow shows "failure" because of these | 2026-11-15 |
| L4 | Inventor 2027 on a copy of the maintainer's prefix: import, parts, assemblies (maintainer) - a smoke test of the build, not a retest of this patch's own feature. SOLIDWORKS and AutoCAD: deferred (not being worked on since 2026-11-09, AGENTS.md) | 2026-11-15 |

## Open points

Not a complete taskbar integration.

## Evidence

Solidworks-for-Linux `docs/wine-repro/tasklist-hosts/README.md`; overview in `docs/patch-status.md`.

