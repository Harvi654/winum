# AutoCAD licensing dialog entry trace

During the follow-up trial with the SCM candidate, the registered
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

[The component observer](../../../scripts/observe-autocad-components.py)
can wrap the same run. It samples known Wine and Autodesk processes in
the diagnostic prefix and reports the startup configuration of relevant
SCM entries. It records changes around the marker up to the trace cutoff.
The component-summary.txt report contains executable names, counts and
relative timestamps. The sampled transitions can miss subsecond tasks.
[The diagnostic build and simulated dialog check passed in Actions.](https://github.com/Harvi654/winum/actions/runs/44919240519)
