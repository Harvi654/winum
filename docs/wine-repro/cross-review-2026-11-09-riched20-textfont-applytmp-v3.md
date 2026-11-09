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

## Follow-up: v4 with SCF v5 (2026-11-09)

Reviewed revision 4d5db1e after the SCF v5 follow-up. Read the full v4
patch and README and its diff from v3, the existing test factory/release
helpers, the range-validity/reset and font-construction paths, and the
SCF producer's construction/initialization paths. Revisited the native
per-class Reset rows on both architectures and the measurement's actual
class creation. Unrelated TOM methods and suite contents were skimmed;
no vendor implementation internals were read.

- **P1 resolved by the explicit SCF v5 dependency.** The common editor
  constructor now initializes msftedit to FALSE for every editor, including
  windowless services. Only RichEdit50W window creation sets TRUE. V4's
  README and patch header require that version; production SCF v1 still
  does not supply the field. See the SCF review's v5 follow-up.
- **P2 resolved.** The new local create_class_document helper calls
  CreateWindowW with RichEdit20W for the direct-setter test, replacing the
  ANSI suite factory without changing other tests' factories. It obtains
  and releases the same IRichEditOle/ITextDocument ownership pair. The
  additional same-process loop creates/destroys RichEdit50W then
  RichEdit20W three times; its ordinary S_OK/E_INVALIDARG assertions match
  both native applytmp_now Reset rows. The existing blue/not-bold checks
  now exercise the actual measured W class, with no additional todo.
- **P3 remains open**, as disclosed: later changes/second ApplyNow on one
  font object await the temporary-cycle follow-up measurement. No output
  for that sequence was supplied by this revision.

The implementation hunks in dlls/riched20/richole.c and all msftedit test
hunks are byte-identical to v3. The changed tests do not broaden the
implementation, alter the existing narrow todos or replace Windows values.
Both candidate patches apply with --fuzz=0 among all 62 production entries
in a fresh pinned Wine 11.20 tree. The producer precedes its consumer at
physical series lines 23/24 (logical patch positions 16/17). Both patch
files match their production-position copies in test-run/batch1-4 at
3e089b2, source for
[L2 run 42698489281](https://github.com/Harvi654/winum/actions/runs/42698489281).

Verdict: the former **P1/P2 are resolved** when textfont v4 is paired with
SCF v5; no additional blocking issue was found in the revision. Exact
revised native L1, the completed paired L2 and applicable Inventor smoke
remain required. The author's local/forced-TRUE runs were not independently
executed here; this source review claims no new runtime or full build pass.
Production patches/series is unchanged.

## Follow-up: v5 (2026-11-10)

The new Windows output supports the second-ApplyNow correction and direct
setter control; the second temporary cycle remains an admitted narrow
todo. The new cache refresh also loses the old cached font-name BSTR.
See [the v5 review](cross-review-2026-11-10-riched20-textfont-applytmp-v5.md)
for the P2 ownership finding, partially answered P3 and pending L1/L2.
The earlier class/initialization P1/P2 fixes remain intact.
