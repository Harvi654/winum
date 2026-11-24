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

| File | What it does |
| --- | --- |
| `per-user-dotnet-class.bat` | Compiles `WinumRepro.cs` with the .NET Framework 4 compiler that ships with .NET 4.x (`%WINDIR%\Microsoft.NET\Framework64\v4.0.30319\csc.exe`) into `%TEMP%`, registers the class in HKCU only (`InprocServer32` = `mscoree.dll`, `Class`, `Assembly`, `RuntimeVersion`, `CodeBase`), calls `createobject.vbs` with 64-bit cscript, removes the keys and the DLL. Exit 0 = created, 1 = `CreateObject` failed, 2 = setup failed. |
| `WinumRepro.cs` | The class: `WinumRepro.PerUserClass`, `{6F3C2A51-9B7E-4C1D-8E2F-7A0B5D4C3E21}`, one method. |
| `createobject.vbs` | `CreateObject(<ProgID>)`, prints the error; the three-line form used for NEWPFUI. |

## Expected (Windows 10)

| Console | Integrity | NEWPFUI (maintainer, 2026-11-24) | `per-user-dotnet-class.bat` |
| --- | --- | --- | --- |
| normal | Medium, S-1-16-8192 | created, `err=0x0` | created, `err=0x0` (2026-11-24) |
| elevated | High, S-1-16-12288 | `err=0x1AD` | `err=0x1AD` (2026-11-24) |

The first synthetic version (`docs/archive/server-token-integrity-vbs/per-user-dotnet-class-v1.vbs`,
mscorlib's `System.Collections.ArrayList` under a new CLSID) gave
`err=0x1AD` in **both** consoles on Windows 10 (2026-11-24), so it did not
show the integrity difference. Likely reason: the class's own GUID is not
the CLSID it was registered under; a class of our own with that GUID
avoids the question. Kept as the record.

Run (needs no network; the DLL is built on the spot and deleted):

~~~bat
per-user-dotnet-class.bat
~~~

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
wine cmd /c 'C:\winum-repro\per-user-dotnet-class.bat'
~~~

(copy the three files to `drive_c/winum-repro` first). Without the integrity candidate:
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
