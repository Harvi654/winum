# L3: an owned shaped dialog under Xwayland

Original, AI-assisted (Codex), LGPL-2.1-or-later. `repro.c` uses only own
Win32 windows. Its default target is a hidden, owned, uncaptioned popup of
dialog class #32770. It sets an L-shaped region before showing it, matching
the patch's ARGB-visual eligibility checks. A green owner lies behind a red
160x160 popup. Every five seconds the region changes: missing bottom-right,
missing bottom-left, full rectangle (SetWindowRgn NULL), missing bottom-right
again. This exercises region replacement, clearing and restoration as well
as the initial hidden-window visual change.

Controls: `--ordinary` uses an own non-dialog class, which the patch must
not promote to ARGB. `--late-region` sets the first region only after
showing the window, excluded by the default hidden-only guard. There are
no layered, GL or Vulkan children here; this fixture does not validate
those excluded rendering paths. No CAD components are loaded.

## Why L3 only

SetWindowRgn exists on Windows, but Xwayland's bounding/input shapes,
compositor alpha and Wine's visual selection do not. A native Windows
conformance test cannot exercise the changed X11/compositor path. This
fixture is an L3 visual/input-region reproducer, not Windows L1 evidence.

## Run the CI-built pair on Arch / KDE Wayland

Use [the shared L3 runner](../l3-ci.md). CI builds one full production
series and three variants omitting exactly one winex11 patch each; no
local Wine compilation or system update is needed. From this checkout:

```bash
bash scripts/run-l3-local.sh RUN_ID
```

The runner downloads all artifacts with `gh` (GH_TOKEN unset), verifies
checksums, commit identity and exact omission lists, and runs this fixture
and its documented controls with throwaway prefixes. It also runs the other
two winex11 fixtures and the MEASURE bundle's NdrStubCall3 probes. Follow
its prompts for physical clicks and visual observations. Return the single
`l3-summary-RUN_ID.txt`; completion is not an automatic pass.

The earlier six-build local procedure is retained in
`docs/archive/l3-local-build-2026-11-09/` for history. It is superseded by
the four shared CI builds. Fixture source and interpretation are unchanged.

## Expected comparison and limits

On an affected Xwayland/compositor combination, the unpatched (or disabled)
target retains bounding-shape-only behavior: the missing quadrant can show
as an opaque rectangle. With the patch and its default hidden-first target,
the trace should confirm ARGB selection; outside-region pixels should be
transparent and excluded from input. Clearing the region must reset input
clipping and repaint opaque pixels, then reapplying must clip again.

The ordinary-class and default visible-first controls should receive no
ARGB-promotion trace from this patch. With opt-in value 2 the visible-first
dialog is eligible. Under a compositor that already handles X bounding
shape correctly, both builds may look correct: that is an unreproduced
visual symptom, not proof of the alpha path. On a real Xorg server the
Xwayland-specific path should stay inactive. Trace absence also requires
checking XWAYLAND-extension and ARGB-visual availability before concluding
that an eligible window failed.

2026-11-09: compiled both PE architectures with -Wall -Wextra -Werror.
No cloud Wine execution, paired compositor result or new consumer L4 is
claimed. The L3-only marker records the reason and available fixture;
runtime verification remains pending.

The fixture checks class/owner/style/160x160 bounds and visibility before
treating its target as valid; a 40-second watchdog bounds a stalled GUI.
Early close or failed setup is inconclusive (exit 2). Exit 0 means the
sequence completed, not that pixels or compositor hit testing passed.
A second-model source advisory found no recognizable copied code in this
bounded comparison, which cannot prove originality. Its eligibility and
timeout suggestions were applied before the final strict builds.

## After the test

The runner stops each matching private wineserver and removes its temporary
prefixes, runtimes and raw output, including on interruption. Only the
filtered summary remains. Application prefixes and installed Wine are not
changed. A timeout or absent required observer/fault evidence remains
inconclusive; use the interpretation above before recording L3 success.
