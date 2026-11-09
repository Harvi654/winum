# Cross review: riched20 temporary text font v2 (2026-11-09)

Reviewer: Codex; candidate author: Claude. Snapshot: `3e7279d`.

Read in full: candidate patch/README, v1 patch and patch page, own MEASURE
source and topic README, new msftedit test, batch L1 README and both
msftedit richole L1 summaries. Parsed the two complete native result files,
which match apart from bitness, and inspected the per-class reset/getter/
text rows for the tested sequences and detached control. Read Wine 11.20
`7298a818b79446bae153b8d5aa27235ab3703bf9`'s complete TextFont_Reset,
set_textfont_prop, textfont_apply_range_props, create_textfont and caching
helpers, getter/setter dispatch and the test creation/insertion context.
The rest of TOM and the existing suites were skimmed. No proprietary
internals were used.

## Findings

- **P1: the patch changes an already-compatible RichEdit20W setter path.**
  Native RichEdit20W rejects Reset(tomApplyTmp) with E_INVALIDARG; following
  color/bold setters apply directly (blue, not bold). Native RICHEDIT50W
  accepts it and retains the old text values. Vanilla's reset HRESULT is
  already wrong for RichEdit20W, but its following ordinary color/bold
  setters do apply directly, like Windows. The shared candidate instead
  sets set_cache_enabled/apply_tmp for both classes and drops those
  changes, retaining red/bold. This introduces a measured text-formatting
  regression for RichEdit20W. The README admits the distinction, but a
  shared implementation is not an exception to AGENTS.md's preservation
  rule. Preserve class identity through the editor/TOM path and restrict
  temporary formatting to the class for which it was measured. This
  problem is inherited from v1, not introduced solely by v2's ApplyLater
  adjustment.
- **P2: the Wine test omits the measured class control.** Only
  msftedit:richole gets new assertions. Add ordinary RichEdit20W assertions
  for the measured E_INVALIDARG and subsequent direct color/bold changes;
  do not mark the introduced setter regression todo_wine. A passing
  msftedit test, or riched20's old tests lacking this sequence, cannot
  establish the shared change preserves both contracts.
- **P3: qualify the claim that temporary cache contents are dropped.**
  ApplyNow clears apply_tmp but does not clear/reload props. A second
  ApplyNow reaches textfont_apply_range_props with the temporary values
  still cached. The supplied measurement ends after the first ApplyNow,
  so it does not establish Windows behavior for repeated resets or reuse
  of that font after a temporary cycle. Measure that neighboring sequence
  before claiming complete temporary-state lifetime handling; this review
  does not invent expected results for it.

## Assertions and bounded v2 change

For the measured RICHEDIT50W subset the ordinary S_OK, color/bold/getter
checks, stale-purple case and untouched green neighbors match Windows.
Keeping apply_tmp through ApplyLater corrects v1's write-through for the
measured tmp -> setters -> later -> now sequence. Non-temporary
ApplyLater retains its existing code. apply_tmp is initialized for new
and duplicated objects. No unrelated TOM method is changed.

The two todos are correctly limited to existing differences not fixed
here: SetUnderline(tomWave) and detached Reset(tomApplyTmp), both S_OK on
native msftedit. The patch does not implement visual temporary underlines.
These todos do not hide the color/bold target. They also do not address
the omitted RichEdit20W regression above.

## Checks and verdict

Replaced v1 with v2 in the separate full-series pinned Wine 11.20 tree;
all five reviewed candidates coexist with the remaining production series
using `--fuzz=0`. No production file or series entry was edited. No new
build or Wine runtime run is claimed by this source review.

Both native msftedit:richole L1 results from run 42642179431's bundle
report 121 tests, zero failures/todos/skips, exit 0. The README's local
vanilla 13 failures versus v2 zero is author-provided evidence for the
msftedit subset. The public batch1-1 Actions page (`56bb9ee`) was still
in progress at review time, 9/10 matrix jobs complete and compare waiting;
environment policy blocks API retrieval. No paired L2 report was
independently retrieved.

Verdict: **request changes before promotion**. Separate the class behavior
and test both measured contracts, then rerun native L1 and paired
riched20/msftedit L2 for the revision. Inventor's symbol/color/spell-check
smoke result remains separate; success with the old msftedit-style
workaround cannot justify changing RichEdit20W's measured direct setters.

## Follow-up: v3 in 687afe6 (2026-11-09)

P1 is addressed in design by rejecting tomApplyTmp outside RichEdit50W,
but not fully resolved in the pair: SCF v4's new class bit is uninitialized
for nonmatching/windowless editors. P2 now has ordinary direct-setter
controls, but their factory creates RichEdit20A despite the W-class
provenance; align that control with the measured class. P3's overbroad
lifetime wording is resolved; repeated-cycle behavior remains pending the
new MEASURE output. The separate v3 and SCF v4 reviews contain the details.
No approval or revised native L1/L2 pass is implied by this follow-up.
