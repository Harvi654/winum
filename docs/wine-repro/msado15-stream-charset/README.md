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
| Offline Windows 10 | [Text](results/windows-10/result-x86_64.txt) | [Text](results/windows-10/result-i686.txt) |
| Vanilla Wine 11.22 | CI | CI |
| Winum 11.22-2 | [Text](results/winum-11.22-2/result-x86_64.txt) | [Text](results/winum-11.22-2/result-i686.txt) |

The offline Windows results supply the Wine test expectations. Existing
Unicode behavior is the compatibility control. Promotion requires the
Wine unit on Windows, paired Wine validation and a Nastran run with both
the msado15 and wbemdisp candidates in a fresh Inventor 2027.2 prefix copy.


### Winum 11.22-2 baseline

Both architectures returned the same operation statuses. The C
Automation call reported DISP_E_EXCEPTION with E_NOTIMPL in
EXCEPINFO.scode for the operations below. VBScript reported 0x1BD for
the unsupported calls.

| Operation | Winum 11.22-2 observation |
| --- | --- |
| WriteText with adWriteChar | Unicode succeeded. UTF-8 in both spellings, US-ASCII, Windows-1252 and ISO-8859-1 returned E_NOTIMPL. |
| WriteText with adWriteLine, ReadText with adReadLine | E_NOTIMPL for the exercised separators. |
| SaveToFile, LoadFromFile, CopyTo | E_NOTIMPL in the measured calls. |
| Position zero and SetEOS | SetEOS succeeded. |

These rows distinguish the charset failure in the Nastran trace from
other stream operations that can be measured independently.
[CI run](https://github.com/Harvi654/winum/actions/runs/45267251197).


### Offline Windows 10 reference

The two architecture reports match apart from the architecture header.
Both probes and both cscript processes exited zero. The C and VBScript
runs saved the same bytes for each charset and file case.

| Operation | Measured Windows 10 behavior |
| --- | --- |
| Empty first write | UTF-8 emits EF BB BF, Unicode emits FF FE, and the three single-byte charsets emit no bytes. |
| UTF-8 spellings | Both spellings have byte positions 3, 13, 16, 18 after the empty write, sample, CRLF line and LF line. Saved hex is efbbbf41c48de282acf09f9883420d0a430a. |
| Single-byte encodings | US-ASCII and ISO-8859-1 save 41633f3f3f420d0a430a. Windows-1252 saves 4163803f3f420d0a430a. The sample demonstrates best-fit c for U+010D and one question mark per surrogate code unit. |
| Bounded ReadText | In UTF-8, reading one unit from position zero returns A at position 4. Reading the next three returns U+010D, U+20AC and the high surrogate at position 12. |
| UTF-8 LoadFromFile | Both input files become streams of 18 bytes at position 0. CRLF read returns the sample at position 15, LF read returns Z at position 17, and ReadText(-1) returns Q at position 18. |
| Charset after Open | Succeeds at position zero. At the measured nonzero positions C reports DISP_E_EXCEPTION with 0x800a0c93, while VBScript reports error 3219. |
| SetEOS | Position zero and SetEOS leave a zero-byte stream and zero-byte saved file. |
| CopyTo | Text to binary copies the saved bytes. Binary to text raises 0x800a0c93. |

The Unicode transcript keeps a native comparison point. Existing Wine
Unicode behavior remains the compatibility control for the candidate.

### Offline Wine unit (L1)

The first Wine unit covers five non-Unicode charset spellings and
encodings, bounded and line reads, file bytes, both UTF-8 fixture forms,
truncation and both CopyTo directions. The original Wine msado15 suite
continues to cover its Unicode path.

Run the checksum-verified L1 artifact on the same offline Windows 10
VM in a fresh directory with `run-tests.bat`. Return the two
`result-msado15-stream-charset-*.txt` files and `exitcodes.txt`.
This step checks the Wine assertions against the native reference before
the runtime candidate is written.
[L1 artifact](https://github.com/Harvi654/winum/actions/runs/45269501050/artifacts/14864795757).

The offline unit passed on x86_64 and i686 with 335 checks each and
zero failures, todos or skips. Both processes exited zero.
[Offline L1 results](results/l1-windows-10/).
