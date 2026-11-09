# Cross review: riched20 SCF_WORD v4 (2026-11-09)

Reviewer: Codex; candidate author: Claude. Candidate revision `687afe6`,
read from main `fcdf19d`.

Read in full: candidate patch/README, production v1 patch/page, v3 README,
both own measurement sources and investigation README. Parsed all native
results from windows-2026-11-09 and windows-2026-11-09-boundary, checking
each new table's format/return/insertion-format values against both
architectures. Read the complete Wine 11.20
`7298a818b79446bae153b8d5aa27235ab3703bf9` editor constructor,
handle_EM_SETCHARFORMAT, cursor lookup, create_windowed_editor and
create_text_services; inspected the editor structure, selection/word
helpers, class registration, msftedit forwarding and both test insertion
contexts. Unrelated RichEdit functions/suites were skimmed. No vendor
implementation internals were used.

## Findings

- **P1: the new class bit is uninitialized for non-msftedit editors.**
  ME_MakeEditor allocates with malloc, not calloc, and initializes the
  existing fields individually. v4 adds msftedit to ME_TextEditor but
  never assigns FALSE in that constructor. create_windowed_editor only
  assigns TRUE on a positive RichEdit50W class match. Thus RichEdit20W,
  riched32 and windowless services can read an indeterminate bit, including
  one retained when a previous RichEdit50W editor's allocation is reused.
  They can incorrectly take the new msftedit word rule. Initialize the
  bit to FALSE in the common constructor before exposing the editor;
  retain the positive class assignment for windowed RichEdit50W. Merely
  assigning FALSE in the nonmatching window-class branch would leave
  windowless services uninitialized. This also blocks the dependent
  textfont v3, whose Reset reads this same bit.
- **P2: the tests do not deliberately alternate class lifetimes.** The
  native-derived RichEdit20W controls are valuable, but they run in a
  different unit/process from the new msftedit tests. After fixing P1,
  add a same-process RichEdit50W -> RichEdit20W creation/destruction
  regression sequence using the measured distinguishing word-end case.
  Repeat it to exercise allocation reuse; keep both class expectations
  ordinary and rerun native L1 for the actual test. A clean allocator's
  initial zero pages or a passing single local run cannot initialize a C
  field that the source never writes.
- **P3: the classification scan repeatedly starts at the document origin.**
  Each word_class_at calls cursor_from_char_ofs, sometimes several times
  for an apostrophe, while extending a range character by character.
  This can become expensive with many runs/paragraphs. No performance
  regression is established by the short native texts; consider advancing
  a cursor locally if longer-text profiling shows it matters. Do not
  broaden the claimed script/word-rule compatibility on that basis.

## Native values, todos and preservation

All 106 expected format strings per class match the corresponding native
rows on both architectures: 50 word/selection rows, six face rows and 50
boundary rows. Return 1 and the asserted bold insertion-point state match
the recorded values. msftedit has zero todo rows; riched20 has 81,
representing the explicitly retained word-movement gaps. Those todos do
not hide the nonempty-selection fix. The test does not claim coverage of
every measured flag combination or typed-character sequence merely by
checking the same insertion points.

For an explicitly false class bit the empty-selection branch is vanilla's
word movement verbatim. Nonempty selection expansion is removed for both
classes, supported by the measured selections. The paragraph-mark case
uses an end-paragraph run rather than classifying a mark as a normal word.
SCF_ALL, plain-text rejection and flags without SCF_SELECTION keep their
existing dispatch. Once initialized, the bit is immutable instance data;
case-insensitive class comparison is appropriate. Windowless behavior is
intended to remain the old rule, but that intention is not implemented
reliably until P1 is fixed.

## Dependency, checks and verdict

The common series already orders SCF at entry 23 before textfont at 24.
Substitution with v4 therefore produces the field that textfont v3 needs;
production SCF v1 alone does not. The private IUnknown returned by
create_text_services with NULL outer is the inner interface used by the
new CONTAINING_RECORD, so no additional COM-layout error was identified.

Applied v4 instead of v1 in the full-series pinned Wine 11.20 tree,
alongside toolbar v3, with --fuzz=0. This is a source/apply check, not a
new full L0, native L1 or Wine runtime run. The public page for L2 run
42662094032 (test-run/batch1-3, branch head `805372e`) was in progress;
no completed paired report or returned revised L1 bundle results were
available. Author-provided local match counts were not independently
rerun.

Verdict: **request changes for the uninitialized discriminator** before
promotion or interpreting the paired class tests as reliable. The table
oracles themselves match the supplied Windows output. Fix initialization,
add the lifetime control and validate exact-body L1/L2; do not replace the
measured RichEdit20W values with msftedit values to make the test pass.

## Follow-up: v5 (2026-11-09)

Reviewed revision 4d5db1e from main 99e45cd. Read the complete v5 patch
and README, its diff from v4, and the complete common constructor and
text-service/window creation paths in the pinned Wine 11.20 source above.
The unchanged tables/word implementation retain the preceding review;
unrelated RichEdit code was not reread in full.

- **P1 resolved.** ME_MakeEditor now explicitly sets msftedit to FALSE
  before returning the editor to create_text_services. This covers the
  windowless path too; create_windowed_editor still sets TRUE only for a
  RichEdit50W class. The allocation and initialization paths no longer
  rely on zero-filled or previously used heap memory.
- **P2 resolved in the test source.** test_scf_word_classes loads both
  DLLs and creates/destroys RichEdit50W then RichEdit20W three times in
  one process. Both format expectations and the return/insertion-format
  checks are ordinary assertions. At text 0, selection 2,2, the all-dots
  msftedit result and BBB-plus-dots riched20 result match all four native
  rows (two classes, x86_64/i686). No new expected value was inferred from
  Wine. Alternation exercises lifetimes but does not guarantee that the
  allocator reuses an address; the source initialization is the actual fix.
- **P3 remains open**, unchanged: repeated document-origin lookups have
  not been profiled on longer text. This revision does not broaden word
  classification or change any of the previously reviewed table/todo rows.

All 62 production series entries apply with --fuzz=0 in a fresh pinned
Wine tree with only SCF v5 substituted. Its patch is byte-identical to the
production-position copy in test-run/batch1-4 at 3e089b2, the source for
[L2 run 42698489281](https://github.com/Harvi654/winum/actions/runs/42698489281).
That run was in progress at this review; exact revised native L1 output
has not been returned. The author's forced-TRUE/local Wine runs were not
independently executed here. No new build or runtime pass is claimed.

Verdict: the former **P1/P2 are resolved**; no additional blocking issue
was found in the v5 delta. Promotion still requires the actual revised
L1/L2 and applicable consumer validation. Production patches/series is
unchanged.
