# riched20 TOM paragraph expansion

On the winum-11.22-3 base, Revit 2027.4 reaches the first plan view and
its windowless TOM range returns E_NOTIMPL for Expand(tomParagraph).
The subsequent TextEngine read from 0x10 is the application symptom.
The measured Windows return for the already complete short range is
S_FALSE, which is a success HRESULT. The patch expands to the editor's
paragraph boundaries and reports the measured HRESULT and delta.

[MEASURE results](../../../docs/wine-repro/riched20-textrange-expand-paragraph/README.md#windows-10-results-2026-11-26)
and [test-first patch](../../tests-first/riched20-tests-textrange-expand-paragraph.patch).

| Level | Status |
| --- | --- |
| MEASURE | Windows 10 offline, 2026-11-26, x86_64 and i686, both window classes and both windowless hosts. |
| L0 | Overlay workflow builds x86_64 and i386 after zero-fuzz application to winum-11.22-3. |
| L1 | Offline Windows 10 run pending for the focused Wine test in both architectures. |
| L2 | Paired winum-11.22-3 versus the Expand candidate, riched20 and msftedit. The baseline expects E_NOTIMPL via todo_wine, while the candidate requires the exact result. |
| L4 | Revit first plan view and sample project with the combined riched20 overlay and existing ole32 overlay. Then repeat with the Expand-only overlay to test whether the tab-stop change is needed. Inventor and AutoCAD text editing smoke pending. |
| Cross review | Claude pending. |

The L4 overlay builds both riched20 changes into a single DLL for each
architecture. It uses the tab-stop patch from commit
`5e28f985333d214841ccfd1ad86b5e5fc281b16e`. Verify the extracted overlay
with `sha256sum -c SHA256SUMS` before using it in a disposable copy of
the test build.
