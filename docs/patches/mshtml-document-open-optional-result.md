# mshtml-document-open-optional-result.patch

<!-- winum-test: missing -->

*Status page. Levels as in winum's AGENTS.md. "Not recorded" means no result was written down, not that the check failed.*

- **DLL:** `mshtml`
- **Origin:** Solidworks-for-Linux (private for now), `patches/`
- **In winum:** `patches/series` from winum-11.20-1

## Problem as the user saw it

Simulation: Create Study failed.

## Cause and fix

`IHTMLDocument2::open` accepts a NULL result pointer.

## Wine test

None in the production patch. The
[v2 candidate](../../patches/candidates/mshtml-document-open-optional-result-v2/README.md)
adds test_doc_open_optional_result beside dom.c's existing open tests.
Its ordinary ok() assertions compare supplied/NULL result pointers on
separate loaded copies of our own HTML. Values come from our Windows
MEASURE; the exact standalone body then passed native assertion L1.
The production marker stays missing because that test is not promoted.

## Validation

| Level | Result |
| --- | --- |
| L0 | full builds full-1/full-2 |
| L1 | v2 candidate: Windows 10 x86_64/i686, 34 checks each, 0 failures, exit 0 in both returned result sets (2026-11-09). Production has no integrated test yet |
| L2 | not recorded |
| L3 | compiled; Gecko-dependent probes not run |
| L4 | Simulation study, mesh, solver, results |
| L5 | in use since 2026-10-28 (maintainer to confirm) |
| Cross review | not recorded (written before the cross-review rule) |

## Open points

Formal cross review, Gecko-enabled paired mshtml.dom L2 and consumer validation
remain open before candidate promotion. Do not count a disabled mshtml backend
or native MEASURE alone as runtime validation.

## Evidence

Solidworks-for-Linux `docs/wine-repro/mshtml-open/README.md`; overview in `docs/patch-status.md`.


## History

- 2026-11-09: Recorded native public-API MEASURE and exact-body assertion L1
  for the v2 candidate. Both architectures passed 34 checks in two returned
  result sets. Full-series fuzz-zero apply and complete PE DLL/test links
  passed; implementation hunks and production series remain unchanged.
