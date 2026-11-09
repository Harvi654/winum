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
