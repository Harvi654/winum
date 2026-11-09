# server-input-attachment-components.patch

<!-- winum-test: missing -->

*Status page. Levels as in winum's AGENTS.md. "Not recorded" means no result was written down, not that the check failed.*

- **DLL:** `server`
- **Origin:** Solidworks-for-Linux (private for now), `patches/`
- **In winum:** `patches/series` from winum-11.20-1

## Problem as the user saw it

Typing did not work in SOLIDWORKS Installation Manager (WebView2 window embedded in two host windows).

## Cause and fix

Detaching one embedded window keeps the remaining input attachments connected; the active window stays on the original input object; the removed edge is restored on allocation failure.

## Wine test

None in production v1. The separate
[`server-input-attachment-components-v2`](../../patches/candidates/server-input-attachment-components-v2/README.md)
candidate adds user32:attachment from the maintainer's Windows measurements
and limits implicit desktop reparenting to preserve already-compatible
state. Its exact Wine assertion test passed on Windows 10 x86_64/i386
(2026-11-09): 10195 checks per architecture, zero failures/skips and exit 0;
see [the native L1 record](../wine-repro/server-input-attachment-components/l1-2026-11-09/README.md).
Claude approved v2 subject to paired L2 (d9119b4). Run 42695491325
completed with compare exit 1; per-test attribution and candidate-specific
L2 clearance remain pending. The production v1 still
contains no test, so its marker remains missing until candidate promotion.

## Validation

| Level | Result |
| --- | --- |
| L0 | SOLIDWORKS full builds (full-1 2026-10-25, full-2 2026-10-26) |
| L1 | production v1 has no test; separate v2 candidate passed Windows 10 x86_64/i386 (10195 checks each, 0 failures, 2026-11-09) |
| L2 | v2 run 42695491325: compare exit 1, detailed attribution pending; no v2 clearance yet |
| L3 | graph shim: 61,458 cases incl. allocation failures, 0 invariant failures, ASan/UBSan |
| L4 | four GUI probes and typing in Installation Manager; combined CAD smoke |
| L5 | in use since 2026-10-28 (maintainer to confirm a week) |
| Cross review | not recorded (written before the cross-review rule) |

## Open points

Queue destruction and detach-all are outside the change.

## Evidence

Solidworks-for-Linux `docs/wine-input-fix.md`; overview in `docs/patch-status.md`.

