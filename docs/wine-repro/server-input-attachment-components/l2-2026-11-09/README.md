# Paired L2 report received (2026-11-09)

Maintainer-returned compare report from run 42695491325,
test-run/batch4-1 at 36b639e. Original report bytes retained with SHA256SUMS.
Reviewed in full before committing: only synthetic test unit/source names,
counts and failure messages, including numeric own-window handles; no user
or machine names, paths, e-mails, credentials or vendor text were found.
This is a filtered comparison report, not raw application/Wine debug logs.

All five groups are present, 420 units. The comparison reports six units
with new failure locations, five with removed locations, two with moved
locations and 39 other changes. The overall workflow failed its compare.

Both attachment architectures remove failures at attachment.c:769, :772
and :774 and have no new failure locations reported. The report is a delta,
not an absolute zero-failure assertion: the structured g1 results are still
needed to confirm completion, ordinary failures and expected todos.

The neighboring user32.i386.win adds four mouse-message failures at
win.c:4866, :4870, :4879 and :4882. In the series source these are the
button-message sequence after ShowWindow(popup,SW_HIDE), followed by a
popup reopened from WM_COMMAND. Their cause is not established; do not
classify them as timing-only or as an input-components regression without
checking the paired data and rerunning the relevant unit. The reported
+157-line movement of other win failures is separate. The remaining new
locations are in comctl32 toolbar (both architectures), ntdll exception
(i386) and urlmon sec_mgr (both architectures). No new gdi32/win32u failure
locations are listed in this report.

Candidate-specific L2 is not cleared. Claude's approval remains conditional
on a passing series attachment test and no unexplained neighboring
regressions. This full-series-versus-vanilla comparison alone cannot
attribute every failure to the input-components candidate.

## Where the outstanding JSON files are

On run 42695491325, download artifacts tests-series-g1 and tests-vanilla-g1.
Each upload contains the contents of out/: results-series-g1.json or
results-vanilla-g1.json is at the top of its ZIP, alongside logs/ and
(for series) l1-bundle/. The Windows assertion outputs inside l1-bundle
are a different stage of validation. No new Windows run is needed to
retrieve these CI-generated summaries.
