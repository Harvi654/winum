# ADODB.Stream text charsets

The candidate adds UTF-8 in either case, US-ASCII, Windows-1252 and
ISO-8859-1 to msado15 text streams. It uses the Windows code-page APIs,
tracks byte positions, and incorporates the measured UTF-8 BOM and
surrogate handling. Existing Unicode ReadText and WriteText use their
original byte path.

The file methods support saving and loading byte buffers. UTF-8 text
loads normalize the measured input with and without a BOM. The measured
text-to-binary CopyTo path includes the BOM, and binary-to-text returns
the ADO illegal-operation status.

| Level | Status |
| --- | --- |
| L0 | Pending candidate CI for both DLL architectures and full WoW64 paired build. |
| L1 | Offline Windows 10 passed 335 checks on x86_64 and i686 with zero failures, todos or skips. [Results](../../../docs/wine-repro/msado15-stream-charset/results/l1-windows-10/) |
| L2 | Paired msado15 suite pending. |
| L3 | Candidate C and VBScript comparison pending. |
| L4 | Pending Nastran 2027 with both wbemdisp and msado15 in a fresh Inventor 2027.2 prefix copy. |
| Cross review | Pending. |

[Measurement and exact reference bytes](../../../docs/wine-repro/msado15-stream-charset/README.md).
