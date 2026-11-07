# Status-bar drawing DC trace (2026-11-07)

Original, AI-assisted (Codex), LGPL-2.1-or-later. Diagnostic only, not a
fix, not in the production `patches/series`. No Windows expectation is
invented and no drawing result, invalidation or geometry is changed.

The maintainer's external `winsnap` measurements found clean status pixels
before ray tracing, overlapping pixels afterwards while maximised, and clean
pixels after Restore Down. Before/after geometry, styles, DPI, the observer's
DC origin and all 30 visible-child tree records were identical. All three
screen-DC images were black. No raw application logs or images are committed.

The next measurement must inspect the **actual drawing DC**, rather than
another `GetDC` obtained by an observer. `+sbdiag` logs only DCs whose DCE
window has class `msctls_statusbar32`:

- `SBSTATE`: logical ExtTextOut arguments, DC/window association, DCX flags,
  visible/device rectangles, mapping origins/extents, world-to-viewport
  matrix, background/text colours, text alignment and selected driver table.
- `SBREGION` and `SBRECT`: visible, application, meta, effective and DIB
  clipping, including a rectangle count/hash and the first eight rectangles.
  A missing application clip is different from an empty effective clip.
- `SBSURFACE`: window-driver presence, surface/bitmap identity, dummy flag,
  owning HWND, surface rectangle, DIB mapping rectangle and stride.
- `SBFILL`: explicit ETO_OPAQUE and implicit background fills, with the
  device-space rectangle passed to the selected driver and its real return.
- `SBTEXT`: device-space glyph origin/count at dispatch. `SBRETURN`: the
  public ExtTextOut result. In existing Wine the explicit fill result is
  ignored when returning from a zero-character call; this trace preserves
  that behaviour and reports both results separately.

No strings, glyph IDs, pixel data, titles or paths are logged by this channel.
Driver dispatch is traced, not each downstream backend instruction. Memory-DC
rendering without an associated status HWND, unrelated drawing APIs and host
compositor state are outside its coverage. Region hashes are comparison aids,
not proof of equal shapes; only the first eight rectangles are printed.
Tracing costs time, including holding the GDI lock while reading/logging a
region. If the failure disappears with tracing, the measurement is inconclusive.

## Build and validation

`bash scripts/build-statusbar-dc-trace.sh NEW_OUTPUT` needs the same Linux
build dependencies as `scripts/build-wine.sh` (listed in the diagnostic
workflow). It downloads the public Wine mirror at exact wine-11.20 commit
`7298a818b79446bae153b8d5aa27235ab3703bf9` and verifies SHA-256
`9f516d63e9f814f4dad70ce0a1943211b8a6236008ac3025cf58004091c661db`
on both cached and fresh input. `WINE_SB_SOURCE_ARCHIVE` can select an offline
copy of those exact bytes. A checksum mismatch stops the build.

The script makes a private recipe with the complete production series plus
this trace, checks every patch with `--fuzz=0`, and invokes the existing Wine
builder there. It writes a separate `winum-11.20-1-sbdiag-x86_64.tar.xz`, its
checksum, and corresponding source/checksum. `SBDIAG-INPUTS`, `BUILDINFO` and
`PATCHES.txt` identify inputs and applied patches. It neither installs Wine
nor modifies a prefix. `--prepare-only NEW_OUTPUT` checks the source and
recipe without compiling. Use a new output and, if specified, a new
`WINE_SB_TRACE_WORK` directory; work is kept for inspection/rebuilding.

The **Build status-bar DC trace** workflow builds these as an Actions
artifact, not a release. The source artifact contains the actual patched
`wine-11.20/` without build products and the private `winum/` recipe, including
the original wrapper. To rebuild the already-patched source directly, use the
configure options from `winum/scripts/build-wine.sh` in a new build directory,
then `make` and staged `make install`. The wrapper assumes an ordinary winum
checkout; do not run it on the already-augmented private recipe.

Local validation: full production series plus trace applied without fuzz;
the x86_64 Unix `win32u.so` linked with `-Werror`, including all changed C
objects. Source preparation/checksum rejection and the three summary tests
passed. This is not a complete two-architecture Wine L0. The public observation
fixture and Wine/Inventor runtime checks are separate from that build check.
No Windows L1 assertion or fault fix is claimed for this trace.

Second-model review (2026-11-07): Codex/Terra read the patch in full against
Wine, checked lifetime/locking and data exposure, and found the initially
missing implicit fill path, now traced. No distinctive resemblance to a
known non-Wine implementation was identified; this was not an exhaustive
similarity search. The isolated builder/workflow were also read in full;
their metadata/provenance omissions were corrected before handoff.

See the [measurement procedure](../../../docs/wine-repro/comctl32-statusbar-erase/README.md#experiment-2-actual-drawing-dcs)
for launch, four window states, markers and the filtered report.
