# Cross review: riched20 temporary text font v3 (2026-11-09)

Reviewer: Codex; candidate author: Claude. Candidate revision `687afe6`,
read from main `fcdf19d`.

Read in full: v3 patch/README, v2 patch/review, own original and follow-up
MEASURE sources, investigation README and the native per-class reset,
getter and text rows. Revisited the complete Wine 11.20
`7298a818b79446bae153b8d5aa27235ab3703bf9` TextFont_Reset, property cache/
apply/creation paths and the test factories/insertion context. Read the
SCF v4 discriminator producer, common editor constructor and text-service
construction, and checked main/run series ordering. Unrelated TOM methods
and existing suite contents were skimmed. Only open source and our own
public-API observations were used.

## Findings

- **P1: the dependency's class discriminator is not initialized.**
  The new conditional would correctly reject tomApplyTmp for RichEdit20W
  if msftedit were reliably false. SCF v4 adds that bit to an editor
  allocated by malloc but only assigns TRUE in the RichEdit50W window
  path. It never initializes FALSE in ME_MakeEditor, including for
  windowless services. Thus this pair can still discard the direct
  RichEdit20 formatting the old review required preserving, depending on
  allocation contents. Fix the producer's common initialization before
  accepting the dependent textfont candidate. See the SCF v4 review for
  the concrete source path and lifetime regression control.
- **P2: the new test does not create the class named in its provenance.**
  test_textfont_applytmp_riched20 calls create_interfaces, which calls
  new_richedit, whose class is RICHEDIT_CLASS20A. The native MEASURE and
  new comment/README specifically say RichEdit20W. Create that W class
  explicitly for this control without changing all existing suite
  factories. Alternatively measure/identify A separately. A future exact
  native L1 pass can establish that actual A test too, but the currently
  returned W measurement must not be silently described as an A run.
- **P3: temporary-cache lifetime remains an explicit open measurement.**
  The new header correctly limits its claim to one cycle. A second
  ApplyNow still reaches cached temporary props after apply_tmp is cleared;
  the new follow-up measures that and later changes. No returned oracle
  for those sequences exists yet. This closes the old wording issue,
  not the unmeasured behavior gap.

## Expectations, todos and unchanged paths

For the classes actually measured, S_OK for RICHEDIT50W and E_INVALIDARG
for RichEdit20W agree with both native outputs. Blue/not-bold immediate
setters in the RichEdit20 control are ordinary assertions; msftedit's
red/bold, stale-purple and untouched green neighbors remain ordinary.
The new RichEdit20 todos concern the existing ApplyNow stale write,
unsupported tomWave and detached Reset return, all observed native
differences left outside this change. They do not excuse direct setter
regressions. Existing live/released range checking precedes the new class
lookup, so no new NULL dereference was identified on that path.

Relative to v2, implementation changes are just the class rejection;
ApplyLater's retention of temporary state and ApplyNow handling are
unchanged. apply_tmp itself is explicitly initialized in font creation.
The separate editor class bit is the missing initialization above.

## Dependency, checks and verdict

SCF v4 at series entry 23 introduces the bit; textfont v3 at entry 24
consumes it. Both main's existing order and test-run/batch1-3's order are
correct for substituting these versions together. All three reviewed
candidate patch files are byte-identical to their production-position
copies in that run's `805372e` tree. Textfont v3 is not standalone against
production SCF v1; its README and header explicitly disclose this.

Replaced v1 with v3 in the full-series pinned Wine tree with SCF v4 and
toolbar v3; --fuzz=0 application passed. The richole test insertion has
a line offset when the separate caret test is absent, not fuzzy matching.
No new full L0, Windows L1 or Wine runtime pass is claimed. Run 42662094032
was in progress on the public Actions page; revised native L1 output and
the completed paired report have not been received/retrieved.

Verdict: **request changes for the uninitialized dependency and exact
class control**. The proposed split answers the original design problem,
but is not yet reliable as implemented. Retest the corrected pair on
Windows and in paired riched20/msftedit L2; keep temporary-cycle follow-up
and applicable Inventor validation separate.
