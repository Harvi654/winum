# Per-user .NET COM class and the token integrity level (L3)

Reproducer for `patches/candidates/server-limited-token-medium-integrity`.
Our own script text only; no Autodesk files.

The native .NET Framework COM shim (mscoree.dll / mscoreei.dll) reads the
process token's integrity level before it reads an InprocServer32
registration. At High integrity it ignores a registration that exists
only in HKCU\Software\Classes and looks in HKLM only; `CreateObject` then
fails with 0x1AD (ActiveX can't create object). Wine reported High for
every token, so every per-user .NET class failed, among them Inventor
Nastran 2027's NEWPFUI classes (0x80040154 at Generate Mesh / Run).

| Script | What it does |
| --- | --- |
| `per-user-dotnet-class.vbs` | Registers System.Collections.ArrayList (mscorlib) as a new class `WinumRepro.PerUserArrayList` in HKCU only, creates it, calls `Add`, removes the keys. Exit 0 = created, 1 = failed, 2 = fixture already present, 3 = cleanup error. |
| `createobject.vbs` | `CreateObject(<ProgID>)` and prints the error; the generic three-line form used for NEWPFUI. |

## Expected (Windows 10, measured by the maintainer 2026-11-24 with the NEWPFUI classes)

| Console | Integrity | Result |
| --- | --- | --- |
| normal | Medium, S-1-16-8192 | created, `err=0x0` |
| elevated | High, S-1-16-12288 | `err=0x1AD` |

The synthetic class is still to be run in the VM in both consoles
(`cscript //nologo per-user-dotnet-class.vbs`); the table gets its rows
then.

## Wine

The ProgID and CLSID exist only in HKCU, so Wine also needs the
per-user class candidates (`combase-per-user-classes`,
`kernelbase-hkcr-class-reads`) to get as far as mscoree. The .NET side
must be the native .NET Framework 4.8 with a native `mscoree` override
(as in the Inventor prefix); wine-mono's mscoree does not check the
integrity level.

~~~bash
export WINEPREFIX=/path/to/prefix-with-dotnet48
export WINEDEBUG=-all
wine cscript //nologo 'C:\per-user-dotnet-class.vbs'
~~~

(copy the script to `drive_c` first). Without the integrity candidate:
`err=0x1AD`; with it: `err=0x0`. For the NEWPFUI classes on the
maintainer's machine, with Nastran installed in the Inventor prefix:

~~~bash
wine cscript //nologo 'C:\createobject.vbs' NEWPFUI.NEProgressImpl
~~~

To see the integrity query in a trace (native mscoreei, observed from
outside only):

~~~bash
export WINEDEBUG=+relay
wine cscript //nologo 'C:\createobject.vbs' NEWPFUI.NEProgressImpl 2>&1 | grep -E 'OpenThreadToken|GetTokenInformation|GetSidSubAuthority|RegOpenKeyEx' | tail -20
~~~

The prefix's `HKCU\Software\Wine\Debug` may hold a default
`RelayFromExclude`; then `RelayFromInclude` is ignored.
