# msxml3-parseerror-filepos v3 (candidate)

> **Promoted 2026-11-11** into `patches/series` for winum-11.20-1. This folder stays as the record of the candidate.

Replaces `patches/msxml3-parseerror-filepos.patch` after review and
validation. v2 with one more test check, after Codex's review of the
current v2 (`docs/wine-repro/cross-review-2026-11-09-msxml3-parseerror-v2.md`,
P2/P3). Written by Claude, 2026-11-11.

## Change against v2

- Test: after `loadXML("")` the test now reads `get_errorCode` for both
  measured classes (DOMDocument30, DOMDocument60) and expects S_OK and
  0xc00ce558 (`docs/wine-repro/msxml3-parseerror-filepos/README.md`,
  Windows 10, both architectures). Before, only `filepos` was checked, so
  any other code (for example E_FAIL) would have passed.
- Header: the change is no longer described as "limited to get_filepos";
  it also records the parse error of an empty string (the SOLIDWORKS
  call). Dated v3 paragraph.
- Implementation: unchanged (byte-identical `domdoc.c` and
  `parseerror.c` hunks).

## Local run (Wine 11.20 x86_64, 2026-11-11)

`msxml3:domdoc` with the series' msxml3:

| msxml3 | checks | failures |
| --- | --- | --- |
| series with v3 | 42976 | 0 |
| series without the parseerror implementation, v3 test | 42976 | 33 (among them "Unexpected error code 0" for both classes) |

## Open

- Codex: re-read of the v3 test check and wording.
- L1: `msxml3:domdoc` on Windows 10, both architectures (bundle of
  `test-run/all-6`).
- L2 (`all-6`). SOLIDWORKS L4 deferred.
