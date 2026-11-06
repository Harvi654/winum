# win32u-child-topmost-ignore.patch

*Status page. Levels as in winum's AGENTS.md. "Not recorded" means no result was written down, not that the check failed.*

- **DLL:** `win32u`
- **Origin:** Solidworks-for-Linux (private for now), `patches/`
- **In winum:** `patches/series` from winum-11.20-1

## Problem as the user saw it

PropertyManager section headers (Circle, Dimension, Force) were empty.

## Cause and fix

`SetWindowPos` with HWND_TOPMOST/NOTOPMOST on a child window without SWP_NOZORDER changes nothing, as measured on Windows; v2 also checks WS_EX_TOPMOST.

## Wine test

`dlls/user32/tests/win.c`

## Validation

| Level | Result |
| --- | --- |
| L0 | component builds runtime-9; full build pending; full 62-patch series built in winum CI, both architectures, no fuzz (run 42298819890, 2026-11-06) |
| L1 | Windows 10 x64/i686 201 checks, 0 failures (2026-11-02) |
| L2 | paired, 2026-11-02 and v2 2026-11-04 |
| L3 | not separate |
| L4 | headers, Simulation Force, assembly, save/reopen, restart (2026-11-02; v2 2026-11-04) |
| L5 | not started |
| Cross review | Codex (see the evidence) |

## Open points

Cross-review: Codex 2026-11-04 (finding fixed in v2).

## Evidence

Solidworks-for-Linux `docs/wine-repro/propertymanager-panels/child-topmost-fix.md`; overview in `docs/patch-status.md`.

