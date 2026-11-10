# Cross review: mshtml document open optional result v2 (2026-11-10)

Reviewer: Claude; candidate author: Codex. Candidate as on main `f64f3b9`.

Read in full: the candidate patch and README, the production patch, the
MEASURE README (native table), and the test-series workflow and script
(for the L2 path). Compared the candidate's `htmldoc.c` hunks with the
production patch: byte-identical. Read Wine 11.20's `HTMLDocument_open`
around both hunks and the `START_TEST(dom)` context where the call is
added. The rest of `dom.c` and mshtml was skimmed. The standalone L1
harness (`l1.c`, `test-body.h`, builder) was not re-read; its results are
taken from the README and the returned native summaries.

## Findings

- **P1: none in the implementation.** Both output writes and the AddRef
  are guarded; the early `E_FAIL` path for a detached window is unchanged.
  The change is the minimal one the measurement supports (NULL accepted,
  S_OK in all three measured states).
- **P2: paired L2 cannot run in the current workflow.**
  `scripts/test-series.sh` exports `WINEDLLOVERRIDES="mscoree,mshtml="`
  and no test group lists `mshtml`, so neither variant runs
  `mshtml:dom`; the README's "Gecko-enabled paired mshtml.dom L2" is not
  something the existing L2 can provide, and the batch runs so far said
  nothing about this patch. Either a separate Gecko-enabled job (a
  pinned, SHA-256-checked wine-gecko download, AGENTS.md) or a written
  reason why L2 is local only is needed before the page can claim L2.
  Not a defect of the candidate.
- **P3: an unused interface.** `identity` (IUnknown) is queried and
  released but never used; harmless, could go.
- **P3: scope is the loaded state.** The test checks the loaded document
  only, while the MEASURE also covered created and initialised documents
  (both S_OK). The README states this; the implementation guard applies
  to all states, so the narrower test is acceptable.

## Test

Expected values are the measured ones: S_OK from open and close with a
supplied pointer and with NULL, a non-NULL dispatch for a supplied
pointer (its interface is deliberately not asserted - Windows answered
E_NOINTERFACE for IHTMLWindow2), and a parent window before open and
after close. No `todo_wine`. Every early exit releases what was acquired;
a stream that failed to be created frees its HGLOBAL, a created stream
owns it (`fDeleteOnRelease`). The five-second ready-state wait pumps
messages and ends with an ordinary failure, not a skip. On vanilla Wine
the NULL call faults, as the header says; that is the regression.

## Checks and verdict

All series entries apply at `--fuzz=0` with this candidate substituted,
also together with every other current candidate (`test-run/all-1`,
64 entries). Native assertion L1: 34 checks, 0 failures, both
architectures (README, 2026-11-09). No Wine runtime run by this reviewer.

Verdict: **approve the bounded implementation and its test.** Promotion
still needs an mshtml L2 that can actually run (P2) or the maintainer's
recorded decision to accept local evidence for it.
