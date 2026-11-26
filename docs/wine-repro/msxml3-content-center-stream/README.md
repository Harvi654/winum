# MSXML Content Center stream measurement

This probe isolates the stream load and XML processing instruction seen
just before Inventor changes its Content Center filter path under Wine.
It uses synthetic XML and can run without Inventor or a network
connection. The [Inventor trace notes](https://github.com/Harvi654/Autodesk-Inventor-for-Linux/blob/92f7769/docs/arch-hyprland.md#content-center-filtering-by-a-standard-crashes-inventor-issue-45-open)
describe the application symptom. The trace alone does not identify
the return value that changes the application path.

## Matrix

Each architecture runs 52 cases using DOMDocument 3.0 and 6.0. The
program creates the `xml` processing instruction before loading, then
crosses the following stream variations:

- UTF-16LE with or without a BOM, declaration, and simple root. Cases
  with no root include a zero-byte stream, BOM only, and a declaration
  with or without a BOM.
- `IXMLDOMDocument::load` with a `VT_UNKNOWN` stream and
  `IPersistStreamInit::Load`. The latter has no `VARIANT_BOOL` argument,
  so the probe records `success=NA`.
- Append the PI to the document, or in a separate fresh case to the
  root when one was requested. The caller in the trace is followed by
  `get_documentElement`, and its append destination is uncertain.
- Control cases append to a newly created empty document and to a
  document already containing a declaration after `loadXML`.

The probe records the load HRESULT, parse error code and reason, root
HRESULT and name, append HRESULT, and before/after document child
types, names, and XML. String output escapes control and non-ASCII
characters so the serialized XML can be compared byte for byte.
The direct IRunnableObject query is diagnostic only. Empty-input and
append errors are measurement results. Exit 0 means the probe reached
the end of every case.

## Run on offline Windows 10

Verify SHA256SUMS, extract to a new folder, and run `run-measure.bat`.
Return `result-msxml3-content-center-stream-x86_64.txt`,
`result-msxml3-content-center-stream-i686.txt` and `exitcodes.txt`.
Each complete probe ends with `measure_complete cases=52 failed=0`.

CI runs the same binaries against the pinned winum-11.22-3 release.
Compare corresponding cases by API result and DOM state. Keep localized
parse errors separate from code or document changes. Any candidate test
and DLL change follow the offline Windows results. The application
check is Inventor 2027 Content Center → Editor → filter → ISO, then a
smoke test. Claude will review any resulting candidate.

## winum-11.22-3 baseline

CI completed all 52 cases on both x86_64 and i686 with exit 0. The case
records match across architectures. For example, DOMDocument60 loaded
the BOM-prefixed stream with an existing declaration and returned true.
Appending the new `xml` PI to the document returned `S_OK` and left
top-level nodes `xml`, `root`, `xml` in that order. The native results
are pending, so this is a baseline observation.

- [Verified offline VM bundle](https://github.com/Harvi654/winum/actions/runs/45902758084/artifacts/15155758981)
- [winum baseline results](https://github.com/Harvi654/winum/actions/runs/45902758084/artifacts/15156198839)
