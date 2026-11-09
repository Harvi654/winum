# Blocked typedesc offsets test draft (2026-11-09)

Preserved work in progress; **do not run or promote this draft as L1/L2**.
Neither the candidate nor the standalone program is in production or the
VM bundle manifest. Nested native-generated aliases were measured correctly
on Windows, but this constructor is unsuitable for an isolated Wine reader
regression test without addressing the separate writer path below.

An advisory second-model review, followed by source inspection, found that
Wine 11.20's WMSFT_append_arraydesc saves an address inside arraydesc_seg,
recursively appends the element description, then writes through that saved
address. For CARRAY -> PTR -> CARRAY, the recursive append can realloc/move
the same segment, invalidating that address. This pre-existing writer issue
can crash or corrupt fixture creation before the z-offset reader is tested.
The production reader fix is unchanged by the draft and does not fix it.

Both standalone architectures compiled with MinGW GCC 14.2 and
-Wall -Wextra -Werror; the complete 62-patch series with this replacement and
pointee v3 applied with --fuzz=0. Both oleaut32 DLL and full test EXE targets
linked in a reduced no-X/no-Wayland/no-Freetype compile profile. None of those
checks establishes safe runtime generation or L1/L2.

Additional advisory hardening suggestions, still open in this archived body:
stop traversal after a non-alias TYPEATTR and stop loading after a failed
serialized-coverage audit. No conspicuous copied-code block was identified
in the permitted open Wine/project sources; this is not proof of originality
and not Claude's formal cross review.

Next: construct the boundary fixture using only a small native PTR(I4) seed
and our own serialized segments, avoiding nested array serialization in Wine.
Measure that generated fixture on Windows before choosing final assertions.
The draft and its old README are retained for provenance, not as current
instructions. No production patch, validation marker or series was changed.
