# win32u-fixed-negative-mat2.patch

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
| L0 | component builds; full L0 transcript not attached; with the full winum series: pending CI |
| L1 | Windows contract confirmed (patch-status.md, 2026-11-01) |
| L2 | paired x64/i686 confirmed |
| L3 | strict -6 fails / -8 passes |
| L4 | partial: drawing responsiveness and readable dimensions; orange hover/zoom artefacts and remaining smoke open |
| L5 | not started |
| Cross review | not recorded (written before the cross-review rule) |

## Open points

In SOLIDWORKS series by the maintainer's instruction.

## Evidence

Solidworks-for-Linux `docs/wine-repro/drawing-text/README.md`; overview in `docs/patch-status.md`.

