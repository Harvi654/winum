' Original, AI-assisted (Claude), LGPL-2.1-or-later.
' Usage: cscript //nologo createobject.vbs <ProgID>
' Prints the error of CreateObject; exit 0 = created, 1 = failed.
On Error Resume Next
Set o = CreateObject(WScript.Arguments(0))
WScript.Echo WScript.Arguments(0) & " err=0x" & Hex(Err.Number) & " " & Err.Description
If Err.Number <> 0 Then WScript.Quit 1
