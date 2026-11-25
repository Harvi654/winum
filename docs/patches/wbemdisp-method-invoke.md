# wbemdisp-method-invoke.patch

<!-- winum-test: l1 -->

- **DLL:** `wbemdisp` (`dlls/wbemdisp/locator.c`), test in `wbemdisp`
- **Status:** In the winum-11.22-3 series. [Candidate history](../../patches/candidates/wbemdisp-method-invoke/README.md)
- **Target:** winum-11.22-3

## Symptom

Inventor Nastran 2027 did not appear in Inventor at all: its installer's custom action `Register_Addin` (`cscript addin.vbs`) stopped with "Object doesn't support this action" when it called a StdRegProv method directly, so the add-in was never registered (Wine bug 39463).

## Cause and fix

VBScript calls `oReg.GetStringValue ...` through `IDispatch::Invoke` with `DISPATCH_METHOD` (or `METHOD | PROPERTYGET`); Wine's wbemdisp only answered property gets. Windows 10 executes the method, passes positional inputs in signature order, copies outputs back by reference and returns the provider status separately (measured). The patch routes dynamic method members through the method signature and `IWbemServices::ExecMethod`; type-library and property dispatch are unchanged. Named arguments are outside the measured scope.

In plain words: scripts can now call WMI methods such as `StdRegProv.GetStringValue` directly, as installers do.

## How it was found

`WINEDEBUG=+msi` for the custom action, `+wbemdisp` (`fixme:wbemdisp:object_Invoke flags 0x1 not supported`); native C and VBScript reference `docs/wine-repro/wbemdisp-method-invoke/`.

## Wine test

`dlls/wbemdisp/tests/method.c`: 13 StdRegProv calls (create, set, get, enum, delete, wrong argument counts), each with both flag combinations; the test's `todo_wine` was dropped when it moved into the patch (all 26 Invoke checks pass).

## Validation

| Level | Result |
| --- | --- |
| L0 | Applies with `--fuzz=0` after the full series on Wine 11.22; built x86_64 and i386 in the combined next-release build [45485575952](https://github.com/Harvi654/winum/actions/runs/45485575952) (2026-11-24) and by "Build Wine" on the release branch |
| L1 | Offline Windows 10: 135 checks per architecture, no failures, todos or skips |
| L2 | Paired wbemdisp, wbemprox, vbscript: 18 units, no regressions [45261162354](https://github.com/Harvi654/winum/actions/runs/45261162354); [Run 45485567767](https://github.com/Harvi654/winum/actions/runs/45485567767): the released 11.22-2 series against it plus the seven 11.22-3 patches, 452 units in five groups, both architectures; no new failure (details in `docs/wine-repro/next-release-1/README.md`) |
| L3 | 30 C calls and the VBScript output matched the Windows reference [45260888480](https://github.com/Harvi654/winum/actions/runs/45260888480) |
| L4 | Nastran's `Register_Addin` completes (maintainer, 2026-11-23). With the combined next-release build (maintainer, 2026-11-24/25): Inventor 2027.2 + Nastran prefix copy - part, assembly, drawing, open/save/save-as and reopen, Stress Analysis, Frame Analysis, Frame Generator, Nastran Generate Mesh / Run / results without the 0x80040154 dialog; AutoCAD 2027.1.1 fresh install from its Create Installer image (33 steps, exit 0), sign-in, open, SAVE, SAVEAS, reopen. SOLIDWORKS deferred under AGENTS.md |
| L5 | Pending |
| Cross review | Written by Codex; reviewed and approved by Claude (see the candidate README) |

## Open

- Known limitation: the direct IDispatch path maps `ERROR_UNSUPPORTED_TYPE` from `StdRegProv.GetStringValue` to `WBEM_E_TYPE_MISMATCH` as measured, while `ExecMethod_` keeps the provider result; a separate wbemprox candidate if a program needs it.

## Notes for a Wine bug report

Wine bug 39463: VBScript calling a StdRegProv method directly. Public test: the wbemdisp test above.

## History

- v1 2026-11-23: candidate.
- v2 2026-11-25: no code change; test moved into the patch, todo dropped; promoted to the series.
