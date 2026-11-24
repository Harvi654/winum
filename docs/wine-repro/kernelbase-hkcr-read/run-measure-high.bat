@echo off
setlocal
pushd "%~dp0" || exit /b 1
set "failed=0"
if exist exitcodes-high.txt (
    popd
    echo Already run. Use a fresh copy.
    exit /b 2
)
>exitcodes-high.txt type nul
for %%a in (x86_64 i686) do call :run %%a
type exitcodes-high.txt
popd
exit /b %failed%

:run
echo Running HKCR measurement %1 at High integrity
hkcr-read-%1.exe --elevated >result-hkcr-read-high-%1.txt 2>&1
set "code=%errorlevel%"
>>exitcodes-high.txt echo %1=%code%
if not "%code%"=="0" set "failed=1"
exit /b 0
