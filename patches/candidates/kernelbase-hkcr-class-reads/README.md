# HKCR class reads

Windows 10 opens an HKCU-only CLSID through HKCR and through an open HKCR
CLSID handle, in the matching physical view. In a collision the user
value wins and a value present only in the machine key remains readable.

The candidate uses an HKCR handle tag for read opens in the CLSID
namespace. Machine keys back the handle whenever present, including
when the same user key exists. An HKCU-only key can back a read handle
when the machine key is absent. Merged value reads check the rights
on the backing handle and look in the matching user view. Explicit
physical hive handles have no HKCR tag. Explicit write opens and
enumeration, write, delete and notification handlers use their existing
code paths. A user-only fallback reached through the legacy
MAXIMUM_ALLOWED open grants read access.

CI models a Windows-like HKCU WOW64 layout in the isolated test prefix.
On an existing Wine prefix without that structure, Wine's registry
write path can put 32-bit HKCU registrations in its default view.
This candidate leaves the registry write path and prefix layout alone.
The measured coverage is non-elevated reads. Full HKCR merging,
elevation-specific semantics and unusual WOW64 flag changes relative
to an existing handle are outside this experiment.

| Level | Status |
| --- | --- |
| L0 | v1 built both DLL architectures with zero fuzz. v2 verification pending. [v1 CI](https://github.com/Harvi654/winum/actions/runs/45295261497) |
| L1 | Offline Windows 10 passed 57 checks per architecture, zero errors or skips. [Measurement](../../../docs/wine-repro/kernelbase-hkcr-read/README.md#exact-wine-test-l1) |
| L2 | Paired advapi32, kernelbase, ole32, combase and shell32 suites use a fresh prefix. On i686 a missing physical HKCU WOW64 CLSID subtree skips the user-only matrix. The x86_64 negative explicit 32-bit check still runs. [Paired CI](https://github.com/Harvi654/winum/actions/runs/45455927981) |
| L3 | v1 passed native assertions on both architectures with the Windows-like WOW64 subtree. v2 verification pending. [v1 CI](https://github.com/Harvi654/winum/actions/runs/45295261497) |
| L4 | The v1 overlay contributed to the successful 2026-11-24 Nastran run with the token candidate. Recheck v2 in the combined build. Full Inventor and AutoCAD smoke tests are open. [Combined result](https://github.com/Harvi654/winum/blob/236463a/patches/candidates/server-limited-token-medium-integrity/README.md#validation) |
| Cross review | Claude reviewed v1. v2 folds the exact test into the patch and removes the passing todos. Fresh-prefix L2 follows. [Review](https://github.com/Harvi654/winum/blob/bc9ad7e/docs/wine-repro/cross-review-2026-11-24-kernelbase-hkcr-class-reads.md) |
