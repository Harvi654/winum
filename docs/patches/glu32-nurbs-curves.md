# glu32-nurbs-curves.patch

<!-- winum-test: l1 -->

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

### winum-11.22-1 (Wine 11.22)

| Level | Result | Date |
| --- | --- | --- |
| L0 | the whole series on the official Wine 11.22 archive, both architectures, `--fuzz=0` ([run 43703211594](https://github.com/Harvi654/winum/actions/runs/43703211594)) | 2026-11-15 |
| L1 | unchanged from winum-11.20-2: the patch and its test are the same, and Windows did not change | - |
| L2 | [run 43703213332](https://github.com/Harvi654/winum/actions/runs/43703213332), group g3 re-run [43739605851](https://github.com/Harvi654/winum/actions/runs/43739605851), paired against vanilla Wine 11.22: only the known flaky failures (ntdll threadpool, ntdll time, urlmon `url.c:3535` not repeated in the re-run); the workflow shows "failure" because of these | 2026-11-15 |
| L4 | Inventor 2027 on a copy of the maintainer's prefix: import, parts, assemblies (maintainer) - a smoke test of the build, not a retest of this patch's own feature. SOLIDWORKS and AutoCAD: deferred (not being worked on since 2026-11-09, AGENTS.md) | 2026-11-15 |

## Open points

Curves only; surfaces/trimming unsupported.

## Evidence

Solidworks-for-Linux `docs/wine-repro/nurbs-curves.md`; overview in `docs/patch-status.md`.

