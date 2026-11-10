# mshtml document open optional result: v2 candidate

Original, AI-assisted (Codex), LGPL-2.1-or-later. Not submitted upstream.
This candidate keeps the production implementation unchanged and adds a
Windows-derived conformance test. It is not in the production series.

## Native contract and scope

The maintainer's own public-API MEASURE ran on offline Windows 10 x86_64 and
i686 on 2026-11-09. All three document states (fresh, InitNew, own HTML
stream loaded to readyState=complete) returned S_OK from open and close
with either a supplied result pointer or NULL. Supplied results were
non-NULL; parentWindow queries before open and after close succeeded with
non-NULL windows. All four processes completed three cases, api_errors=0,
exit 0. [Measurement details](../../../docs/wine-repro/mshtml-document-open-optional-result/README.md).

The Wine test selects the loaded state, using a fresh document for each
result-pointer mode. The exact helper is added beside test_doc_open in
dlls/mshtml/tests/dom.c and called once after check_ie in START_TEST(dom).
It loads our empty HTML from memory, bounds its ready-state wait to five
seconds, verifies the positive control, calls open with NULL and checks
the measured success and subsequent close/parentWindow. There are no guessed
return values or todos concealing the tested defect. A setup failure is a
failure, not a skipped success-path test.

The returned dispatch's IHTMLWindow2 query failed on Windows. Its precise
identity is unrelated to the optional output argument; this test asserts
only the measured non-NULL dispatch and releases it. It does not impose
Wine's window-result type on Windows or change the implementation's result
type as part of this fix. Other document states, result identity and DOM
content after open are outside this regression's scope.

Vanilla Wine dereferences the NULL output at entry; this test does not
skip or catch that fault as an acceptable native result. The candidate
guards both output writes and the result's AddRef, exactly as production.
Static source analysis identifies the fault; actual paired Wine runtime
pass/fail evidence remains required. Gecko must be installed and mshtml
enabled. A disabled backend cannot establish the optional-output contract.

## Standalone assertion L1

```sh
bash scripts/build-l1-mshtml-open.sh /path/to/NEW-output
```

Run run-measure.bat from a fresh directory in offline Windows 10. Return
result-mshtml-open-l1-x86_64.txt, result-mshtml-open-l1-i686.txt and
exitcodes.txt, including any failed or incomplete process. A complete
successful assertion run should report 34 checks and zero failures, exit 0.
The MEASURE's successful API calls do not substitute for assertion L1;
the completed assertion results are recorded below.

The builder checks byte-for-byte equality of the named function in the
patch and test-body.h, without BEGIN/END markers. l1.c compiles that body
unchanged; CoInitialize matches START_TEST(dom), and a separate 30-second
watchdog bounds the standalone process including COM cleanup. No vendor
files are needed in the bundle or copied from the VM. The main measurement
list also contains mshtml-open-l1; use the dedicated builder to verify body
parity before accepting its native results.

## Applying and validation

Use the candidate patch in place of mshtml-document-open-optional-result.patch
at its existing position in patches/series, keeping every other entry.
Apply with patch -p1 --fuzz=0. Do not apply both versions or change the
production series. The implementation hunks are byte-for-byte identical
to production; only the header and dom.c test are added.

- Standalone x86_64/i686 builds with -Wall -Wextra -Werror passed and bundle
  checksums verified on 2026-11-09. Compilation is not native execution.
- Full-series apply and complete DLL/test build results are recorded below.
- Native assertion L1 passed on both architectures (34 checks each); formal
  cross review, Gecko-enabled paired mshtml.dom L2 and consumer validation
  are pending. The production page marker stays
  missing until the native-tested candidate can be reviewed and promoted.

A read-only second-model advisory checked both complete original sources
and their relationship to the public Wine dom.c/htmldoc.c tests. It found
balanced ownership, recommended matching CoInitialize and gating on a usable
pre-open window, and found no conspicuous copied nontrivial block. Both
adjustments are included. This limited advisory is not proof of originality,
formal cross review or native L1.

## Focused build result (2026-11-09)

All 62 production series entries applied with --fuzz=0, substituting this
candidate at the existing mshtml entry. Both complete mshtml.dll and
mshtml_test.exe PE targets linked for x86_64 and i386. The source was the
pinned upstream Wine 11.20 commit 7298a818b79446bae153b8d5aa27235ab3703bf9
archive (SHA-256 9f516d63e9f814f4dad70ce0a1943211b8a6236008ac3025cf58004091c661db).
The build used --enable-archs=x86_64,i386 --without-x --without-wayland
--without-freetype. This is focused compilation, not a complete graphical
Wine build, native assertion L1 or runtime execution. The standalone L1
builder's function-parity check and both -Wextra -Werror builds also passed;
all bundle hashes verified. Implementation hunks match production exactly.


## Windows assertion L1 passed (2026-11-09)

The maintainer returned the combined follow-up bundle results and a separate
dedicated L1 result set. Both sets are identical: x86_64 and i686 each report
`SUMMARY checks=34 failures=0`, no FAIL lines, exit 0. These are executions
of the candidate's exact named function under CoInitialize, including both
the supplied-output control and NULL-output case on loaded documents.
Native assertion L1 is complete, not inferred from the earlier MEASURE.
Formal cross review, Gecko-enabled paired Wine runtime validation and
consumer gates remain open. Production patches and series are unchanged;
the production marker remains missing until a reviewed candidate is promoted.
Raw logs are summarized rather than committed.

## L2: test-run/all-1 (2026-11-10)

[docs/wine-repro/l2-all-1-2026-11-10](../../../docs/wine-repro/l2-all-1-2026-11-10/README.md), run 42788810738, with every current candidate: not covered: mshtml is disabled in the test series and in no group (cross review P2).
