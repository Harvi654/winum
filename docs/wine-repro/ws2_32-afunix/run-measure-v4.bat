@echo off
setlocal DisableDelayedExpansion
pushd "%~dp0" || exit /b 2

for %%F in (exitcodes.txt result-x86_64.txt result-i686.txt) do if exist "%%F" goto already_run
for %%F in (afunix-v4-x86_64.exe afunix-v4-i686.exe) do if not exist "%%F" goto setup_error

set "sample=%TEMP%\winum-afunix-v4-%RANDOM%-%RANDOM%"
mkdir "%sample%" || goto setup_error
ver > exitcodes.txt
set "failed=0"
call :measure x86_64
call :measure i686
rd "%sample%"
type exitcodes.txt
popd
pause
exit /b %failed%

:measure
mkdir "%sample%\%1"
if errorlevel 1 (
    >>exitcodes.txt echo %1=2
    set "failed=1"
    exit /b 0
)
echo Running AF_UNIX MEASURE for %1...
afunix-v4-%1.exe "%sample%\%1" > result-%1.txt 2>&1
set "code=%ERRORLEVEL%"
>>exitcodes.txt echo %1=%code%
if not "%code%"=="0" set "failed=1"
rd "%sample%\%1"
exit /b 0

:already_run
echo Results already exist. Extract a fresh copy of the bundle.
goto stop

:setup_error
echo Could not find the EXEs or create a sample directory.

:stop
popd
pause
exit /b 2
