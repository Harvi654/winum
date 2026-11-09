# V3 setup round 1: returned series coverage (2026-11-10)

Run 42713756358, test-run/batch4-2 at 83d702508d1c7d6cf340634a961405cf9cfb9ccd.
Original structured series coverage bytes retained and hashed. The complete
schema, counters and failure strings were checked before committing; only
own-window numeric handles, synthetic stage names, source locations and
CI status are present, with no personal/machine identifiers.

| Architecture | Checks | Completed / skipped scenarios | Ordinary failures | Todos | Exit |
| --- | --- | --- | --- | --- | --- |
| i386 | 10235 | 44 / 0 | 301 | 12 | 255 |
| x86_64 | 10235 | 44 / 0 | 305 | 12 | 255 |

All remaining failures are active/focus observations. The series coverage
gate correctly rejected ordinary failures, not missing or skipped scenarios.
The first retained failure is isolated-nonedge-reverse0-anchorA,
joined-probe-B, observer A: active/focus is NULL instead of A's own window.
There are no explicit attachments in this graph at that stage. Initial
preparation succeeded; the remaining difference starts when another
independent queue receives local focus. The JSON stores only the first
message per source location, so it cannot identify every remaining case.

Round 1 improved the result from 2377/2370 ordinary failures to 301/305
without adding todos or dropping scenarios. The next requested option is
to separate component observations and API results from absolute inactive-
queue state. V3 remains unvalidated on Wine; revised native L1 is still
required. No conclusion about vanilla's v3 run is inferred from this
series-only return.
