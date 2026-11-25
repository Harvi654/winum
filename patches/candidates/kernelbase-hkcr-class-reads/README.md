> **Promoted** to `patches/series` for winum-11.22-3 (2026-11-25); the
> current patch and page are `patches/kernelbase-hkcr-class-reads.patch` and `docs/patches/kernelbase-hkcr-class-reads.md`, with the
> test inside the patch. This folder stays as the record.

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
Offline Windows 10 gave the same measured HKCR reads at High integrity
as in the non-elevated run, including user-only keys. Full HKCR merging
and unusual WOW64 flag changes relative to an existing handle remain
outside this experiment.

| Level | Status |
| --- | --- |
| L0 | v2 applied with zero fuzz and both DLL architectures built. [CI](https://github.com/Harvi654/winum/actions/runs/45455927388) |
| L1 | Offline Windows 10 passed 57 checks per architecture, zero errors or skips. [Measurement](../../../docs/wine-repro/kernelbase-hkcr-read/README.md#exact-wine-test-l1). [High integrity HKCR measurement](../../../docs/wine-repro/kernelbase-hkcr-read/README.md#high-integrity-measurement) matched the non-elevated reads across all 13 cases per architecture. |
| L2 | Paired advapi32, kernelbase, ole32, combase and shell32 suites use a fresh prefix. On i686 a missing physical HKCU WOW64 CLSID subtree skips the user-only matrix. The x86_64 negative explicit 32-bit check still runs. [Paired CI](https://github.com/Harvi654/winum/actions/runs/45455927981) |
| L3 | v2 passed 57 assertions per architecture with the modeled WOW64 subtree. Native read status, markers and successful value types matched all 13 cases. Baseline Wine already produces an unusable type output from failed RegGetValueW calls; this separate behavior is outside the merge assertions. [CI](https://github.com/Harvi654/winum/actions/runs/45455927388) |
| L4 | The v1 overlay contributed to the successful 2026-11-24 Nastran run with the token candidate. Recheck v2 in the combined build. Full Inventor and AutoCAD smoke tests are open. [Combined result](https://github.com/Harvi654/winum/blob/236463a/patches/candidates/server-limited-token-medium-integrity/README.md#validation) |
| Cross review | Claude reviewed v1. v2 folds the exact test into the patch and removes the passing todos. Fresh-prefix L2 follows. [Review](https://github.com/Harvi654/winum/blob/bc9ad7e/docs/wine-repro/cross-review-2026-11-24-kernelbase-hkcr-class-reads.md) |
