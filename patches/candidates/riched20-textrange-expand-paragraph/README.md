# riched20 TOM paragraph expansion

On the winum-11.22-3 base, Revit 2027.4 reaches the first plan view and
its windowless TOM range returns E_NOTIMPL for Expand(tomParagraph).
With Expand and default tab stop applied, Expand now succeeds in the
Revit trace. MoveEnd(tomParagraph) is the next unsupported call. The
read at TextEngine.dll+0x2ac70 still occurs. The third application
attempt will extend the candidate to all six paragraph operations,
guided by the new offline MEASURE results. If it does not move the
failure, record the state and defer this Revit issue.
The measured Windows return for the already complete short range is
S_FALSE, which is a success HRESULT. The patch expands to the editor's
paragraph boundaries and reports the measured HRESULT and delta.

[MEASURE results](../../../docs/wine-repro/riched20-textrange-expand-paragraph/README.md#first-probe-windows-10-expand-results-2026-11-26)
and [test-first patch](../../tests-first/riched20-tests-textrange-expand-paragraph.patch).

| Level | Status |
| --- | --- |
| MEASURE | Windows 10 offline, 2026-11-26, x86_64 and i686, both window classes and both windowless hosts. |
| L0 | Both DLLs built on 2026-11-26 after zero-fuzz application to winum-11.22-3. The first build exposed stale hunk context after textfont-applytmp. Context was repaired in v2 without changing the C code or test assertions. [Successful build](https://github.com/Harvi654/winum/actions/runs/45868178047). |
| L1 | Expand-only test passed on offline Windows 10 on 2026-11-26, 187 checks each on x86_64 and i686, both exit 0, no failures, todos, flaky checks or skips. Expanded navigation L1 awaits the new measurement. [Verified L1 bundle](https://github.com/Harvi654/winum/actions/runs/45868178012/artifacts/15141830711). |
| L2 | Paired winum-11.22-3 versus the Expand candidate, riched20 and msftedit. The baseline marks its differing return via todo_wine, while the candidate requires the exact result. [Paired CI](https://github.com/Harvi654/winum/actions/runs/45868178013). |
| L4 | First combined overlay still stops at TextEngine.dll+0x2ac70 in Revit 2027.4. Retest with the six paragraph operations and default tab stop together in the DLL, with the same ole32 overlay. Inventor and AutoCAD text editing smoke pending. |
| Cross review | Claude pending. |

The [first verified L4 overlay](https://github.com/Harvi654/winum/actions/runs/45868178047/artifacts/15140129851)
builds both riched20 changes into a single DLL for each architecture. It uses the tab-stop patch from commit
`5e28f985333d214841ccfd1ad86b5e5fc281b16e`. Verify the extracted overlay
with `sha256sum -c SHA256SUMS` before using it in a disposable copy of
the test build. Copy each architecture into the corresponding directory
below `lib/wine`, with the ole32 overlay in the same runtime. On the
offline Windows VM, run `run-tests.bat` from a fresh directory and
return the two `result-riched20-textrange-expand-paragraph-*.txt` files
and `exitcodes.txt`.
