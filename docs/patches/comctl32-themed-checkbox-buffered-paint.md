# comctl32-themed-checkbox-buffered-paint.patch

<!-- winum-test: l1 -->

*Status page. Levels as in winum's AGENTS.md. "Not recorded" means no result was written down, not that the check failed.*

- **DLL:** `comctl32`
- **Origin:** Solidworks-for-Linux (private for now), `patches/`
- **In winum:** `patches/series` from winum-11.20-1

## Problem as the user saw it

With the theme on, checkboxes in PropertyManager section headers (Dual Dimension, For construction, Direction 2) were missing.

## Cause and fix

A themed BS_AUTOCHECKBOX buffers only its background (parent background and WM_CTLCOLORSTATIC); box and text are drawn on the target DC.

## Wine test

`dlls/comctl32/tests/button.c`

## Validation

| Level | Result |
| --- | --- |
| L0 | component builds runtime-10/-10b; full build pending; full 62-patch series built in winum CI, both architectures, no fuzz (run 42298819890, 2026-11-06) |
| L1 | Windows 10 x64/i686 15 checks, 0 failures (2026-11-03, third run; pixel comparison removed) |
| L2 | paired -9 vs -10b, 2026-11-03: button 0 new failures |
| L3 | not separate |
| L4 | visual 2026-11-03 (theme on); full smoke on the shared build open |
| L5 | not started |
| Cross review | Codex (see the evidence) |

### winum-11.22-1 (Wine 11.22)

| Level | Result | Date |
| --- | --- | --- |
| L0 | the whole series on the official Wine 11.22 archive, both architectures, `--fuzz=0` ([run 43703211594](https://github.com/Harvi654/winum/actions/runs/43703211594)) | 2026-11-15 |
| L1 | unchanged from winum-11.20-2: the patch and its test are the same, and Windows did not change | - |
| L2 | [run 43703213332](https://github.com/Harvi654/winum/actions/runs/43703213332), group g3 re-run [43739605851](https://github.com/Harvi654/winum/actions/runs/43739605851), paired against vanilla Wine 11.22: only the known flaky failures (ntdll threadpool, ntdll time, urlmon `url.c:3535` not repeated in the re-run); the workflow shows "failure" because of these | 2026-11-15 |
| L4 | Inventor 2027 on a copy of the maintainer's prefix: import, parts, assemblies (maintainer) - a smoke test of the build, not a retest of this patch's own feature. SOLIDWORKS and AutoCAD: deferred (not being worked on since 2026-11-09, AGENTS.md) | 2026-11-15 |

## Open points

Cross-review: Codex 2026-11-04, no blocking defect. Needs the CAD smoke on the shared build.

## Evidence

Solidworks-for-Linux `docs/wine-repro/propertymanager-panels/themed-checkbox-fix.md`; overview in `docs/patch-status.md`.

