# Cross-thread cancellation of windowless timers

AutoCAD 2027 registers a windowless 120-second timer on its UI thread.
After restoring a signed-in session, it attempts to cancel the timer
from another thread. The Wine trace showed a failed cancellation,
delivery at 120 seconds, and the connection alert 459 ms later.
A Windows API capture showed successful cross-thread cancellation.

The original standalone Win32 probe measured this behavior on Windows
10 x64 and Winum 11.22. With two live timers on different threads,
Windows assigned distinct IDs and cancelled only the target. Wine
assigned equal IDs and cancelled the caller's own timer. With no
timer on the caller, Wine returned ERROR_INVALID_PARAMETER.

The candidate makes windowless IDs and cancellation process-wide and
keeps delivery on the creating thread. It retains the existing scope
for replacing timers with SetTimer and for timers associated with an
HWND. The candidate includes a user32 regression test alongside the
standalone probe. Cross-process behavior and cross-thread replacement
are outside the measured cases.

The paired standalone probe reproduced the baseline failures and passed
with the candidate on x64 and i686. The AutoCAD 2027 smoke test passed
with the combined runtime. The maintainer retained sign-in across a
restart, reported two intervals of 16 and 10 minutes without the alert,
and verified OPEN, SAVE, SAVEAS and opening an existing DWG. The full
Wine test suites and Inventor smoke test are still pending.

[Paired CI probe](https://github.com/Harvi654/winum/actions/runs/45042604192) · [AutoCAD validation](https://github.com/Harvi654/Autodesk-AutoCAD-on-Linux/blob/main/docs/autocad-experiment-2026-11.md#2026-11-22-cross-thread-timer-cancellation)
