# mshtml-document-open-optional-result.patch

<!-- winum-test: missing -->

*Status page. Levels as in winum's AGENTS.md. "Not recorded" means no result was written down, not that the check failed.*

- **DLL:** `mshtml`
- **Origin:** Solidworks-for-Linux (private for now), `patches/`
- **In winum:** `patches/series` from winum-11.20-1

## Problem as the user saw it

Simulation: Create Study failed.

## Cause and fix

`IHTMLDocument2::open` accepts a NULL result pointer.

## Wine test

None in the patch.

## Validation

| Level | Result |
| --- | --- |
| L0 | full builds full-1/full-2 |
| L1 | no Wine test; native NULL contract not measured |
| L2 | not recorded |
| L3 | compiled; Gecko-dependent probes not run |
| L4 | Simulation study, mesh, solver, results |
| L5 | in use since 2026-10-28 (maintainer to confirm) |
| Cross review | not recorded (written before the cross-review rule) |

## Open points

None recorded.

## Evidence

Solidworks-for-Linux `docs/wine-repro/mshtml-open/README.md`; overview in `docs/patch-status.md`.

