# comctl32-ownerdraw-existing-capture v2: Windows order instead of a skip

> **Promoted 2026-11-11** into `patches/series` for winum-11.20-1. This folder stays as the record of the candidate.

Candidate only: `patches/series`, the production patch and its page marker
are unchanged. Substitute `comctl32-ownerdraw-existing-capture.patch` from
this folder for the production file at its place in the series (entry 45).

## Why v1 is replaced

v1 skipped `SetCapture` in the v6 button's `WM_LBUTTONDOWN` when an
owner-drawn button already held the capture, so its superclass got no
`WM_CAPTURECHANGED`. The Windows measurement
(`docs/wine-repro/comctl32-ownerdraw-existing-capture/`, results
`windows-2026-11-09/`) shows that Windows does send it - in v5 and v6, for
every button type. What differs is the order: Windows takes the capture
before the focus, so the capture change arrives before the focus is drawn
(`WM_DRAWITEM`, `ODA_FOCUS`); Wine's v6 button set the focus first.
Wine's own user32 button already takes the capture first.

## The change

`dlls/comctl32_v6/button.c`, `WM_LBUTTONDOWN`: an owner-drawn button
calls `SetCapture` before `SetFocus`. Other button types keep Wine's order:
theirs was not measured (no parent drawing to mark the focus), and a patch
for a paused program must not change what vanilla already does like
Windows (AGENTS.md).

## Test

`test_ownerdraw_capture` in `dlls/comctl32/tests/button.c` (v6): an
owner-drawn button subclassed like a superclass, the events in order -
`C` the button gets `WM_CAPTURECHANGED` naming itself, `O` another window
loses the capture to it, `F` the parent's `WM_DRAWITEM` with `ODA_FOCUS`,
`S` `BM_SETSTATE(TRUE)` - for four starting states:

| Case | Windows | vanilla Wine | v1 | v2 |
| --- | --- | --- | --- | --- |
| no capture | `FS` | `FS` | `FS` | `FS` |
| superclass captures in its `WM_LBUTTONDOWN` | `CFS` | `FCS` | `FS` | `CFS` |
| the button held the capture before | `CFS` | `FCS` | `FS` | `CFS` |
| another window held the capture | `OFS` | `FOS` | `FOS` | `OFS` |

(vanilla and v2 columns confirmed by the local run below; v1 from the
source.) Also: after the down the button has capture and focus, after the
up one `BN_CLICKED` and no capture. The test skips when the parent cannot
be made the foreground window.

## Open points

- SOLIDWORKS' L4 is deferred (AGENTS.md, 2026-11-09). When it is done:
  if the PropertyManager buttons do not react with v2, the next step is a
  filtered `+msg` trace of one click and a MEASURE of what the superclass
  does differently - not a return to v1, which contradicts Windows.
  Hypothesis: the superclass arms the press when the focus is drawn and
  disarms it on the capture change, so only the order matters.
- The order for push buttons and check boxes is not measured; they are
  unchanged.
- Inventor uses the same v6 buttons; its smoke test covers owner-drawn
  buttons there.

## Validation of v2

- L0: the full series applies at `--fuzz=0` on Wine 11.20 with this file
  substituted (with the toolbar and header v2 candidates), 62/62
  (2026-11-09); the later themed-checkbox patch still applies.
- L1 (2026-11-09): **passed** - `comctl32:button` from the L1 bundle of `test-run/batch1-1`
  (run 42642179431) on Windows 10, x86_64 and i386: 14224 tests, 0 failures, exit 0
  (`docs/wine-repro/batch1-measure/l1-2026-11-09/`).
- L2, cross review (Codex): pending. L4: SOLIDWORKS deferred (AGENTS.md);
  Inventor's smoke test covers owner-drawn buttons there.

## Local run (2026-11-09)

Wine 11.20 built locally for x86_64 (Ubuntu, X11 under Xvfb), test units run
with the new tests applied, first on vanilla DLLs, then with all six batch 1
v2 candidates. Quick feedback only; L2 is the CI run.

| | vanilla | v2 |
| --- | --- | --- |
| comctl32:button | 3 failures, all in `test_ownerdraw_capture` (`FCS`, `FCS`, `FOS`) | 12023 tests, 0 failures |

Round 1 of the fix: passed. (A first local version changed the order for
all non-split buttons; it was limited to owner-drawn buttons before
commit, the test result is the same.)

## L2 (2026-11-09)

`test-run/batch1-1`, run 42642179431 (both architectures, paired with
vanilla): 5 regressions in the report, all old and known - toolbar setrows
v1, server mousemove v1, urlmon v1 and its tests-first test - which the
`candidates-4` candidates fix. None comes from the batch 1 candidates.

## Cross review (2026-11-09)

Codex: approved (`docs/wine-repro/cross-review-2026-11-09-comctl32-ownerdraw-existing-capture-v2.md`).
Together with L1 and L2 above, ready for the series once the maintainer
decides; `patches/series` is not edited by the author.
Open from the review, not done (would change the reviewed revision): P3 (optional): unregister the window class on the foreground-skip path too.

## L1 of test-run/batch1-4 (2026-11-09)

Run 42698489281, Windows 10, x86_64 and i386: the new test passed
(`docs/wine-repro/batch1-measure/l1-2026-11-09-batch1-4/`).

## L1 and L2 of test-run/batch1-6 (2026-11-10)

Run 42772681118 (`docs/wine-repro/batch1-measure/l1-2026-11-10-batch1-6/`).
L1: unchanged since batch1-4, where its L1 passed; in this run as part of the series. L2 (paired, both architectures): 6 regressions, all known and
none from batch 1 (production setrows v1, mousemove v1, urlmon v1 and its
tests-first test; ntdll threadpool timing).

## L2: test-run/all-1 (2026-11-10)

[docs/wine-repro/l2-all-1-2026-11-10](../../../docs/wine-repro/l2-all-1-2026-11-10/README.md), run 42788810738, with every current candidate: no new failure in the touched DLLs, both architectures.
