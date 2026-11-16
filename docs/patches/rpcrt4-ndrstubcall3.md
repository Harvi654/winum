# rpcrt4-ndrstubcall3.patch

<!-- winum-test: l3-only: a valid native call needs NDR64 tables that only MIDL generates; Wine COM sends NDR on both sides (maintainer decision 2026-11-09, docs/wine-repro/rpcrt4-ndrstubcall3/L3.md) -->

*Status page. Levels as in winum's AGENTS.md. "Not recorded" means no result was written down, not that the check failed.*

- **DLL:** `rpcrt4`
- **Origin:** Autodesk-Inventor-for-Linux, commit `4031558`, first shipped in `wine-11.19-1`
- **In winum:** `patches/series` from winum-11.20-1

## Exception (maintainer decision, 2026-11-09)

L3 only. Windows has NdrStubCall3 (x86_64), but a valid call needs NDR64
format tables, which only Microsoft's MIDL generates; with open tools the
Windows measurement could not reach the server (`docs/wine-repro/rpcrt4-ndrstubcall3/README.md`).
Wine's COM marshals NDR on both sides, so the patch uses the interface's
NDR entry and the NDR64 path is never taken in Wine. Reproducer:
[L3.md](../wine-repro/rpcrt4-ndrstubcall3/L3.md) (three Wine-only
scenarios). L4: Stress Analysis Mesh View; at the next L4 also check the
log for "NDR64 syntax is not supported". Follow-up: a native oracle if an
open NDR64 generator ever exists.

## Problem as the user saw it

Stress Analysis Mesh View: Inventor disappeared without a message (unimplemented `NdrStubCall3`).

## Cause and fix

`NdrStubCall3` is implemented like `NdrClientCall3`: the NDR format strings from `pSyntaxInfo`, then the existing `NdrStubCall2` interpreter.

## Wine test

None in the patch.

## Validation

| Level | Result |
| --- | --- |
| L0 | Inventor Wine release `wine-11.19-1` (2026-10-19, CI) and later; winum CI, full build of the 27-patch series, both architectures, no fuzz (2026-11-06) |
| L1 | **no Wine test** - needs one, measured on Windows first |
| L2 | not recorded per patch |
| L3 | 2026-11-11, [run 42999903587](../wine-repro/l3-ci-2026-11-11/README.md), KDE Wayland: both and fallback hr 0 / value 42, ndr64-only refused (0x80070725, server not called), i686 no export - as designed |
| L4 | Mesh View works. (Inventor, maintainer) |
| L5 | in Inventor's runtime since `wine-11.19-1` (2026-10-19); a week of use not formally recorded - maintainer to confirm |
| Cross review | not recorded (written before the cross-review rule) |

### winum-11.22-1 (Wine 11.22)

| Level | Result | Date |
| --- | --- | --- |
| L0 | the whole series on the official Wine 11.22 archive, both architectures, `--fuzz=0` ([run 43703211594](https://github.com/Harvi654/winum/actions/runs/43703211594)) | 2026-11-15 |
| L1 | no Windows L1 (`l3-only`, reason above); the patch is the same as in winum-11.20-2 | - |
| L2 | [run 43703213332](https://github.com/Harvi654/winum/actions/runs/43703213332), group g3 re-run [43739605851](https://github.com/Harvi654/winum/actions/runs/43739605851), paired against vanilla Wine 11.22: only the known flaky failures (ntdll threadpool, ntdll time, urlmon `url.c:3535` not repeated in the re-run); the workflow shows "failure" because of these | 2026-11-15 |
| L4 | Inventor 2027 on a copy of the maintainer's prefix: import, parts, assemblies (maintainer) - a smoke test of the build, not a retest of this patch's own feature. SOLIDWORKS and AutoCAD: deferred (not being worked on since 2026-11-09, AGENTS.md) | 2026-11-15 |

## Open points

Wine test missing.

## Evidence

Autodesk-Inventor-for-Linux [`docs/arch-hyprland.md`](https://github.com/Harvi654/Autodesk-Inventor-for-Linux/blob/main/docs/arch-hyprland.md) (search for the patch name).

