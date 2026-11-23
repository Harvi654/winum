' Original MEASURE script, AI-assisted (Codex), LGPL-2.1-or-later.
Option Explicit
Const HKCU = &H80000001
Dim provider, key, leaf, missing, value, rc

Function Describe(ByVal item)
    If IsArray(item) Then
        Describe = VarType(item) & ":[" & Join(item, ",") & "]"
    ElseIf IsNull(item) Then
        Describe = "1:null"
    ElseIf IsEmpty(item) Then
        Describe = "0:empty"
    Else
        Describe = VarType(item) & ":" & CStr(item)
    End If
End Function

Sub Report(ByVal label, ByVal failure, ByVal result, ByVal output)
    WScript.Echo "vbs case=" & label & " err=0x" & Right("00000000" & Hex(failure), 8) & _
        " result=" & Describe(result) & " out=" & Describe(output)
End Sub

key = WScript.Arguments(0) & "\vbs"
leaf = key & "\leaf"
missing = key & "\missing"
On Error Resume Next
Set provider = GetObject("winmgmts:{impersonationLevel=impersonate}!\\.\root\default:StdRegProv")
Report "bind", Err.Number, Empty, Empty
If Not IsObject(provider) Then WScript.Quit 2

Err.Clear
provider.CreateKey HKCU, key
Report "create_statement", Err.Number, Empty, Empty

Err.Clear
rc = Empty
rc = provider.CreateKey(HKCU, leaf)
Report "create_leaf", Err.Number, rc, Empty

Err.Clear
rc = Empty
rc = provider.SetStringValue(HKCU, key, "sample", "winum-value")
Report "write_string", Err.Number, rc, Empty

Err.Clear
rc = Empty
value = "out-sentinel"
rc = provider.GetStringValue(HKCU, key, "sample", value)
Report "read_string", Err.Number, rc, value

Err.Clear
rc = Empty
value = "out-sentinel"
rc = provider.EnumKey(HKCU, key, value)
Report "enum", Err.Number, rc, value

Err.Clear
rc = Empty
rc = provider.SetDWORDValue(HKCU, key, "sample", 42)
Report "write_dword", Err.Number, rc, Empty

Err.Clear
rc = Empty
value = "out-sentinel"
rc = provider.GetStringValue(HKCU, missing, "sample", value)
Report "read_missing", Err.Number, rc, value

Err.Clear
rc = Empty
rc = provider.DeleteKey(HKCU, leaf)
Report "delete_leaf", Err.Number, rc, Empty

Err.Clear
rc = Empty
rc = provider.DeleteKey(HKCU, key)
Report "delete", Err.Number, rc, Empty
WScript.Echo "vbs direct_complete"
