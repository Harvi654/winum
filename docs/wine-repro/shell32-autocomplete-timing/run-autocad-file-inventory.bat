@echo off
rem Original, AI-assisted (Codex), LGPL-2.1-or-later.
setlocal
pushd "%~dp0" || exit /b 2
if exist windows-autocad-files.tsv goto exists
if not exist autocad-file-inventory.exe goto missing

echo Hashing AutoCAD program files and .NET 10. This can take a few minutes.
autocad-file-inventory.exe > windows-autocad-files.tmp
if errorlevel 1 goto failed
move /y windows-autocad-files.tmp windows-autocad-files.tsv >nul || goto failed
echo Done. Copy only windows-autocad-files.tsv to the Arch comparison folder.
pause
popd
exit /b 0

:exists
echo A result already exists. Use a fresh copy of the folder for another run.
goto failed

:missing
echo Put autocad-file-inventory.exe next to this script.

:failed
pause
popd
exit /b 2
