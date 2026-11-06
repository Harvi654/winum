# server-input-attachment-components.patch

*Status page. Levels as in winum's AGENTS.md. "Not recorded" means no result was written down, not that the check failed.*

- **DLL:** `server`
- **Origin:** Solidworks-for-Linux (private for now), `patches/`
- **In winum:** `patches/series` from winum-11.20-1

## Problem as the user saw it

Typing did not work in SOLIDWORKS Installation Manager (WebView2 window embedded in two host windows).

## Cause and fix

Detaching one embedded window keeps the remaining input attachments connected; the active window stays on the original input object; the removed edge is restored on allocation failure.

## Wine test

None in the patch.

## Validation

| Level | Result |
| --- | --- |
| L0 | SOLIDWORKS full builds (full-1 2026-10-25, full-2 2026-10-26) |
| L1 | no Wine test (server change); Windows reference missing |
| L2 | not recorded |
| L3 | graph shim: 61,458 cases incl. allocation failures, 0 invariant failures, ASan/UBSan |
| L4 | four GUI probes and typing in Installation Manager; combined CAD smoke |
| L5 | in use since 2026-10-28 (maintainer to confirm a week) |
| Cross review | not recorded (written before the cross-review rule) |

## Open points

Queue destruction and detach-all are outside the change.

## Evidence

Solidworks-for-Linux `docs/wine-input-fix.md`; overview in `docs/patch-status.md`.

