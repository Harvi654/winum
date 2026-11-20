@echo off
rem Original, AI-assisted (Codex), LGPL-2.1-or-later.
setlocal DisableDelayedExpansion
set "source=%~dp0"
set "result=%source%result-autocad-dwg-windows.txt"
if exist "%result%" goto already_run
if not exist "%source%line-2018.dwg" goto missing
if not exist "%source%dwg-open-events.lsp" goto missing

set "stage=%TEMP%\winum-dwg-native-%RANDOM%-%RANDOM%"
if exist "%stage%" goto setup_failed
mkdir "%stage%" || goto setup_failed
copy /y "%source%line-2018.dwg" "%stage%\probe.dwg" >nul || goto setup_failed
copy /y "%source%dwg-open-events.lsp" "%stage%\open-events.lsp" >nul || goto setup_failed
certutil -hashfile "%stage%\probe.dwg" SHA256 >"%stage%\hash.txt" || goto setup_failed
findstr /I /X /L /C:"1f4edd4c11aaf28b3295fd69fa59376f9cb911c637aa47a6073408ecac4797a3" "%stage%\hash.txt" >nul || goto hash_failed
set "acadpath=%stage:\=/%"

echo Open trial. Launch native AutoCAD 2027 and start one blank drawing.
echo Set FILEDIA to 0. Paste the setup line, run ._REGEN, then paste the open line.
echo ^(progn ^(load "%acadpath%/open-events.lsp"^) ^(winum-open-trace "%acadpath%/open.txt"^)^)
echo ^(winum-open-activex "%acadpath%/open.txt" "%acadpath%/probe.dwg"^)
echo Check whether the line drawing appears. Close AutoCAD, then press any key here.
pause >nul

echo.
echo Save trial. Relaunch native AutoCAD 2027 and start one blank drawing.
echo Set FILEDIA to 0. Paste the setup line, draw one line, and run ._REGEN.
echo ^(progn ^(load "%acadpath%/open-events.lsp"^) ^(winum-save-trace "%acadpath%/save.txt" "%acadpath%/saved.dwg"^)^)
echo Paste the save line once.
echo ^(winum-save-activex "%acadpath%/save.txt" "%acadpath%/saved.dwg"^)
echo Check whether the drawing takes the name saved.dwg. Close AutoCAD, then press any key here.
pause >nul

>"%result%" echo reference=native-windows
>>"%result%" ver
>>"%result%" echo fixture_sha256=verified
>>"%result%" echo trial=open-activex
findstr /I /X /L /C:"reactors_ready" /C:"control_regen" /C:"activex_begin" ^
    /C:"activex_open_enter" /C:"activex_open_return" /C:"activex_open_expected_dwg" /C:"activex_error" ^
    /C:":vlr-begindwgopen" /C:":vlr-dwgfileopened" /C:":vlr-enddwgopen" /C:":vlr-documentcreated" ^
    /C:":vlr-documenttobeactivated" /C:":vlr-documentbecamecurrent" "%stage%\open.txt" ^
    >>"%result%" 2>nul || >>"%result%" echo markers=missing
>>"%result%" echo trial=save-activex
findstr /I /X /L /C:"reactors_ready" /C:"save_reactors_ready" /C:"control_regen" /C:"activex_save_enter" ^
    /C:"save_begin" /C:"save_target_complete" /C:"save_other_complete" /C:"activex_save_return" /C:"activex_save_error" ^
    "%stage%\save.txt" >>"%result%" 2>nul || >>"%result%" echo markers=missing
if not exist "%stage%\saved.dwg" >>"%result%" echo dwg_saved=missing
if exist "%stage%\saved.dwg" for %%F in ("%stage%\saved.dwg") do >>"%result%" echo dwg_saved=present dwg_bytes=%%~zF

echo.
echo Done. The filtered result is result-autocad-dwg-windows.txt next to this script.
pause
exit /b 0

:already_run
echo A result already exists. Use a fresh copy of the bundle for another run.
goto failed

:missing
echo Put line-2018.dwg and dwg-open-events.lsp next to this script.
goto failed

:hash_failed
echo The input file did not pass SHA-256 verification.
goto failed

:setup_failed
echo Could not stage the test files or calculate the SHA-256.

:failed
pause
exit /b 2
