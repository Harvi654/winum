' Original, AI-assisted (Claude), LGPL-2.1-or-later.
' Registers a .NET Framework class (System.Collections.ArrayList from
' mscorlib) as a new COM class in HKCU\Software\Classes only, the way
' per-user .NET add-ins register, creates it once and removes the keys.
' Needs .NET Framework 4.x with mscoree.dll as the COM server (on Wine the
' native .NET 4.8 with a native mscoree override, as in the Inventor
' prefix). Usage: cscript //nologo per-user-dotnet-class.vbs
Option Explicit
Const CLSID = "{6F3C2A51-9B7E-4C1D-8E2F-7A0B5D4C3E21}"
Const PROGID = "WinumRepro.PerUserArrayList"
Dim sh, base, o, num, desc
Set sh = CreateObject("WScript.Shell")
base = "HKCU\Software\Classes\"

On Error Resume Next
sh.RegRead base & "CLSID\" & CLSID & "\"
If Err.Number = 0 Then
    WScript.Echo "fixture already present, remove " & base & "CLSID\" & CLSID & " first"
    WScript.Quit 2
End If
Err.Clear
On Error GoTo 0

sh.RegWrite base & PROGID & "\", PROGID, "REG_SZ"
sh.RegWrite base & PROGID & "\CLSID\", CLSID, "REG_SZ"
sh.RegWrite base & "CLSID\" & CLSID & "\", PROGID, "REG_SZ"
sh.RegWrite base & "CLSID\" & CLSID & "\ProgId\", PROGID, "REG_SZ"
sh.RegWrite base & "CLSID\" & CLSID & "\InprocServer32\", "mscoree.dll", "REG_SZ"
sh.RegWrite base & "CLSID\" & CLSID & "\InprocServer32\ThreadingModel", "Both", "REG_SZ"
sh.RegWrite base & "CLSID\" & CLSID & "\InprocServer32\Class", "System.Collections.ArrayList", "REG_SZ"
sh.RegWrite base & "CLSID\" & CLSID & "\InprocServer32\Assembly", _
    "mscorlib, Version=4.0.0.0, Culture=neutral, PublicKeyToken=b77a5c561934e089", "REG_SZ"
sh.RegWrite base & "CLSID\" & CLSID & "\InprocServer32\RuntimeVersion", "v4.0.30319", "REG_SZ"

On Error Resume Next
Set o = CreateObject(PROGID)
num = Err.Number : desc = Err.Description
If num = 0 Then o.Add 1 : num = Err.Number : desc = Err.Description
WScript.Echo "per-user .NET class err=0x" & Hex(num) & " " & desc
Set o = Nothing
Err.Clear

sh.RegDelete base & "CLSID\" & CLSID & "\InprocServer32\"
sh.RegDelete base & "CLSID\" & CLSID & "\ProgId\"
sh.RegDelete base & "CLSID\" & CLSID & "\"
sh.RegDelete base & PROGID & "\CLSID\"
sh.RegDelete base & PROGID & "\"
If Err.Number <> 0 Then WScript.Echo "cleanup error 0x" & Hex(Err.Number) : WScript.Quit 3
If num <> 0 Then WScript.Quit 1
