# comctl32-ownerdraw-existing-capture.patch

<!-- winum-test: missing -->

*Status page. Levels as in winum's AGENTS.md. "Not recorded" means no result was written down, not that the check failed.*

- **DLL:** `comctl32`
- **Origin:** Solidworks-for-Linux (private for now), `patches/`
- **In winum:** `patches/series` from winum-11.20-1

## Problem as the user saw it

PropertyManager: the small checkmark and red X did not react to mouse clicks.

## Cause and fix

An owner-drawn v6 button keeps an existing mouse capture instead of taking it away.

## Wine test

None in the patch.

## Validation

| Level | Result |
| --- | --- |
| L0 | full builds full-1/full-2 |
| L1 | no Wine test; native capture ordering not measured |
| L2 | not recorded |
| L3 | superclass/reentrant capture reproducer (x64) |
| L4 | Line, Mate accept/cancel, assembly save/reopen |
| L5 | in use since 2026-10-28 (maintainer to confirm) |
| Cross review | not recorded (written before the cross-review rule) |

## Open points

None recorded.

## Evidence

Solidworks-for-Linux `docs/wine-repro/confirmation-input/README.md`; overview in `docs/patch-status.md`.

