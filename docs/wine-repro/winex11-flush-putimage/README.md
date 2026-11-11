# L3: send small PutImage updates before the next event

Original, AI-assisted (Codex), LGPL-2.1-or-later. An own Win32 host starts
an own second process which creates a child under the host's window. The
writer obtains a DC once and sends six tiny 8x8 red/blue DIB updates, 1200 ms
apart, without a message pump, DC release or explicit GdiFlush between
them. Tiny writes avoid filling Xlib's request buffer by accident. An
independent host-process timer samples the foreign child's DC through GetPixel
and prints the first sample plus subsequent color changes. A DC read is a
pixel-observation proxy: paired runtime/call evidence must establish that
this Wine configuration reads the direct drawable. The host is a separate
process, so it does not explicitly flush the writer's X connection. A final GdiFlush is a recovery boundary.

This targets the no-window-surface/direct-drawable case in
X11DRV_PutImage and xrenderdrv_PutImage. The `--gdi-flush` control explicitly
flushes after every write. The optional Linux observer logs actual XPutImage
and XFlush calls without adding an X request. Its logging can affect timing;
compare uninstrumented runs as well. It observes explicit calls, not every
internal Xlib buffer drain. No vendor process, window or text is inspected.

## Why L3 only

Windows has GDI but has no Xlib request buffer or Wine X11 driver. Native
Windows execution cannot establish whether an X11 submission was flushed.
This is a Linux/X11 timing reproducer, not a Windows L1 conformance test.

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

Without the patch, small writer requests may stay buffered during the idle
interval: DRAW advances while SAMPLE retains the old color, then catches
up at a later request or the recovery flush. With the patch, the writer's
PutImage path explicitly flushes the same display after submission, and
SAMPLE should track red/blue within the polling/rendering latency. The
GdiFlush control should remove buffering delay in either build.

Match XPUT and following XFLUSH by process and display; the host reader's
flushes do not count as writer flushes. DRAW tick is GetTickCount64;
SAMPLE is relative to REPRO start_tick. The observer clock is a separate
Linux monotonic clock, so compare its own call ordering rather than
assuming identical tick origins. No hard millisecond deadline is asserted.

If no XPUT appears, the changed path was not demonstrated. If GetPixel
returns invalid, or all three runs stay at the background color, the
observer/path is inconclusive. If the unpatched build already flushes by
another path, that run did not reproduce delayed presentation; it is not
evidence of a patch failure. The DC observation does not establish final Wayland compositing. This does not cover WebView2's separate GPU repaint
problem, which was reported to persist independently.

2026-11-09: strict MinGW build passed for x86_64/i686. The Linux x86_64 observer also compiles with -Wall -Wextra -Werror
against the distribution Xlib headers. No
fixture execution in cloud, no paired timing result or new L4 is claimed.
Page marker records the reason for L3, not a completed runtime pass.

The final summary records total/valid polls; no usable polling, timer failure
or command timeout is inconclusive (exit 2), even if the writer finished.
GdiFlush control/recovery return values are logged. A second-model source
advisory found no recognizable copied code (not proof of originality);
its observer-completeness and pathname-bound suggestions were applied.
The advisory is not a paired runtime result.

## After the test

The runner stops each matching private wineserver and removes its temporary
prefixes, runtimes and raw output, including on interruption. Only the
filtered summary remains. Application prefixes and installed Wine are not
changed. A timeout or absent required observer/fault evidence remains
inconclusive; use the interpretation above before recording L3 success.
