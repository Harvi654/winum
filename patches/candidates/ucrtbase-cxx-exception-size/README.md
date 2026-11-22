# x86-64 UCRT C++ exception frame size

Windows 10 x86-64 returns 32 from `__CxxQueryExceptionSize` in `ucrtbase` and `vcruntime140`. The export is absent from its legacy `msvcrt.dll`. Wine 11.22 returns 16 from all three. The UCRT candidate returns `sizeof(cxx_frame_info)`, the four-pointer record used by `__CxxRegisterExceptionObject`, for x86-64 UCRT. Independent review checked the shared CRT implementation and the `vcruntime140` forwarder. V2 explicitly excludes ARM64EC. The x86-64 code is identical to the first candidate tested in AutoCAD.

Wine introduced the x86-64 query in 2012 with `sizeof(cxx_exception_type)`, a descriptor made up of four 32-bit fields on x86-64. In 2016, Wine implemented registration using the separate `cxx_frame_info` record. The 2024 refactor consolidated the query and carried the original expression forward. The commit messages give no compatibility rationale for returning the descriptor size in x86-64 UCRT. [2012 query](https://github.com/wine-mirror/wine/commit/881374ee962bd73ba2df71b678983baa0a88610d) · [2016 registration](https://github.com/wine-mirror/wine/commit/11ccffea08f993aaf94aa1b2a36dd3594e5a9d43) · [2024 refactor](https://github.com/wine-mirror/wine/commit/e5e52a9a9fbc1c552613a8144a608d83489b67bf)

The AutoCAD 2027 Save As trial changed only x86-64 `ucrtbase.dll` in the same Winum and .NET 10.0.15 setup. The reference returned 16 and ended in fail-fast `0xc0000409`. The candidate returned 32, created the target DWG, reached the command-ended reactor, and exited normally. The paired desktop Open, QSAVE and file-dialog check is staged on the same topic branch. [Save As A/B](https://github.com/Harvi654/winum/commit/d108da9) · [Combined desktop test](https://github.com/Harvi654/winum/commit/54011be)

On 2026-11-21, both UCRT variants and Wine test binaries built for x86-64 and i386 from the patched Wine 11.22 source with `--fuzz=0`. The paired run completed 8 of 8 UCRT units on each architecture for both variants. The new x86-64 assertion failed on the baseline with 16 and passed on v2. The comparison recorded one fixed assertion and zero new failures. [Paired build and test report](https://github.com/Harvi654/winum/actions/runs/44905557665)

Run `run-measure.bat` from the `autocad-crt-size-measure` artifact on Windows 10 and return its two result files and `exitcodes.txt`. The `autocad-crt-size-ab` artifact contains both Wine test binaries under `tests/`. Run `ucrtbase-x86_64.exe cpp` and `ucrtbase-i386.exe cpp` for the native test record. The Wine regression assertion is in [`patches/tests-first`](../../tests-first/ucrtbase-tests-cxx-exception-size.patch). Native i686 measurement and the remaining consumer tests are pending before promotion to the normal series.

## Cross review (second model)

Claude Opus 5.5, 2026-11-22: approved; patch and test read in full
against Wine 11.22. The returned size is what callers allocate for the
frame record they pass to `__CxxRegisterExceptionObject`, and Wine's
`cxx_frame_info` holds four pointers (`frame_info` with `object` and
`next`, `rec`, `context`) = 32 bytes on x86-64, written in full by
`__CxxRegisterExceptionObject`. Returning `sizeof(cxx_exception_type)`
(16) let a caller's 16-byte buffer be overrun by 16 bytes - a stack
corruption that a /GS cookie check turns into exactly the fail fast
(`0xc0000409`) seen in CoreCLR during AutoCAD's Save As. The change is
limited to x86-64 UCRT/vcruntime builds (`_MSVCR_VER >= 140`), where
Windows returns 32; on i386 both structures are 16 bytes, and legacy
`msvcrt.dll` keeps 16 (Windows has no such export there). L1 on Windows
10 is still needed before the series.
