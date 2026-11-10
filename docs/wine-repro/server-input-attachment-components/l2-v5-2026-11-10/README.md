# V5 focused paired result (2026-11-10)

[Run 42772336963](https://github.com/Harvi654/winum/actions/runs/42772336963),
test-run/batch4-4 at ee07735, is green: both matrix jobs and compare pass.
Public notices confirm on each series architecture 12039 checks, zero
ordinary failures, 16 todos, successful exit, all 44 scenarios with zero
scenario skips, 592 component snapshots and 2180 positive comparisons.
The run's gate additionally rejects flaky checks and unexplained skips.

Absolute inactive-queue state is unavailable on Wine: absolute_state=0,
three preceding absolute snapshots and one explicitly explained scope
skip in both variants. Do not call this full absolute-state validation.
Native v5 L1 separately passed both scopes, all 592 absolute snapshots,
zero failures/skips on both architectures; see ../l1-v5-2026-11-10/.

The public summary exposes vanilla i386's 290 ordinary failures; its
vanilla x86_64 scope notice was not available in the retrieved summary.
The green matrix gate requires a vanilla target signal on both sides,
but no unobserved absolute vanilla count is inferred here. The derived
JSON contains only transcribed public synthetic notices, checked for
personal data. Original artifact JSON/logs were not downloaded.

This clears the focused attachment component pair. It does not replace
new neighboring-suite L2 for the final candidate or promotion review.
The original batch4-1 neighboring v1/timing failures remain under the
maintainer's earlier attribution; they were not chased or excused by new
todos here. No production patch, series entry or marker was changed.
