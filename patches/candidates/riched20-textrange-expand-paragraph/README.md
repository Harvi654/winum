# riched20 TOM paragraph expansion

On winum-11.22-3, Revit 2027.4 initially stopped at
TextEngine.dll+0x2ac70 while building the first plan. The first combined
RichEdit overlay added Expand and default tab stop. That run reached the
next unsupported method, MoveEnd(tomParagraph). The v3 overlay covers
six paragraph operations, with Windows-matched HRESULT, endpoints and
delta. On 2026-11-26 the combined v3 overlay and ole32 opened New Project
at the Czech plan view “Podlaží 1” using DirectX Software (WARP on wined3d).
The TextEngine crash no longer occurred. The remaining hardware error
(0x80000100), missing drawing output, and managed Rectangle tool error
are tracked separately in the Revit repository's
`docs/revit-2027-4-czech-desktop.md`. Revit work is paused.

[MEASURE results](../../../docs/wine-repro/riched20-textrange-expand-paragraph/README.md#windows-10-paragraph-navigation-results-2026-11-26)
and [test-first patch](../../tests-first/riched20-tests-textrange-expand-paragraph.patch).

| Level | Status |
| --- | --- |
| MEASURE | Windows 10 offline, 2026-11-26. All 10,408 results agree between x86_64 and i686, and all four hosts agree. |
| L0 | Both v3 DLLs built on 2026-11-26 after zero-fuzz application to winum-11.22-3 with the tab-stop candidate. [Combined build](https://github.com/Harvi654/winum/actions/runs/45871395012). |
| L1 | Expand-only test passed on offline Windows 10 on 2026-11-26, 187 checks each on x86_64 and i686, both exit 0, no failures, todos, flaky checks or skips. Expanded v3 navigation passed on offline Windows 10 on 2026-11-26, 1,847 checks each on x86_64 and i686, both exit 0, with no failures, todos, flaky checks or skips. [L1 results](../../../docs/wine-repro/riched20-textrange-expand-paragraph/results/windows-10/navigation-v3-l1.tsv). [Verified v3 L1 bundle](https://github.com/Harvi654/winum/actions/runs/45871395166/artifacts/15142882225). |
| L2 | Paired winum-11.22-3 versus the navigation candidate, riched20 and msftedit. The baseline marks its differing return via todo_wine, while the candidate requires the exact result. Both suites are running (2026-11-26). [Paired run](https://github.com/Harvi654/winum/actions/runs/45871395054). |
| L3 | x86_64 headless check on the release runtime plus the combined overlay matched all 5,204 Windows observations across the two CreateTextServices hosts. The navigation-only overlay also matched all 5,204 x86_64 windowless observations. The local headless run could not create the windowed controls or launch the i686 probe. Both DLL architectures built in CI. |
| L4 | Revit 2027.4 New Project passed with the combined v3 RichEdit and ole32 overlays on 2026-11-26. Navigation-only Revit check and Inventor and AutoCAD text editing smoke pending. |
| Cross review | Claude pending. |

The [v3 L4 overlay](https://github.com/Harvi654/winum/actions/runs/45871395012/artifacts/15142867376)
builds paragraph navigation and default tab stop into a single DLL
for each architecture. It uses the tab-stop patch from commit
`5e28f985333d214841ccfd1ad86b5e5fc281b16e`. Verify the extracted overlay
with `sha256sum -c SHA256SUMS` before using it in a disposable copy of
the test build. Copy each architecture into the corresponding directory
below `lib/wine`, with the ole32 overlay in the same runtime. On the
offline Windows VM, run `run-tests.bat` from a fresh directory and
return the two `result-riched20-textrange-expand-paragraph-*.txt` files
and `exitcodes.txt`.

The [earlier Expand plus tab-stop overlay](https://github.com/Harvi654/winum/actions/runs/45868178047/artifacts/15140129851)
is retained for comparison. The process reached MoveEnd after Expand
with that build.


## Navigation-only Revit comparison

The [navigation-only v3 overlay](https://github.com/Harvi654/winum/actions/runs/45875408643/artifacts/15144173677)
contains both RichEdit DLL architectures. BUILDINFO confirms that it
was built on winum-11.22-3 with the navigation patch alone. Use the same
ole32 overlay and DirectX Software setting as in the successful combined
test. New Project reaching “Podlaží 1” would establish that the default
tab stop change is unnecessary for this Revit fix. Text layout with
tab stops remains a separate inclusion decision.

## Shared patch order

On 2026-11-26 the full winum-11.22-3 series and all three candidates
applied to one verified Wine 11.22 tree with `--fuzz=0`. The tested order
immediately after `riched20-textfont-applytmp.patch` was:

1. `riched20-default-tab-stop.patch` at 5e28f98
2. `riched20-textrange-expand-paragraph.patch` v3 at 7b32f89
3. `riched20-target-device-scroll.patch` at 38e4b23

This is an application check for the proposed order, not a combined build
or combined suite run.
