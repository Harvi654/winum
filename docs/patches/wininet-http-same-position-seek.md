# wininet-http-same-position-seek.patch

<!-- winum-test: l1 -->

*Status page. Levels as in winum's AGENTS.md. "Not recorded" means no result was written down, not that the check failed.*

- **DLL:** `wininet`
- **Origin:** Solidworks-for-Linux (private for now), `patches/`
- **In winum:** `patches/series` from winum-11.20-1

## Problem as the user saw it

SOLIDWORKS media download (about 16 GB) failed while reading the media list.

## Cause and fix

`InternetSetFilePointer` to the current position of an HTTP body succeeds without a new request.

## Wine test

None in the production patch. The separate
[v2 candidate](../../patches/candidates/wininet-same-position-seek-v2/README.md)
adds a Windows-derived 12-case loopback regression to dlls/wininet/tests/http.c,
keeping the one-line fix unchanged. Its exact-body native assertion bundle
passes on both architectures. Only the independent cached chunked/close
seek-rejection gap has Wine-only todos; body and request checks are ordinary.
The candidate is not yet in patches/series.

## Validation

| Level | Result |
| --- | --- |
| L0 | full builds full-1/full-2; winum-11.20-1 series (promoted 2026-11-11): every entry applies with `--fuzz=0` on the official 11.20 archive; the tree is identical to that of `test-run/all-6` (run 42997560598) |
| L1 | 200/0, both arch (promoted file: `wininet-same-position-seek-v2`) |
| L2 | not recorded; winum-11.20-1: ok (`wininet:http`) |
| L3 | x64 probe 42/48 -> 48/48 fixtures |
| L4 | media list retrieved, download reached 97% (completion not confirmed); combined smoke; winum-11.20-1: SOLIDWORKS, deferred |
| L5 | in use since 2026-10-28 (maintainer to confirm) |
| Cross review | Claude, approve |

## Open points

Cache/seek behaviour outside the fixtures unproven.
Observed vanilla/patched comparison, paired wininet L2, formal cross review
and consumer validation remain open. Native assertion L1 does not promote
the candidate or change the production test marker.

## Evidence

Solidworks-for-Linux `docs/wine-repro/wininet-body/same-position-seek.md`; overview in `docs/patch-status.md`.

## History

- **2026-11-11, promoted** into `patches/series` for winum-11.20-1 from `wininet-same-position-seek-v2` (maintainer approval of `docs/release/winum-11.20-1.md` at `ef64451`). Open: chunked/close-delimited seeks todo_wine.
