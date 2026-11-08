# L1 bundle of the eight existing tests (2026-11-08, first VM run)

Bundle: `l1-bundle/` of artifact `tests-series-g1`, run 42483162074
(series at `d920b73`), Wine's own test programs built with `patches/series`.
Run in the maintainer's offline Windows 10 VM with `run-tests.bat`. The VM
restarted by itself during the run, so only part of the results came back,
some files cut off (written data lost at the hard reset) and
`exitcodes.txt` empty. Results below are copied as failure and summary lines
only: the full outputs list the VM's SIDs and computer name and are not
committed.

| Unit | Arch | Result |
| --- | --- | --- |
| `advapi32:security` | x86_64 | no failure up to the cut (file ends in the token dump, no summary line) - incomplete |
| `advapi32:security` | i386 | cut after 10 lines - incomplete |
| `msxml3:domdoc` | x86_64 | **complete: 36828 tests, 1 failure, 3 skipped** - `domdoc.c:12959: Test failed: Unexpected hr 0x1.` |
| `msxml3:domdoc` | i386 | empty |
| `ntdll:exception`, `ole32:storage32`, `shell32:shlfolder`, `shell32:shelldispatch` | both | not reached / lost |

## Finding: `msxml3-parseerror-filepos` differs from Windows

`domdoc.c:12959` is the first check the patch adds: for a new document
(no parse error) `IXMLDOMParseError::get_filepos` returns **S_FALSE (1)** on
Windows 10, with `filepos` 0 (the next check, `filepos == 0`, passed); the
patch returns S_OK. The NULL-pointer check passed. The patch and its test
must follow Windows: a MEASURE of `get_filepos` (with `get_line`,
`get_linepos`, `get_errorCode` for comparison) for a new document, after
`loadXML("")` fails (the SOLIDWORKS case) and after a real parse error,
then v2 of the patch.

The `msxml3-domdoc-importnode` checks (`test_importNode`, after line 19037)
passed on Windows x86_64: the file has no failure there.

## Next

Run the remaining units one at a time to find which one restarts the VM
(likely suspects: `ntdll:exception`, which exercises debug registers and
extended CPU state). Windows Event Viewer (System, source "Kernel-Power"
41 or "BugCheck") tells whether Windows crashed or the VM was reset from
outside.

## Second run, one unit at a time (Windows 10 VM on a Mac, 2026-11-08)

| Unit | x86_64 | i386 |
| --- | --- | --- |
| `advapi32:security` | 3653 checks, 0 failures | 3666 checks, 4 failures in upstream `test_system_security_access` (lines 6804-6836, registry SACL access under WOW64), not in the patch's test |
| `msxml3:domdoc` | 1 failure (parseerror, above) | the same 1 failure |
| `ole32:storage32` | 19102, 0 failures | 19102, 0 failures |
| `shell32:shlfolder` | 3730, 0 failures | 2083, 0 failures |
| `shell32:shelldispatch` | 480, 0 failures | output lost - re-run |
| `ntdll:exception` | restarts the VM | restarts the VM |

The VM restart comes from `ntdll:exception` (also on the second hypervisor);
its patch check is repeated standalone in `docs/wine-repro/ntdll-xstate-locate/`.
L1 recorded for kernelbase-account-domain-sid, msxml3-domdoc-importnode,
both ole32 patches and shell32-getimage.
