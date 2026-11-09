# Cross review: msxml3 parseerror filepos v2 (2026-11-09)

Reviewer: Codex; v2 author: Claude. Reviewed at `c27a789`.

Read in full: candidate patch/README, production v1 patch, MEASURE source,
investigation README and both Windows result files. Read the relevant Wine
11.20 (`7298a818b79446bae153b8d5aa27235ab3703bf9`) getter implementations,
create_parse_error, test_parseerror and the create_document macros/object
selection. The rest of parseerror.c/domdoc.c and the complete domdoc suite
were not audited. No proprietary source, symbols or binaries were inspected.

## Implementation and native comparison

The getter now rejects NULL with E_INVALIDARG, writes the object's filepos,
and returns S_FALSE when code is zero, S_OK otherwise. create_parse_error
uses calloc and sets code/reason; it does not populate filepos. Consequently
new and successfully parsed documents return S_FALSE/0; empty-input errors
return S_OK/0. These agree with the supplied Windows observations. Returning
0 for malformed input does not agree with native positions 8 and 12; the
candidate explicitly defers position tracking and marks its position-8
assertion todo_wine. It is a partial getter implementation, not complete
native position reporting.

Compared with v1, only the implementation's return-code decision changes;
the NULL check and stored-value assignment remain. Test expectations for
the original no-error case change from S_OK to the measured S_FALSE, and
empty/malformed input cases are added. This corrects the old test rather
than relaxing an assertion to agree with Wine.

## Findings

- **P1: none identified in the bounded getter change.** The branch on code
  correctly distinguishes the measured no-error/error states. Nonzero
  malformed offsets remain an acknowledged missing feature.
- **P2: the test's class does not match the new MEASURE oracle.**
  create_document expands to CLSID_DOMDocument2. MEASURE explicitly creates
  DOMDocument30 and DOMDocument60. The earlier native L1 failure establishes
  the no-error HRESULT for the existing test, but the newly asserted malformed
  offset 8 has not been measured for this exact class in the supplied output.
  Use the measured class via create_document_version(30, ...) or measure the
  legacy class before treating every added expectation as native-validated.
  Sharing msxml3's implementation is not evidence that native class contracts
  are identical. Native L1 of the revised assertions remains required.
- **P2: coverage is smaller than the supplied state matrix.** There is no
  successful load after an error to test the S_FALSE/0 reset, no third-line
  malformed case (native filepos 12), and NULL is only checked on the initial
  error object. The three currently tested states cannot establish all rows
  of MEASURE or both measured class versions. Extend the test with these
  measured cases; keep nonzero positions todo_wine until they are implemented.
- **P3: clarify the other-getter description.** Wine's get_errorCode already
  returns S_FALSE when code is zero and get_reason returns S_FALSE for no
  reason; get_line/get_linepos still return S_OK. The investigation's broad
  wording about getters should not imply that all were still wrong. Changing
  those other getters is outside this candidate.

The retained todo_wine around the IXMLDOMParseError2 interface check predates
v2 and is separate from filepos. The added position-8 todo corresponds to a
real remaining Wine difference; no todo masks the getter's corrected HRESULT,
NULL contract or empty-input result. BSTRs and obtained error objects are
released in the new normal paths.

## Checks and verdict

Replaced v1 with the candidate at its existing series position and applied
the entire production series to a separate Wine 11.20 tag tree with
`--fuzz=0`; all patches applied, including importnode before it and subsequent
msxml3 changes. No production patch or series entry was edited.

Vanilla's E_NOTIMPL necessarily fails the new getter assertions; v1 fails the
no-error S_FALSE assertion; v2 satisfies those source-level checks and leaves
the malformed position as an expected todo. This review did not execute Wine,
Windows, a build or paired L2, and does not claim an observed test pass.

Verdict: the bounded implementation correction is supported by the Windows
data. Revise the class alignment and state coverage before calling the new
test fully derived from that matrix. Native L1, paired msxml3 L2 and the
consumer's L4 remain prerequisites for promotion. Keep the candidate outside
production during that validation.

## Revision check: 54894ed (2026-11-09)

Read the complete revision diff and applied the revised candidate to clean
Wine 11.20 parseerror.c/domdoc.c with `--fuzz=0`. The measured 30/60 classes,
fresh document per load, five states, per-state NULL checks and getter wording
address the original class/matrix points. The implementation is unchanged.
However, two new **P1 test blockers** prevent approval:

- The calls were inserted into test_put_dataType, around line 6656, but
  test_parseerror_filepos is first declared/defined around line 12966. There
  is no forward declaration: the implicit external declaration conflicts
  with the later static definition. Move the calls into test_parseerror
  after the helper definition (or provide a proper declaration).
- Three unrelated `<a/>` inputs changed in test_save, test_get_xml and
  test_prohibitdtd. In test_get_xml the existing assertions still require
  `<a/>`, so its newly loaded `<a><b/></a>` necessarily contradicts them.
  Restore those unrelated inputs; keep the new XML in the new case table.

The explicit successful-load case now matches the supplied fresh-document
measurement. Same-object error-to-success reset coverage is still separate.
No build/runtime pass is claimed. Verdict: original points addressed in
intent, but the revised test needs the two corrections above before L1/L2.

## Current revision review: 9bb1703 / 54cfb6a / 0955db1 (2026-11-09)

Reviewer: Codex; candidate author: Claude. Reviewed the candidate on main at
6c87a74. The requested a1c2d0e object is not in the fetched repository after
git pull, so this review does not claim that exact revision. The available
history contains the empty-string implementation fix in 9bb1703, call
placement in 54cfb6a and removal of unrelated test edits in 0955db1. The
actual reviewed patch SHA-256 is
9d011211c31a96e8124bccc0ea5dc5958d8c9967c873b33ec580a24322a189d4.

Read in full for this pass: the current candidate patch/README, production
v1 patch, own MEASURE source, investigation README, both complete Windows
result files, this earlier review and the complete 9bb1703/54cfb6a/0955db1
candidate diffs. In pinned Wine 11.20, read all of parseerror.c and the
complete domdoc_loadXML, domdoc_get_parseError, attach_doc_node,
test_parseerror, _create_object, is_clsid_supported and BSTR-pool helpers;
also the support table and the adjacent existing NULL-load tests. Other
large domdoc.c/test functions were sampled only for changed-call context,
not audited in full. No native implementation, symbols or vendor binary
contents were inspected.

### Native comparison and scope

The current table uses a fresh DOMDocument30/60 for every input, matching
the actual measurement. For both classes and both architectures:

| State | Measured filepos HRESULT / value | Current candidate |
| --- | --- | --- |
| New document | S_FALSE / 0 | ordinary checks; source agrees |
| loadXML("") | S_OK / 0 | ordinary checks; empty-string fix supplies nonzero code |
| `<a><b></a>` | S_OK / 8 | ordinary HRESULT; value 8 is todo_wine, stored value remains 0 |
| `<a>\n  <b>\n</a>` | S_OK / 12 | ordinary HRESULT; value 12 is todo_wine, stored value remains 0 |
| Fresh document, valid XML | S_FALSE / 0 | ordinary checks; source agrees |
| NULL filepos output in every state | E_INVALIDARG | ordinary checks; source agrees |

loadXML's HRESULT and VARIANT_BOOL are checked too. Their initial boolean
is deliberately opposite to the expected result, detecting an unwritten
output. The four nonzero-position todos across the two classes represent
real missing position tracking. No todo masks the new empty-input HRESULT,
NULL contract or no-error result. The pre-existing legacy ParseError2
interface todo is unrelated and unchanged.

The implementation fix is necessary: the old empty-input load branch never
assigned doc->error, so a fresh document still appeared error-free. The new
if(data) assignment records E_SAX_MISSINGROOT, whose open-source enum value
is exactly the measured 0xc00ce558. A NULL string still takes its unchanged
path. The existing nonempty path already assigns the parse result to
doc->error, including S_OK for valid XML. create_parse_error copies the
code into a zero-initialized object, so the getter's S_OK/S_FALSE decision
and stored position follow as described. No resource API or ownership
transfer is added; the existing generic reason allocation now also runs
for empty input. Documents, obtained error objects and pooled argument
BSTRs are released on normal test paths.

This remains a partial filepos implementation. It does not populate the
nonzero malformed positions or establish parity for other getters' values.
get_line/get_linepos still return S_OK without an error, and the reason is
the existing generic "error" rather than the measured 45/47-character
native reasons. Same-object error-to-success reset, NULL-string error-state
semantics, embedded-NUL input and other class versions are outside the new
native matrix; none should acquire guessed expectations from this review.

### Current findings

- **P1: none found in the current bounded change.** Both earlier test
  blockers are closed. The helper is defined before test_parseerror, and
  its two class calls are inside that function. The candidate leaves
  test_put_dataType, test_save, test_get_xml and test_prohibitdtd unchanged;
  no unrelated `<a/>` replacement remains.
- **P2: directly protect the new empty-input error code.** The candidate
  now changes the observable get_errorCode result to S_OK/0xc00ce558, but
  its regression only checks filepos. Any nonzero replacement code, such
  as generic E_FAIL, would satisfy every new empty-filepos check despite
  disagreeing with the supplied Windows output. Add an ordinary
  get_errorCode HRESULT/value assertion for the empty-string case on both
  measured classes before promotion. The current implementation constant
  is correct; this finding is a missing regression guard for its new
  public behavior, not a demonstrated runtime failure. No new MEASURE is
  needed for those already recorded values. Do not blanket-todo that
  exact empty-input contract or import unmeasured invalid-input cases.
- **P3: remaining scope wording is stale.** The candidate README still says
  the change affects only the no-error case that SOLIDWORKS does not hit;
  9bb1703 specifically repairs its empty-string call. The header's earlier
  "limited to get_filepos" wording also predates the doc->error change.
  The investigation README still groups all getters as returning S_OK
  without an error, although get_errorCode/get_reason already return
  S_FALSE there. Update those descriptions without broadening this patch
  to the other getters or implying complete position/reason support.

### Checks and verdict for the current file

Replaced v1 at its existing series position and applied all 62 production
entries with --fuzz=0 to a fresh pinned Wine 11.20 tree: importnode precedes
the candidate and schema-cache-namespace follows it. Compared the named
unrelated test bodies before/after the candidate and checked helper/call
placement. Built the touched domdoc.c and parseerror.c DLL objects and the
domdoc.c test object for x86_64/i386. All six PE compilation targets pass
without compiler warnings in this focused build. Configuration used
--enable-archs=x86_64,i386 --without-x --without-wayland --without-freetype.
Source archive SHA-256:
9f516d63e9f814f4dad70ce0a1943211b8a6236008ac3025cf58004091c661db.
This is object compilation, not full DLL/test linking, graphical L0, native
L1 or Wine runtime execution. No candidate/production code or series entry
was changed by this review.

The commit message for 0955db1 reports that the new checks passed in
candidates-4 while unrelated XML edits caused other failures. That is
author-reported context; this review did not fetch or rerun its Actions
logs and does not turn it into a verified current paired L2 result.

Verdict: the measured bounded implementation is supported, and the old P1
placement/unrelated-edit issues are resolved. Add the P2 empty-error-code
guard and correct the P3 descriptions before promotion. Native L1 of the
final assertions and paired msxml3 L2 remain open. SOLIDWORKS L4 is deferred
under the current AGENTS.md policy for that inactive consumer; it is not
claimed performed. Keep the candidate outside production during these
remaining test/review gates, and retain the acknowledged position todos.
