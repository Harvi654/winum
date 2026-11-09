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

The returned structured g1 summaries confirm that attachment completes on
both architectures but still fails with the candidate. The comparison's
"fixed" label includes locations whose failure count merely decreased;
:769 and :772 were not eliminated. Only the capture failures at :774
were eliminated. The absolute counts below supersede any interpretation
of the delta as an assertion pass.

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

Candidate-specific L2 **failed its assertion gate**. Claude's approval
remains conditional on a passing series attachment test and no unexplained
neighboring regressions. This full-series-versus-vanilla comparison alone cannot
attribute every failure to the input-components candidate.

## Returned structured g1 results

On run 42695491325, download artifacts tests-series-g1 and tests-vanilla-g1.
Each upload contains the contents of out/: results-series-g1.json or
results-vanilla-g1.json is at the top of its ZIP, alongside logs/ and
(for series) l1-bundle/. The Windows assertion outputs inside l1-bundle
are a different stage of validation. No new Windows run is needed to
retrieve these CI-generated summaries.


Both JSON files are now recorded, with all 50 user32 units present on
each side and the same keys. Their complete schemas and all string fields
were inspected: numeric counters/exit status, synthetic test names,
source locations and own-window/display test messages only. No personal
or machine identifiers were found. Original bytes are retained in
SHA256SUMS. These are filtered CI summaries, not raw Wine debug logs.

| Architecture | Variant | Checks | Ordinary failures | Expected todos | Flaky / skipped | Finished | Exit |
| --- | --- | --- | --- | --- | --- | --- | --- |
| i386 | vanilla | 10195 | 2569 | 12 | 0 / 0 | yes | 255 |
| i386 | series v2 | 10195 | 2377 | 12 | 0 / 0 | yes | 255 |
| x86_64 | vanilla | 10195 | 2561 | 12 | 0 / 0 | yes | 255 |
| x86_64 | series v2 | 10195 | 2370 | 12 | 0 / 0 | yes | 255 |

| Assertion site | i386 vanilla -> series | x86_64 vanilla -> series |
| --- | --- | --- |
| attachment.c:769 active window | 1235 -> 1191 | 1231 -> 1186 |
| attachment.c:772 focus window | 1238 -> 1186 | 1234 -> 1184 |
| attachment.c:774 capture | 96 -> 0 | 96 -> 0 |

All remaining series failures are ordinary active/focus assertions. No
unexpected todo successes are listed for attachment. The 12 expected
todos match the eight direct-destruction active/focus observations and
four self-detach error observations; no new todo was added. The final
ordinary 44-scenario completion assertion did not fail. Thus this is a
completed failing test, not a timeout or an assumed pass from fewer
failure locations. The native Windows L1 pass remains valid.

The first retained message at both active/focus sites is already in
isolated-nonedge-reverse0-anchorA, stage isolated, observer D: D has its
own active/focus window where Windows returned NULL. This stage precedes
explicit attachment/removal, and the same mismatch occurs in vanilla.
It points first to initial activation/foreground conditions or another
Wine state difference, not by itself to the new component split. The JSON
retains only the first message per source line, so it cannot explain all
2377/2370 failures or prove that SetForegroundWindow was refused.

Next diagnostic: repeat the unit in isolation and observe the initial
GetGUIThreadInfo/GetForegroundWindow state and the activation call results
through public APIs/Wine debug channels, comparing the CI Xvfb conditions
with the maintainer's window-managed session. Keep the native oracle and
ordinary assertions intact while identifying the cause. Merely adding
thousands of todos or replacing Windows values with Wine's is not a fix.

For user32:win, i386 increases from 2 to 6 ordinary failures (the four
mouse-message locations above); x86_64 stays at 2. The two baseline
transparent-window failures move from :10689/:10690 to :10846/:10847
with unchanged messages and counts. No other g1 unit adds a failure
location or stops finishing in the paired comparison. The i386 mouse
sequence still needs a focused paired rerun/attribution. No extra CI run
or maintainer-session execution was performed while recording these data.


## Maintainer attribution and scope decision (2026-11-09)

The maintainer identifies win.c:4866-4882 as the known intermittent
production server-mousemove-on-show-hide v1 failure, removed by its v2
candidate (clean in test-run/candidates-4). Toolbar :2134 and sec_mgr :2444
come from old v1 patches; ntdll i386 exception is unrelated to server/window.c.
This classification supersedes the earlier request to chase the mouse
sequence in this task. No new investigation/rerun of those issues is part
of the attachment harness revision.

The blocker is the unreached initial foreground/active state in attachment,
also on vanilla under Xvfb. Proceed in order: robust setup and prerequisite
skip; then separate foreground-independent checks if needed; only then
retain dependent coverage in native L1 alone if necessary. The three-round
limit applies. V3 is setup round 1 and requires fresh native L1 and useful
paired L2; zero completed scenarios is not validation.
