# riched20 ITextRange paragraph expansion

Revit 2027.4 reaches TOM while creating its first plan view. Its
windowless range attempts Expand(tomParagraph) on a duplicate;
Wine 11.22 currently returns E_NOTIMPL. The following failure is
a read from 0x10 inside TextEngine.dll. The trace establishes the
order of the calls. The cause of the crash remains an L4 question.

[MEASURE program and offline instructions](../../../docs/wine-repro/riched20-textrange-expand-paragraph/README.md).

| Level | Evidence |
| --- | --- |
| MEASURE | Native Windows 10 reference pending for x86_64 and i686. |
| L0 | Pending on winum-11.22-3. |
| L1 | Pending a focused richole test based on the measured outcomes. |
| L2 | Pending paired riched20 and msftedit suites. |
| L4 | Pending Revit New Project and a sample project with both riched20 changes in one DLL. Repeat with only Expand to decide whether the default-tab-stop candidate is needed. |
| Cross review | Claude after test and fix. |

Scope depends on the native measurement. If line results depend on
layout, leave that unit outside this candidate. For application
validation, build one overlay by applying the default tab stop and
Expand candidates on top of the release series. Test the Expand-only
variant before considering either candidate for the series.
