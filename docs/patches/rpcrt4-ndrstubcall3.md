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
| L3 | not recorded |
| L4 | Mesh View works. (Inventor, maintainer) |
| L5 | in Inventor's runtime since `wine-11.19-1` (2026-10-19); a week of use not formally recorded - maintainer to confirm |
| Cross review | not recorded (written before the cross-review rule) |

## Open points

Wine test missing.

## Evidence

Autodesk-Inventor-for-Linux [`docs/arch-hyprland.md`](https://github.com/Harvi654/Autodesk-Inventor-for-Linux/blob/main/docs/arch-hyprland.md) (search for the patch name).

