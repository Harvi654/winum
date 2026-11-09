# Cross review: comctl32 toolbar padding height v3 (2026-11-09)

Reviewer: Codex; candidate author: Claude. Revision: `687afe6`, read from
main `fcdf19d`. This is a review of v3, not approval of a later revision.

Read in full: v3 patch/README, v2 patch/README and review, original patch,
own measurement source and follow-up source, topic README and followup2
list. Revisited the complete Wine 11.20 (`7298a818b79446bae153b8d5aa27235ab3703bf9`)
MeasureButton, CalcToolbar, SetPadding, SetBitmapSize, SetButtonSize and
creation defaults, plus the new test and its v5/v6 insertion context.
Parsed the complete native measurement files and checked the 384 rows
per architecture matching the new test matrix; repeated textual output
was not read line by line. The rest of toolbar.c/tests was skimmed.
No proprietary code or native implementation internals were inspected.

## Findings

- **P1: the original no-string regression is resolved in source.**
  No-text height keeps DEFPAD_CY, and same-size SetBitmapSize returns
  before recalculation. The exact previously missing operation now has
  ordinary assertions for every content/order/padding combination.
  The native rows confirm unchanged button size across that operation
  and the subsequent 16+padding minimum on both architectures.
- **P2: the -1,-1 reset after a nondefault padding has no oracle yet.**
  The new SetPadding branch resets both dimensions to defaults for
  0xffffffff regardless of the old padding. Existing Windows output only
  exercised that input from default state, which cannot distinguish
  "reset to default" from "leave old value unchanged". The follow-up
  program correctly measures this distinction, but its Windows output
  is not present. Do not treat the new branch's nondefault-state behavior
  as established until that result arrives; adjust or limit it if needed.
  Also retain the pending changed-bitmap-size/one-axis-sentinel neighbor
  measurements before claiming compatibility beyond the measured matrix.
  This is an evidence/scope gate, not a claim that a particular unknown
  Windows result has already disproved the branch.
- **P3: correct the follow-up list path in the README.** It names
  batch1-measure/followup.list, whereas the two new follow-up programs
  are in followup2.list. The new test/header otherwise address the old
  P3: they describe the text-dependent calculation rather than implying
  CalcToolbar only reaches it with an actual image list.

## Tests and preservation of compatible behavior

All asserted values have native observations: default-relative string
sizes avoid the VM's font metric, no-string/default controls stay 23x22,
before/after padding order is explicit, larger padding readback is no
longer clamped, and same-size bitmap changes preserve the current size.
All new checks are ordinary; no todo hides the repaired path.

The text-dependent height and same-size return fix the measured v2
regressions rather than modifying their expectations. Non-list sizes in
the tested matrix agree with the original native rows. List height remains
an admitted independent Wine difference. The author's 7200-row local
vanilla/v2/v3 comparison is useful evidence but was not rerun by this
reviewer; a finite matrix is not proof about the newly exposed sentinel
state above or every toolbar operation.

## Checks and verdict

All 62 entries applied with --fuzz=0 in a separate pinned Wine 11.20 tree
with v3 substituted at entry 10. No production patch or series was changed.
This was an apply/source check, not a new full L0 or Wine execution.

Run 42662094032 on test-run/batch1-3 was still shown as in progress on
the public Actions page when checked. No completed paired report was
retrieved. The maintainer is running that run's L1 bundle; exact v3 native
assertion results have not yet been returned. The old v2 L1 pass is not
reused as a v3 pass.

Verdict: the old P1/P2/P3 findings are addressed by the revision, but
**promotion remains conditional on the sentinel follow-up**, exact-body
L1, paired L2 and the Inventor palette smoke test. No new measured
regression was identified in the covered matrix. The P2 uncertainty must
not be silently replaced with an assumed Windows value.

## Follow-up: v4 (2026-11-10)

The returned sentinel/changed-bitmap measurement answers P2; v4 now keeps
unchanged axes and uses height padding on a changed bitmap without
reintroducing same-size recalculation. The list-path P3 is corrected.
See [the v4 review](cross-review-2026-11-10-comctl32-toolbar-padding-height-v4.md)
for the native row checks, residual scope and pending exact L1/L2 gates.
