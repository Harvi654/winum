# ADODB.Stream text charsets

On 2026-11-23 the Nastran 2027 registration reached a UTF-8 WriteText
operation after WMI dispatch succeeded with the wbemdisp candidate. The
maintainer observed the unsupported charset trace in msado15 and the
assembly entry still lacked its installation path. This probe uses only
synthetic input and exercises ADODB.Stream independently of Nastran.
[WMI L4 checkpoint](https://github.com/Harvi654/winum/blob/be82d6e/patches/candidates/wbemdisp-method-invoke/README.md#inventor-nastran-2027-l4).

## Cases

The C program calls IDispatch directly. It also runs the original VBScript
with cscript from the matching Windows system directory. Both record
operation status, result values, and Size, Position and EOS after each
step. The C program reports each saved file in exact hexadecimal bytes.
Text return values are printed as UTF-16 code units to keep the output
independent of the console code page.

| Case | Calls and data |
| --- | --- |
| Write and save | Text Stream, each of utf-8, UTF-8, Unicode, us-ascii, windows-1252 and iso-8859-1. WriteText with adWriteChar for empty text and A, U+010D, U+20AC, U+1F603. Write B and C using adWriteLine with CRLF and LF respectively. SaveToFile uses adSaveCreateOverWrite. |
| Read back | ReadText with counts 1 and 3, adReadLine and adReadAll. ReadText results show the exact UTF-16 units. |
| Change Charset | An independent text stream changes Charset after Open at position zero. It writes text and tries changing Charset again at the resulting position. |
| Load | The two UTF-8 spellings read the same fixture with and without EF BB BF. Payload hex is 41 C4 8D E2 82 AC F0 9F 98 83 0D 0A 5A 0A 51. Read by count, by CRLF and LF line, and through end of stream. |
| Truncate and copy | Position=0 and SetEOS, then save the truncated stream. CopyTo in both directions between text and binary streams with count -1, checking both positions and the output. |

Each run creates a unique temporary directory for fixtures and outputs,
then removes it. The program prints call failures as observations. An
exit of zero means the harness completed.

## Offline Windows 10 x64

Get the CI artifact named **msado15-stream-charset-measure** and verify
**SHA256SUMS** before transferring it to the offline VM. Extract to a fresh
directory and run **run-measure.bat**. Return only these text files.

- result-msado15-stream-charset-x86_64.txt
- result-msado15-stream-charset-i686.txt
- exitcodes.txt

Build locally from the repository root with both MinGW compilers on PATH.

~~~sh
probe=docs/wine-repro/msado15-stream-charset
bash scripts/build-measure-bundle.sh out "$probe/measure.list"
cp "$probe/stream.vbs" out/
(cd out && sha256sum ./*.exe ./*.vbs ./README.txt ./measure.manifest ./run-measure.bat > SHA256SUMS)
(cd out && sha256sum -c SHA256SUMS)
~~~

## Results and next validation

| Runtime | x86_64 | i686 |
| --- | --- | --- |
| Offline Windows 10 | Pending | Pending |
| Vanilla Wine 11.22 | CI | CI |
| Winum 11.22-2 | CI | CI |

The offline Windows results supply the Wine test expectations. Existing
Unicode behavior is the compatibility control. Promotion requires the
Wine unit on Windows, paired Wine validation and a Nastran run with both
the msado15 and wbemdisp candidates in a fresh Inventor 2027.2 prefix copy.
