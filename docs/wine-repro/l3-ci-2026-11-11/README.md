# L3 comparison run, 2026-11-11

Shared L3 workflow ([l3-ci.md](../l3-ci.md)), run 42999903587, commit
bffbf79; collected by the maintainer on the desktop (KDE, Wayland session,
Xwayland) with `scripts/run-l3-local.sh`. Collection complete
(`COLLECTION complete=1`), every scenario exit 0, no API errors. Exit 0
means the sequence completed, not a pass; the interpretation below follows
each fixture's README. Only this summary is recorded, not the collected
output.

Runtimes: `full` = the CI build of the full production series (Wine 11.20);
`without-<patch>` = the same series without that one patch; `system` = the
distribution's Wine 11.21 (reference only).

## rpcrt4-ndrstubcall3 ([L3.md](../rpcrt4-ndrstubcall3/L3.md))

| Scenario | full, x86_64 | system Wine | full, i686 |
| --- | --- | --- | --- |
| both (`/protocol all` layout) | `available=1`, hr 0, value 42, server called once | `available=0` | `available=0` |
| ndr64-only | `available=1`, hr 0x80070725, server not called, `NDR64 syntax is not supported` logged | `available=0` | `available=0` |
| fallback (no entries) | `available=1`, hr 0, value 42, server called once | `available=0` | `available=0` |

Result: **as designed**, with one correction. The table in L3.md predicted
RPC_E_SERVERFAULT (0x80010105) for ndr64-only; the patch raises
RPC_X_WRONG_STUB_VERSION (1829), which COM returns as 0x80070725. The
prediction was wrong, not the patch: the server is not called and the
FIXME is logged, as intended. L3.md is corrected. i686 has no export, as
on Windows.

## winex11-flush-putimage ([README](../winex11-flush-putimage/README.md))

The six 8x8 writer XPUT records in each observed N/Y scenario were
matched to the next explicit XFLUSH with the same Unix PID and display.
The two initial 32x32 records from another process are excluded. Delays
below are in frame order (phases 0 through 5):

| Runtime / render setting | XPUT -> next explicit writer XFLUSH (ms) |
| --- | --- |
| without-putimage / N | 7708, 6507, 5305, 4104, 2903, 1702 |
| without-putimage / Y | 7707, 6505, 5304, 4103, 2901, 1700 |
| full / N and Y | 0 for every frame (same millisecond tick) |

This confirms the patch's explicit-call behavior in these observed paths.
It does **not** measure how long requests actually remained in Xlib's
buffer: the observer sees explicit XPutImage/XFlush calls, not internal
buffer drains, X server receipt or compositor presentation. Both wrappers
log before entering the real Xlib function; 0 ms is timestamp granularity,
not zero execution or delivery latency. The unpatched next XFLUSH appears
**after** `RECOVERY final_gdi_flush=1`, so it must not be called the final
GdiFlush itself; the record does not identify its exact caller.

All twelve plain/control/observed scenarios sampled only the background
colour (valid GetPixel reads), including the explicit-GdiFlush controls.
The DC sampler therefore did not demonstrate presentation timing or an
unpatched-versus-patched pixel improvement. That side remains
**inconclusive**; it does not cancel the explicit-call evidence above.

## winex11-shape-alpha-xwayland ([README](../winex11-shape-alpha-xwayland/README.md))

Maintainer's visual observations:

| Scenario | without-shape | full |
| --- | --- | --- |
| default (hidden-first popup) | inconclusive | transparent |
| ordinary class | inconclusive | transparent |
| visible-first | inconclusive | transparent |
| disabled (opt-out) | transparent | transparent |
| visible-optin | transparent | transparent |

The ARGB-visual trace appears only for the default target and the
visible-optin case, never for the ordinary class or visible-first:
eligibility works as designed. The opaque rectangle was not seen without
the patch on this compositor, so the visual bug is **unreproduced** here
(README: both builds may look correct). The full build was reported
transparent in all five cases; that is a maintainer visual observation,
not an independently observed fix or a blanket no-regression result.
The unpatched default/ordinary runs have no CLICK records, and the visual
entries are inconclusive. CLICK lines elsewhere identify target and phase,
not click coordinates, so they alone do not prove every missing-quadrant
and full-rectangle hit-test control. The ARGB selection/negative controls
are supported, while complete visual/input-region validation stays bounded
by those observations.

## winex11-xinput-bad-device ([README](../winex11-xinput-bad-device/README.md))

The helper resolved `XOpenDevice` and `XGetDeviceButtonMapping` (XI_HOOK)
in all three modes and both builds, but recorded no XI_CALL and no FAULT
line at all, even in the healthy `observe` mode. Wine never called the
legacy device queries during the run, so the injected error was never
raised. Raw and ordinary clicks were delivered in every mode and build.
Result: **uncovered** - the patched branch was not reached on this Xwayland
session (README: "If mapping is never called (no FAULT), the driver/XI2
path was not covered"). This is no evidence for or against the patch.

## Independent record check (Codex, 2026-11-11)

Read the complete four reproducer interpretation documents and this record;
parsed the maintainer's attached `l3-summary.txt` across all 40 scenarios,
including timestamp pairing, all ten visual entries, RPC outcomes and
XInput hook/call/fault counts. All 40 EXIT records are zero and the final
collection marker is present; fixture summaries report no API errors.
Compared the relevant observer logging and RPC implementation source. No
Wine/CAD execution or independent desktop observation was performed here.
Only this bounded interpretation is committed, not the attachment/raw logs.

The RPC correction to 0x80070725 is supported: the patch raises
RPC_X_WRONG_STUB_VERSION (1829 / 0x725), returned here as HRESULT
0x80070725. `functional=0` is expected for that negative scenario; it is
not an API setup failure or a successful NDR64 call. The old program's
`expect=serverfault` label was a prediction, not the measured result.
The system reference is Wine 11.21, not an otherwise identical Wine 11.20
build with only the rpcrt4 patch omitted.

## Maintainer decision and follow-up (2026-11-11)

- **XInput does not block the winum-11.20-1 tag**, by the maintainer's
  explicit decision. Its l3-only reason is the absence of a Windows
  counterpart; the maintainer reports the patch fixed Inventor's Hole
  command crash. This decision does not turn the uncovered branch into
  a successful fault-injection result. The existing marker stays unchanged.
- **Open after release:** revise the XInput fixture/procedure to make Wine
  actually call the legacy button mapping (a source-device switch is a
  candidate trigger, not yet a verified solution). First establish XI_CALL
  in healthy observe mode, then same-PID XI_HOOK/FAULT in open/map modes,
  and compare survival, fallback and raw/ordinary clicks in matched builds.
  No such reproducer revision has been prepared or validated in this review.
- PutImage pixels remain inconclusive. A validated drawable or composited
  observation method is needed before claiming presentation timing; a
  different sampler is a follow-up proposal, not an established requirement
  or implementation in this record.
