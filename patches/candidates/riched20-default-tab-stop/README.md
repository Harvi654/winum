# riched20 ITextDocument default tab stop

Revit 2027.4 reports a failure setting the default tab size when building
the first plan view. Wine 11.22 stubs the ITextDocument default tab-stop
methods. The journal also reports an access violation during view
construction. The controlled L4 comparison below confirms that Revit
requires this change together with paragraph navigation v3.

[Native MEASURE program and instructions](../../../docs/wine-repro/riched20-default-tab-stop/README.md).

| Level | Evidence |
| --- | --- |
| MEASURE | Two offline Windows 10 rounds agree across architectures. [Observations](../../../docs/wine-repro/riched20-default-tab-stop/README.md#round-2-result-2026-11-26). |
| L0 | Both DLL architectures built after zero-fuzz application to winum-11.22-3. [Overlay build](https://github.com/Harvi654/winum/actions/runs/45854157578). |
| L1 | Offline Windows 10 passed 96 checks each on x86_64 and i686 on 2026-11-26. Both exited 0 with no failures, todos, flaky checks or skips. [Results](../../../docs/wine-repro/riched20-default-tab-stop/results/l1-windows-10/summary.tsv). [Verified L1 artifact](https://github.com/Harvi654/winum/actions/runs/45853726852/artifacts/15135495595). |
| L2 | Paired riched20 and msftedit passed on 2026-11-26. Ten units compared across x86_64 and i386, zero regressions. Each richole architecture ran 181,682 checks on the candidate (181,587 baseline), with no failures or skips in that unit. [CI](https://github.com/Harvi654/winum/actions/runs/45854157626). |
| L4 | Revit 2027.4 New Project reached “Podlaží 1” on 2026-11-26 with the combined v3 navigation, tab-stop and ole32 overlays in DirectX Software mode. Each fix in isolation still crashed at TextEngine.dll+0x2ac70. See the comparison below. |
| Cross review | Pending independent review by Claude. |

The editor stores the width in twips. Ordinary selection edits keep it
and WM_SETTEXT restores 720 twips. The setter relayouts implicit stops,
and the existing msftedit class flag selects the measured modern range.
A zero or overflow result stays visible to the getter. Layout uses
the normal interval for these values because their layout behavior
was outside the native range probe.

Language and typography option messages differ from Windows in the
trace and reference probe. The successful combined Revit run used the
existing handling of those messages.

## Revit L4 comparison (2026-11-26)

Revit 2027.4 was tested with the ole32 overlay and DirectX Software
rendering (WARP on wined3d).

| RichEdit overlay | New Project result |
| --- | --- |
| Default tab stop only | Crashed at `TextEngine.dll+0x2ac70`. |
| Paragraph navigation v3 only | Crashed at `TextEngine.dll+0x2ac70`. The trace logged two calls to the `ITextDocument2Old_fnGetDefaultTabStop` stub and one to the `ITextDocument2Old_fnSetDefaultTabStop` stub. [Overlay](https://github.com/Harvi654/winum/actions/runs/45875408643/artifacts/15144173677). |
| Paragraph navigation v3 and default tab stop | Opened “Podlaží 1”. The TextEngine crash did not occur. [Overlay](https://github.com/Harvi654/winum/actions/runs/45871395012/artifacts/15142867376). |

Both RichEdit changes are required for this Revit New Project fix.

The [tab-stop-only overlay](https://github.com/Harvi654/winum/actions/runs/45854157578/artifacts/15135865485) is retained for comparison.
