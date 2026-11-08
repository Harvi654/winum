# Cross review: oleaut32-typedesc-pointee v3 (2026-11-08)

Reviewer: Claude (did not write v1-v3). Author: Codex, commit `ac57052`,
`patches/candidates/oleaut32-typedesc-pointee-v3/`.

Read in full: the candidate patch (header, `typelib.c` hunk, the whole
`test_encoded_pointee` test), its README and `series`, and the diff against
production v2. Checked against Wine 11.20 `dlls/oleaut32/typelib.c`
(`MSFT_ReadAllTypeDesc` area, `MSFT_GetTdesc`, `typedescvt_to_variantvt`) and
`dlls/rpcrt4/ndr_typelib.c` (`get_param_info`). Skimmed:
`docs/wine-repro/batch2-measure/README.md` (the Windows result behind v3).
Not done: building or running anything.

## The change

v2 turned a simple pointee encoded as `VT_ARRAY | vt` into
`VT_PTR -> VT_SAFEARRAY -> vt`; v3 stores it as `VT_PTR -> (VT_ARRAY | vt)`,
which is what Windows 10 returns for the same file (MEASURE, both
architectures). The storage (second half of the owned descriptor table),
the range checks, ordinary pointees and the `VT_VARIANT` fallback for
unsupported values are unchanged from v2. The code change is two lines.

## Findings

- **P1 (validation, not code) - consumers of the new representation.**
  Inventor's Design Accelerator (Shaft) worked with v2 (L4). Two Wine paths
  read this descriptor:
  - `ITypeInfo::Invoke` / `DispInvoke` (`typedescvt_to_variantvt`): v2 gives
    `VT_BYREF | VT_ARRAY | VT_I4` through the `VT_SAFEARRAY` case, v3 the same
    value through the default case - **identical**.
  - the typelib marshaller in rpcrt4 (`ndr_typelib.c`, cross-apartment
    calls): `get_param_info` has a case for `VT_SAFEARRAY` but none for
    `VT_ARRAY | vt`, which falls to `get_basetype` and is not a base type -
    with v3 such a parameter is likely **not marshalable** in Wine, where v2
    worked. If Design Accelerator calls the interface across apartments,
    v3 brings the failure back.
  So v3 must not replace v2 before L4 of Design Accelerator (Shaft and one
  gear generator). If it fails there, the fix is a second, rpcrt4 patch
  treating a `VT_ARRAY | vt` pointee like `VT_SAFEARRAY` (what Windows'
  marshaller must do for this file to work), not a return to v2's
  non-Windows descriptor.
- **P3 - test.** Well bounded: it edits only its own generated file, checks
  the MSFT layout before writing one byte, reads the returned pointee through
  `ReadProcessMemory`, and cleans up. Its layout assumptions (header flags at
  0x14, one type at 0x20, segment directory after the header, the typedesc
  segment as entry 9) hold for files written by Windows' and Wine's
  `ICreateTypeLib2`, as the MEASURE run showed on Windows; a layout change
  ends in one `ok(0)` rather than a wrong write.
- **P3 - header.** Follows AGENTS.md; the v3 paragraph says what changed and
  why. The "BEGIN/END winum" comment markers in the test are useful for the L1
  builder but not Wine style; acceptable here (not sent upstream).

## Verdict

Code approved. Next: L1 of the new test on Windows 10 (builder
`scripts/build-l1-typedesc-pointee.sh`), paired L2 (oleaut32 typelib,
tmarshal; rpcrt4 ndr tests), then **L4 Design Accelerator** before it
replaces v2.
