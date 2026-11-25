@echo off
rem Original, AI-assisted (Claude), LGPL-2.1-or-later.
rem Run once from a normal console and once from an elevated console
rem ("Run as administrator"). The result files are named after the
rem console's integrity level, so both runs can share this directory.
setlocal
pushd "%~dp0" || exit /b 1
set "level=medium"
whoami /groups | find "S-1-16-12288" >nul && set "level=high"
whoami /groups | find "S-1-16-16384" >nul && set "level=system"
echo Console integrity: %level%
set "failed=0"
>exitcodes-%level%.txt type nul
for %%a in (x86_64 i686) do call :run %%a
type exitcodes-%level%.txt
popd
exit /b %failed%

:run
echo Running token integrity measurement %1
token-integrity-%1.exe >result-token-integrity-%1-%level%.txt 2>&1
set "code=%errorlevel%"
>>exitcodes-%level%.txt echo %1=%code%
if not "%code%"=="0" set "failed=1"
exit /b 0
