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
| L0 | Pending. Build both architectures on the winum-11.22-3 series with zero fuzz. |
| L1 | Focused richole assertions build for both architectures. Native execution pending. |
| L2 | Pending. Pair riched20 and msftedit with and without the candidate on the same series. |
| L4 | Pending. Use the riched20 overlay with the ole32 overlay. Verify Revit New Project and a sample project on the desktop PC, plus Inventor and AutoCAD text editing. |
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

For L4, verify SHA256SUMS in the extracted candidate overlay. Use a test
copy of the winum-11.22-3 runtime and copy each DLL into its corresponding
architecture directory under lib/wine. Include the ole32 overlay in
that same runtime for the Revit checks.
