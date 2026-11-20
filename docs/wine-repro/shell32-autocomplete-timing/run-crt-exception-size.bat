@echo off
rem Original, AI-assisted (Codex), LGPL-2.1-or-later.
setlocal DisableDelayedExpansion
set "source=%~dp0"
set "result=%source%result-autocad-crt-exception-size.txt"
set "dotnet=%ProgramFiles%\dotnet\dotnet.exe"
if not exist "%dotnet%" goto missing
"%dotnet%" "%source%crt-exception-size.dll" > "%result%" 2>&1
set "probe_exit=%errorlevel%"
type "%result%"
echo Result: result-autocad-crt-exception-size.txt
pause
exit /b %probe_exit%

:missing
echo The x64 .NET host was not found at "%dotnet%".
pause
exit /b 2
