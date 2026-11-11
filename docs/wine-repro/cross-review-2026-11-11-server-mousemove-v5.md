# Cross review: server mouse move after show/hide v5 (2026-11-11)

Reviewer: Codex; candidate author: Claude. Reviewed candidate introduced
in `7ac9058`, after setrows review `9b96bc7`. No Windows, Wine or CAD
program was executed for this review. No proprietary internals inspected.

## Scope

Read in full: v5 patch/README, v4-to-v5 diff, previous v2 and v3 reviews
including the v4 follow-up, investigation README and MEASURE source/both
native outputs, all-3/all-4 validation READMEs, all four all-4 mousemove
outputs and both exitcode files, and the production patch status page.
The all-5 validation README was also read for the later ClipCursor check.
The older patches were compared in relevant hunks, not re-audited as a
whole. No new audit of the surrounding Wine server implementation is
claimed; the earlier pinned Wine 11.20 review remains the baseline.

Fetched all-4/all-5 read-only. Independently compared the complete v5
candidate bytes with `patches/server-mousemove-on-show-hide.patch` at
`ef76d8a` (all-4) and `f5b9397` (all-5): identical. Compared the server
hunks in v4/v5: byte-identical. The new revision changes test acceptance
bounds and explanatory text, not timer ownership, delay or routing.

## Evidence and changed interpretation

The original own Windows MEASURE records exactly one move in every
selected positive scenario, one after the ten-operation burst, and zero
for idle, repaint and cursor-outside controls, on both architectures.
Recipient expectations remain derived from those observations and the
fixture, including the newly created child and the already-hidden window
move. A 15-32 ms observation is not a timing assertion in the Wine test.

The old v3/v4 native failures established excess counts, not their cause.
At the previous review host mouse input was a hypothesis. The all-4
README now records the maintainer's explanation: moving the mouse to the
Linux host dock after starting the batch through Looking Glass affected
the first (x86_64) run. The maintainer reports the excess was traced to
that input. This is new operator evidence, not an inference from the old
summaries. Those files contain no input provenance/event timestamps to
independently establish it. Do not relabel six/seven moves as Windows'
quiet show/hide contract or claim this reviewer reproduced the cause.

All-4's actual v5 summaries show 40/34 checks on x86_64 runs 1/2 and
34/34 on i386, all zero failures, zero skipped, zero todos and exit 0.
The README attributes the extra six receiver assertions in run 1 to seven
moves in one scenario. The one-line output alone does not identify that
scenario. These are real v5 passes under its broader assertions; they do
not prove that every run was free of host input.

## Findings

- **P1: no new source-safety issue found in this test-only delta.** The
  receiver array remains bounded, scenarios and expected HWNDs unchanged,
  and only the hidden-window count remains todo_wine. Wrong-recipient
  assertions for stored events are still ordinary failures.
- **P2: no new blocker in the bounded count revision.** In an undisturbed
  fixture, missing synthesized moves still give zero and fail every
  ordinary positive check. Ten uncoalesced delivered moves fail the burst
  bound of 1..9; zero also fails. Idle, repaint and both outside-fixture
  controls remain exactly zero. Thus the test retains the regression
  signal for the original missing-show/hide-move bug and rejects one
  delivered move per operation in this burst. It no longer establishes
  exactly one delivered move or complete coalescing: 2..9 pass.
- **P3: positive single-scenario bounds are effectively unbounded.**
  The window procedure stops incrementing at 16, and the single-change
  maximum is also 16. Any number above 16 is accepted if the first 16
  receivers match; later incorrect receivers are not recorded either.
  This fits the requested presence check but must not be described as a
  finite actual-message upper bound. The burst still rejects saturation,
  because its maximum is 9; the zero controls still reject any stored move.
- **P3: host input can mask a missing synthesized event.** A physical move
  to the expected HWND can satisfy a positive assertion even with the fix
  absent. It could already substitute for the old exact-one assertion;
  v5 additionally accepts contaminated multi-move runs. Keep native and
  paired Wine runs undisturbed and retain the vanilla failure comparison.
  Broader bounds are test tolerance, not proof of causal delivery. The
  exact zero controls help expose interference but cannot exclude all of it.
- **P3: the v4 final-drain cap qualification remains open.** A final
  1000-message batch need not empty the queue. V5 does not change that
  pump, so its comment's absolute "never left" guarantee remains too
  strong. No new pump defect is introduced by the count revision.

The inherited server-global cross-desktop cancellation (P2), lack of a
client-message ordering barrier (P2), and timeout-allocation reference
cleanup/environment parsing limits (P3) are not fixed by v5. Their earlier
records remain applicable; this test covers one thread/desktop and a
short burst, not those server paths.

## Own checks, validation and verdict

A host-side check of the exact count predicate with the procedure's
`min(actual, 16)` storage behavior confirmed: 0 fails positive checks;
1 and 9 pass the burst; 10, 16, 17 and 100 fail it; all nonzero counts
fail zero controls; 17 and 100 pass the single-change count check. This
is an arithmetic/source check, not execution of user32 or the patch.
Recipient-loop inspection confirms every stored HWND is checked.

The candidate README reports local Wine 11.20 x86_64/Xvfb: vanilla
20 checks/13 failures, patched 33/zero, twice. These are author-reported
runs, not this reviewer's tests or committed per-scenario local output.
The all-4 record identifies run `42865253227` and bundle `ef76d8a`:
its paired L2 has one unrelated ntdll time difference, and the series-only
mousemove unit reports 33 checks/one todo/no failure. The later all-5
record reports the ClipCursor difference also reproduced on vanilla.
Neither L2 investigation was independently rerun here. Inventor's working
32-ms default is maintainer-reported L4, also recorded in all-3 build
`42835150574`; no other CAD L4 or L5 is established here.

Verdict: **approve v5's bounded test revision for the missing-event and
partial-coalescing regression checks, with the stated tolerance and
inherited server limitations.** Exact v5 native L1 is now present on both
architectures, unlike at the v4 follow-up. The original quiet Windows
oracle remains one move; the relaxed test does not establish a new native
contract of arbitrary positive counts. This review does not close the
inherited server P2 findings or change the production series, source,
release markers or another author's test-run branch.
