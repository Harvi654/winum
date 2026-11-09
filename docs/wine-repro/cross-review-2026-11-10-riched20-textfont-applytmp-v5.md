# Cross review: riched20 temporary text font v5 (2026-11-10)

Reviewer: Codex; candidate author: Claude. Revision 5fa1e14. Follow-up to
the v4 section of cross-review-2026-11-09-riched20-textfont-applytmp-v3.md.

Read in full: v5 patch/README and diff from v4, both follow-up MEASURE
sources, returned follow-up text for both architectures, topic README,
followup2/3 lists, and batch1-4 L1 README. Revisited the pinned Wine 11.20
7298a818b79446bae153b8d5aa27235ab3703bf9 reset, cache, property getter/
setter, font construction/destruction and name-allocation paths; the
msftedit test factory and SCF v5 flag producer. The unchanged riched20
control retains the previous review. Unrelated TOM methods and repetitive
preexisting L1 suite errors were skimmed. No vendor internals were read.

## Findings

- **P2: the additional cache refresh loses an owned font-name BSTR.**
  TextFont_Reset's new apply_tmp -> tomApplyNow path calls
  textfont_cache_range_props. For FONT_NAME that helper calls
  textfont_getname_from_range with &font->props[FONT_NAME].str. A
  successful get_textfont_prop_for_pos allocates a new SysAllocString,
  and the helper overwrites that property pointer without freeing its
  previous BSTR. The font already has a cached name from create_textfont
  and the preceding tomApplyTmp refresh. TextFont_Release frees only the
  last stored name. V5 therefore adds another lost allocation whenever
  temporary state ends successfully; the older tomApplyTmp refresh
  already exposes the same helper ownership issue. Frequent spell-check
  cycles accumulate it. Safely free/replace the cached name in the refresh
  paths this patch adds, or avoid an unnecessary name allocation. If the
  shared helper is changed, initialize the name slot before its first
  invocation on the malloc-allocated font; blindly freeing that initial
  uninitialized slot would introduce another bug. This is a source-based
  ownership finding, not a runtime allocation measurement.
- **The former P3 is partly answered.** Native now_now retains red/bold
  after both ApplyNow calls on msftedit. Reloading current properties at
  the first temporary completion prevents a subsequent ApplyNow from
  replaying temporary cache values in this measured default state. The
  new second-ApplyNow assertions are ordinary, not hidden by a todo.
  Native set_after also supports the new ordinary direct-bold-change
  control after the first cycle.
- **P3: repeated temporary cycles remain an observed gap.** Native
  tmp_again ends red/not-bold after a second temporary cycle on the same
  object. V5 still drops that change. Its todo is limited to the final
  bold assertion; the color, returns and first-cycle checks stay ordinary.
  This is an admitted unresolved behavior, not a fully fixed cache
  lifetime. The first follow-up does not show precisely when that second
  change takes effect. The step-by-step/new-object/bold-first probes in
  measure-followup2.c have no returned Windows output yet. Multiple cycles
  matter to the stated spell-check use, so keep the L4 limit explicit.

No new measured functional regression in the default-state v4-to-v5 delta
was identified. The refresh helper respects get_cache_enabled for numeric
properties, so the new default-state evidence must not be generalized to
all CacheParms/TrackParms-before-temporary combinations without measuring
those sequences.

## Test evidence and unchanged controls

Both follow-up files are identical after bitness normalization. Checked
all four sequences for each class (16 TEXT and 18 FONT observations per
architecture), including unchanged red/bold after now_now and red/not-bold
for set_after/tmp_again on msftedit; recorded Reset/setter return values
support the new assertions. The added test expresses those final outcomes;
its exact query/operation history still requires its own native L1.

The riched20 W-class control, alternating-class return checks, class
rejection, initialization and detached-font handling are unchanged. The
SCF v5 dependency still initializes the discriminator in the common
constructor and precedes textfont in the series. The old RichEdit20W
stale write, unsupported wave underline and detached Reset differences
remain their narrow todos. No old todo was broadened to hide ApplyNow.

The newly returned batch1-4 native evidence is for textfont v4: msftedit
richole has 121 checks/zero failures on both architectures. The added
riched20 controls also pass; i386's two suite failures are the separately
reported existing clipboard-open failures. These results do not establish
a native pass for v5's new second-ApplyNow/repeated-cycle assertions.

## Checks and verdict

All 62 production entries apply with --fuzz=0 in a fresh pinned source
tree with toolbar v4, setrows v2, SCF v5 and textfont v5 substituted.
The consumer and SCF dependency are byte-identical to their respective
production-position copies in test-run/batch1-5 at efabe26, source for
[run 42718217301](https://github.com/Harvi654/winum/actions/runs/42718217301).
That paired run was in progress when checked. No new Wine/native runtime
or full build pass is claimed; author-reported local runs were not rerun.

Verdict: **request the cached-name ownership fix before promotion**.
The new native output supports the second-ApplyNow correction and direct
setter control, but does not close the repeated-cycle gap. Require exact
v5 native L1, completed paired L2 and repeated-use Inventor validation.
Production patches/series is unchanged.
