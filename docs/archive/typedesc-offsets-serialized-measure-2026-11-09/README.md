# Retired serialized offset MEASURE (2026-11-09)

The supplied native Windows run does **not** pass. Low/high reference cases
in both architectures reach S_OK for LoadTypeLibEx, GetTypeInfo and
GetTypeAttr, print alias PTR/CARRAY/PTR/CARRAY/I4 and both bounds 2, then
terminate before SUMMARY:

| Variant | Process result |
| --- | --- |
| x86_64, low and high | -1073740940 = 0xc0000374 (heap corruption) |
| i686, low and high | -1073741819 = 0xc0000005 (access violation) |

The final logged line is the scalar descriptor. The exact failing public
cleanup call is not identified by this instrumentation. Do not infer a
Microsoft implementation defect, a complete native contract, L1, or safety
of the fixture from the preceding successful getter calls. No native code,
symbols or disassembly was examined. Only our synthetic input/API output was
observed; raw logs are not committed.

Host serializer/sanitizer and cross-compile checks passed previously, but
they did not establish the complete native lifetime contract. The probe is
removed from the active VM manifest and retained here with its old README
for provenance. Do not rerun it as a conformance test or suppress the cleanup
to force exit zero.

The preceding nested-array draft was blocked by a separate open Wine writer
issue. One further fixture design will use only normal ICreateTypeLib2
serialization, with flat CARRAY elements pointing to generated record types;
it avoids nested CARRAY writer recursion and custom file modifications.
That is a measurement first. If it does not bring the result closer, stop
the z-offset attempt, retain the evidence and continue the other batch work
without inventing test expectations or an L1 marker.
