' Original MEASURE script, AI-assisted (Codex), LGPL-2.1-or-later.
Option Explicit
Const adTypeBinary = 1, adTypeText = 2, adCRLF = -1, adLF = 10
Const adReadAll = -1, adReadLine = -2, adWriteChar = 0, adWriteLine = 1, adSaveCreateOverWrite = 2

Dim dir, charsets, sample, i, kind
dir = WScript.Arguments(0)
charsets = Array("utf-8", "UTF-8", "Unicode", "us-ascii", "windows-1252", "iso-8859-1")
sample = "A" & ChrW(&H010D) & ChrW(&H20AC) & ChrW(&HD83D) & ChrW(&HDE03)
For i = 0 To UBound(charsets)
    MeasureWrite charsets(i), i
Next
For i = 0 To 1
    For Each kind In Array("bom", "bare")
        MeasureLoad charsets(i), kind
    Next
Next
WScript.Echo "vbs complete=1"

Function Code(ByVal n)
    Code = "0x" & Right("00000000" & Hex(n), 8)
End Function

Function Describe(ByVal v)
    Dim j, units
    Describe = CStr(VarType(v)) & ":"
    If VarType(v) = vbString Then
        units = ""
        For j = 1 To Len(v)
            If j > 1 Then units = units & ","
            units = units & LCase(Right("0000" & Hex(AscW(Mid(v, j, 1)) And &HFFFF&), 4))
        Next
        Describe = Describe & "[" & units & "]"
    ElseIf Not IsEmpty(v) And Not IsNull(v) Then
        If VarType(v) = vbBoolean Then
            Describe = Describe & CStr(CLng(v))
        Else
            Describe = Describe & CStr(v)
        End If
    End If
End Function

Function State(ByVal s)
    Dim size, pos, eos
    On Error Resume Next
    Err.Clear
    size = s.Size
    If Err.Number <> 0 Then size = Code(Err.Number)
    Err.Clear
    pos = s.Position
    If Err.Number <> 0 Then pos = Code(Err.Number)
    Err.Clear
    eos = s.EOS
    If Err.Number <> 0 Then eos = Code(Err.Number)
    State = " Size=" & Describe(size) & " Position=" & Describe(pos) & " EOS=" & Describe(eos)
    Err.Clear
End Function

Sub Report(ByVal s, ByVal label, ByVal errorCode, ByVal result)
    WScript.Echo "vbs step=" & label & " err=" & Code(errorCode) & " result=" & Describe(result) & State(s)
    Err.Clear
End Sub

Function OpenStream(ByVal mode, ByVal charset)
    Dim s, v
    On Error Resume Next
    Set OpenStream = Nothing
    Set s = CreateObject("ADODB.Stream")
    v = Err.Number
    WScript.Echo "vbs create type=" & mode & " err=" & Code(v)
    If v <> 0 Then Exit Function
    s.Type = mode
    Report s, "type", Err.Number, Empty
    If mode = adTypeText Then
        s.Charset = charset
        Report s, "charset_set", Err.Number, Empty
        v = s.Charset
        Report s, "charset_get", Err.Number, v
    End If
    s.Open
    v = Err.Number
    Report s, "open", v, Empty
    If v = 0 Then Set OpenStream = s
End Function

Sub ReadText(ByVal s, ByVal label, ByVal count)
    Dim result
    On Error Resume Next
    result = s.ReadText(count)
    Report s, label, Err.Number, result
End Sub

Sub MeasureWrite(ByVal charset, ByVal index)
    Dim s, b, base, other, v
    On Error Resume Next
    base = dir & "\vbs-" & index
    WScript.Echo "vbs case=write charset=" & charset
    Set s = OpenStream(adTypeText, charset)
    If s Is Nothing Then Exit Sub
    s.WriteText "", adWriteChar
    Report s, "write_empty", Err.Number, Empty
    s.WriteText sample, adWriteChar
    Report s, "write_char", Err.Number, Empty
    s.LineSeparator = adCRLF
    Report s, "set_crlf", Err.Number, Empty
    s.WriteText "B", adWriteLine
    Report s, "write_line_crlf", Err.Number, Empty
    s.LineSeparator = adLF
    Report s, "set_lf", Err.Number, Empty
    s.WriteText "C", adWriteLine
    Report s, "write_line_lf", Err.Number, Empty
    s.SaveToFile base & "-initial.bin", adSaveCreateOverWrite
    Report s, "save_initial", Err.Number, Empty

    s.Position = 0
    Report s, "rewind_read", Err.Number, Empty
    ReadText s, "read_n1", 1
    ReadText s, "read_n3", 3
    ReadText s, "read_line_lf", adReadLine
    ReadText s, "read_remaining", adReadAll

    s.Position = 0
    Report s, "rewind_copy", Err.Number, Empty
    Set b = OpenStream(adTypeBinary, "")
    If Not b Is Nothing Then
        s.CopyTo b, -1
        Report s, "copy_text_to_binary", Err.Number, Empty
        WScript.Echo "vbs target=text_to_binary" & State(b)
        b.SaveToFile base & "-binary.bin", adSaveCreateOverWrite
        Report b, "save_binary_target", Err.Number, Empty
        b.Close
        Report b, "close", Err.Number, Empty
    End If
    s.Position = 0
    Report s, "rewind_eos", Err.Number, Empty
    s.SetEOS
    Report s, "set_eos", Err.Number, Empty
    s.SaveToFile base & "-truncated.bin", adSaveCreateOverWrite
    Report s, "save_truncated", Err.Number, Empty
    s.Close
    Report s, "close", Err.Number, Empty

    WScript.Echo "vbs case=change_at_zero charset=" & charset
    Set s = OpenStream(adTypeText, "Unicode")
    If s Is Nothing Then Exit Sub
    s.Charset = charset
    Report s, "charset_after_open", Err.Number, Empty
    v = s.Charset
    Report s, "charset_after_open_get", Err.Number, v
    s.WriteText sample, adWriteChar
    Report s, "write_before_change", Err.Number, Empty
    other = "Unicode"
    If Left(charset, 1) = "U" Then other = "utf-8"
    s.Charset = other
    Report s, "charset_after_write", Err.Number, Empty
    v = s.Charset
    Report s, "charset_after_write_get", Err.Number, v
    s.Close
    Report s, "close", Err.Number, Empty
End Sub

Sub MeasureLoad(ByVal charset, ByVal kind)
    Dim s, b, path
    On Error Resume Next
    path = dir & "\utf8-" & kind & ".bin"
    WScript.Echo "vbs case=load charset=" & charset & " fixture=" & kind
    Set s = OpenStream(adTypeText, charset)
    If s Is Nothing Then Exit Sub
    s.LoadFromFile path
    Report s, "load_text", Err.Number, Empty
    ReadText s, "read_n", 1
    ReadText s, "read_all_after_n", adReadAll
    s.Position = 0
    Report s, "rewind_lines", Err.Number, Empty
    s.LineSeparator = adCRLF
    Report s, "set_crlf", Err.Number, Empty
    ReadText s, "read_crlf", adReadLine
    s.LineSeparator = adLF
    Report s, "set_lf", Err.Number, Empty
    ReadText s, "read_lf", adReadLine
    ReadText s, "read_tail", adReadAll
    s.Position = 0
    Report s, "rewind_eos", Err.Number, Empty
    s.SetEOS
    Report s, "set_eos", Err.Number, Empty

    Set b = OpenStream(adTypeBinary, "")
    If Not b Is Nothing Then
        b.LoadFromFile path
        Report b, "load_binary", Err.Number, Empty
        b.CopyTo s, -1
        Report b, "copy_binary_to_text", Err.Number, Empty
        WScript.Echo "vbs target=binary_to_text" & State(s)
        s.Position = 0
        Report s, "rewind_from_binary", Err.Number, Empty
        ReadText s, "read_from_binary", adReadAll
        b.Close
        Report b, "close", Err.Number, Empty
    End If
    s.Close
    Report s, "close", Err.Number, Empty
End Sub
