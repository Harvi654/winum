# Cross review: wininet same-position HTTP seek v2 (2026-11-10)

Reviewer: Claude; candidate author: Codex. Candidate as on main `f64f3b9`.

Read in full: the candidate patch and README, the production patch, and
Wine 11.20's `HTTPREQ_SetFilePointer` and the `content_pos` updates in
the read paths of `http.c`. The `http.c` implementation hunk is
byte-identical to production. The MEASURE README's summary was read; the
standalone L1 harness and builder were not re-read. The rest of
`tests/http.c` was skimmed around `START_TEST`.

## Findings

- **P1: none.** `content_pos` advances by what the caller has received
  (`ret_read` in the read paths), so `res == content_pos` is exactly a
  seek to the current position; only then is the read buffer kept. Every
  other successful seek resets it as before, and the NO_CACHE_WRITE /
  DONT_CACHE and failed-state early returns are unchanged.
- **P2: none in the test.** Twelve cases (three framings x no seek,
  FILE_CURRENT+0, FILE_BEGIN 17, NO_CACHE_WRITE + FILE_CURRENT) with the
  measured values: position 17 and error 0 for the cached Content-Length
  seeks, INVALID_SET_FILE_POINTER / 12016 for chunked, close-delimited and
  NO_CACHE_WRITE, the whole 16384-byte body, EOF and one request per case.
  `todo_wine_if` covers only the result/error of the cached chunked and
  close-delimited seeks, which Wine accepts (a separate gap, stated); the
  body, EOF and request checks stay ordinary, so a broken buffer cannot
  hide. On vanilla the reset makes the following reads restart at the
  cache position, so the body check fails - the regression.
- **P3: the test runs first in `START_TEST(http)`**, before the old-IE
  skip; it needs only loopback and its own Winsock init/cleanup, so this
  is fine, and it keeps it independent of the suite's own server.
- **P3: robustness.** The server thread's stop failure ends the process
  (`TerminateProcess`), which an L2 run records as a crash of the unit -
  acceptable for a bounded fixture, and the 45 s budget makes it unlikely.

## Checks and verdict

All series entries apply at `--fuzz=0` with this candidate substituted,
also with every current candidate together (`test-run/all-1`, 64
entries). Native assertion L1: 200 checks, 0 failures, 12 cases, 12
requests, both architectures (README, 2026-11-09). Paired L2: `wininet`
is in group g5; `test-run/all-1` (run 42788810738) is the first run with
this candidate. No Wine runtime run by this reviewer.

Verdict: **approve.** Promotion after `test-run/all-1`'s paired L2 shows
`wininet:http` without a new failure (expected: the eight Wine-only todos,
counted as todo, not failures).
