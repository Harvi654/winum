# riched20 ITextDocument default tab stop

Revit 2027.4 reports a failure setting the default tab size when building
the first plan view. Wine 11.22 stubs the ITextDocument default tab-stop
methods. The journal also reports an access violation during view
construction. Revit testing will establish whether this candidate
addresses that crash.

[Native MEASURE program and instructions](../../../docs/wine-repro/riched20-default-tab-stop/README.md).

| Level | Evidence |
| --- | --- |
| MEASURE | x86_64 and i686 probes build. Offline Windows 10 results pending. |
| L0 | Pending. Build both architectures on the winum-11.22-3 series with zero fuzz. |
| L1 | Pending. Add the measured assertions to riched20 tests and run both executables on offline Windows 10. |
| L2 | Pending. Pair riched20 and msftedit with and without the candidate on the same series. |
| L4 | Pending. Use the riched20 overlay with the ole32 overlay. Verify Revit New Project and a sample project on the desktop PC, plus Inventor and AutoCAD text editing. |
| Cross review | Claude after the test and fix. |

Native results determine tab-stop validation, storage and layout
effects. The language and typography messages are separate observations
until compared against the same Wine base.
