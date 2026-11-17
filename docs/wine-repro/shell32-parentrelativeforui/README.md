# Measure shell display names for the Inventor Open breadcrumb

## Symptom on Wine

The maintainer checked Inventor with separate prefixes on Arch Linux/KDE
using winum-11.22-1 (Wine 11.22). The Inventor project's main branch pins
winum-11.20-2.

| Inventor | Open at `D:\` | Open in a subfolder of `D:\` |
| --- | --- | --- |
| 2026 | `>` separators with no folder names | Same symptom |
| 2027 | `>` separators with no folder names | Same symptom |

## Trace and hypothesis

Two Inventor 2027 runs in a prefix copy provided the following filtered
trace evidence. Process and thread identifiers are masked. Counts cover
each saved run.

```text
<PID>:<TID>:fixme:pidl:SHGetNameFromIDList Unsupported SIGDN 80094001
```

| Run | Unsupported `SIGDN` | Count |
| --- | --- | ---: |
| 1 | `0x80094001` | 20 |
| 2 | `0x80094001` | 34 |

`0x80094001` is `SIGDN_PARENTRELATIVEFORUI`. In Wine 11.22,
`SHGetNameFromIDList` initializes the output name to `NULL` and returns
`E_FAIL` (`0x80004005`) for this value after successfully binding to the
parent folder. Wine's `IShellItem::GetDisplayName` delegates to that
function, so a request with the same value reaching this branch through
the interface has the same result. The observed calls to
`ShellItem_GetDisplayName` requested `0x0`, `0x8004c000`, and
`0x80058000`. The trace did not establish the caller of the unsupported
requests.

The hypothesis is that the dialog requests this form for a breadcrumb
segment and needs a nonempty display name to label it. Measure the
HRESULT and returned name for both APIs on a filesystem folder, a drive
root, Documents and This PC. The program also prints
`SIGDN_NORMALDISPLAY`, `SIGDN_PARENTRELATIVE`, and
`SIGDN_PARENTRELATIVEFORADDRESSBAR` for comparison. The output
distinguishes a null pointer, an empty string and a nonempty string.
Windows results will determine the Wine test's expected values.

## Build

From the repository root, with both MinGW-w64 cross compilers on PATH:

```sh
export MEASURE_OUT="$PWD/measure-shell32-parentrelativeforui"
bash scripts/build-measure-shell32-parentrelativeforui.sh "$MEASURE_OUT"
(cd "$MEASURE_OUT" && sha256sum -c SHA256SUMS)
```

Use a new output directory. The script builds x86_64 and i686 executables
with `-Wall -Wextra -Werror`.

## Run in the offline Windows 10 VM

Copy both executables into the offline Windows 10 snapshot that has never
been networked. In `cmd.exe`, from the directory containing the
executables, create a synthetic sample directory and run both builds.
Use a fresh output directory for a repeat measurement.

```bat
mkdir C:\winum-measure-A\BreadcrumbSample
ver
measure-x86_64.exe C:\winum-measure-A\BreadcrumbSample C:\ > result-x86_64.txt 2>&1
measure-i686.exe C:\winum-measure-A\BreadcrumbSample C:\ > result-i686.txt 2>&1
findstr /N /R "." result-x86_64.txt result-i686.txt
```

Return only the Windows build from `ver` and the MEASURE text. Mask any
user names and personal paths. Keep the HRESULTs and the distinction
between `NULL`, `""` and a nonempty string.

## Windows results

Pending. Append the Windows build, date and the masked text results for
x86_64 and i686 in this section after the offline measurement.
