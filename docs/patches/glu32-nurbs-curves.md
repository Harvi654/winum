# glu32-nurbs-curves.patch

*Status page. Levels as in winum's AGENTS.md. "Not recorded" means no result was written down, not that the check failed.*

- **DLL:** `glu32`
- **Origin:** Solidworks-for-Linux (private for now), `patches/`
- **In winum:** `patches/series` from winum-11.20-1

## Problem as the user saw it

Sketch splines crashed or were not drawn (GLU NURBS curves were stubs).

## Cause and fix

GLU NURBS curve evaluation (vertex 3/4 maps) with feedback geometry.

## Wine test

`dlls/glu32/tests/glu.c`

## Validation

| Level | Result |
| --- | --- |
| L0 | candidate-nurbs-1 and full-2 |
| L1 | Windows 10 x64/i686 476 checks, 0 failures (ef49725) |
| L2 | GLU32 suite 15/15 unpatched and patched (x64, isolated host) |
| L3 | 14,544 coordinate comparisons, ASan/UBSan |
| L4 | splines, extrusion, assemblies, Simulation, restart |
| L5 | in progress since 2026-10-26 (maintainer to confirm) |
| Cross review | not recorded (written before the cross-review rule) |

## Open points

Curves only; surfaces/trimming unsupported.

## Evidence

Solidworks-for-Linux `docs/wine-repro/nurbs-curves.md`; overview in `docs/patch-status.md`.

