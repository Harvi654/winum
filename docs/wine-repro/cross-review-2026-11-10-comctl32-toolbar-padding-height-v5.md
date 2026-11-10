# Cross review: comctl32 toolbar padding height v5 (2026-11-10)

Reviewer: Codex; candidate author: Claude. Candidate introduced in
4036bab; validation record 684ab5d. Follow-up to the v4 review.

Read in full: v4 and v5 candidate patches/READMEs and their diff, the
v4 review, batch1-4/5/6 validation READMEs, and both batch1-6 native
comctl32:toolbar output files. Revisited pinned Wine 11.20
7298a818b79446bae153b8d5aa27235ab3703bf9 rebuild_toolbar_ex and
rebuild_toolbar, the candidate test's complete lifetime and v5/v6 entry
points, and the unchanged MeasureButton/SetBitmapSize/SetPadding bodies.
Reparsed all 24 PAD and 160 BMP follow-up rows per architecture. Earlier
measurement interpretation remains in the v4 review. Skimmed the
repetitive historical i386 failures and unrelated suites. No proprietary
code or binaries were inspected.

## Findings

- **The test-lifetime defect is resolved.** rebuild_toolbar_ex first reads
  and may destroy *hToolbar. Passing the old uninitialized local HWND is
  undefined behavior and may destroy a fixture window. V5 initializes
  that local to NULL before its first rebuild. This is done anew for each
  invocation, including after activating v6. Later matrix iterations
  assign CreateWindowEx's result before use. The other new test function
  also assigns its handle before using it. No additional lifetime issue
  was found in this delta.
- **No implementation or oracle expansion.** Ignoring the dated header,
  v5 differs from v4 by exactly that initializer. All DLL hunks, expected
  values, ordinary assertions, styles, same-bitmap controls and both class
  activation paths are unchanged. There is no new todo or disabled case.
  The 24 PAD rows and 128 changed/32 unchanged BMP rows per architecture
  still support the asserted formulas and sentinel rules. The unchanged
  TBSTYLE_LIST and string-width limits remain disclosed in the v4 record.
- **Correction to earlier evidence interpretation.** The i386 cascade
  was a test bug, not a justified environment explanation. The earlier
  v4 review did not identify the uninitialized read; its requirement for
  successful new native execution was necessary. Batch1-6 now supplies
  that execution rather than treating the old failure as an L1 pass.

No new P1/P2/P3 finding in the v4-to-v5 source/test delta. The initializer
cannot change production toolbar behavior; the prior source approval and
its scope limits therefore still apply.

## Validation and verdict

Both native batch1-6 toolbar files contain 5309 checks, zero failures,
todos, flaky checks or skips; exitcodes.txt records exit 0 for both
architectures. This is exact v5 L1; the old v3/v4 i386 failures are not
reused as success.

The candidate is byte-identical to the production-position copy in
[test-run/batch1-6](https://github.com/Harvi654/winum/actions/runs/42772681118)
at 9262481caf3f189e9c5849a4a458206c146a7336. Independently applied all
62 actual branch entries to the pinned source at --fuzz=0; production
series ordering is unchanged. No new Wine/native runtime or full build
was run for this review.

The public run finishes with compare failure, not an entirely green L2.
The maintainer's recorded paired assessment in
batch1-measure/l1-2026-11-10-batch1-6/README.md attributes the six reported
regressions to existing setrows v1, mousemove v1, urlmon v1/tests-first,
and the ntdll threadpool timer check; the former i386 toolbar cascade is
absent. That per-failure attribution was not independently reconstructed
from downloaded original paired JSON in this review. Do not relabel the
whole run green or count unrelated failures as caused by this initializer.

Verdict: **approve v5's test fix and retain the bounded v4 source approval**.
Native L1 now passes both architectures. The recorded L2 has no new batch-1
regression per the maintainer's assessment; the Inventor palette L4 check
remains open. No production patch, series or test marker was changed.
