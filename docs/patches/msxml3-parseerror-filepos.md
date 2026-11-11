# msxml3-parseerror-filepos.patch

<!-- winum-test: l1 -->

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
| L0 | full builds full-1/full-2; winum-11.20-1 series (promoted 2026-11-11): every entry applies with `--fuzz=0` on the official 11.20 archive; the tree is identical to that of `test-run/all-6` (run 42997560598) |
| L1 | v3 all-6 36900/0 both architectures (promoted file: `msxml3-parseerror-filepos-v3` (2026-11-11: the empty-string `get_errorCode` check for both classes, wording; implementation as v2)) |
| L2 | not recorded; winum-11.20-1: v3 all-6: no new failure |
| L3 | 32/64-bit standalone probes, three fixtures |
| L4 | Installation Manager passes the serial step; combined smoke; winum-11.20-1: SOLIDWORKS, deferred |
| L5 | in use since 2026-10-28 (maintainer to confirm) |
| Cross review | Codex: v2 approved with P2/P3; v3 approved (`e1635a1`) |

## Open points

Stored positions can still be zero.

## Evidence

Solidworks-for-Linux `docs/wine-repro/msxml-filepos/README.md`; overview in `docs/patch-status.md`.

## History

- **2026-11-11, promoted** into `patches/series` for winum-11.20-1 from `msxml3-parseerror-filepos-v3` (2026-11-11: the empty-string `get_errorCode` check for both classes, wording; implementation as v2) (maintainer approval of `docs/release/winum-11.20-1.md` at `ef64451`).
