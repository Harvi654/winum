# Review before release: vkd3d-proton ray-payload long-vector patch

Date: 2026-11-12. Reviewer: Claude (also the porter of the patch, so this
is not the independent cross review - that was Codex on 2026-11-07,
`cross-review-2026-11-07-vkd3d-ray-payload.md`). Purpose: check the patch
once more before its first release (`gpu/vkd3d-proton/version.env`,
build 1) and record what the release does and does not claim.

**Read in full:** `gpu/vkd3d-proton/dxil-spirv-ray-payload-long-vector.patch`;
in vkd3d-proton `v3.0.1` (`3b10bd7a`, dxil-spirv `62dbb07`): the patched
functions `Converter::Impl::get_type_id` (vector case), `emit_load_instruction`,
`emit_store_instruction`, `resolve_llvm_actual_value_type`,
`Converter::Impl::rewrite_value`, `Operation` (`ir.hpp`: `MaxArguments`,
`add_id`, `add_literal`). Checked that the patch applies to that tree with
`patch -p1 --fuzz=0`. **Not repeated:** the conversion runs (the workflow
does them on every build) and the wider converter, which the cross review
covered.

## Findings

1. **License line was wrong (fixed).** The header said LGPL-2.1-or-later
   "like vkd3d-proton", but both changed files are dxil-spirv's and carry
   `SPDX-License-Identifier: MIT` (`subprojects/dxil-spirv/LICENSE`: MIT).
   The patch is MIT like the files; the DLLs built from vkd3d-proton are
   LGPL-2.1-or-later, so the release carries the complete source. Header
   corrected, code unchanged; the header now also says the port was
   AI-assisted and not submitted upstream.

2. **No regression for shaders that converted before.** The type change in
   `get_type_id` applies to every LLVM vector longer than 4, not only to ray
   payloads. In 3.0.1 such a vector always became an `OpTypeVector` with more
   than 4 components, which `spirv-val` rejects; the converter has no other
   path for them (searched: no long-vector handling and no
   `SPV_EXT_long_vector` in 62dbb07). So every shader the patch changes was
   invalid before; vectors of 2-4 components are untouched.

3. **Operand limit handled correctly.** `Operation` stores at most
   `MaxArguments` = 13 operands (`add_id` asserts `num_arguments <
   MaxArguments` before storing). `construct_reshape_value` uses
   `OpCompositeConstruct` up to 13 constituents and a chain of
   `OpCompositeInsert` above that (`size() > MaxArguments`), which is the
   right boundary. **Not covered by a test:** no fixture has more than 12
   scalars at one level (a `float4x4` payload member would be 16); the six
   fixtures all exercise the `OpCompositeConstruct` branch. P3, below.

4. **What the reshape assumes.** Leaves are compared by count and scalar
   width only; same-width int/float leaves are bitcast one by one, mixed
   widths or booleans make the conversion fail with a log line (before the
   patch the shader was invalid anyway). The order is the flattened order of
   the SPIR-V types the converter built from the LLVM types, which matches
   LLVM's memory order for the struct/array/vector payload types; the patch
   comment says it is not a general physical-layout bitcast, and that holds.

5. **Dynamic indexing into a long vector** (`OpVectorExtractDynamic` on what is
   now an array) or vector arithmetic on it would still be invalid SPIR-V -
   as before the patch. Not seen in the fixtures or in Inventor; not a
   regression.

## Open, not blocking the release

- P3: a fixture with more than 13 constituents (`float4x4` payload member)
  for the `OpCompositeInsert` branch; it needs DXC (`lib_6_3`) on a machine
  where it does not abort.
- Inherited from the cross review: P2-2 (`float2x2` payload copy, an
  aggregate `OpBitcast` that 3.0.1 already emits) and the P3 coverage list.
- The CI container and packages are not pinned (rolling Arch); the release's
  `BUILDINFO.txt` records the tool versions instead.

## Validation the release relies on

- Workflow: all six synthetic shaders rejected by 3.0.1 with the long-vector
  diagnostic, accepted with the patch (enforced on every build since the
  cross review).
- L4 (maintainer and testers): Inventor 2027 GPU ray tracing on an RX 6400
  (RDNA 2, 2026-11-06), an RTX 5080 (tester, 2026-11-07) and an RX 9060 XT
  (RDNA 4, RADV, 2026-11-12), with the DLLs of the workflow artifact
  (run 42339679557 for the last).
