# MEASURE: EndDialog on windows that are not dialogs

Inventor 2027 (issue Harvi654/Autodesk-Inventor-for-Linux#45): in the
Content Center editor, picking a standard in the filter ends with an MFC
C++ exception (`e06d7363` raised from `mfc140u.dll`) that nothing catches,
and .NET ends the process. A `+win,+dialog,+seh` trace on winum-11.22-3
(2026-11-26) shows, right before it, `EndDialog(<window>, IDCANCEL)` on
Inventor's MDI client window (`NtUserSetWindowFNID ... 800d`, MDICLIENT,
the document area), which Wine answers with `got invalid window handle`
and `FALSE` (`dlls/user32/dialog.c`, no dialog data for that window). The
next dialog's controls are created normally, then MFC throws. Whether
Windows treats such a call differently is what this program measures.

`measure.c` calls `EndDialog(hwnd, IDCANCEL)` on an MDI client (also with
the focus on it), an ordinary child window, an ordinary top-level window,
an owned popup whose owner is disabled, a real modeless dialog with a
disabled owner (control case), a destroyed window and NULL. Per case: the
return value, the last error, and the window's existence, visibility,
enabled state, focus and its owner's enabled state before and after; at
the end whether the MDI client is still visible and can create a child.

Build: `bash scripts/build-measure-bundle.sh OUT
docs/wine-repro/user32-enddialog-nondialog/measure.list`, or the CI
workflow `build-user32-enddialog-measure.yml`, which also runs it under
winum-11.22-3. In the offline Windows 10 VM run `run-measure.bat` from the
desktop without touching mouse or keyboard; return both
`result-enddialog-nondialog-*.txt` and `exitcodes.txt`.

## Results

Pending.
