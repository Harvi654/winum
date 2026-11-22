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

## Cross review (second model)

Claude Opus 5.5, 2026-11-22: approved; patch and test read in full
against Wine 11.22's `server/queue.c`. Windowless timer IDs move from the
queue to the process and `kill_win_timer` with no window looks the ID up
in every queue of the calling process, then frees it on the queue that
owns it - matching the measured Windows behaviour (distinct IDs across
threads, cross-thread cancel of the target only). Delivery stays on the
creating thread, window timers are untouched, and replacing a timer by
ID still needs the creating queue (unmeasured, left as Wine had it). No
request or reply structure changes, so the server protocol is the same.
`free_timer` on another thread's queue is safe in the single-threaded
server (it reschedules that queue's timeout). Notes: the new user32 test
uses 100 ms timers and a 500 ms wait - fine, but watch it for flakiness
on a loaded CI; L1 of that test on Windows 10 and the Inventor L4 are
still needed before the series.
