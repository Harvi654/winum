@echo off
rem Original, AI-assisted (Codex), LGPL-2.1-or-later.
setlocal DisableDelayedExpansion
set "source=%~dp0"
set "result=%source%result-autocad-managed-save-windows.txt"
set "acad=%ProgramFiles%\Autodesk\AutoCAD 2027\acad.exe"
if exist "%result%" goto already_run
if not exist "%acad%" goto missing_acad
if not exist "%source%dwg-open-events.lsp" goto missing_bundle
if not exist "%source%managed-save-observer.dll" goto missing_bundle

tasklist /FI "IMAGENAME eq acad.exe" /NH | findstr /B /I /L /C:"acad.exe " >nul
if not errorlevel 1 goto running
set "stage=%TEMP%\winum-managed-save-%RANDOM%-%RANDOM%"
mkdir "%stage%" || goto setup_failed
copy "%source%dwg-open-events.lsp" "%stage%\open-events.lsp" >nul || goto setup_failed
copy "%source%managed-save-observer.dll" "%stage%\managed-save-observer.dll" >nul || goto setup_failed
set "WINUM_ACAD_EXCEPTION_LOG=%stage%\managed-exceptions.txt"
set "acadpath=%stage:\=/%"

echo Starting AutoCAD 2027. In a blank drawing set FILEDIA to 0.
echo Paste this line in AutoCAD:
echo ^(progn ^(load "%acadpath%/open-events.lsp"^) ^(setq winum-trace-file-window T^) ^(winum-save-trace "%acadpath%/open-events.txt" "%acadpath%/saved.dwg"^)^)
echo Draw a line and run ._REGEN. Run ._NETLOAD and load this DLL:
echo "%stage%\managed-save-observer.dll"
echo Run WINUMEXCEPTIONS, then ._SAVEAS with the default DWG format and this path:
echo "%stage%\saved.dwg"
echo Close AutoCAD after Save As. This window will then show the filtered result.
start "" /wait "%acad%"
set "acadexit=%errorlevel%"

>"%result%" echo reference=native-windows trial=managed-save launcher_exit=%acadexit%
>>"%result%" ver
findstr /I /X /L /C:"save_reactors_ready" /C:"control_regen" /C:":vlr-commandwillstart" ^
    /C:"save_begin" /C:"save_target_complete" /C:"save_other_complete" ^
    /C:":vlr-commandended" /C:":vlr-commandcancelled" /C:":vlr-commandfailed" ^
    "%stage%\open-events.txt" >>"%result%" 2>nul || >>"%result%" echo markers=missing
if not exist "%stage%\saved.dwg" >>"%result%" echo dwg_saved=missing
if exist "%stage%\saved.dwg" for %%F in ("%stage%\saved.dwg") do >>"%result%" echo dwg_saved=present dwg_bytes=%%~zF
if not exist "%WINUM_ACAD_EXCEPTION_LOG%" goto missing_log
set "record_count=0"
for /f "delims=" %%L in ('findstr /L /C:"managed_exception " "%WINUM_ACAD_EXCEPTION_LOG%"') do set /a record_count+=1 >nul
>>"%result%" echo managed_probe_records=%record_count%
findstr /L /C:"managed_probe=" /C:"managed_frame " /C:"managed_native_frame " ^
    /C:" type=System.NotImplementedException hr=0x80004001" ^
    "%WINUM_ACAD_EXCEPTION_LOG%" >>"%result%"
goto report

:missing_log
>>"%result%" echo managed_probe=log_missing
:report
type "%result%"
echo Result: result-autocad-managed-save-windows.txt
pause
exit /b 0

:already_run
echo A result already exists. Extract the bundle into a fresh folder for a new run.
goto failed
:missing_acad
echo AutoCAD 2027 was not found at "%acad%".
goto failed
:missing_bundle
echo The AutoLISP file and observer DLL must be next to this script.
goto failed
:running
echo Close AutoCAD before opening this script so the new process receives the observer settings.
goto failed
:setup_failed
echo Could not stage the probe under TEMP.
:failed
pause
exit /b 2
