@echo off
setlocal DisableDelayedExpansion
pushd "%~dp0" || goto directory_error

for %%F in (exitcodes.txt result-x86_64.txt result-i686.txt) do if exist "%%F" goto already_run
for %%F in (measure-x86_64.exe measure-i686.exe) do if not exist "%%F" goto missing_exe

set "sample=%TEMP%\winum-shell32-enter-%RANDOM%-%RANDOM%"
mkdir "%sample%" || goto sample_error

ver > exitcodes.txt
echo Running x86_64 MEASURE...
measure-x86_64.exe "%sample%" > result-x86_64.txt 2>&1
set "code64=%ERRORLEVEL%"
>>exitcodes.txt echo x86_64=%code64%

echo Running i686 MEASURE...
measure-i686.exe "%sample%" > result-i686.txt 2>&1
set "code32=%ERRORLEVEL%"
>>exitcodes.txt echo i686=%code32%

rd "%sample%"
echo Results: exitcodes.txt, result-x86_64.txt, result-i686.txt
type exitcodes.txt
popd
pause
if not "%code64%"=="0" exit /b 1
if not "%code32%"=="0" exit /b 1
exit /b 0

:already_run
echo Results already exist. Use a fresh copy of the bundle.
goto setup_error

:missing_exe
echo Keep both MEASURE executables beside this batch file.
goto setup_error

:sample_error
echo Could not create the temporary sample folder.

:setup_error
popd
pause
exit /b 2

:directory_error
echo Could not open the bundle directory.
pause
exit /b 2
