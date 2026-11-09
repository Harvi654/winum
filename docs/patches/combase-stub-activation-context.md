# combase-stub-activation-context.patch

<!-- winum-test: missing -->

*Status page. Levels as in winum's AGENTS.md. "Not recorded" means no result was written down, not that the check failed.*

- **DLL:** `combase`
- **Origin:** Autodesk-Inventor-for-Linux, commit `cc2c036`, first shipped in `wine-11.19-1`
- **In winum:** `patches/series` from winum-11.20-1

## Problem as the user saw it

Stress Analysis: the mesher's call back into Inventor crashed (`FEA_Addin_Utils.dll`), because a class from an add-in manifest was not found.

## Cause and fix

The original workaround addresses incoming COM calls on an RPC worker without
an activation context. It records the context when the stub manager is created
and activates that saved context around IRpcStubBuffer_Invoke. This restored
the maintainer's Stress Analysis path, but the claimed general Windows rule
is not established: our native IClassFactory observations below do not select
a persistent first-marshaling context. The existing CAD workaround is retained
while its scope is investigated.

## Wine test

None in the production patch. The
[v2 candidate](../../patches/candidates/combase-stub-activation-context-v2/README.md)
adds a standard-marshaled IClassFactory regression covering both STA/MTA
server directions, two marshal contexts and three caller contexts. Its
expected values come from our own complete Windows MEASURE. It is BLOCKED:
native L1 passed, but production conformance has not been established.

## Validation

| Level | Result |
| --- | --- |
| L0 | Inventor Wine release `wine-11.19-1` (2026-10-19, CI) and later; winum CI, full build of the 27-patch series, both architectures, no fuzz (2026-11-06) |
| L1 | v2 candidate: Windows 10 x86_64/i686, 291 checks each, 0 failures, exit 0 (2026-11-09). Production has no integrated test yet |
| L2 | Focused [paired Wine run 42567082861](https://github.com/Harvi654/winum/actions/runs/42567082861) launched on 2026-11-09; runtime verdict pending |
| L3 | not recorded |
| L4 | Stress Analysis mesh in Inventor 2026 (wine-11.20-2 onwards). (Inventor, maintainer) |
| L5 | in Inventor's runtime since `wine-11.19-1` (2026-10-19); a week of use not formally recorded - maintainer to confirm |
| Cross review | not recorded (written before the cross-review rule) |

## Open points

Native observations and assertion L1 are complete for the IClassFactory
fixture. Paired Wine comparison is needed to isolate the current patch's
MTA capture behavior; the local STA shortcut is a distinct source path.
Do not generalize the observations to every COM interface or remove a working
CAD workaround without a reviewed correction and consumer validation. The
production marker stays missing until an appropriate test/fix is promoted.

## Evidence

Autodesk-Inventor-for-Linux [`docs/arch-hyprland.md`](https://github.com/Harvi654/Autodesk-Inventor-for-Linux/blob/main/docs/arch-hyprland.md) (search for the patch name).


## History

- 2026-11-09: Recorded complete Windows MEASURE and native assertion L1 for
  the blocked v2 conformance candidate. All production implementation hunks
  and patches/series remain unchanged; formal review and runtime/CAD gates
  remain open.
