# Typedesc offsets v2 candidate

Replacement for `patches/oleaut32-typedesc-z-offsets.patch`, applied at the
same position after typedesc-pointee. It is not incremental after v1. The
offset implementation is unchanged; v2 adds the Windows-derived test and
the dated patch header. No production series or patch page is changed.

The [test and native evidence](../../../docs/wine-repro/typedesc-offsets/README.md)
cover signed-low-word and nonzero-high-word references for pointers,
CARRAY array-description offsets, and CARRAY element descriptions in a
generated 5000-alias library. The standalone `l1.c` includes the exact same
body as the Wine test and is listed in the VM bundle manifest.

The test sources are original, AI-assisted (Codex), LGPL-2.1-or-later.
Native L1 assertions, paired oleaut32 typelib/tmarshal L2, Claude's cross
review and CAD validation remain pending. The earlier native MEASURE is
the oracle, not an L1 pass. Leave the production winum-test marker unchanged
until the assertions have passed on both Windows architectures.
