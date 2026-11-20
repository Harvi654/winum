@echo off
rem Original, AI-assisted (Codex), LGPL-2.1-or-later.
setlocal DisableDelayedExpansion
set "source=%~dp0"
set "result=%source%result-autocad-save-window.txt"
if exist "%result%" goto already_run
if not exist "%source%dwg-open-events.lsp" goto missing

set "stage=%TEMP%\winum-dwg-save-%RANDOM%-%RANDOM%"
mkdir "%stage%" || goto setup_failed
copy "%source%dwg-open-events.lsp" "%stage%\open-events.lsp" >nul || goto setup_failed
set "acadpath=%stage:\=/%"

echo Launch AutoCAD 2027 and create one blank drawing. Set FILEDIA to 0.
echo Paste this line in the AutoCAD command prompt:
echo ^(progn ^(load "%acadpath%/open-events.lsp"^) ^(setq winum-trace-file-window T^) ^(winum-save-trace "%acadpath%/open-events.txt" "%acadpath%/saved.dwg"^)^)
echo Draw one line, run ._REGEN, and prepare a fresh Procmon capture for acad.exe.
echo Run ._SAVEAS with the default DWG format and this path:
echo "%stage%\saved.dwg"
echo Stop the capture when the command returns. Press any key here to collect the markers.
pause >nul

>"%result%" echo reference=native-windows trial=save-events
>>"%result%" ver
findstr /I /X /L /C:"reactors_ready" /C:"save_reactors_ready" /C:"control_regen" ^
    /C:":vlr-commandwillstart" /C:"save_command_enter" /C:"save_begin" ^
    /C:"save_target_complete" /C:"save_other_complete" /C:":vlr-commandended" ^
    /C:":vlr-commandcancelled" /C:":vlr-commandfailed" /C:"save_command_exit" ^
    "%stage%\open-events.txt" >>"%result%" 2>nul || >>"%result%" echo markers=missing
if not exist "%stage%\saved.dwg" >>"%result%" echo dwg_saved=missing
if exist "%stage%\saved.dwg" for %%F in ("%stage%\saved.dwg") do >>"%result%" echo dwg_saved=present dwg_bytes=%%~zF
echo Filtered result: result-autocad-save-window.txt next to this script.
pause
exit /b 0

:already_run
echo A result already exists. Use a fresh copy of the bundle for another run.
goto failed

:missing
echo Put dwg-open-events.lsp next to this script.
goto failed

:setup_failed
echo Could not stage the AutoLISP file under TEMP.

:failed
pause
exit /b 2
