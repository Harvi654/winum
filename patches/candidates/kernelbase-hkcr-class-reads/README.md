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
| L0 | Candidate build pending. |
| L1 | Offline Windows 10 passed 57 checks per architecture, zero errors or skips. [Measurement](../../../docs/wine-repro/kernelbase-hkcr-read/README.md#exact-wine-test-l1) |
| L2 | Paired advapi32, kernelbase, ole32, combase and shell32 suites pending. |
| L3 | Candidate comparison pending. |
| L4 | Pending Nastran and Inventor smoke tests. AutoCAD when available. |
| Cross review | Claude review pending. |
