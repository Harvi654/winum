# win32u-fixed-negative-mat2.patch

<!-- winum-test: l1 -->

*Status page. Levels as in winum's AGENTS.md. "Not recorded" means no result was written down, not that the check failed.*

- **DLL:** `win32u`
- **Origin:** Solidworks-for-Linux (private for now), `patches/`
- **In winum:** `patches/series` from winum-11.20-1

## Problem as the user saw it

Drawings: rotated or mirrored dimension text was unreadable and slow.

## Cause and fix

`GetGlyphOutline` with a MAT2 containing negative FIXED values is handled like Windows.

## Wine test

`dlls/gdi32/tests/font.c`

## Validation

| Level | Result |
| --- | --- |
| L0 | component builds; full L0 transcript not attached; with the full winum series: pending CI; full 62-patch series built in winum CI, both architectures, no fuzz (run 42298819890, 2026-11-06) |
| L1 | Windows contract confirmed (patch-status.md, 2026-11-01) |
| L2 | paired x64/i686 confirmed |
| L3 | strict -6 fails / -8 passes |
| L4 | partial: drawing responsiveness and readable dimensions; orange hover/zoom artefacts and remaining smoke open |
| L5 | not started |
| Cross review | not recorded (written before the cross-review rule) |

### winum-11.22-1 (Wine 11.22)

| Level | Result | Date |
| --- | --- | --- |
| L0 | the whole series on the official Wine 11.22 archive, both architectures, `--fuzz=0` ([run 43703211594](https://github.com/Harvi654/winum/actions/runs/43703211594)) | 2026-11-15 |
| L1 | unchanged from winum-11.20-2: the patch and its test are the same, and Windows did not change | - |
| L2 | [run 43703213332](https://github.com/Harvi654/winum/actions/runs/43703213332), group g3 re-run [43739605851](https://github.com/Harvi654/winum/actions/runs/43739605851), paired against vanilla Wine 11.22: only the known flaky failures (ntdll threadpool, ntdll time, urlmon `url.c:3535` not repeated in the re-run); the workflow shows "failure" because of these | 2026-11-15 |
| L4 | Inventor 2027 on a copy of the maintainer's prefix: import, parts, assemblies (maintainer) - a smoke test of the build, not a retest of this patch's own feature. SOLIDWORKS and AutoCAD: deferred (not being worked on since 2026-11-09, AGENTS.md) | 2026-11-15 |

## Open points

In SOLIDWORKS series by the maintainer's instruction.

## Evidence

Solidworks-for-Linux `docs/wine-repro/drawing-text/README.md`; overview in `docs/patch-status.md`.

