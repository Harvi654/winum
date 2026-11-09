# msxml3-parseerror-filepos v2 (candidate)

Replaces `patches/msxml3-parseerror-filepos.patch` after review and
validation. Written by Claude, 2026-11-08 (v1: Codex, for SOLIDWORKS).

Why: v1's own Wine test failed on Windows 10 (`domdoc.c:12959`, L1 bundle
2026-11-08): without a parse error Windows returns S_FALSE from
`get_filepos`. The MEASURE in `docs/wine-repro/msxml3-parseerror-filepos/`
gives the Windows values; v2 returns S_FALSE without an error and extends
the test with the `loadXML("")` (the SOLIDWORKS case) and malformed-XML
cases. The position itself stays 0 in Wine (not recorded by msxml3;
`todo_wine` for the malformed case). The header now follows AGENTS.md.

Applies with `--fuzz=0` in series order (after msxml3-domdoc-importnode,
before msxml3-schema-cache-namespace), checked 2026-11-08.

Open: cross review (Codex); L1 of the new test (the series L1 bundle
contains msxml3 domdoc); L2; L4 in SOLIDWORKS when that work resumes (the
change only affects the no-error case, which SOLIDWORKS does not hit).

## 2026-11-09: empty string

L2 run 42552802374 (`test-run/candidates-2`) failed the new check after
`loadXML("")` (`domdoc.c:12987`, S_FALSE instead of S_OK): Wine's
`domdoc_loadXML` takes a separate branch for an empty string and never
set `doc->error`, so v2 would have answered S_FALSE in exactly the call
SOLIDWORKS makes. The candidate now records `E_SAX_MISSINGROOT`
(0xc00ce558, the code Windows returned) there. The tests were also
revised after cross review (both measured classes, all measured states).
