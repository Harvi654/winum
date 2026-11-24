@echo off
rem Original, AI-assisted (Claude), LGPL-2.1-or-later.
rem Builds WinumRepro.cs with the .NET Framework 4 compiler that ships with
rem .NET 4.x, registers the class in HKCU\Software\Classes only (InprocServer32
rem = mscoree.dll, CodeBase to the DLL), calls CreateObject from 64-bit
rem cscript, then removes the keys and the build directory.
rem Exit 0 = created, 1 = CreateObject failed, 2 = setup failed.
setlocal
set "here=%~dp0"
set "csc=%WINDIR%\Microsoft.NET\Framework64\v4.0.30319\csc.exe"
if not exist "%csc%" (echo no .NET Framework 4 compiler at %csc% & exit /b 2)
set "out=%TEMP%\winum-repro-%RANDOM%"
set "clsid={6F3C2A51-9B7E-4C1D-8E2F-7A0B5D4C3E21}"
set "key=HKCU\Software\Classes\CLSID\%clsid%"
set "prog=HKCU\Software\Classes\WinumRepro.PerUserClass"
reg query "%key%" >nul 2>&1 && (echo fixture already present: %key% & exit /b 2)
mkdir "%out%" || exit /b 2
"%csc%" /nologo /target:library /out:"%out%\WinumRepro.dll" "%here%WinumRepro.cs" || (rmdir /s /q "%out%" & exit /b 2)
set "codebase=file:///%out:\=/%/WinumRepro.dll"
reg add "%prog%" /ve /d WinumRepro.PerUserClass /f >nul
reg add "%prog%\CLSID" /ve /d %clsid% /f >nul
reg add "%key%" /ve /d WinumRepro.PerUserClass /f >nul
reg add "%key%\ProgId" /ve /d WinumRepro.PerUserClass /f >nul
reg add "%key%\InprocServer32" /ve /d mscoree.dll /f >nul
reg add "%key%\InprocServer32" /v ThreadingModel /d Both /f >nul
reg add "%key%\InprocServer32" /v Class /d WinumRepro.PerUserClass /f >nul
reg add "%key%\InprocServer32" /v Assembly /d "WinumRepro, Version=1.0.0.0, Culture=neutral, PublicKeyToken=null" /f >nul
reg add "%key%\InprocServer32" /v RuntimeVersion /d v4.0.30319 /f >nul
reg add "%key%\InprocServer32" /v CodeBase /d "%codebase%" /f >nul
"%WINDIR%\system32\cscript.exe" //nologo "%here%createobject.vbs" WinumRepro.PerUserClass
set "code=%errorlevel%"
reg delete "%key%" /f >nul
reg delete "%prog%" /f >nul
rmdir /s /q "%out%"
exit /b %code%
