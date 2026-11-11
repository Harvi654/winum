# server-input-attachment-components.patch

<!-- winum-test: l1 -->

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
completed with compare exit 1. Returned g1 summaries confirm the candidate
attachment test completes but fails 2377 checks on i386 and 2370 on x86_64,
all active/focus assertions, with the original 12 todos. Initial-state
mismatches also occur in vanilla and are the current blocker. The
maintainer attributes the neighboring mouse failures to the existing
mousemove v1 patch; those are outside this task. A separate
[v3 setup candidate](../../patches/candidates/server-input-attachment-components-v3/README.md)
keeps the implementation unchanged and requires new native L1 and paired
L2. L2 is not cleared. The production v1 still
contains no test, so its marker remains missing until candidate promotion.

## Validation

| Level | Result |
| --- | --- |
| L0 | SOLIDWORKS full builds (full-1 2026-10-25, full-2 2026-10-26); winum-11.20-1 series (promoted 2026-11-11): every entry applies with `--fuzz=0` on the official 11.20 archive; the tree is identical to that of `test-run/all-6` (run 42997560598) |
| L1 | 16719/0, both arch (promoted file: `server-input-attachment-components-v5`) |
| L2 | v2 run 42695491325: attachment finishes but fails 2377 (i386) / 2370 (x86_64) active/focus checks; four new i386 win mouse-message failures; not cleared; winum-11.20-1: ok (`user32:attachment` 12039, 16 todo; neighbouring suites) |
| L3 | graph shim: 61,458 cases incl. allocation failures, 0 invariant failures, ASan/UBSan |
| L4 | four GUI probes and typing in Installation Manager; combined CAD smoke; winum-11.20-1: Inventor smoke (all-1); SOLIDWORKS deferred |
| L5 | in use since 2026-10-28 (maintainer to confirm a week) |
| Cross review | Claude, approve |

## Open points

Queue destruction and detach-all are outside the change.

## Evidence

Solidworks-for-Linux `docs/wine-input-fix.md`; overview in `docs/patch-status.md`.



## Current test revision (2026-11-10)

V3 prepared all 44 scenarios but still failed 301/305 active/focus checks,
starting in a no-edge local-focus control. The separate
[v4 candidate](../../patches/candidates/server-input-attachment-components-v4/README.md)
keeps the implementation unchanged and separates actor-relative component
checks from exact inactive-queue state. Both scopes must pass new native
L1; Wine must report which scope ran. New native/paired validation and
review of the revised test are pending; the production marker stays missing.


V4's focused pair completes every component scenario but still reports
12 ordinary errors per architecture (vanilla 302). The separate
[v5 candidate](../../patches/candidates/server-input-attachment-components-v5/README.md)
is the final test adjustment: distinguish the unchanged unfocused-child
active/focus window-choice gap from ordinary positive component sharing.
Expected native rows and implementation are unchanged. Revised native L1,
paired L2 and cross review remain pending; the production marker is not
promoted. The round-2 original JSON has first messages per source line,
so later contexts' attribution needs verification in the final run.

## History

- **2026-11-11, promoted** into `patches/series` for winum-11.20-1 from `server-input-attachment-components-v5` (maintainer approval of `docs/release/winum-11.20-1.md` at `ef64451`). Open: absolute inactive-state view not available in CI.
