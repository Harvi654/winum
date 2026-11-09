# wininet-http-same-position-seek.patch

<!-- winum-test: missing -->

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
| L0 | full builds full-1/full-2 |
| L1 | 2026-11-09: v2 exact-body test passes on offline Windows 10 x86_64/i686, 200 checks / zero failures / 12 cases / 12 requests / exit 0 each (96d3ca3 sources). Production has no test until candidate promotion. |
| L2 | not recorded |
| L3 | x64 probe 42/48 -> 48/48 fixtures |
| L4 | media list retrieved, download reached 97% (completion not confirmed); combined smoke |
| L5 | in use since 2026-10-28 (maintainer to confirm) |
| Cross review | not recorded (written before the cross-review rule) |

## Open points

Cache/seek behaviour outside the fixtures unproven.
Observed vanilla/patched comparison, paired wininet L2, formal cross review
and consumer validation remain open. Native assertion L1 does not promote
the candidate or change the production test marker.

## Evidence

Solidworks-for-Linux `docs/wine-repro/wininet-body/same-position-seek.md`; overview in `docs/patch-status.md`.

