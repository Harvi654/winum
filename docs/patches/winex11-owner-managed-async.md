# winex11-owner-managed-async.patch

<!-- winum-test: l3-only: X11 window manager behaviour, no Windows counterpart; reproducer in docs/wine-repro -->

*Status page. Levels as in winum's AGENTS.md. "Not recorded" means no result was written down, not that the check failed.*

- **DLL:** `winex11.drv`
- **Origin:** Solidworks-for-Linux (private for now), `patches/`
- **In winum:** `patches/series` from winum-11.20-1

## Problem as the user saw it

Simulation mesh stalled (owner promotion of a window across threads).

## Cause and fix

Cross-thread owner promotion of managed windows is done asynchronously.

## Wine test

None in the patch.

## Validation

| Level | Result |
| --- | --- |
| L0 | full builds full-1/full-2 |
| L1 | no Wine test (X11 driver) |
| L2 | not recorded |
| L3 | API model synchronous/asynchronous checks |
| L4 | completed mesh, solver, results |
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

May reorder transient/window-manager events across threads.

## Evidence

Solidworks-for-Linux `docs/wine-repro/x11-owner-managed/README.md`; overview in `docs/patch-status.md`.

