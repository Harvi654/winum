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
| L1 | Focused offline Windows 10 run pending. [Verified L1 bundle](https://github.com/Harvi654/winum/actions/runs/45903443989/artifacts/15155829701). |
| L2 | Paired msxml3 suites running. [CI](https://github.com/Harvi654/winum/actions/runs/45903443934). |
| L3 | Local x86_64 overlay matched the native append HRESULT and resulting DOM in all 12 root append cases that reach the append on both platforms. All 36 document cases matched the winum baseline. The focused test ran 27 checks without failures. The baseline recorded two expected HRESULT failures. |
| L4 | Inventor 2027 Content Center → Editor → filter → ISO, then Inventor smoke test. |
| Cross review | Claude pending. |


Verify the overlay with `sha256sum -c SHA256SUMS` and replace both
`msxml3.dll` files in their corresponding architecture directories below
`lib/wine` in a test copy of winum-11.22-3. Test ISO selection and confirm
that the list refreshes. If it still fails, record whether the application
actually appends to the document or an element and which load path it
takes. There is no evidence that its stream is empty.
