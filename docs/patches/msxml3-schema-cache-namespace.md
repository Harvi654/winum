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

### winum-11.22-1 (Wine 11.22)

| Level | Result | Date |
| --- | --- | --- |
| L0 | the whole series on the official Wine 11.22 archive, both architectures, `--fuzz=0` ([run 43703211594](https://github.com/Harvi654/winum/actions/runs/43703211594)) | 2026-11-15 |
| L1 | unchanged from winum-11.20-2: the patch and its test are the same, and Windows did not change | - |
| L2 | [run 43703213332](https://github.com/Harvi654/winum/actions/runs/43703213332), group g3 re-run [43739605851](https://github.com/Harvi654/winum/actions/runs/43739605851), paired against vanilla Wine 11.22: only the known flaky failures (ntdll threadpool, ntdll time, urlmon `url.c:3535` not repeated in the re-run); the workflow shows "failure" because of these | 2026-11-15 |
| L4 | Inventor 2027 on a copy of the maintainer's prefix: import, parts, assemblies (maintainer) - a smoke test of the build, not a retest of this patch's own feature. SOLIDWORKS and AutoCAD: deferred (not being worked on since 2026-11-09, AGENTS.md) | 2026-11-15 |

## Open points

MSXML6 only; imports/includes outside fixtures.

## Evidence

Solidworks-for-Linux `docs/wine-repro/xml-schema-namespace/README.md`; overview in `docs/patch-status.md`.

