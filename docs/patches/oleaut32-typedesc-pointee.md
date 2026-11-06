# oleaut32-typedesc-pointee.patch

**Problem as the user saw it:** Inventor's Design Accelerator (Shaft,
gears, ...) said "Failed to create shaft". Its add-in could not register
itself because a call into Inventor's API failed.

**Cause:** `ITypeLib2_Constructor_MSFT()` (`dlls/oleaut32/typelib.c`) reads
the typedesc table of an MSFT type library. For a `VT_PTR` whose pointee is
a simple type, Wine used the raw value as an index into `std_typedesc[]`.
`ApplicationAddInSite::CreateDocumentSubTypeHandler` parameter 5 is a
pointer to `VT_ARRAY | vt` (a SAFEARRAY of that type), which is past the
end of that array, so Wine read garbage (vt `0x69d8`) and
`ITypeInfo::Invoke` failed the call with `E_INVALIDARG`.

**The fix in plain words:** a simple pointee in range is used as before; a
pointee with `VT_ARRAY` becomes a pointer to a SAFEARRAY of that type;
anything else is logged with a FIXME and treated as VARIANT instead of
reading past the array.

**How it was found:** `ITypeInfo_fnInvoke failed to convert param 5 to
vt(invalid ...)` in a `+ole` log while Design Accelerator started; the
Inventor project's diagnostic build (`build-wine-dll.sh oleaut32 --diag`)
named the method. Details in Autodesk-Inventor-for-Linux,
`docs/arch-hyprland.md`, section "Design Accelerator".

## Validation

| Level | Result | Date, build |
| --- | --- | --- |
| L0 | v1 in Inventor `wine-11.20-6`; v2 full build in winum CI ([run 42179812897](https://github.com/Harvi654/winum/actions/runs/42179812897), `e8e73d4`, both architectures, no fuzz) | 2026-10-16 (v1); 2026-11-06 (v2) |
| L1 | no Wine test yet | open |
| L2 | v2 x86_64: `typelib` 7724/0, `tmarshal` 754/0 (Codex review build, same results with and without the cache candidate); i686 open | 2026-11-06 |
| L4 | v1: Design Accelerator Shaft works in Inventor 2026/2027 | 2026-10 |
| L5 | v1: in use since October 2026 | |

## Open points

- A Wine test with a type library that has such a pointee (needs a
  synthetic `.tlb`, made with our own tools, and a MEASURE of what Windows
  returns from `GetFuncDesc` for it).
- Cross review of v2: done 2026-11-06 by Codex, "OK for series"
  ([cross-review-2026-11-06-oleaut32.md](../wine-repro/cross-review-2026-11-06-oleaut32.md)).
  Its inherited P3-1 (`MSFT_GetTdesc` does not check `ctTypeDesc`) is a
  separate hardening task.

## Notes for a Wine bug report

Inventor calls a method through `IDispatch::Invoke`; its type library
stores a pointer parameter whose pointee is `VT_ARRAY | vt` as a simple type. Wine indexes `std_typedesc` with the raw value and returns
`E_INVALIDARG`; Windows makes the call. A synthetic test needs its own type
library with such a parameter.

## History

- **v1** (2026-10-16, Inventor `3e48163`): the fix; the SAFEARRAY
  description was a separate `calloc` that was never freed when the type
  library was released (one small block per such typedesc per load).
- **v2** (2026-11-06): no separate allocation. The typedesc table gets a
  second half of the same size, and the SAFEARRAY description for entry
  `i` is entry `cTD + i` of that half, so it is freed with the table in
  `ITypeLib2_fnRelease`. Loops over the table still stop at `ctTypeDesc`.
  Matters more with the type library cache draft, which loads Inventor's
  large type library many times. The rest of the patch is unchanged;
  `oleaut32-typedesc-z-offsets.patch` still applies without fuzz.
