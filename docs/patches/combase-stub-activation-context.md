# combase-stub-activation-context.patch

<!-- winum-test: missing -->

*Status page. Levels as in winum's AGENTS.md. "Not recorded" means no result was written down, not that the check failed.*

- **DLL:** `combase`
- **Origin:** Autodesk-Inventor-for-Linux, commit `cc2c036`, first shipped in `wine-11.19-1`
- **In winum:** `patches/series` from winum-11.20-1

## Problem as the user saw it

Stress Analysis: the mesher's call back into Inventor crashed (`FEA_Addin_Utils.dll`), because a class from an add-in manifest was not found.

## Cause and fix

Incoming COM calls ran on an RPC worker thread without an activation context. The stub manager now records the activation context active when it is created, and `rpc_execute_call` activates it around `IRpcStubBuffer_Invoke`, as Windows runs calls in the object's context.

## Wine test

None in the patch.

## Validation

| Level | Result |
| --- | --- |
| L0 | Inventor Wine release `wine-11.19-1` (2026-10-19, CI) and later; winum CI, full build of the 27-patch series, both architectures, no fuzz (2026-11-06) |
| L1 | **no Wine test** - needs one, measured on Windows first |
| L2 | not recorded per patch |
| L3 | not recorded |
| L4 | Stress Analysis mesh in Inventor 2026 (wine-11.20-2 onwards). (Inventor, maintainer) |
| L5 | in Inventor's runtime since `wine-11.19-1` (2026-10-19); a week of use not formally recorded - maintainer to confirm |
| Cross review | not recorded (written before the cross-review rule) |

## Open points

Wine test missing; Windows behaviour for the context of incoming calls not measured by our own program.

## Evidence

Autodesk-Inventor-for-Linux [`docs/arch-hyprland.md`](https://github.com/Harvi654/Autodesk-Inventor-for-Linux/blob/main/docs/arch-hyprland.md) (search for the patch name).

