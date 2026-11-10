# Cross review: riched20 temporary text font v6 (2026-11-10)

Reviewer: Codex; candidate author: Claude. Candidate 4036bab, validation
record 684ab5d. Follow-up to the v5 review's P2 ownership and P3 cycle
findings. No proprietary source, shaders, binaries or symbols were read.

Read in full: v6 patch/README and v5-to-v6 diff, v5 review, topic README,
both follow-up MEASURE sources, both returned followup2 files, the
batch1-6 validation README, four native richole result files and exitcodes.
Revisited the pinned Wine 11.20 7298a818b79446bae153b8d5aa27235ab3703bf9
font cache/refresh, Reset, property setter/getter, GetName, name-allocation,
construction, duplicate and Release paths. Checked SCF v5's flag
initialization/producer, textfont consumer and their actual branch order.
The unchanged older class-control tests retain the earlier full review;
unrelated TOM methods, SCF boundary tests and old suite failure output
were skimmed. No new runtime execution was performed for this review.

## Findings

- **V5 P2 resolved for the refresh paths introduced by this patch.**
  Both tomApplyTmp entry and its tomApplyNow completion now call
  textfont_refresh_range_props. It frees the owned cached-name BSTR,
  clears the slot, then allocates/reloads through the existing cache
  helper. The font already acquired its initial cache in create_textfont;
  the new helper is not called on the malloc-allocated, uninitialized
  name slot. GetName returns a separate allocation, and duplicates own
  their copied name, so these callers do not retain the freed pointer.
  The destructor frees the final current name. Both Reset paths check the
  live range first; detached fonts do not enter them. No new double-free
  or uninitialized free was found. This is source ownership reasoning,
  not a measured allocation/leak test. Vanilla's separate tomCacheParms
  overwrite remains unchanged and is explicitly outside this fix; this
  is not a claim that every font-cache allocation path is repaired.
- **The new ordinary repeated-cycle test has native support.** All 28
  FONT, 28 TEXT and 26 RESET observations per architecture were checked
  against the reported class/sequence, including HRESULTs and color/bold
  values; outputs are identical after bitness normalization. For msftedit
  all four observed sequences (same, new_font, bold_first, twice_tmp)
  retain red/bold in font and text. The new s=2 test exercises the same-
  object sequence with reads between operations, as ordinary assertions.
  Its exact query history, including additional text checks, also passes
  the returned native v6 test. It is not hidden by the older cycle todo.
  The other three followup2 probes remain measurement coverage rather
  than three additional Wine conformance sequences.
- **P3 remains open, now with more precise scope.** In the earlier
  tmp_again sequence, with no reads during the second temporary cycle,
  native Windows finishes not-bold; v6 still drops the change. That
  single final bold assertion retains its narrow todo. The newer reads-
  between-steps sequence finishes bold on Windows and v6. These are
  different call histories: the new result does not invalidate the old
  one or identify which getter, selection operation or other history
  causes the difference. The topic records the three-round stop and
  does not claim complete repeated-cycle compatibility. Preserve this
  limit for Inventor's repeated spell-check L4; no further measurement
  or guessed implementation is requested by this review.

No new P1/P2 defect found in the bounded v5-to-v6 delta. CacheParms/
TrackParms-before-temporary combinations remain outside the new default-
state test evidence, as disclosed in the prior review. The ordinary
RichEdit20W rejection/direct-setter controls, alternating class checks,
initial apply_tmp state, wave-underline and detached-font limitations are
unchanged. No todo was broadened to hide an ApplyNow regression.

## Dependency and validation

SCF v5 initializes editor->msftedit to FALSE in the common ME_MakeEditor
constructor and only sets it TRUE for the RichEdit50W window class in
create_windowed_editor. The textfont Reset consumer still uses that flag.
SCF precedes textfont in the actual series (file lines 23/24); both candidate
patches are byte-identical to their copies in test-run/batch1-6 at
9262481caf3f189e9c5849a4a458206c146a7336. They must be promoted together
in that order, not textfont with the old production flag producer.

All 62 actual branch entries independently apply at --fuzz=0 to the
pinned Wine tree. Native batch1-6 files confirm on both x86_64/i386:
msftedit:richole 182 checks and riched20:richole 183279, zero failures,
todos, flaky checks or skips, and exit 0. Those are v6's exact tests;
the earlier 150-check v5 pass and the older chat attachments are not
substitutes. Source applicability and native output inspection are the
checks performed here; no fresh full build or Wine runtime pass is claimed.

[Paired run 42772681118](https://github.com/Harvi654/winum/actions/runs/42772681118)
finishes with compare failure. The maintainer's committed batch1-6
assessment attributes its six regressions to production setrows v1,
mousemove v1, urlmon v1/tests-first and ntdll threadpool timing, with no
new batch-1 regression. Original full paired JSON was not independently
downloaded/reclassified here; do not call the entire run green.

Verdict: **the ownership P2 is answered; approve the bounded v6 revision**.
The unread repeated-cycle P3 and repeated-use Inventor L4 remain open;
v6 is not a full repeated-cycle implementation. Production patch,
patches/series and test marker remain unchanged by this review.
