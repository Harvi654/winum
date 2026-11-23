# server-cross-thread-killtimer.patch

<!-- winum-test: l1 -->

- **DLL:** `server` (wineserver, `server/queue.c`, `server/process.c`), test in `user32`
- **Status:** In the winum-11.22-2 series. [Candidate history](../../patches/candidates/server-cross-thread-killtimer/README.md)
- **Target:** winum-11.22-2

## Symptom

AutoCAD 2027, signed in, showed a licensing alert "Connection Lost" about
two minutes after start, every time, although the licensing service was
running and answering.

## Cause and fix

AutoCAD starts a windowless 120-second timer (`SetTimer(NULL, ...)`) on its
UI thread and, once the session is restored, cancels it from another
thread with `KillTimer(NULL, id)`. Wine kept windowless timer IDs per
thread queue and looked only in the calling thread's queue: the
cancellation failed (or, with a timer of the same ID on the caller, hit the
wrong one), the timer fired at 120 s and the alert followed 459 ms later.

Windows 10 x64 (measured) gives live windowless timers on different
threads of a process distinct IDs and cancels the requested one from any
thread of the process, without touching the caller's own timer. The patch
makes windowless ID allocation and cancellation process-wide. Delivery
stays on the thread that created the timer; replacing a timer by ID still
needs the creating thread, and timers with a window are unchanged (neither
was measured differently). The server protocol is unchanged.

In plain words: a timer could only be stopped by the thread that started
it; on Windows any thread of the program can stop it, and AutoCAD relies
on that.

## How it was found

A Wine trace of AutoCAD showed the failed cancellation, the timer firing at
120 s and the alert right after; a Windows API capture of the same program
showed the cancellation succeeding. A standalone Win32 probe
(`docs/wine-repro/user32-cross-thread-killtimer/measure.c`) measured the
cases on Windows 10 and Wine.

## Wine test

`dlls/user32/tests/msg.c`: two threads with live windowless timers; checks
distinct IDs, cancellation of the other thread's timer from the caller,
and that the caller's own timer survives.

## Validation

| Level | Result |
| --- | --- |
| L0 | Applies with `--fuzz=0` after the full series on Wine 11.22 (2026-11-23); built x86_64 and i386 in the combined 11.22-2 build [45064416572](https://github.com/Harvi654/winum/actions/runs/45064416572) |
| L1 | Passed on offline Windows 10, x86_64 and i686: the new test passed; the `user32:msg` unit has 4 failures there that are not from this test (timing and `WM_CHAR` checks of the existing suite) (maintainer, 2026-11-22) |
| L2 | [Run 45049314260](https://github.com/Harvi654/winum/actions/runs/45049314260): paired against the release series without the patch, user32 with the neighbouring server and windowing suites, both architectures; no new failures, the new test fails without and passes with the patch |
| L3 | The standalone probe: baseline failures reproduced, candidate passed, x64 and i686 ([run 45042604192](https://github.com/Harvi654/winum/actions/runs/45042604192)) |
| L4 | AutoCAD 2027 with the candidate: sign-in kept across a restart, 16 and 10 minutes without the alert, OPEN, SAVE, SAVEAS (maintainer, 2026-11-22); combined 11.22-2 build: AutoCAD passed (maintainer, 2026-11-23). Inventor 2027 with the combined build: start, part, assembly, drawing, save/reopen, restart passed (maintainer, 2026-11-22). SOLIDWORKS deferred under AGENTS.md |
| L5 | Pending |
| Cross review | Written by Codex; read in full and approved by Claude Opus 5.5 (2026-11-22), see the candidate README. Watch the new test for timing flakiness on a loaded CI (100 ms timers, 500 ms wait) |

## Open

- Cross-process `KillTimer` and replacing another thread's timer by ID were
  not measured; the patch leaves them as Wine had them.

## Notes for a Wine bug report

AutoCAD 2027 creates a windowless timer on one thread and kills it from
another (`KillTimer(NULL, id)`). Windows 10 cancels it; Wine returns
failure or cancels the caller's timer with the same ID. Public synthetic
test: the `msg.c` check above.

## History

- v1 2026-11-22: candidate, AutoCAD tests.
- v2 2026-11-23: no code change; promoted to the series.
