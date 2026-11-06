# wininet-http-same-position-seek.patch

*Status page. Levels as in winum's AGENTS.md. "Not recorded" means no result was written down, not that the check failed.*

- **DLL:** `wininet`
- **Origin:** Solidworks-for-Linux (private for now), `patches/`
- **In winum:** `patches/series` from winum-11.20-1

## Problem as the user saw it

SOLIDWORKS media download (about 16 GB) failed while reading the media list.

## Cause and fix

`InternetSetFilePointer` to the current position of an HTTP body succeeds without a new request.

## Wine test

None in the patch.

## Validation

| Level | Result |
| --- | --- |
| L0 | full builds full-1/full-2 |
| L1 | no Wine test; Windows reference missing |
| L2 | not recorded |
| L3 | x64 probe 42/48 -> 48/48 fixtures |
| L4 | media list retrieved, download reached 97% (completion not confirmed); combined smoke |
| L5 | in use since 2026-10-28 (maintainer to confirm) |
| Cross review | not recorded (written before the cross-review rule) |

## Open points

Cache/seek behaviour outside the fixtures unproven.

## Evidence

Solidworks-for-Linux `docs/wine-repro/wininet-body/same-position-seek.md`; overview in `docs/patch-status.md`.

