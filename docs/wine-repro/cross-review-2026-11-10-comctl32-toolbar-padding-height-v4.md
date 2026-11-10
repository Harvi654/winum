# Cross review: comctl32 toolbar padding height v4 (2026-11-10)

Reviewer: Codex; candidate author: Claude. Revision 5fa1e14. Follow-up to
cross-review-2026-11-09-comctl32-toolbar-padding-height-v3.md.

Read in full: v4 patch/README, v3-to-v4 diff, the original and follow-up
MEASURE sources, topic README and followup2.list; pinned Wine 11.20
7298a818b79446bae153b8d5aa27235ab3703bf9 MeasureButton, CalcToolbar,
SetPadding and SetBitmapSize and the test insertion/activation context.
Parsed every PAD/BMP row of both returned follow-up files and compared
bitness-normalized output. The earlier v3 review covers the unchanged
original test matrix. Read the batch1-4 L1 README, toolbar summaries and
first i386 setup errors; its repetitive failure output and unrelated
Wine functions/tests were skimmed. No proprietary implementation read.

## Findings

- **P2 from v3 resolved.** All 24 PAD rows per architecture (12 per
  comctl32 version) retain each axis whose input is 0xffff, including
  after custom 3x4, 0x0 and 16x16. Returned old values equal the prior
  padding; 0xfffe remains an ordinary stored value. V4 removes v3's
  unconditional default reset and handles the axes independently, as
  measured. The new ordinary checks cover the custom/sentinel sequence.
- **The no-text height change is now supported by the follow-up.** All
  128 changed-bitmap rows per architecture agree with the new test's
  formulas: no strings means bitmap plus current padding; with strings,
  the asserted height delta accounts for bitmap height and whether the
  old size used default or custom padding. The other 32 BMP rows retain
  the same-size control. Both native architectures and v5/v6 agree.
  Consequently restoring padding height without text does not restore
  v2's known same-bitmap regression: the unchanged-size early return is
  retained byte-for-byte in behavior. The earlier default/no-text and
  padding-order controls remain ordinary assertions.
- **P3 documentation finding resolved.** The follow-up source/list
  references now identify the real followup2.list. The header explicitly
  distinguishes unchanged bitmap size from changed-size recalculation.
- **P3 scope limits remain disclosed, not newly introduced regressions.**
  TBSTYLE_LIST's height gap and the 12 follow-up string-width retention
  differences are unchanged. The new string test intentionally asserts
  height, not the unsupported retained width. Do not describe the patch
  as implementing every measured toolbar size behavior.

No new P1/P2 implementation issue was found in this revision. The native
follow-up closes the previous evidence gate rather than substituting an
assumed Windows value. The new sequential partial-axis checks compose
that observed per-axis rule; their exact test execution still needs L1.

## Checks and verdict

Applied all 62 production entries with toolbar v4 and toolbar-setrows v2
substituted in a fresh pinned Wine tree, --fuzz=0. The v4 patch is
byte-identical to its production-position copy in test-run/batch1-5 at
efabe26, source for [run 42718217301](https://github.com/Harvi654/winum/actions/runs/42718217301).
The public page was in progress when checked; no completed paired report
was retrieved. This is an application/source review, not a new full build
or runtime pass. Author-reported local match/failure counts were not rerun.

The returned batch1-4 native run exercised v3, not v4. Its x86_64 toolbar
unit passed; i386 failed to create a toolbar in the existing basic test
before the new padding checks, so it supplies no successful i386 L1 for
this revision. V4 requires the new bundle on both architectures regardless
of the proposed environment explanation for that earlier setup failure.

Verdict: **the v3 P2 gate is answered and the revision is acceptable in
source**, subject to exact v4 native L1, completed paired L2 and the
Inventor palette check. No production patch or patches/series was changed.

## V5 follow-up (2026-11-10)

[The v5 review](cross-review-2026-11-10-comctl32-toolbar-padding-height-v5.md)
confirms that only the test's initial HWND changes to NULL; implementation
and Windows assertions are unchanged. The old i386 cascade was a test
bug, not an environment cause. Exact v5 L1 passes 5309 checks on both
architectures. No new source finding; the earlier measured scope limits
and Inventor L4 remain. Batch1-6's known L2 failures are recorded with
the maintainer's attribution rather than described as a green whole run.
