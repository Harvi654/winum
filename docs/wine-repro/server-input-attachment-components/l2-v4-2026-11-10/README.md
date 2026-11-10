# V4 focused pair, round 2 (2026-11-10)

Source: public annotations of [run 42724139459](https://github.com/Harvi654/winum/actions/runs/42724139459),
commit 2d2fc9d, test-run/batch4-3. public-notice-summary.json preserves
transcribed counters only; it is not an original artifact or full failure
list. The synthetic notices were checked for personal names, paths and
secrets before recording. No such data was found.

| Variant | Architecture | Checks | Failures | Todos | Scope skips | Completed scenarios |
| --- | --- | ---: | ---: | ---: | ---: | ---: |
| vanilla | i386 | 12035 | 302 | 12 | 1 | 44 |
| vanilla | x86_64 | 12035 | 302 | 12 | 1 | 44 |
| series | i386 | 12035 | 12 | 12 | 1 | 44 |
| series | x86_64 | 12035 | 12 | 12 | 1 | 44 |

All four units finished, with zero skipped scenarios, 592 component
snapshots and 2180 positive-reference checks. The absolute-state control
was unavailable in each: absolute_state=0, three preceding absolute
snapshots, one explicit scope skip. This is not a pass of full absolute
Windows state. Vanilla exits 255; series exits 12. The series gate fails
on ordinary errors, so L2 is not cleared. The focused run does not replace
neighboring-suite L2 or revised native L1.

The three published representative series failures, on both architectures,
are children-late1-unparent0 / one-child: reference A active/focus differs
from its own expected window, and A's positive active comparison fails.
These notices locate a remaining reparent/child state difference but do
not establish that all twelve failures share that cause. Obtain the
complete attachment-coverage-series.json from tests-series-g1 before
choosing the third and final adjustment. Do not broaden todos or disable
checks based only on these examples. The original Windows tables and
server implementation have not changed.

No further runtime round was started. Revised native L1 remains held for
the combined VM round after textfont-followup2 returns.


## Original series coverage returned

The maintainer returned attachment-coverage-series.json (SHA256SUMS
preserves the exact uploaded bytes), checked for personal paths, machine
and user names, credentials and URLs; none were found. Both architectures
have the same histogram: two reference-active, two reference-focus, four
positive-active and four positive-focus failures. All first messages are
children-late1-unparent0 / one-child. Contrary to the earlier request's
wording, this JSON stores the first message per source line, not all twelve
contextual messages. The later occurrences' scenario names are unavailable.

The two late-child one-child rows have the same measured native state,
CBCD for active and focus. The four-line histogram is consistent with
both such rows choosing the host A instead of creator C. This is a
source-supported hypothesis for the later occurrences, not a recovered
full log. The first case demonstrably occurs immediately after creating
child 0, before child 1 exists and before SetParent or destruction.

The candidate changes detach_thread_input and desktop reparent handling;
its attach_parent_thread/create/attach_thread_input/assign_thread_input
paths are byte-identical to vanilla. Wine attaches creator C's input to
parent A's input, retaining A's nonzero fields. This explains the first
reported active/focus pair without implicating the component split. A
narrow native-window-choice todo may describe that unchanged Wine gap,
while positive non-NULL component sharing remains an ordinary assertion.
No additional server behavior should be changed to obtain a green test.
