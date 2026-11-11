# L3 comparison run, 2026-11-11

Shared L3 workflow ([l3-ci.md](../l3-ci.md)), run 42999903587, commit
bffbf79; collected by the maintainer on the desktop (KDE, Wayland session,
Xwayland) with `scripts/run-l3-local.sh`. Collection complete
(`COLLECTION complete=1`), every scenario exit 0, no API errors. Exit 0
means the sequence completed, not a pass; the interpretation below follows
each fixture's README. Only this summary is recorded, not the collected
output.

Runtimes: `full` = the CI build of the full candidate series (Wine 11.20);
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

Writer XPUT matched to the next XFLUSH of the same process and display
(observed scenarios, both N and Y variants, 6 frames each):

| Runtime | Delay XPUT -> writer XFLUSH |
| --- | --- |
| without-putimage | 1.7, 2.9, 4.1, 5.3, 6.5, 7.7 s (each frame waits for the final recovery flush) |
| full | 0 ms for every frame |

The pixel sampler (GetPixel in the host process) stayed at the background
colour in every run, both builds, so the pixel timing is **inconclusive**
on this session (README: "all runs stay at the background color"). The
X request side is **confirmed**: without the patch, PutImage requests stay
in the Xlib buffer until an unrelated flush; with it, each one is flushed
at once.

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
(README: both builds may look correct); with the patch nothing regressed.

## winex11-xinput-bad-device ([README](../winex11-xinput-bad-device/README.md))

The helper resolved `XOpenDevice` and `XGetDeviceButtonMapping` (XI_HOOK)
in all three modes and both builds, but recorded no XI_CALL and no FAULT
line at all, even in the healthy `observe` mode. Wine never called the
legacy device queries during the run, so the injected error was never
raised. Raw and ordinary clicks were delivered in every mode and build.
Result: **uncovered** - the patched branch was not reached on this Xwayland
session (README: "If mapping is never called (no FAULT), the driver/XI2
path was not covered"). This is no evidence for or against the patch.

## Follow-up

- XInput: a fixture change (or a session) that makes Wine query the device
  mapping, for example a source-device switch, is needed before the
  branch is covered; prepared by Codex.
- PutImage pixels: a sampler that reads the composited output (not the
  DC) would be needed for the pixel side; the X side is already shown.
