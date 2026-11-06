# comctl32-header-null-layout.patch

*Status page. Levels as in winum's AGENTS.md. "Not recorded" means no result was written down, not that the check failed.*

- **DLL:** `comctl32`
- **Origin:** Solidworks-for-Linux (private for now), `patches/`
- **In winum:** `patches/series` from winum-11.20-1

## Problem as the user saw it

SOLIDWORKS crashed at start (`HDM_LAYOUT` with a NULL layout).

## Cause and fix

`HDM_LAYOUT` with a NULL pointer fails instead of dereferencing it.

## Wine test

None in the patch.

## Validation

| Level | Result |
| --- | --- |
| L0 | full builds full-1/full-2 |
| L1 | no Wine test; native NULL contract not measured |
| L2 | not recorded |
| L3 | v5/v6 null/valid layout checks (x64) |
| L4 | SOLIDWORKS starts; combined smoke |
| L5 | in use since 2026-10-28 (maintainer to confirm) |
| Cross review | not recorded (written before the cross-review rule) |

## Open points

None recorded.

## Evidence

Solidworks-for-Linux `docs/wine-repro/header-layout/README.md`; overview in `docs/patch-status.md`.

