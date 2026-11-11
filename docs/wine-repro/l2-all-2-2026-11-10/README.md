# L2: test-run/all-2 (2026-11-10)

Run 42815100059 on `test-run/all-2` (`04ae708`): every current candidate
(as all-1, with wbemprox timezone v3 and mouse move v3), paired with
vanilla Wine 11.20, groups g1-g5 (now with `kernel32` and, with pinned
Gecko, `mshtml`), both architectures. First run in which the compare step
also checks units that only the series has.

516 units. Regressions: 3. Fixed: 1. Moved: 2 (`user32.win`, as in all-1).

- `ntdll.i386.time`, `time.c:460` x1 - the same timing check as in all-1;
  no patch touches the time code.
- `urlmon.i386.protocol` did not finish with the series (exit 124, the
  unit timeout); x86_64 finished. urlmon v2 changes `sec_mgr.c` only and
  `urlmon:sec_mgr` passed on both architectures. To be re-run (all-3).
- `user32.x86_64.input`, `input.c:6398` x1 ("got pos (49,51)"), a cursor
  position check. i386 passed. Mouse move v3 (the server sends a delayed
  move after show/hide) is the candidate closest to it; to be checked in
  all-3 with v4, and if it comes back, looked at before mouse move is
  promoted.
- Fixed: `ntdll.i386.exception` three locations.

Units only in the series, all finished without failures: msi
`ancestor`, `costing`, `maintenance`, `selection`; `user32:attachment`
(16 todo), `user32:mousemove` (33, 1 todo), `msftedit:editor`,
`mspatcha:signature`, `shell32:selection`, `wbemprox:timezone` (83, the
v3 `skip()` instead of `win_skip`). `kernel32:actctx` 3296 -> 3863 checks
and `mshtml:dom`/`htmldoc`/`events` ran on both sides without a new
failure.

Follow-up (2026-11-11): the `user32.x86_64.input` "got pos (49,51)"
check came back in all-5 and fails in vanilla Wine as well (3 of 6
local runs); see `../l1-all-5-2026-11-11/`.
