# IHTMLDocument2::open optional result: Windows MEASURE

Original, AI-assisted (Codex), LGPL-2.1-or-later. Native HRESULTs and returned
interfaces are unknown until Windows output arrives. No conformance test or
L1 pass is claimed; production patches, series and test marker are unchanged.

## Controlled documents and result-pointer modes

Each process creates three fresh CLSID_HTMLDocument instances through public
COM APIs: created-only, IPersistStreamInit.InitNew, and Load of our own empty
HTML document from an owned HGLOBAL/IStream. The loaded case pumps messages
and waits up to five seconds for readyState=complete. The InitNew case is an
immediate observation, not an assumption that asynchronous work has finished.

Every case observes parentWindow before open. It calls open with text/html and
three omitted optional arguments (VT_ERROR/DISP_E_PARAMNOTFOUND), then reports
the HRESULT, whether a result was returned, an optional IHTMLWindow2 QI, and
parentWindow after the operation. Successful opens are closed. Return values
of open, close and window queries are observations, not guessed requirements.
No window text, pointer address, document data or filesystem path is printed.

control.c supplies the output pointer. measure.c passes NULL. The modes run
in separate processes, so a NULL-pointer fault cannot erase the positive
control results. A 30-second independent watchdog bounds the whole process,
including asynchronous loading and COM cleanup, without waiting on stdio.
All BSTR, stream/HGLOBAL and interface ownership is balanced. A canonical
IUnknown reference is held until after releasing IHTMLDocument2, consistent
with the lifetime pattern in Wine's native IE tests.

The fixture uses only MinGW declarations/import libraries and our own HTML.
initguid.h instantiates the SDK's HTMLDocument CLSID definition. No Microsoft
or Autodesk files are copied, no symbols/disassembly are inspected, no host
application is installed and no URL is fetched. No registry edit is made by
the program. The document is loaded from memory and contains no external
resources, scripts or private content.

## What can establish the contract

A control with a successful open, non-NULL dispatch result and a usable
pre-open parentWindow is needed to establish the successful-open path. A
matching prepared NULL-result case can then measure whether omitting the
output is supported. If every control fails, or the loaded case never becomes
ready, this fixture only establishes failure/early-pointer behavior; do not
infer success semantics or write a passing test from that partial coverage.
A failed QI of the returned dispatch is reported without guessing its type.

All three cases must reach CASE_END and SUMMARY with completed=3 and
api_errors=0 for complete setup. A zero exit alone is not a conformance pass:
the actual open HRESULTs may still be failures. A timeout, missing SUMMARY,
nonzero exit or failed setup must be returned and investigated, not marked
as L1 or a native contract. The existing Wine patch guards both its initial
output assignment and its successful window-result/AddRef path; only a
successful native control can support coverage of the latter.

## Build and offline Windows run

Both pointer modes compile for x86_64/i686 with MinGW and -Wall -Wextra
-Werror, static CRT, -lole32 -loleaut32 -luuid. Compilation is not execution.
The main bundle list contains both entries; this topic alone can be built:

```sh
export HTML_MEASURE="$HOME/.cache/winum-mshtml-measure-$(date +%Y%m%d-%H%M%S)"
bash scripts/build-measure-bundle.sh "$HTML_MEASURE" docs/wine-repro/mshtml-document-open-optional-result/measure.list
```

Copy the directory into a new directory in offline Windows 10 and run
run-measure.bat. No Inventor, browser or interactive window is needed. Return
result-mshtml-open-control-x86_64.txt, result-mshtml-open-control-i686.txt,
result-mshtml-open-null-x86_64.txt, result-mshtml-open-null-i686.txt and
exitcodes.txt, including failed or partial processes. Only text output leaves
the VM. Raw logs will not be committed.

A subsequent Wine test must use these observations, preferably beside the
existing dom.c open tests with a known loaded document. Wine runtime testing
needs Gecko and an enabled mshtml backend; a skipped/disabled backend does
not prove this fix. No L3-only classification is asserted for an API with a
native Windows counterpart.

## Preparation advisory

A second-model read-only advisory read both sources and the relevant public
Wine mshtml tests. It recommended observing parentWindow before open and
requiring a successful control before interpreting NULL-result success.
Those observations are included; ownership review found no clear stream or
COM leak. The API setup follows common public patterns, with an original
three-state/process-isolated fixture. This limited resemblance check cannot
prove originality or replace formal review or native execution.
