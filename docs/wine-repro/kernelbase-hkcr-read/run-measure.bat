@echo off
setlocal
pushd "%~dp0" || exit /b 1
set "failed=0"
>exitcodes.txt type nul
for %%a in (x86_64 i686) do call :run %%a
type exitcodes.txt
popd
exit /b %failed%

:run
echo Running HKCR measurement %1
hkcr-read-%1.exe >result-hkcr-read-%1.txt 2>&1
set "code=%errorlevel%"
>>exitcodes.txt echo %1=%code%
if not "%code%"=="0" set "failed=1"
exit /b 0
