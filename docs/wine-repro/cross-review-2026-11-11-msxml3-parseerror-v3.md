# Cross review: msxml3 parseerror filepos v3 (2026-11-11)

Reviewer: Codex; candidate author: Claude. Candidate `bfede73`, read at
`4e83fe3`. Read in full: v3 patch/README, complete v2-to-v3 diff, the
previous v2 review including its final findings, and the updated topic
README. Rechecked the empty-input observations in both stored Windows
files. The unchanged implementation and surrounding Wine internals retain
the earlier review; they were not re-audited in full. No vendor internals
were inspected and no Wine/Windows program was run here.

- **P1: none found in this bounded test delta.** DLL hunks are unchanged.
- **P2 empty-error-code guard resolved in source.** Each fresh-document
  empty-string case obtains its parse error, checks that acquisition,
  then asserts get_errorCode returns S_OK and writes exactly 0xc00ce558.
  The sentinel -1 detects an unwritten output. The helper is invoked for
  DOMDocument30 and DOMDocument60; both assertions are ordinary, with no
  todo or conditional omission after successful acquisition. The temporary
  error interface is released. Both measured architectures/classes supply
  exactly these HRESULT/value expectations. Generic E_FAIL or code zero
  now fails, unlike the old filepos-only check.
- **P3 stale scope wording resolved.** Header and README acknowledge the
  empty-input error-state change; the topic distinguishes already-correct
  errorCode/reason HRESULTs from unchanged line/linepos behavior. The
  header's initial getter comparison describes measured Windows behavior,
  not an assertion that every Wine getter was fixed.

The malformed-position todos (8 and 12), generic reason, same-object reset,
NULL-string and other unmeasured states remain outside the implementation
claim. No unrelated test input or class was changed by v3.

Fetched all-6 read-only: `e9ba167129b80f663e65c35d44152c0ad4d47791` has
an exactly byte-identical production-position candidate. The public page
for [run 42997560598](https://github.com/Harvi654/winum/actions/runs/42997560598)
showed **In progress** at review time. API retrieval is forbidden in this
cloud environment; no finished paired artifact or exact v3 native L1 was
available on main. The README's local patched 42976/0 versus control
42976/33 comparison is author-reported, not independently executed here.

Verdict: **approve the bounded v3 revision; the previous P2/P3 source
requests are answered.** Exact-final native L1 on both architectures and
completed paired msxml3 L2 from all-6 remain validation gates, not passes
inferred from a run number. SOLIDWORKS L4 stays deferred. This review
changes no candidate, production series or marker.
