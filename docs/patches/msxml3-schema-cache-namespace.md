# msxml3-schema-cache-namespace.patch

<!-- winum-test: l1 -->

*Status page. Levels as in winum's AGENTS.md. "Not recorded" means no result was written down, not that the check failed.*

- **DLL:** `msxml3`
- **Origin:** Solidworks-for-Linux (private for now), `patches/`
- **In winum:** `patches/series` from winum-11.20-1

## Problem as the user saw it

Saving an assembly failed (XML schema validation without the target namespace).

## Cause and fix

MSXML6 schema caches adopt the schema's target namespace.

## Wine test

`dlls/msxml3/tests/schema.c`

## Validation

| Level | Result |
| --- | --- |
| L0 | full-2 (2026-10-26) |
| L1 | Windows 10 x64/i686 350 checks, 0 failures (913200b) |
| L2 | recorded within scope (patch-status.md) |
| L3 | nine fixtures incl. invalid input; ASan/UBSan |
| L4 | assemblies with mates, Save As/reopen, Simulation, restart (full-2, 2026-10-26) |
| L5 | in progress since 2026-10-26 (maintainer to confirm) |
| Cross review | not recorded (written before the cross-review rule) |

## Open points

MSXML6 only; imports/includes outside fixtures.

## Evidence

Solidworks-for-Linux `docs/wine-repro/xml-schema-namespace/README.md`; overview in `docs/patch-status.md`.

