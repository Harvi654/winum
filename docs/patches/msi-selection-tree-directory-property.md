# msi-selection-tree-directory-property.patch

<!-- winum-test: missing -->

*Status page. Levels as in winum's AGENTS.md. "Not recorded" means no result was written down, not that the check failed.*

- **DLL:** `msi`
- **Origin:** Solidworks-for-Linux (private for now), `patches/`
- **In winum:** `patches/series` from winum-11.20-1

## Problem as the user saw it

SOLIDWORKS installation: the feature tree overwrote INSTALLDIR.

## Cause and fix

SelectionTree and PathEdit bind to the directory property as on Windows.

## Wine test

None in the patch.

## Validation

| Level | Result |
| --- | --- |
| L0 | full builds full-1/full-2 |
| L1 | no Wine test; native direct/indirect assertions missing |
| L2 | not recorded |
| L3 | synthetic direct/indirect bindings (x64) |
| L4 | installation; file set identical to Windows |
| L5 | in use since 2026-10-28 (maintainer to confirm) |
| Cross review | not recorded (written before the cross-review rule) |

## Open points

None recorded.

## Evidence

Solidworks-for-Linux `docs/wine-repro/msi-feature-map/README.md`; overview in `docs/patch-status.md`.

