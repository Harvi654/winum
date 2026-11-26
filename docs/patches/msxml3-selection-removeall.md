# MSXML selection removal and the Inventor Content Center filter

**Status:** Candidate and tests based on the completed offline Windows 10
measurement. Application validation is pending.

Inventor 2027 on winum-11.22-3 crashes when the Content Center editor's
standard filter is set to ISO. Windows refreshes the open editor. The
Wine trace ends with an unhandled C++ exception on the path that
closes and reopens the browser window.

## Findings in order

1. The call to EndDialog uses the MDI client. The offline Windows
   probe also returns FALSE for a non-dialog target, so this did not
   account for the different path.
2. The window-tree comparison surfaced the CBS_HASSTRINGS combo-box
   style difference. A temporary overlay changing it left the crash
   reproducible.
3. The XML stream and XML processing instruction sequence occurs while
   saving settings. The measured document-level append matches Windows.
   The element-append candidate fixed a separate discrepancy and the
   crash remained. The candidate and the trace overlay stay outside
   the series.
4. The focused trace loads ContentCenterSettings.xml using a BSTR file
   path. It finds the settings root, eight filter groups and a
   FilterPartCategories node. The latter has no string children, so
   selectNodes("./string") correctly returns an empty selection.
   QueryInterface to IXMLDOMSelection succeeds, then removeAll reaches
   the Wine E_NOTIMPL stub. ISupportErrorInfo is unavailable on the
   selection and the unhandled C++ exception follows.

The native measurement confirms S_OK on the empty selection. It also
shows the same result for populated selections, irrespective of the
cursor. After successful removal the old selection stays empty, and
a fresh query can see newly inserted matches.

## Validation

| Level | State |
| --- | --- |
| MEASURE | [Probe and instructions](../wine-repro/msxml3-selection-removeall/README.md). 80 matching cases on both native architectures. |
| L0 | Both patches apply with zero fuzz to winum-11.22-3. Overlay build pending. |
| L1 | Focused offline Windows test pending. |
| L2 | Paired msxml3 suites pending. |
| L4 | Set ISO in the Content Center editor and verify the list refreshes. Change the filter, restart Inventor and check that the saved setting reloads. |
| Review | Maintainer after the candidate. |

[Inventor issue #45](https://github.com/Harvi654/Autodesk-Inventor-for-Linux/issues/45)
