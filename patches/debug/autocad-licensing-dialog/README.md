# AutoCAD licensing dialog entry trace

During the single-session trial with the SCM candidate, the registered
licensing service was RUNNING at the alert and at the end of the 45-second
capture. Its trace had no selected error around the marker. The observed
0xc0000120 status is STATUS_CANCELLED, which maps to ERROR_OPERATION_ABORTED
in Wine's socket implementation. The trace by itself cannot attribute the
cancelled operations to the desktop alert.

The diagnostic common controls DLL records module names, offsets and
memory classification at entry to the exact TaskDialogIndirect notice.
It runs before the modal loop and the trace launch uses no relay thunks.
This is observational instrumentation in Wine. It leaves the dialog and
licensing service behavior intact and belongs outside the normal series.

Use the existing session-report candidate test directory as input to the
artifact's preparation script. The script copies its prefix and runtime
and stages the diagnostic DLL in the test copy. Report the filtered
service-summary.txt after a reproduction with the Windows VM AutoCAD
closed. Proprietary modules are identified only by the running process's
loader information and offsets.
