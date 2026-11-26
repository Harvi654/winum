# riched20 ITextDocument default tab stop

Revit 2027.4 reports a failure setting the default tab size when building
the first plan view. Wine 11.22 stubs the ITextDocument default tab-stop
methods. The journal also reports an access violation during view
construction. Revit testing will establish whether this candidate
addresses that crash.

[Native MEASURE program and instructions](../../../docs/wine-repro/riched20-default-tab-stop/README.md).

| Level | Evidence |
| --- | --- |
| MEASURE | Two offline Windows 10 rounds agree across architectures. [Observations](../../../docs/wine-repro/riched20-default-tab-stop/README.md#round-2-result-2026-11-26). |
| L0 | Both DLL architectures built after zero-fuzz application to winum-11.22-3. [Overlay build](https://github.com/Harvi654/winum/actions/runs/45854157578). |
| L1 | Offline Windows 10 passed 96 checks each on x86_64 and i686 on 2026-11-26. Both exited 0 with no failures, todos, flaky checks or skips. [Results](../../../docs/wine-repro/riched20-default-tab-stop/results/l1-windows-10/summary.tsv). [Verified L1 artifact](https://github.com/Harvi654/winum/actions/runs/45853726852/artifacts/15135495595). |
| L2 | Paired riched20 and msftedit passed on 2026-11-26. Ten units compared across x86_64 and i386, zero regressions. Each richole architecture ran 181,682 checks on the candidate (181,587 baseline), with no failures or skips in that unit. [CI](https://github.com/Harvi654/winum/actions/runs/45854157626). |
| L4 | Revit 2027.4 New Project reached “Podlaží 1” on 2026-11-26 with the combined v3 navigation, tab-stop and ole32 overlays in DirectX Software mode. The navigation-only comparison is pending. Text layout with tabs remains a separate reason to consider this candidate. |
| Cross review | Claude after the test and fix. |

The editor stores the width in twips. Ordinary selection edits keep it
and WM_SETTEXT restores 720 twips. The setter relayouts implicit stops,
and the existing msftedit class flag selects the measured modern range.
A zero or overflow result stays visible to the getter. Layout uses
the normal interval for these values because their layout behavior
was outside the native range probe.

Language and typography option messages differ from Windows in the
trace and reference probe. This candidate isolates the tab-stop
changes so L4 can determine whether the other messages matter.

For L4, verify SHA256SUMS in the extracted [candidate overlay](https://github.com/Harvi654/winum/actions/runs/45854157578/artifacts/15135865485). Use a test
copy of the winum-11.22-3 runtime and copy each DLL into its corresponding
architecture directory under lib/wine. Include the ole32 overlay in
that same runtime for the Revit checks.
