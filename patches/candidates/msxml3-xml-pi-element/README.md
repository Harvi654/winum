# MSXML XML declaration append under an element

Inventor 2027 takes a different path after the Content Center ISO
filter on winum-11.22-3. The trace shows a DOMDocument60 stream load,
root lookup and an XML processing instruction append before the UI
path diverges. The synthetic probe found that an element append
accepts this node on Wine and fails on Windows. A document append
matches Windows, including when a declaration is already present.
The application trace does not identify the append destination.

The candidate checks only appendChild to an element for the exact
`xml` processing instruction in the measured MSXML 3 and 6 versions.
It returns the measured HRESULT before mutating the tree.

| Level | State |
| --- | --- |
| MEASURE | Offline Windows 10 and winum-11.22-3 completed all 52 cases on x86_64 and i686. [Comparison](../../../docs/wine-repro/msxml3-content-center-stream/README.md#offline-windows-10-comparison). |
| Test | `patches/tests-first/msxml3-tests-xml-pi-element.patch` tests the two native HRESULTs and checks that the returned node is null and the XML unchanged. |
| L0 | Both DLL architectures built after zero-fuzz application to winum-11.22-3. [Verified overlay](https://github.com/Harvi654/winum/actions/runs/45903444191/artifacts/15156702582). |
| L1 | Passed on offline Windows 10, 2026-11-26. Both x86_64 and i686 passed 27 checks with exit 0 and no failures, todos, flaky checks or skips. [L1 results](../../../docs/wine-repro/msxml3-content-center-stream/results/l1-windows-10/summary.tsv). [Verified L1 bundle](https://github.com/Harvi654/winum/actions/runs/45903443989/artifacts/15155829701). |
| L2 | Paired msxml3 suites running. [CI](https://github.com/Harvi654/winum/actions/runs/45903443934). |
| L3 | Local x86_64 overlay matched the native append HRESULT and resulting DOM in all 12 root append cases that reach the append on both platforms. All 36 document cases matched the winum baseline. The focused test ran 27 checks without failures. The baseline recorded two expected HRESULT failures. |
| L4 | Failed on 2026-11-26 at 23:20 local time. Inventor 2027 still crashed on ISO selection with the verified overlay from commit 0529f17. The reported trace calls domdoc_appendChild on the same document pointer as creation, while this candidate changes element appends. |
| Cross review | Claude pending. |


The ISO trace points to a document append. For that destination Windows
and winum both return S_OK and produce top-level xml/root/xml for the
loaded document with a declaration. The candidate remains a standalone
compatibility finding with passing L1. It stays outside the series until
a consumer needs it.

The missing-root stream return and the BOM-less UTF-16LE DOMDocument30
behavior are independent [measured differences](../../../docs/wine-repro/msxml3-content-center-stream/README.md#offline-windows-10-comparison).
The next Inventor test uses the
[diagnostic overlay](../../debug/msxml3-content-center-trace/README.md)
on the release behavior to capture the return values of the real sequence.
