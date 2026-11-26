# riched20 TOM paragraph expansion

On the winum-11.22-3 base, Revit 2027.4 reaches the first plan view and
its windowless TOM range returns E_NOTIMPL for Expand(tomParagraph).
With Expand and default tab stop applied, Expand now succeeds in the
Revit trace. MoveEnd(tomParagraph) is the next unsupported call. The
read at TextEngine.dll+0x2ac70 still occurs. The third application
attempt implements the six measured paragraph operations in v3. If it does not move the
failure, record the state and defer this Revit issue.
The measured Windows return for the already complete short range is
S_FALSE, which is a success HRESULT. The six methods share the editor's paragraph boundaries and report the
measured HRESULT, endpoints and delta.

[MEASURE results](../../../docs/wine-repro/riched20-textrange-expand-paragraph/README.md#windows-10-paragraph-navigation-results-2026-11-26)
and [test-first patch](../../tests-first/riched20-tests-textrange-expand-paragraph.patch).

| Level | Status |
| --- | --- |
| MEASURE | Windows 10 offline, 2026-11-26. All 10,408 results agree between x86_64 and i686, and all four hosts agree. |
| L0 | Both v3 DLLs built on 2026-11-26 after zero-fuzz application to winum-11.22-3 with the tab-stop candidate. [Combined build](https://github.com/Harvi654/winum/actions/runs/45871395012). |
| L1 | Expand-only test passed on offline Windows 10 on 2026-11-26, 187 checks each on x86_64 and i686, both exit 0, no failures, todos, flaky checks or skips. Expanded v3 navigation passed on offline Windows 10 on 2026-11-26, 1,847 checks each on x86_64 and i686, both exit 0, with no failures, todos, flaky checks or skips. [L1 results](../../../docs/wine-repro/riched20-textrange-expand-paragraph/results/windows-10/navigation-v3-l1.tsv). [Verified v3 L1 bundle](https://github.com/Harvi654/winum/actions/runs/45871395166/artifacts/15142882225). |
| L2 | Paired winum-11.22-3 versus the navigation candidate, riched20 and msftedit. The baseline marks its differing return via todo_wine, while the candidate requires the exact result. Both suites are running (2026-11-26). [Paired run](https://github.com/Harvi654/winum/actions/runs/45871395054). |
| L3 | x86_64 headless check on the release runtime plus the combined overlay matched all 5,204 Windows observations across the two CreateTextServices hosts. The local headless run could not create the windowed controls or launch the i686 probe. Both DLL architectures built in CI. |
| L4 | First combined overlay still stops at TextEngine.dll+0x2ac70 in Revit 2027.4. Retest with the six paragraph operations and default tab stop together in the DLL, with the same ole32 overlay. Inventor and AutoCAD text editing smoke pending. |
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
