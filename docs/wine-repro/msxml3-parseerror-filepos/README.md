# msxml3 IXMLDOMParseError getters on Windows (MEASURE)

`measure.c` records, for `DOMDocument30` and `DOMDocument60`, the HRESULT
and value of `get_errorCode`, `get_line`, `get_linepos`, `get_filepos`
(also with NULL) and `get_reason` for a new document and after `loadXML` of
"", malformed XML, an error on the third line, and valid XML. Built by the
MEASURE bundle workflow.

## Windows 10 (VM on a Mac, 2026-11-08), x86_64 and i686 identical, msxml3 = msxml6

| State | loadXML | getters' HRESULT | errorCode | line / linepos / filepos | reason length |
| --- | --- | --- | --- | --- | --- |
| new document | - | **S_FALSE** (all) | 0 | 0 / 0 / 0 | 0 |
| `""` | S_FALSE | S_OK | 0xc00ce558 | 0 / 0 / 0 | 45 |
| `<a><b></a>` | S_FALSE | S_OK | 0xc00ce56d | 1 / 9 / 8 | 47 |
| `<a>\n  <b>\n</a>` | S_FALSE | S_OK | 0xc00ce56d | 3 / 3 / 12 | 47 |
| `<a><b/></a>` (valid) | S_OK | **S_FALSE** (all) | 0 | 0 / 0 / 0 | 0 |

`get_filepos(NULL)` is E_INVALIDARG in every state. `filepos` is the
0-based character offset of the offending character (`linepos` its 1-based
column). Results: `windows-2026-11-08/`.

Wine 11.20: the parse error object keeps only the code and the reason, so
line, linepos and filepos are always 0, and its getters return S_OK also
without an error. Candidate v2 of the patch:
`patches/candidates/msxml3-parseerror-filepos-v2/`.
