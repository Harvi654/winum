# MSXML selection removal and the Inventor Content Center filter

**Status:** v2 fixes the ISO-filter crash in Inventor 2027 and preserves
the filter across restart. Paired L2, AutoCAD and Revit application
checks, and maintainer review are tracked below.

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
3. The UTF-16 stream and XML processing instruction sequence occurs
   while saving settings. The element-append candidate matched a
   native compatibility difference, but the verified overlay still
   crashed on ISO. The operation in the Inventor trace was
   domdoc_appendChild on the same document instance as creation. For
   that case Windows and winum return S_OK and produce top-level
   xml/root/xml. [Element-append measurements](../wine-repro/msxml3-content-center-stream/README.md#offline-windows-10-comparison).
4. A debug-only msxml3 overlay on the release behavior logged stream
   load and parse results, root lookup and append results with the
   destination type. It also provided a bounded local DOM preview.
   That experiment separated settings serialization from the failing
   call. The XML PI candidate and the debug patch stay outside the
   series. [Trace overlay](https://github.com/Harvi654/winum/actions/runs/45904679098/artifacts/15158046259).
5. The focused +msxml trace loads ContentCenterSettings.xml using a
   BSTR file path. It finds the settings root, eight filter groups and a
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
| L0 | Both DLL architectures built with v2 after zero-fuzz application to winum-11.22-3. [Verified overlay](https://github.com/Harvi654/winum/actions/runs/45907358465/artifacts/15158681305). |
| L1 | Offline Windows 10 passed 149 checks per architecture on 2026-11-26. Both exit codes are 0, with no failures, todos, flaky checks or skips. The test is unchanged in v2. [Results](../wine-repro/msxml3-selection-removeall/results/l1-windows-10/summary.tsv). [L1 bundle](https://github.com/Harvi654/winum/actions/runs/45907168374/artifacts/15158600698). |
| L2 | Paired msxml3 suites running for v2. [CI](https://github.com/Harvi654/winum/actions/runs/45907358527). |
| L3 | Local x86_64 v2 matched all 40 complete native removeAll case records, including XML serialization, both selection paths and the advanced cursor. All 149 focused assertions passed without todos. The unpatched release showed ten expected HRESULT differences. |
| L4 Inventor 2027 | Passed on the maintainer's test copy of winum-11.22-3 with the v2 overlay (run 45907358465). The maintainer verified the runtime path, both SHA256SUMS entries (x86_64 `7d536b5b…`, i386 `289cdc8f…`), and ran with +msxml. ISO refreshed the list without a crash. A changed filter survived restart and was reloaded. The trace recorded 27 removeAll calls, no removeAll stub message, and no e06d7363 exceptions. |
| L4 AutoCAD | Open. Start, test a drawing, save and reopen. |
| L4 Revit | Open. Start, test a project, save and reopen. |
| Review | Cross review (Claude Opus 5.5, 2026-11-27): code and test read in full; approved for the series with one condition. See below. |

## Cross review (2026-11-27)

Read in full: the candidate patch, the test patch, the measurement
README and this page; `domnode_unlink_child`, `domnode_release`, the
selection destructor and the attribute list in Wine 11.22's msxml3.

- Reference counting is right. Each selected node is still held by the
  selection while it is unlinked; `domnode_unlink_child` moves its
  count off the old parent chain and `domnode_release` then drops the
  selection's own reference, as the selection destructor does. For an
  ancestor and a descendant in one selection (measured, test case 4),
  the ancestor survives its release on the descendant's count and is
  freed when the descendant is unlinked from it; the descendant is
  freed by its own release. No use after free.
- The empty-tag flag is set only when the removed node is the element
  parent's only child at that moment, so a parent loses its children
  one by one and is marked on the last, as measured (`<left/>`,
  `<group/>`).
- Condition for the series: the test must not keep `todo_wine` on
  `removeAll` once the fix is in the series (with the fix the call
  succeeds and a todo block that succeeds is a failure). As with the
  RichEdit paragraph test, carry the series copy without the todo.

Open points, not blocking (Inventor selects elements only):

- Attribute selections (`selectNodes("@a")`) are not measured. Wine
  keeps attributes on their own list with the same `entry` link, so the
  patch would detach the attribute; what Windows does is unknown.
- A selection holding a document type node is skipped (Wine's existing
  restriction); not measured.
- Only `removeAll` sets the empty-tag flag; whether `removeChild` of the
  last child serializes the same way on Windows is not measured here.
- `domnode_unlink_child` is no longer static; acceptable for a
  one-DLL patch.

Series inclusion requires the completed L2 comparison, the AutoCAD and
Revit L4 checks, and maintainer cross review. The intended release is
the Wine 11.23 rebase or, if it ships first, winum-11.22-4 with the
four RichEdit and ole32 patches.

[Inventor issue #45](https://github.com/Harvi654/Autodesk-Inventor-for-Linux/issues/45)
