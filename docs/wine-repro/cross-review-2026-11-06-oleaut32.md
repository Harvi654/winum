# Cross-review: oleaut32 typedesc ownership and released-library cache

Date: 2026-11-06. Reviewer: Codex. Both patches were written by Claude.
This review changes documentation only.

The cache candidate has two reproduced **P2** regressions and must stay out
of `series`. The v2 typedesc allocation change has no new ownership blocker.
There is also an inherited **P3** bounds-checking gap, described separately.
No P1 finding was identified.

## Inputs and scope

Both project checkouts were deleted and cloned afresh after the history
rewrite; no old history was pulled or merged. Rules read: winum `AGENTS.md`,
README and recent commits; SOLIDWORKS `AGENTS.md` and current `HANDOFF.md`.

- Reviewed winum `main`: `927becd2fc42dc9de3010c2c691d1da817918b7f`.
- Typedesc v2 commit: `e8e73d4f82caa6f2aeb86b5ff06efd5c077d762b`.
- Only upstream reference: public Wine tag `wine-11.20`, commit
  `7298a818b79446bae153b8d5aa27235ab3703bf9` (tag object
  `b117f32aada84ede119f6c5e7ec385f16315d6f6`).
- The tag's `dlls/oleaut32/typelib.c` is byte-identical to that file in the
  official Wine 11.20 release archive. The cached archive was rechecked:
  SHA256 `eac8ca448f9c0087f963026b4dc276d1e2b7fbb64056edc093e59bbb7e56fe28`.

Patch SHA256 values:

| Patch | SHA256 |
| --- | --- |
| `oleaut32-typedesc-pointee.patch` v2 | `057adbd2cd0ead26f0ff7e662059a6782212f767ea1bf97485283fde25523ec5` |
| `oleaut32-typedesc-z-offsets.patch` | `034ac2c37cb661d79577ded44d738740afbf2c343fdc8386ec2ffde7b32bae58` |
| `candidates/oleaut32-typelib-cache/oleaut32-typelib-keep-released.patch` | `d01436fa41056780ccf47ea269ba5ffe2b5f9f3d109e7e4495f827fb0a205070` |

Line numbers below refer to two precisely defined applied sources:

- **S**: that Wine tag plus all 27 patches of winum `patches/series`, in order.
- **C**: S plus the cache candidate. Review-only scheduling hooks are excluded
  from these line numbers and were removed from the scratch sources afterward.

Both complete patches, the complete offset patch, their descriptions, the
cache test, bundle builder, MEASURE program and checked-in Windows results
were read. Whole-file searches of `typelib.c` covered every `pTypeDesc`,
`ctTypeDesc`, `lptdesc`, imported-library pointer and cache/lock use. The
corresponding allocation, copying, ownership, destruction, loading, creator
interface and GUID lookup functions were read in context. Unrelated Invoke,
SLTG and serializer code was inspected where it touches these descriptors;
this is not a line-by-line review of every unrelated method in `typelib.c`.
The build script, patch application helper and CI workflow were also read.
No Microsoft implementation, vendor binary, disassembly or debug symbols
were used. Runtime probes use our own synthetic TLBs and DLLs built from
Wine source, in disposable cloud prefixes; no maintainer prefix was used.

## Findings

### P2-1: unsaved edits survive the last Release and contaminate a later load

**Location:** C `dlls/oleaut32/typelib.c:5006`,
`ITypeLib2_fnRelease`'s retention condition; C:4873,
`ITypeLib2_fnQueryInterface`; C:9162, `ICreateTypeLib2_fnSetName`.
The candidate adds retention but no dirty flag or creator-interface exclusion.

Wine exposes `ICreateTypeLib2` on a library loaded from disk. Its setters
modify the same parsed object without writing the file. `reusable` remains
true, and file size/time remain unchanged. After Release reaches zero,
`LoadTypeLib` therefore resurrects the edited object instead of reading the
unchanged file. Vanilla Wine destroys that object and reads the file again.
This affects subsequent callers of the same path; the cache is no longer
just reusing a parsed representation of the file.

**How to verify:** build an own TLB with name `disk_original`, one interface,
and `SaveAllChanges`, then release its creator. In one process:

```c
LoadTypeLib(path, &tl);
ITypeLib_QueryInterface(tl, &IID_ICreateTypeLib2, (void **)&ctl);
ICreateTypeLib2_SetName(ctl, L"unsaved_edit"); /* no SaveAllChanges */
ICreateTypeLib2_Release(ctl);
ref = ITypeLib_Release(tl);                  /* observed 0 on both */
LoadTypeLib(path, &tl);
ITypeLib_GetDocumentation(tl, -1, &name, NULL, NULL, NULL);
```

The x64 probe actually ran on both uninstrumented Wine DLLs:

| Engine | Last Release | Name after reloading | Probe failures |
| --- | --- | --- | --- |
| S, no cache candidate | 0 | `disk_original` | 0 |
| C, cache candidate | 0 | `unsaved_edit` | 1 |

SetVersion, SetGuid and type-info editing have the same missing exclusion;
SetName is the minimal reproduced example. Before admission, prevent
retention of an edited object (conservatively, exposing its creator interface
could disqualify it), and add a regression case. This is a demonstrated Wine
regression, not a guessed Windows result: measure the native creator/edit
sequence before adding Windows conformance assertions for it.

### P2-2: the cache can stamp an old parsed image with a replacement file's metadata

**Location:** C `dlls/oleaut32/typelib.c:3416`, `TLB_ReadTypeLib` closes the
parsed image through `IUnknown_Release(pFile)`; C:3426 subsequently calls
`get_file_info(pszPath, ...)` and makes the old object reusable. Validation
in `released_tlb_valid` at C:2879 compares against that later path sample.

A writer can replace or rewrite the file in between. The library contains
image A, but its saved size/time describe image B. On the next load, the
metadata check succeeds against B and the old A object is reused. Even a
size-changing replacement is missed. This differs from the deliberate
limitation of two different files having equal size and timestamps.

**How to verify, deterministically:** add a scheduling barrier only to a
scratch build of each applied source, immediately after
`IUnknown_Release(pFile)` in `TLB_ReadTypeLib`. Signal a named event and wait
for a second event. The same hook is used on S and C; it changes scheduling,
not the parser, validation, refcounts or data returned.

1. Write own library A with one interface.
2. Reader thread calls `LoadTypeLib` and pauses at that barrier: A is parsed
   and its file handle/mapping has already been released.
3. Writer writes library B with three interfaces at exactly the same path,
   closes it, then signals the reader to continue.
4. Join the reader, observe its library, Release it to zero, then call
   `LoadTypeLib` again without another file change.

The x64 probe actually ran with this forced interleaving:

| Engine, same scheduling hook | First load's count | Count on disk | Next load's count | Probe failures |
| --- | --- | --- | --- | --- |
| S | 1 | 3 | 3 | 0 |
| C | 1 | 3 | 1 | 1 |

The initial load returning A is compatible with having started before the
replacement. Persistently returning A afterward is the new cache regression.
`cache_section` cannot prevent it: the writer need not use oleaut32's cache.

Before admission, obtain a stamp associated with the exact opened image
while that image is protected, or decline reuse when consistency cannot be
established. A path query after closing the image is insufficient. Cover
this interleaving with a sequence test; do not fix it by asserting a guessed
Windows race outcome. Neither repository patch was edited for this probe.

### P3-1: an inherited raw typedesc lookup does not enforce ctTypeDesc

**Location:** S `dlls/oleaut32/typelib.c:2313`, `MSFT_GetTdesc`, particularly
S:2318. Identical unchecked lookup exists in upstream Wine 11.20.
Callers include `MSFT_DoFuncs`, `MSFT_DoVars` and the alias reader.

For a nonnegative encoded offset, the function indexes
`pTypeDesc[type / (2 * sizeof(INT))]` without checking `ctTypeDesc` or offset
alignment. Thus it would be inaccurate to say that every indexed access in
the entire file is bounded by `ctTypeDesc`. With v2, an invalid file index in
`[cTD, 2*cTD)` can address the private auxiliary half; larger indices remain
out of bounds. This is an inherited malformed-input validation gap, not a
new leak or invalid free introduced by v2, and is not a blocker for replacing
v1's allocation with v2's allocation.

**How to verify:** in a source-level harness, use `ctTypeDesc = 1`, allocate
two entries and put a distinguishable vt in the second. Call the unchanged
`MSFT_GetTdesc` with encoded offset `2 * sizeof(INT)`: the second entry is
copied despite its index equalling the logical count. Alternatively mutate
only the encoded type offset in an own synthetic TLB. This verification
recipe follows directly from the source; a malformed-TLB runtime test was
not executed in this review. Track bounds/alignment hardening separately.

## Typedesc v2: ownership and lifetime audit

**a. Logical table versus allocation.** S:3523-3531 keeps `ctTypeDesc = cTD`
and allocates `2*cTD` slots. Both constructor passes stop at `i < cTD`.
The `VT_CARRAY` cleanup at S:4914 also stops at `ctTypeDesc`; there is no
bulk copy/reallocation that assumes the physical allocation equals the
logical count. Slot `cTD+i` is used only as the auxiliary SAFEARRAY descriptor
for slot i. The offset patch guards its table-reference indexes against
`cTD`, not `2*cTD`. The unchecked legacy getter is P3-1 above.

**b. Freeing.** No `free(lptdesc)` or equivalent individual-pointee free was
found anywhere in this file. The first-half `VT_CARRAY` entries own separate
`lpadesc` allocations, which are freed individually. Auxiliary second-half
entries are `VT_SAFEARRAY`, and their child points into static
`std_typedesc`. One `free(pTypeDesc)` frees the first half and the second
half together. No second-half allocation needs another destructor pass.
The cache candidate's extracted destructor keeps this same ownership.

**c. Returned descriptors.** `MSFT_GetTdesc` makes internal shallow copies,
whose nested pointers remain library-owned. In contrast,
`TLB_SizeTypeDesc`/`TLB_CopyTypeDesc` recursively size and copy PTR, SAFEARRAY
and CARRAY graphs into flat caller allocations. `TLB_CopyElemDesc` uses that
copier for function results, parameters and variables.
`TLB_AllocAndInitFuncDesc` and `TLB_AllocAndInitVarDesc` therefore do not export
pointers into the auxiliary half. Aliases are deep-copied in `MSFT_DoTypeInfo`
and again for `GetTypeAttr`. The ReleaseFuncDesc/ReleaseVarDesc/ReleaseTypeAttr
paths free those returned allocations, not the table's `lptdesc` targets.

ITypeInfo's 0-to-1 AddRef holds its parent ITypeLib; its final Release drops
that reference. The alternate dual type-info copy also holds the parent.
Moving the auxiliary descriptor inside the parent allocation therefore does
not shorten any supported descriptor or type-info lifetime. This is source
ownership analysis, not a new native Windows lifetime measurement. It does
not authorize calling a descriptor-release method on an already released
ITypeInfo interface.

**d. Following patch.** All 27 series entries applied to the public tag with
`patch --batch --fuzz=0 -p1`. Both offset-patch hunks succeeded, with a harmless
+2 line offset and **zero fuzz**. The complete cache patch then also applied
with zero fuzz (+43 line offsets on its later hunks).

**e. Build.** Both architectures of `oleaut32.dll` and
`oleaut32_test.exe` built successfully for S and C, using the full feature
configure options from winum's build, with `--enable-tests` instead of
`--disable-tests`, `--enable-archs=x86_64,i386`, MinGW/GCC 14, five make jobs.
Targets were exactly:

```text
dlls/oleaut32/x86_64-windows/oleaut32.dll
dlls/oleaut32/i386-windows/oleaut32.dll
dlls/oleaut32/tests/x86_64-windows/oleaut32_test.exe
dlls/oleaut32/tests/i386-windows/oleaut32_test.exe
```

This is a component build, **not** a newly completed full-runtime L0.
The supplied [CI run](https://github.com/Harvi654/winum/actions/runs/42179812897)
for e8e73d4 could not be independently queried here: the HTTPS proxy returned
HTTP 403. Its success is not assumed. No TLS or network-policy bypass was used.

## Cache: lock, destruction, import and eviction audit

| Path | What was checked | Conclusion |
| --- | --- | --- |
| Release, C:4994-5029 | Lock precedes the decrement and spans import releases, released-list insertion, eviction and destruction | A concurrent cache lookup cannot see the object between decrement-to-zero and insertion/destruction |
| Path load, C:3372-3390 | Validation and AddRef happen before dropping `cache_section` | Reactivation removes the object from the released list before an eviction can select it |
| GUID import search, C:8081-8099 | Released candidates are validated and AddRef'd under the same lock | Invalid/deleted file entries are skipped; a live match is pinned before unlocking |
| AddRef, C:4901-4917 | Only 0-to-1 resurrection edits the released list | Both legitimate cache resurrection sites hold the lock; ordinary holders already imply a nonzero parent reference |
| Imports, C:5013-5017 and C:8118-8131 | Drop each stored import reference and set its pointer to NULL at zero parent references; reload takes a stored reference and releases its temporary one | A retained parent adds no import reference while idle; externally held imported ITypeInfo objects retain their own parent as before |
| Eviction, C:5019-5022 | Add at head, remove tail when count exceeds eight; reactivation/destruction each decrement the count | Bound is eight **released** objects; live objects are not eviction candidates; order is least recently released |

`ITypeLibImpl_Destroy`'s core, from the cache-entry removal through
`free(This)`, is whitespace-equivalent to the cleanup statements in Wine
11.20's original final Release. It still frees the path, string/name/GUID
lists, custom data, CARRAY allocations, typedesc table, imported-library
references and names, reference records, aliases, typeinfos and their array,
then the object. The new prefix removes the released-list link/count.
There is no omitted old cleanup or extra individual typedesc free.

Import Release can recurse into the same critical section, as before;
Windows critical sections are recursive. Ordinary final Release versus Load
has no newly identified object-lifetime race under valid COM ownership.
The pre-existing duplicate-load FIXME and unsynchronized first-import
publication were not introduced by this candidate. This source audit and
the forced file interleaving are not an exhaustive thread-stress proof.

For an ordinary subsequent size/time change, path lookup destroys the idle
stale object before reopening; for deletion, reopening fails. GUID lookup
skips the stale object and tries disk/registry resolution. A stale GUID-only
entry may remain idle until path lookup or eviction; it remains within the
eight-entry bound. Equal-size/equal-time replacements are a stated limitation
of this metadata design. P2-2 is a different failure even when size changes.

## What the test proves, and what it misses

`test_LoadTypeLib_released`, C `dlls/oleaut32/tests/typelib.c:7403`, checks
successful reload after zero references, the final ITypeLib refcount,
GetTypeInfoOfGuid, changed content/count after deleting and recreating the
same path, and DeleteFile succeeding after release. These are useful
correctness checks; it intentionally passes without the optimization too.
It is not a speed or cache-retention test.

The checked-in Windows results at
[oleaut32-typelib-cache/windows-2026-11-06/](oleaut32-typelib-cache/windows-2026-11-06/)
report **26 checks, zero failures/todo/flaky/skipped, exit 0**, on x86_64 and
i686. These are the maintainer's recorded results, not a Windows rerun here.
The bundle builder extracts those functions unchanged. A newly built x64
bundle also ran here on both uninstrumented S and C DLLs: **26/0, exit 0**
on each, with successful bootstrap and cleanup. This demonstrates that the
current test passes despite both reproduced P2 regressions.

Missing coverage includes unsaved creator/type-info edits, the post-parse
stamp interleaving, final Release versus concurrent Load, imported-library
refcounts across retention/reactivation, invalidation through the GUID path,
and ninth-entry eviction/reactivation order. The final DeleteFile check does
not itself attempt another load of the now-missing file. Add focused sequence
coverage for cache internals and native MEASURE where a public API expectation
is not already established; do not replace the existing measured assertions
with guessed values.

## Existing x64 suites run during this review

Identical `oleaut32_test.exe` from the C test build (including the added
26-check case) ran on S and C in separate new prefixes under Xvfb. Only the
Wine-built oleaut32 component differs between those two runtime copies.

| Module | Checks S / C | Failures S / C | Todo S / C | Skipped S / C | Exit S / C |
| --- | --- | --- | --- | --- | --- |
| typelib | 7724 / 7724 | 0 / 0 | 43 / 43 | 1 / 1 | 0 / 0 |
| tmarshal | 754 / 754 | 0 / 0 | 19 / 19 | 0 / 0 | 0 / 0 |

Both comparisons are `no_new_failures`, including matching coverage and
failure locations; bootstrap and prefix cleanup succeeded for all four
runs. These are cache-candidate pairs against **v2 S**, not a v1-versus-v2
pair. They exercise v2 too, but do not establish a paired v2 regression
result against v1. The x64 suites do not cover the two new P2 sequences.
No i686 execution or CAD smoke is claimed here.

## Review artifacts and limits

Scratch root: `/workspace/oleaut32-review-20261106`. It holds the public tag,
S/C source trees, zero-fuzz application results, component build outputs,
the own synthetic probe source and the two event-barrier DLL variants.
All scratch source scheduling changes were restored afterward. Only this
English review document is committed; no patch or `series` entry is changed.
The reproducers use fixed synthetic names and return numeric observations;
no raw runtime output or vendor files are committed.

The component builds, existing 26-check test, x64 suite pairs and two x64
regression probes above are independently verified here. They do not establish new Windows
expected values, i686 runtime L2, full-runtime L0, either application's L4,
or L5. The validation gaps already listed in the patch descriptions remain
open unless separately supplied by the maintainer.

## Verdicts

- **`oleaut32-typedesc-pointee.patch` v2: OK for series**, as a code-review
  verdict on the v1-to-v2 allocation/ownership revision already in `series`.
  It fixes the leak without introducing a new lifetime/freeing defect, and
  the following patch and both component builds pass. P3-1 is inherited.
  This review does not mark the documented missing Windows pointee test or
  outstanding full validation as passed, and is not release authorization.
- **`oleaut32-typelib-keep-released.patch`: not ready for series.** P2-1
  returns unsaved edits to a new caller; P2-2 can indefinitely reuse an old
  image with a replacement file's stamp. Fix and cover both, then repeat
  cross-review and the remaining validation. The existing Windows 26/0
  result is valid for its scope and does not resolve these failures.

## v2 re-review

Date: 2026-11-06. Reviewer: Codex. This is a **thread-safety review**.
A fresh clone initially selected v2 `83d821f`; at the maintainer's explicit
request it was then fast-forwarded with `git pull --ff-only origin main`.
The final reviewed and tested target is **v2.1,
`de1557ceec47f3057ab7414e042aa7aff27d50c0`**, not the intermediate v2.
The entire final patch, its v2/v2.1 header paragraphs and the changed test
were read. The original report above is preserved as the v1 review.

Final candidate SHA256:
`28663e14d4a22fec9ff185699441bb15cba27e1dc1754a8cbc0707b072c2769c`.
**S** below is the same public Wine 11.20 tag plus the 27 normal series
patches; **C2.1** is S plus this final candidate. New line numbers refer to
C2.1's applied `dlls/oleaut32/typelib.c`, without scheduling hooks.

### Build and paired execution

Both source trees were prepared afresh from the public `wine-11.20` tag.
All 27 series patches and the final candidate applied with `--fuzz=0`.
Using the same full configure feature options as the first review,
`--enable-archs=x86_64,i386 --enable-tests`, MinGW/GCC 14 and five jobs,
these four targets built successfully for **both S and final C2.1**:

```text
dlls/oleaut32/x86_64-windows/oleaut32.dll
dlls/oleaut32/i386-windows/oleaut32.dll
dlls/oleaut32/tests/x86_64-windows/oleaut32_test.exe
dlls/oleaut32/tests/i386-windows/oleaut32_test.exe
```

The initially completed v2 build was followed by a rebuild from the final
v2.1 source and test text. The reported C2.1 results use that final build.
These are component builds, not a full-runtime L0. Identical final C2.1
x64 test EXEs ran on S and C2.1, each module in a new prefix/HOME under
Xvfb. Only the Wine-built oleaut32 component differed between runtime copies.

| Module | Checks S / C2.1 | Failures S / C2.1 | Todo S / C2.1 | Skipped S / C2.1 | Exit S / C2.1 |
| --- | --- | --- | --- | --- | --- |
| typelib | 7733 / 7733 | 0 / 0 | 43 / 43 | 1 / 1 | 0 / 0 |
| tmarshal | 754 / 754 | 0 / 0 | 19 / 19 | 0 / 0 | 0 / 0 |

Both pairs are `no_new_failures`, with matching coverage. All four
bootstraps and cleanups succeeded. No i686 runtime L2 is claimed.

### Original P2 probes repeated

The **unchanged original x64 probe executable** was reused for both original
cases. Its SHA256 is
`f19739797e2943e4ce5d6911d00846aca0a0c86fffa96a35015c9673d074b95c`.

| Probe | S | C2.1 | Status of original finding |
| --- | --- | --- | --- |
| P2-1: loaded-library SetName without saving, final Release, reload | Release 0; `disk_original`; exit 0 | Release 0; `disk_original`; exit 0 | Original reproducer fixed |
| P2-2: replace one-interface file with three-interface file after parsing/closing, then release/reload | First count 1, reload 3; exit 0 | First count 1, reload 3; exit 0 | Original reproducer fixed |

For P2-2, the same event barrier was added only to scratch builds of S and
C2.1, immediately after `IUnknown_Release(pFile)` (C2.1:3423).
The writer finished the replacement before the reader continued. C2.1's
pre-open sample at :3399 and post-close comparison at :3433-3434 disagree,
so that parsed object is not retained. This confirms the actual requested
interleaving; it is not a timing guess or a new Windows race expectation.

An additional own probe loaded the original type info, obtained ITypeComp,
queried it for ICreateTypeInfo2 and changed the type name without saving.
Both S and C2.1 released the library to zero and reloaded `interface0`,
not `unsaved_interface`; both exited 0. The delegated creator-QI route is
therefore covered too. Every runtime probe used an own synthetic TLB and a
new disposable cloud prefix. No maintainer prefix was used.

The checked-in `windows-2026-11-06-v2` results were also read: both
architectures report 33 checks/0 failures, with S_OK for loaded-library
ICreateTypeLib2 and SetName. The final source turns those two traces into
assertions; its exact final text has not been rerun on Windows here. The
new x64 suites above include those assertions. No native result was guessed.

### QueryInterface lifetime argument and remaining P2

The v2.1 reference-lifetime argument is **correct for the final Release
reader**: a valid library caller holds a reference, and a valid type-info
caller keeps the parent library alive through ITypeInfo's reference
(:5825-5835, :5892-5908). The final ITypeLib Release cannot reach zero while
such a caller is active. The interlocked reference operations provide the
ordering for the later final-release decision. There is no new
`cache_section` acquisition in either final creator-QI branch, so the
intermediate v2 creator-QI lock-order concern does not apply to C2.1.
This does not assert that every unrelated oleaut32 lock ordering is safe.

**P2-v2-1: concurrent creator-QI calls still race with each other.**
Exact locations: `ITypeLibImpl.reusable` is an ordinary BOOL at :1110;
`ITypeLib2_fnQueryInterface` writes it at **:4899**, and
`ITypeInfo_fnQueryInterface` writes the same parent field at **:5810**.

Two valid callers can execute those stores concurrently. For example, two
MTA threads separately load the same cached library, one keeps ITypeLib and
the other an ITypeInfo from it, then both query their respective creator
interfaces. Both hold valid references throughout; neither needs to edit
metadata or race a final Release. The two ordinary stores of FALSE are not
ordered relative to each other. The same issue exists with two library QIs
or two type-info QIs. Reference ownership prevents destruction; it does not
serialize concurrent writers. Equal stored values do not remove the C data
race, even though an aligned x86 store often gives the intended final value.

**How verified:** a minimal native pthread model retains these two field
stores, the type-info parent-reference rule and sequentially consistent
reference-counter operations. A two-thread barrier starts 100 creator-QI
iterations per caller, releasing every returned reference. The original
library reference and original type-info reference remain held until both
threads join, so the final-release reader cannot overlap either writer.
GCC ThreadSanitizer reports a **4-byte write/write data race** between
`qi_library` and `qi_typeinfo`, exit **66**. A control changing only the two
flag stores to atomic exchanges reports no race, exits **0**, and prints
`library_refs=2 typeinfo_refs=1 reusable=0` before normal final cleanup.

This is a native thread-safety model, **not** a claim that Wine's complete
DLL was built with ThreadSanitizer or that a Wine runtime failure from this
race was observed. The corresponding unsynchronized source accesses are
visible directly in the two QI functions. Reproduction sources are retained
in the scratch directory as `qi-writers.c`; builds use:

```text
gcc -g -O1 -fsanitize=thread -fno-pie -no-pie -pthread qi-writers.c -o qi-writers-tsan
gcc -g -O1 -fsanitize=thread -fno-pie -no-pie -pthread -DATOMIC_CONTROL qi-writers.c -o qi-writers-atomic-tsan
```

Before series, use a Wine-style atomic flag update, e.g. InterlockedExchange
on an appropriately typed 32-bit field, at both creator-QI stores. Preserve
the reference-lifetime reasoning for the final reader. Do not restore the
cache lock in QI merely to serialize these writes. The atomic control is
only a model; no production patch was edited or supplied by this review.

### Other ways to change or obtain the loaded object

No additional supported public metadata-editing interface bypass was found
in the inspected Wine 11.20 implementation. This is distinct from saying
that the object has no internal state changes:

- ICreateTypeLib and ICreateTypeLib2 share the guarded library QI branch;
  ICreateTypeInfo and ICreateTypeInfo2 share the guarded type-info branch.
  Their QI forwarding implementations, both ITypeComp variants, and the
  alternate dual type-info object lead back to those branches and the
  owning library. An interface request disqualifies the parent even if
  the caller subsequently uses only a base creator interface.
- GetFuncDesc, GetVarDesc, GetTypeAttr, GetLibAttr, documentation/custom-data
  getters and ITypeComp::Bind return allocated copies of the data relevant
  here. Changing those caller-owned descriptors/strings does not edit the
  parsed library. The deep-copy audit above still applies.
- GetRefTypeInfo changes imported-library memoization, and AddRef/Release
  change counters and idle-list membership. Import pointers are cleared
  while the parent is idle. These are implementation state, not unguarded
  client metadata setters. The pre-existing first-import publication and
  duplicate-load issues mentioned in the first review remain outside this
  patch's change; they are not newly proved thread-safe here.
- Parser pointer-size normalization happens during construction. The
  GetFuncDesc layout path can change internal layout, but loaded type infos
  start with `needs_layout == FALSE`; the assignments setting it TRUE are
  creator methods. Such an object has already been disqualified by creator
  QI. RegisterTypeLib/UnRegisterTypeLib change registration, not the cached
  parsed metadata. CreateDispTypeInfo creates a separate in-memory library.

Size/time equality remains a metadata-only validity condition, not proof of
content identity across arbitrary replacements with restored metadata. The
original single-replacement P2-2 case is fixed; no stronger filesystem
snapshot guarantee or exhaustive concurrent file-replacement proof is claimed.

### Thread-safety verdict

**C2.1 at de1557c is not ready for series:** both original P2 reproducers
now pass, both architecture builds pass, and the existing x64 suite pairs
show no new failures, but **P2-v2-1** remains between concurrent non-atomic
QI flag writers. The final-release lifetime argument is sound within its
scope; it does not establish writer/writer thread safety. Make the flag
updates atomic and repeat this focused check. Remaining validation gates
(full-runtime L0, i686 L2, applicable application smoke tests and exact-final
Windows test execution) are not converted into passes by this review.

Scratch artifacts: `/workspace/oleaut32-review-v2-20261106`. Original and
scheduling-hook DLLs are kept separately. Applied source scheduling hooks
were removed afterward; normal build DLL outputs were restored from the
uninstrumented copies. Only this new English section is committed. Repository
patches and `series` are unchanged; the historical report is preserved.
