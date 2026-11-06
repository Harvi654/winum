# winex11-owner-managed-async.patch

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

## Open points

May reorder transient/window-manager events across threads.

## Evidence

Solidworks-for-Linux `docs/wine-repro/x11-owner-managed/README.md`; overview in `docs/patch-status.md`.

