# Cross review: comctl32 header NULL layout v2 (2026-11-09)

Reviewer: Codex; candidate author: Claude. Snapshot: `3e7279d`.

Read in full: candidate patch/README, production v1 and its patch page,
MEASURE source, shared comctl-support.h, topic README, x86_64 Windows
measurement, both committed header L1 results and batch L1 README. Parsed
and checked all 40 observations in each native measurement file, including
the i686 counterpart. Read the complete Wine 11.20
`7298a818b79446bae153b8d5aa27235ab3703bf9` `HEADER_Layout`, the HDM_LAYOUT
dispatch, existing `test_hdm_layout`, and test initialization/v6 loading
and insertion context. The rest of header.c and the suite were skimmed,
not audited in full. No proprietary internals were inspected.

## Findings

- **P1: none identified.** The guard precedes every output write and is
  short-circuited safely: NULL layout, NULL prc or NULL pwpos returns
  FALSE. For valid arguments, the existing implementation is unchanged.
  Vanilla dereferences these NULL arguments rather than already providing
  the measured Windows behavior.
- **P2: none identified in the bounded change or expected values.** All
  asserted return, last-error and sentinel values match the observations.
  New assertions are ordinary `ok()` checks; no todo hides the target.
- **P3: the preservation assertion could cover the complete documented
  output.** The test initializes all of WINDOWPOS but checks only x, cy
  and flags. The measurement also observes unchanged hwndInsertAfter,
  y and cx. Assert those measured fields as well (without inventing an
  expectation for hwnd, which the MEASURE did not print). Repeat the NULL
  member cases through SendMessageW if direct A/W coverage is desired;
  presently only the NULL-layout case is sent through both entry points.
  These are test-strengthening suggestions, not evidence of a faulty
  guard: both dispatches reach the same function.

## Scope and evidence

v1 checked only the layout pointer. v2 adds precisely the two member checks
and a test invoked in both v5 and v6. Native measurements cover all four
NULL variants, A/W and normal/hidden header styles on both architectures:
FALSE, unchanged deadbeef last error, unchanged supplied outputs and a
still-responsive control. The test's valid-call control checks dimensions
relative to the returned height rather than hard-coding 20 versus 24.
The pre-existing valid WINDOWPOS flag discrepancy (Wine 0x4, Windows 0x14)
is neither asserted as compatible nor modified. No existing compatible
valid layout behavior is changed.

Replaced production v1 with the candidate at its normal position in the
separate full-series Wine 11.20 tree; all entries apply with `--fuzz=0`
(toolbar-padding v2 also substituted). No repository production patch or
series entry changed. This is an apply/source check, not a new build.

Both native comctl32:header L1 outputs from run 42642179431's bundle report
1208 tests, zero failures/todos/skips, exit 0. The candidate's local Wine
comparison reports a vanilla NULL-layout crash and 1129 tests/zero failures
with v2. That local run is author-provided evidence. At review time the
public Actions page for test-run/batch1-1 (head `56bb9ee`) still showed
9/10 matrix jobs complete and compare waiting; API retrieval is blocked by
the environment network policy. No paired L2 report was independently
retrieved and no fresh Wine runtime run is claimed.

Verdict: **no blocking source/test finding**; the bounded NULL guard is
supported by Windows L1. Record the paired comctl32 L2 outcome before
promotion. SOLIDWORKS L4 remains deferred under AGENTS.md; this review
does not turn the old v1 consumer result into a new v2 smoke test.
