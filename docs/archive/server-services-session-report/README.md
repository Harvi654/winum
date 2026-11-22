# Service manager session reporting

Archived on 2026-11-22. This diagnostic allowed the service to reach
RUNNING. The Connection Lost alert persisted because cross-thread
KillTimer failed. The diagnostic patch is kept with its source credit.
[Timer investigation](https://github.com/Harvi654/Autodesk-AutoCAD-on-Linux/blob/b158061/docs/autocad-experiment-2026-11.md#2026-11-22-cross-thread-timer-cancellation)

AutoCAD 2027 stayed usable for five minutes on Windows with the registered Autodesk licensing service running. In the Winum reference, the registered service returned error 1053 ten seconds after the start request. The service manager was reported in console session 1. Starting its executable directly allowed AutoCAD to open, followed by the Connection Lost dialog after 2 minutes and 21 seconds. These are separate milestones for testing.

The candidate adapts the service-manager enumeration experiment from [Lorenzo Cozzaglio](https://github.com/lori28167/autocad-wine-linux/blob/ec02824/wine-autocad.patch). Go's [Windows service detection](https://github.com/golang/sys/blob/master/windows/svc/security.go) checks the parent process name and the session field in the process list. The change reports zero for services.exe in the list. It leaves actual session membership, tokens and the PID-specific query untouched. A conforming session-zero implementation would need broader validation. The candidate tests whether this field is enough for sc start to reach RUNNING in the maintainer's prefix.

The paired CI comparison built both servers and ran the x64 and i686 probes. Both reported services.exe in session 1 through process enumeration on the baseline and session 0 through process enumeration on the candidate. The PID-specific query returned session 1 throughout. The probe itself remained in session 1 through both queries. [CI run and artifacts](https://github.com/Harvi654/winum/actions/runs/44917023316)

The maintainer's paired SCM test reached RUNNING with the candidate. StartServiceCtrlDispatcherW appeared about 47 ms after the SCM start message. The desktop Connection Lost alert still appeared around 2 minutes and 20 seconds after launch. A Windows VM AutoCAD session was open concurrently in that trial.

Close the VM AutoCAD session for the next run. Use [the archived service trace](https://github.com/Harvi654/winum/blob/2ae948f/scripts/trace-autocad-licensing-service.sh) with the existing candidate test directory. It starts the registered service, records service status at the dialog, then captures 45 seconds before asking the user to close AutoCAD. Return the filtered service-summary.txt. The raw trace remains in the local test directory. Native process enumeration measurements and normal series validation are pending.
