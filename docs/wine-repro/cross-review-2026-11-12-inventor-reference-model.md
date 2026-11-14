# Cross-review: inventor-reference-model (four ole32/packager patches)

Date: 2026-11-12. Reviewer: Codex. Patch author: Claude (contributed by
krokosik, winum PR #1). Four rounds, on v2 to v5 of the candidate
`patches/candidates/inventor-reference-model` (now `patches/` in the series
since winum-11.20-2). The review notes are recorded as the reviewer gave
them; the resolution of each point is noted after it. The Windows
measurements referred to are in
[ole32-createfromfile-html](ole32-createfromfile-html/README.md) (`pkgprobe`
parts A-O, `propnull`).

**Final verdict (v5):** passes cross review; no remaining blocking defect in
the four patches. Known limitations stay documented: extents inferred
outside the measured configurations, approximate drawing placement,
unmeasured DBCS behaviour, no IDataObject. No distinctive implementation
recognized as copied from Wine Staging, ReactOS or CrossOver (a limited
originality assessment, not a certification).

## Round 1 - v2

Scope: all four patches, both READMEs, the candidate series and every
reproducer file read in full; the changed functions and their callers
checked against the official Wine 11.20 archive (SHA-256 checked); the
normal series plus the candidate applied with `--fuzz=0`. No builds or
tests rerun by the reviewer.

Verdict: `ole32-storage-localdocs` and `ole32-propset-null-strings` look
sound; the two Package patches need revisions and further measurements.

1. **High - Package path checks permit a destination-buffer overflow.**
   `PersistStorage_Load()` appends the extracted basename to `%TEMP%` in
   `This->filename[MAX_PATH]` without checking the combined length;
   collision suffixes grow it unchecked. Older than the patches, but the
   new `OleCreateFromFileEx()` fallback exposes it.
   *Resolved in v3/v4:* the name must fit, a numbered name only when
   needed; else E_FAIL as on Windows (measured, pkgprobe H and K).
2. **Medium - the writer accepts Unicode paths its loader rejects.** The
   UTF-16 length is checked, but the CP_ACP form can exceed 260 bytes on a
   multibyte code page, and the loader rejects `filenameA_len >
   sizeof(filenameA)`.
   *Resolved in v4:* the loader reads such a package through the UTF-16
   name; the writer imposes no unmeasured ANSI limit. DBCS behaviour on
   Windows stays unmeasured (the test VM uses code page 1250).
3. **Medium - effective size limit 1 GiB vs. the writer's ~2 GiB.**
   `keep_native_data()` grew a buffer to 0x40000000 and then returned
   E_OUTOFMEMORY, a new memory-proportional limit.
   *Resolved in v3:* the stream copy goes to a temporary docfile (Read and
   Write only); the writer copies the file in chunks.
4. **Medium - saving to a fresh storage omits `\1CompObj`.**
   `PersistStorage_Save` wrote only `\1Ole10Native`; `OleSave()` writes the
   CLSID but not `\1CompObj`; `test_save()` did not check it.
   *Resolved in v3:* measured (pkgprobe A: Save itself writes the class and
   `\1CompObj` "OLE Package"); implemented and tested.
5. **Medium - view/advise behaviour beyond the measurements.** SetAdvise
   stores flags but never notifies (ADVF_PRIMEFIRST); Draw and the extent
   methods accept every aspect and index; extents assume 96 dpi; EnumAdvise
   returned S_OK with a NULL enumerator; no test exercised IViewObject2.
   *Resolved in v3-v5:* measured (pkgprobe B-D, I, J, L, M): no
   notification for PRIMEFIRST, the same extent and Draw S_OK for every
   aspect and index, EnumAdvise E_FAIL before Advise and E_NOTIMPL with a
   NULL enumerator after; the extent follows the icon and label at the
   screen's DPI; tests added. (The reviewer withdrew the PRIMEFIRST and
   EnumAdvise objections in round 2 once the measurements were in.)
6. **Medium - the fallback predicate needs evidence.** `embed_by_class()`
   keeps compound files and Insertable classes; whether every other class
   should bypass its moniker/IDataObject path was not measured;
   OLERENDER_FORMAT reported success while skipping caching.
   *Resolved in v3:* measured (pkgprobe F, G): only an `Insertable` key
   under the CLSID keeps a class's own path, a ProgID marked Insertable
   still gives a Package; OLERENDER_FORMAT stores no cache on Windows
   either. The ProgID check was dropped; tests added.
7. **Lower - error reporting loses distinctions.** Every
   `GetFileSizeEx()` failure became STG_E_MEDIUMFULL; allocation, read
   failure and a short read all E_FAIL.
   *Resolved in v3:* separated; measured (pkgprobe E): a missing file, a
   directory, a locked file and an overlong path give STG_E_FILENOTFOUND.

Also: refcounts, threading (apartment-threaded packager) and pointer widths
found sound. Test robustness: fixed temporary names, a fixed missing
directory, setup failures followed by unconditional pointer use. Two
accuracy points: the storage v2 note on "no output-pointer check" was
imprecise; the NULL-string patch changes the private
`PropertyStorage_PropVariantCopy()`, not the exported `PropVariantCopy()`
(both corrected in the headers). Before promotion: `docs/patches/` pages
with markers, v3 L2 and L4 records, the other consumer's L4 status.

## Round 2 - v3

Verdict: v3 resolves most first-round findings; the Package patches are not
yet approved; the storage and NULL-string patches have no blocking
findings.

1. **P2 - the temporary-path guard rejects valid paths.** v3 reserved 14
   characters for a collision suffix before trying the original name.
   *Resolved in v4:* the original name's capacity is checked first, a
   suffix only on collision; measured at lengths 251-261 with and without
   a collision (pkgprobe K) and tested.
2. **P2 - Draw replaced the caller's mapping without converting the
   bounds.** *Resolved in v4:* the bounds are mapped to device pixels first
   and the DC is restored; measured on a scaled, offset DC (pkgprobe L);
   the test checks that moving the origin moves the drawing by as much.
3. **P2 - extents deliberately differed from the measured values.**
   *Resolved in v4:* the layout matches every measured case (two icon
   title font sizes, three names).
4. **P2 - DBCS behaviour presented as measured.** *Resolved in v4:* the
   header no longer claims it; see round 1, point 2.

Smaller test issues - shared registry names between concurrent runs, the
sink returned by GetAdvise not released, NULL-string coverage (both
property-set kinds, replacement), the fixed missing directory, no test that
draws - all addressed in v4 (per-process names, release, Unicode and ANSI
sets with replacement, a per-process directory, the mapped-DC drawing
test).

## Round 3 - v4

Verdict: v4 resolves the main v3 findings; two issues remain before the
whole series is approved; the storage and NULL-string patches pass.

1. **P2 - the filename bound counted an existing terminator twice.** In the
   ANSI fallback `MultiByteToWideChar()` returns a length that includes
   the NUL, so a package without the UTF-16 names was refused one
   character too early.
   *Resolved in v5:* trailing terminators are dropped before the length
   check; Windows has the same limit with and without the UTF-16 names
   (measured, pkgprobe O, x64 and x86); a test covers both formats.
2. **P2 - exact extent assertions extrapolated beyond the measured
   configurations.** *Resolved in v5:* exact values only for 96 dpi,
   32x32 icons and a Segoe UI icon title font 15 or 25 pixels high,
   skipped elsewhere; the rule is stated as an inference.

## Round 4 - v5

Verdict: **v5 passes cross review**; no remaining blocking defects. Both
round-3 findings are resolved. v5 compared with v4 against the
surrounding Wine 11.20 code; the new tests and the measurement program
reviewed; the full series plus v5 applies with `--fuzz=0`. No new cleanup,
refcount, bounds or architecture issues. The recorded L1, paired L2 and
Inventor L4 support promotion (not rerun by the reviewer).
