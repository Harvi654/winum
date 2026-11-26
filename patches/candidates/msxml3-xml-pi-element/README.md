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
| L0 | Both patches apply with zero fuzz to winum-11.22-3. Overlay build pending. |
| L1 | Focused Windows 10 run pending. |
| L2 | Paired msxml3 suites pending. |
| L4 | Inventor 2027 Content Center → Editor → filter → ISO, then Inventor smoke test. |
| Cross review | Claude pending. |
