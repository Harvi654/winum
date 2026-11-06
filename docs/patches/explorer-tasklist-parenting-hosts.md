# explorer-tasklist-parenting-hosts.patch

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

## Open points

Not a complete taskbar integration.

## Evidence

Solidworks-for-Linux `docs/wine-repro/tasklist-hosts/README.md`; overview in `docs/patch-status.md`.

