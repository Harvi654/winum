# msxml3-parseerror-filepos.patch

<!-- winum-test: test-not-on-windows -->

*Status page. Levels as in winum's AGENTS.md. "Not recorded" means no result was written down, not that the check failed.*

- **DLL:** `msxml3`
- **Origin:** Solidworks-for-Linux (private for now), `patches/`
- **In winum:** `patches/series` from winum-11.20-1

## Problem as the user saw it

SOLIDWORKS Installation Manager stopped with a fatal exception after the serial number was entered.

## Cause and fix

`IXMLDOMParseError::filepos` returns the stored position instead of failing.

## Wine test

`dlls/msxml3/tests/domdoc.c (test_parseerror)`

## Validation

| Level | Result |
| --- | --- |
| L0 | full builds full-1/full-2 |
| L1 | Wine test exists, never run on Windows |
| L2 | not recorded |
| L3 | 32/64-bit standalone probes, three fixtures |
| L4 | Installation Manager passes the serial step; combined smoke |
| L5 | in use since 2026-10-28 (maintainer to confirm) |
| Cross review | not recorded (written before the cross-review rule) |

## Open points

Stored positions can still be zero.

## Evidence

Solidworks-for-Linux `docs/wine-repro/msxml-filepos/README.md`; overview in `docs/patch-status.md`.

