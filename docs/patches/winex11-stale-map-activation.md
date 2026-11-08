# winex11-stale-map-activation.patch

<!-- winum-test: l1 -->

*Status page. Levels as in winum's AGENTS.md. "Not recorded" means no result was written down, not that the check failed.*

- **DLL:** `winex11.drv`
- **Origin:** Solidworks-for-Linux (private for now), `patches/`
- **In winum:** `patches/series` from winum-11.20-1

## Problem as the user saw it

Extrude: the depth field D1 accepted only one digit, the popup lost focus.

## Cause and fix

A late `_NET_ACTIVE_WINDOW`/WM_TAKE_FOCUS for a replaced X window (older serial, at most 500 ms) is ignored once per kind; cleanup in destroy_whole_window (v4).

## Wine test

`dlls/user32/tests/win.c (test_visible_popup_focus)`

## Validation

| Level | Result |
| --- | --- |
| L0 | component runtime -11e; full build pending; full 62-patch series built in winum CI, both architectures, no fuzz (run 42298819890, 2026-11-06) |
| L1 | Windows 10 x64/i686 44 checks, 0 failures (2026-11-03) |
| L2 | paired v4, 2026-11-05: popup_focus_desktop 10 -> 0, user32/win32u unchanged |
| L3 | extracted functions: run.sh 24/0, run-review.py 18/0 |
| L4 | D1 under KWin, v4 (2026-11-05) |
| L5 | not started |
| Cross review | Codex (see the evidence) |

## Open points

Cross-review: Codex confirmed v4 (80a6df0). Cross-thread window destruction remains an unverified edge.

## Evidence

Solidworks-for-Linux `docs/wine-repro/extrude-edit/popup-focus-fix.md`; overview in `docs/patch-status.md`.

