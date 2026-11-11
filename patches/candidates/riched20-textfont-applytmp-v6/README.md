# riched20-textfont-applytmp v6: no BSTR leak; second follow-up

> **Promoted 2026-11-11** into `patches/series` for winum-11.20-1. This folder stays as the record of the candidate.

Candidate only: `patches/series`, the production patch and its page marker
are unchanged. Substitute `riched20-textfont-applytmp.patch` from this
folder for the production file at its place in the series (entry 24).
Supersedes `../riched20-textfont-applytmp-v5/`; review:
`docs/wine-repro/cross-review-2026-11-10-riched20-textfont-applytmp-v5.md`.
**Depends on `riched20-scf-word-insertion-point` v5.**

## What changed from v5

- P2: the two cache refreshes the patch adds (at tomApplyTmp and when the
  temporary state ends at tomApplyNow) go through
  `textfont_refresh_range_props()`, which frees the cached font-name BSTR
  and clears the slot before `textfont_cache_range_props()` stores a new
  one. The shared helper is unchanged, so the first caching in
  `create_textfont()` (malloc'd font, uninitialised slot) still never
  frees. The existing tomCacheParms path in vanilla Wine has the same
  overwrite; it is not part of this patch. Source-based, as the review.
- The second follow-up (`windows-2026-11-10-followup2/`): with the font
  and the text read after every step, a second temporary cycle's change
  stays temporary on Windows (v5 and v6 give the same locally, all four
  sequences identical). Added to `test_textfont_after_applytmp` as an
  ordinary case. The unread `tmp_again` case (Windows: the change ends up
  in the text) stays `todo_wine`; why the two differ is not known (third
  round on this point - written down in the topic README, not measured
  further).

## Local run (2026-11-10, Wine 11.20 x86_64)

msftedit:richole 178 tests, riched20:richole 181587 tests: 0 failures.

## Validation of v6

- L0: 62/62 at `--fuzz=0` with the batch 1 candidates (2026-11-10).
- L1 (v5's test body passed on both architectures, 150 tests, run
  42718217301; v6 pending), L2 (`test-run/batch1-6`), L4 (Inventor:
  repeated spell-check cycles), cross review (Codex): pending.

## L1 and L2 of test-run/batch1-6 (2026-11-10)

Run 42772681118 (`docs/wine-repro/batch1-measure/l1-2026-11-10-batch1-6/`).
L1: `msftedit:richole` 182 tests and `riched20:richole` 183279 tests, 0 failures, x86_64 and i386. L2 (paired, both architectures): 6 regressions, all known and
none from batch 1 (production setrows v1, mousemove v1, urlmon v1 and its
tests-first test; ntdll threadpool timing).

## L2: test-run/all-1 (2026-11-10)

[docs/wine-repro/l2-all-1-2026-11-10](../../../docs/wine-repro/l2-all-1-2026-11-10/README.md), run 42788810738, with every current candidate: no new failure in the touched DLLs, both architectures.
