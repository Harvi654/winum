# Cross-review: vkd3d-proton ray-payload long-vector port

Date: 2026-11-07. Reviewer: Codex. Patch author/porter: Claude.
Documentation only; no production patch, workflow or fixture is changed.

**Verdict:** the component-wise bridge is sound for the homogeneous long-vector
copy idiom exercised here. I found no new P1/P2 correctness defect in that
bounded path. This is not a general implementation of every LLVM vector or
aggregate bitcast. There is a **P2 CI gate defect**, an independently reproduced
**inherited P2 small-matrix conversion failure**, and P3 coverage/reproducibility
gaps below. Keep the inherited failure separate from regressions introduced by
this patch. Fix the CI comparison before treating it as an enforced paired
regression check; do not describe this result as complete DXR validation.

## Inputs, exact sources and clean room

Reviewed winum `main`: `4bcee94a03aa188b3de73ca11be776d448c59c53`.
Patch SHA-256:
`330b4594b12b06233ad803c2255dbca7333fe53e31881b1383a90754c6b6fe08`.

Wine context reference: upstream `wine-11.20`, commit
`7298a818b79446bae153b8d5aa27235ab3703bf9`; this is not a symbolized identity
for the maintainer's complete Inventor runtime.

Source pins independently checked against the Git objects:

| Component | Commit |
| --- | --- |
| vkd3d-proton `v3.0.1` | `3b10bd7a7ec6a7347e616cf8bea59333afec2255` |
| its dxil-spirv gitlink | `62dbb07f771534c8ce924479efdc6c8fa510361d` |
| dxil-spirv SPIRV-Tools | `64f5770f59db933d46b9cad6edc42b4186409ef4` |
| dxil-spirv SPIRV-Headers | `f88a2d766840fc825af1fc065977953ba1fa4a91` |
| dxil-spirv SPIRV-Cross | `4b7bcb7e5cf71015b3299088d22004bfe4e13a5e` |
| dxbc-spirv | `29c93aeecd55533a357fdd7c95be5587d1c1f506` |
| dxbc-spirv SPIRV-Headers | `c8ad050fcb29e42a2f57d9f59e97488f465c436d` |

**Read in full:** `AGENTS.md`; the complete reviewed patch; its README; all six
`gpu/vkd3d-proton/tests/*.hlsl`; `.github/workflows/build-vkd3d.yml`; both status
bar/fixture investigation READMEs, including all results and the PRIME hang
backtrace. The six corresponding own synthetic DXIL containers were converted
and their emitted IR/SPIR-V inspected. They are not Autodesk shaders.

In the pinned open-source implementation I read the complete modified functions
and added helpers, pointer-cast/remapped-type handling, ordinary load/store and
GEP paths, constant/dynamic element and shuffle paths, RT incoming payload/hit
attribute setup, the complete `opcodes/dxil/dxil_ray_tracing.cpp`, temporary-copy
selection, `Operation` operand storage, and the relevant SPIR-V builder type
inspection functions. CMake inputs and vkd3d's packaging script were read;
Meson inputs were inspected for dependencies/downloads. For the overlap
assessment I read the relevant DXR pipeline-creation/dispatch, swapchain
surface/recreation/present functions, and Wine 11.20 client-surface rectangle
and Vulkan capability adjustment functions.

**Searched/skimmer scope:** unrelated converter opcode families, the rest of
vkd3d's device/command/swapchain code and Wine's window/OpenGL/DPI files were
searched, with surrounding functions read where relevant; this is not a
line-by-line audit of those entire subsystems. Earlier cross-review reports'
introductions/headings were sampled for report style. No claim is made to have
reviewed the entire upstream test corpus, Mesa, Xwayland or DXVK internals.

Only open-source source code, our own synthetic shaders and the maintainer's
recorded public-API/Wine-channel observations were used. No Autodesk shader,
binary internals, Microsoft implementation disassembly or proprietary debug
symbols were examined. Executing the public open-source DXC compiler on these
sources is not inspection of an application's shaders. Scratch changes and
new shader sources remain outside the winum checkout.

## Findings

### P2-1: CI does not enforce the promised failing baseline

**Location:** `.github/workflows/build-vkd3d.yml:47-54`, particularly line 50.

A successful baseline only assigns `b=valid`; it never sets `fail=1`. Baseline
failure of any kind is labelled `INVALID`, without retaining or checking the
validator diagnostic. The job therefore succeeds when both tools accept every
fixture. That defeats the stated negative-control check, even though patched
failures are correctly fatal. A missing/unusable baseline executable is also
indistinguishable from the expected invalid-vector diagnosis in that loop.

**Verified:** replayed the exact shell comparison with the known-good patched
converter in both slots and all six real checked-in DXIL files. Every line was
`baseline=valid patched=valid`, and the loop exited **0**. The real unpatched
build did independently fail the six inputs for the expected vector diagnostic;
the issue is enforcement, not a claim that today's baseline accidentally passes.

Before relying on this as a paired regression gate, require the intended
baseline failure and its diagnostic, require patched success, retain bounded
failure diagnostics and assert the expected fixture count/entry-point coverage.
No workflow change is made in this review task.

### P2-2, inherited: a four-component matrix copy still emits an illegal aggregate bitcast

**Location:** pinned `emit_load_instruction`/`emit_store_instruction` in
`opcodes/opcodes_llvm_builtins.cpp`; the patch changes only the `>4` branches
(patched load lines 1822-1833, store's corresponding branch).

A public HLSL counterexample, compiled with the same DXC release:

```hlsl
struct Payload { float2x2 a; float2x2 b; };
[shader("miss")] void Miss(inout Payload p) { p.b = p.a; }
```

DXC emits a `<4 x float>` load/store through bitcast pointers to the matrix
wrapper `{ [2 x <2 x float>] }`. Neither baseline nor patch reshapes this case.
Both converters exit **1** under the bundled validator:

```text
Expected input to be a pointer or int or float vector or scalar: Bitcast
%22 = OpBitcast %v4float %20
```

`%20` is an aggregate, not a legal numeric scalar/vector operand to `OpBitcast`.
The store's converse aggregate cast is likewise outside the new branch. This
is a demonstrated unsupported valid source shader, **present before this
patch too**. It is not grounds to claim that this port regressed working
small matrices, nor does it invalidate the demonstrated long-vector repair.
Track a separate aggregate-copy fix/test; do not claim this patch fixes all
matrix payload copies or silently extend its scope during review.

### P3-1: checked-in coverage is narrow, and validation is not a value oracle

**Location:** `gpu/vkd3d-proton/tests/`, workflow shader-check loop.

The six files cover closest-hit/miss incoming ray payloads, `float3x4` builtin
assignment, zero/scalar update, and matrix-to-matrix copying. `v1-objtoworld.dxil`
and `v2-local.dxil` are byte-identical (DXC optimized away the named local).
There are five distinct DXIL binaries, not six distinct lowering paths.

No checked-in case exercises ray generation/`TraceRay`, callable data or
`CallShader`, custom hit-attribute reporting, nested arrays/structs, integer,
native-half or double leaves, or the `>Operation::MaxArguments` construction
fallback (12 elements do not cross its limit of 13). None dispatches rays and
reads back a known result: `spirv-val` establishes structural validity, not
component order, matrix semantics, preserved bit patterns or execution safety.

The additional probes below improve this review's evidence but are **not**
committed regression tests. Add lasting focused cases and a small own-shader
GPU readback test before advertising broader payload support. A validation
pass on a heterogeneous struct scalarized by DXC does not test the helper's
mixed-width rejection branch.

### P3-2: source revisions are pinned, but the build environment is not fully pinned

**Location:** workflow lines 23-35 and 55-65.

The v3.0.1 clone is checked against a full root commit. Recursive submodule
checkout follows exact gitlinks, including dxil-spirv and its dependencies;
these are not floating submodule branches. Git verifies object identities.
Both GitHub actions are pinned by full commit. No curl download or fetched DXC
archive is hidden in the reviewed workflow: it consumes committed DXIL files.
The inspected CMake/Meson/package-release paths use checked-out sources and
installed tools, with no further archive-fetch step found in that path.

However, `ubuntu-24.04`, `archlinux:base-devel` and the versions obtained by
`pacman -Syu` are mutable. The normal official-repository signature/integrity
verification is not disabled, but there is no image digest, dated Arch package
snapshot or exact package-version set. This is an incomplete reproducibility
pin, not evidence that signatures/TLS are bypassed. The generated `SHA256SUMS`
hash the resulting DLLs; they do not authenticate those DLLs against previously
known inputs or make the toolchain reproducible.

HLSL is not compiled in CI, so future source/DXIL drift can go unnoticed. For
this exact commit I independently rebuilt all six with the stated DXC release:
**all six DXIL binaries are byte-identical**, so no present mismatch is alleged.
If generation is added to CI, pin the compiler archive and verify its digest.
A current native CLI build does not prove the workflow's cross-built Windows
DLL artifact or its historical CI run; neither was rerun here.

## Type, load/store and copy audit

Line numbers in this section refer to dxil-spirv `62dbb07` with this patch
applied; function names also identify the unpatched source.

- **Types (`get_type_id`, 4351-4357):** vectors of more than four scalar elements
  become a fixed-size array with the same element type/count. The change is
  global, not gated to ray-tracing storage classes. Ordinary vectors remain
  vectors. Incoming payload, callable and hit-attribute variables all use
  this type mapper, so no separate declaration path retains the illegal long
  vector for the tested cases.
- **Pointer identity/storage:** existing pointer-bitcast handling remembers
  the real pointee in `llvm_value_actual_type`, and propagates it through
  GEP/casts; existing effective-storage-class tracking is retained. The port
  correctly omits an old hunk that merely repeated the existing lookup.
- **Remapped load:** first load the real aggregate type, preserve the existing
  memory qualifiers/barrier flag, flatten it, construct the array result and
  rewrite the LLVM instruction's value ID. Consumers then receive the array
  ID, not an unused temporary. Non-remapped loads naturally use the same mapped
  array type as their pointer; other numeric casts retain the existing path.
- **Remapped store:** flatten the mapped source array and reconstruct the
  real pointee type before appending the stored value. The destination pointer,
  storage class and existing qualifiers/barrier handling remain intact.
  Both directions check for reshape failure before using an invalid result ID.
- **Shape check/recursion (1654-1756):** structs, fixed arrays, matrices and
  vectors are traversed in constituent-index order using the actual bundled
  builder's constituent count/type APIs. Both shapes must have equal scalar
  leaf counts and one common integer/float bit width. Flatten/reconstruct use
  the same traversal; unchanged leaf types reuse IDs, different equal-width
  numeric leaves use scalar `OpBitcast`. Bool/pointer/runtime-array or
  mixed-width shapes fail explicitly rather than indexing unchecked leaves.
  This preserves logical component order for the supported copy representation;
  it is not a byte-offset/padding-aware memory-layout converter.
- **Large constructs:** more than 13 constituents use an undef value followed
  by one `OpCompositeInsert` per element. Each insert needs only three operands;
  the 16-element probes exercise this path. Exactly 13 IDs fit `MaxArguments`.
  It does not raise the general IR operand limit or fix unrelated long shuffles.
- **Matrices/nesting:** DXC's payload matrices here are LLVM wrapper structs
  around arrays of short vectors, not a new mathematical transpose operation.
  The existing `ObjectToWorld`/`WorldToObject` builtin transpose stays unchanged.
  Nested members work when the copied aggregate satisfies the uniform-width,
  equal-count precondition. The small-matrix exception is P2-2.
- **Trace/callable copies:** `emit_temp_storage_copy` and
  `emit_temp_storage_resolve` in `dxil_ray_tracing.cpp` load/store whole values
  using one shared mapped pointee type; this same-type bridge needs no new
  bitcast when that pointer's mapped pointee type agrees with the value type.
  These helpers do not independently resolve a reinterpreted pointer's
  `llvm_value_actual_type`; such a case needs its own coverage rather than an
  assumption that every pointer reinterpretation works. `emit_trace_ray_instruction` and `emit_ray_tracing_call_shader`
  retain copy-in/copy-out around their respective operations. Incoming callable
  data uses `IncomingCallableDataKHR`; incoming ray payload uses
  `IncomingRayPayloadKHR`. The supplemental library validates both entry-point
  kinds and `OpExecuteCallableKHR`/`OpTraceRayKHR`. It does not establish that
  every temporary-storage selection branch was dynamically exercised.
- **Hit attributes:** incoming closest-hit/any-hit attributes and outgoing
  intersection attributes use the mapper. `emit_ray_tracing_report_hit`
  copies the whole identically typed attribute before `OpReportIntersectionKHR`.
  A 24-byte `float2x3` custom-attribute probe validates reporting and consumption
  without exceeding DXR's 32-byte attribute limit. No device execution is claimed.
- **Other consumers/limits:** constant-index composite extract/insert and
  same-type composite copies remain suitable for arrays. The untouched dynamic
  element path uses `OpVectorExtractDynamic`, shuffle uses `OpVectorShuffle`,
  and generic arithmetic/select/numeric bitcast lowering is not converted into
  array operations. Do not interpret the global type change as full arbitrary
  long-vector SSA support. I did not reproduce a valid DXC HLSL shader taking
  those unsupported long-vector arithmetic paths, so this is a scope limitation,
  not a newly demonstrated regression. Physical-layout flags/ArrayStride and
  padding-aware reinterpretation also remain outside this logical-copy proof.

## Independently executed checks

The patch applied with `--fuzz=0` to the exact root/submodule revision above.
Native Release CLI builds before and after the patch completed. Validation used
the shipped SPIRV-Tools via `--debug-all-entry-points --validate`, Vulkan 1.3
with the converter's scalar-block-layout option. Generated IR/SPIR-V was
inspected only for these public synthetic inputs, never application shaders.

The original six inputs all exit **1 before / 0 after**, with the expected
illegal 12-component type diagnostic before the patch. This reproduces their
README claim independently, rather than inferring it from historical CI.

Supplemental original probes, compiled with `-T lib_6_3`:

| Probe | Before | Patched | What it actually covers |
| --- | --- | --- | --- |
| nested `float4x4` copy/update | invalid | valid | 16-element reshape, insert fallback, nested struct |
| two `float3x4` array members plus `uint4`, dynamic array selection | invalid | valid | nested arrays/GEP, matrix copies; integer member handled separately |
| `float2x3` custom hit attributes | invalid | valid | intersection `ReportHit`, closest-hit attribute-to-payload copy |
| callable + raygen + miss library with two `float4x4` fields | invalid | valid | callable incoming data, CallShader, TraceRay and subsequent data use |
| `double4x4` copy | invalid | valid | 64-bit scalar leaves |
| `uint4x4` copy | invalid | valid | integer scalar leaves |
| native `float16_t4x4` copy (`-enable-16bit-types`) | invalid | valid | 16-bit float leaves; emitted `OpTypeFloat 16` checked |
| mixed double/float struct copy | valid | valid | ordinary scalarized load/store control; not mixed-width reshaping |
| `float2x2` copy | invalid | invalid | inherited aggregate bitcast failure, P2-2 |

These are converter/validator results, not GPU results or a numerical execution
oracle. No complete upstream regression suite or Windows DLL build was run.

The public DXC archive was `v1.8.2505.1/linux_dxc_2025_07_14.x86_64.tar.gz`,
SHA-256 `f2213da1fc99dc8778c8823078e16ba97c7f80f86a1d4520ab1adf4b462bc48c`,
verified against the digest published in that GitHub release's asset metadata
before extraction/use. Version output identifies build `4950-b106a961`.
The same binary rebuilt the six checked-in DXIL files byte-for-byte. Scratch
sources/results are under `/workspace/shared/vkd3d-review-probes`; converter
source/build under `/workspace/shared/vkd3d-review-20261107` and
`/workspace/shared/vkd3d-review-base`. The latter was rebuilt in place after
preserving the baseline executable/shared library separately; baseline probes
explicitly used that preserved baseline library, not the rebuilt patched one.
Only this English review is committed, not those artifacts or downloaded tools.

## Assessment of the status bar overlap

### Position and causal boundary

**The ray-payload patch cannot directly change HWND geometry, a DC origin,
window regions, DPI state or host window-surface placement.** It changes shader
type/value emission; it has no window-system call or swapchain creation path.
An incorrect shader can produce bad shader results or a GPU fault, but that
is not an identified mechanism for shifting unrelated GDI status text by a
window-frame offset. There is no evidence here for such a causal link.
The patch can indirectly enable Inventor's previously failing RT mode, causing
Inventor to exercise additional window/presentation behavior. Enabling the
trigger is different from implementing the faulty geometry change.

DXR pipeline creation (`libs/vkd3d/raytracing_pipeline.c`) and `DispatchRays`
(`command.c:20112`) create/use pipelines and record `vkCmdTraceRaysKHR`; they
do not call SetWindowPos, reparent windows or change a surface's client origin.
GPU workload/timing can expose an existing driver/synchronization problem,
but I do not think DXR dispatch itself is a window-geometry operation.

**The generic DXGI/WSI path can affect surfaces and presentation.** vkd3d's
`swapchain.c` creates a surface through `IDXGIVkSurfaceFactory`, queries its
capabilities, recreates swapchains using `currentExtent` and selects present
modes. These are independent of the payload patch. Wine 11.20's
`win32u/vulkan.c` surface creation uses/registers a window client surface and
sets its window pixel-format state; its capability adjustment derives extents
from the HWND's present/client rectangle. `win32u/window.c:335`,
`get_client_surface_rects`, explicitly combines child-to-top-level mapping,
client versus visible-frame offsets and DPI conversion. These are concrete
places where a wrong offset could manifest, not evidence that a particular
one is broken. DXGI/application window changes and WSI updates warrant
measurement; a geometry bug has not yet been localized.

The strongest current working hypothesis is a **coordinate/clip/surface-state
transition specific to Inventor's render-mode switch**, exposed by maximization.
It fits the few-pixel vertical separation, other displaced UI and correct
restored state better than a missing erase alone. Identical ExtTextOut/DrawText
calls and unchanged paint boxes do not prove identical DC origin, transforms,
clipping, window region or final composition. Do not infer the exact faulty
layer from those boxes alone. The forthcoming comctl32 classic correction
addresses a separately measured divergence, not this composition diagnosis.

### What the fixture results establish, and what they do not

RX 580 results show that simple Vulkan/D3D12/GL/concurrent GL+D3D12, tested
nesting and those EGL/GLX configurations are insufficient. On RX 6400 PRIME,
status text remained clean where observed, but mixed GL+D3D12 runs were not
all completed passes: three of five repeated runs hung, two completed. Thus
neither PRIME nor concurrent APIs are ruled out under all lifecycle/timing
conditions. Do not upgrade a clean screenshot of a hung run into a 30-second
functional pass or a proof about every configuration.

The recorded main-thread frame is `opengl32+0xbe7cc` called from the fixture's
`frame_gl`; it cannot distinguish wglMakeCurrent from SwapBuffers without
symbols for the exact binary. Another thread was in
`winevulkan+0x1cefa <- d3d12core+0x60f34`; calling it the presentation thread or
naming the blocked Vulkan function is a hypothesis, not a symbolized finding.
No five-second fence error does not locate the block precisely: the UI thread
cannot reach its timer/fence checks while an earlier API call has not returned.
The last title/trace therefore does not identify the instruction at which it
hung. The untimestamp-correlated `Unsupported screen format RA24` message is
not proof of a GPU reset or the cause of either symptom.

Crucially this fixture hang occurs with **stock vkd3d-proton and no DXR work**.
The ray-payload patch is unnecessary for that hang. It may indicate a useful
shared presentation/PRIME stress path, but there is no evidence yet that the
hang and Inventor's overlapping text have the same cause.

A Wine virtual desktop weakens the hypothesis that KDE's management of
Inventor's individual maximization causes the fault. It does not eliminate
Wine's own frame/client math, Xwayland/Mesa or composition of the outer virtual
desktop window. The earlier wording "not the window manager" is stronger than
that experiment alone warrants.

### What remains different from Inventor

These are hypotheses to measure, not claims about undocumented Autodesk internals:

- The fixture always presents GL and D3D12 into **different HWNDs**. An existing
  GL context/pixel format and a new Vulkan/DXGI surface on the **same HWND**,
  or reuse/reparenting of an already presented HWND, is not covered.
- One known simple flip-discard D3D12 chain, one GL context, synchronous render
  loop and one GPU frame in flight do not cover multiple/replaced swapchains,
  another queue/thread, rapid resize/destruction/recreation, dirty-region
  presentation, different sync/tearing flags, scaling/alpha/color-space choices
  or a switch between present modes.
- The synthetic fixed inset hierarchy is not the application's measured style,
  z-order, ownership, visibility, clipping or window-region history. Layered
  children/overlays and DirectComposition-style targets are untested; no
  evidence yet establishes that the failing viewport uses them.
- The fixture is system-DPI-aware. Per-monitor/thread awareness, monitor-scale
  transitions and frame/client offsets in Inventor's actual process/windows
  have not been matched. Global desktop scale is not a substitute for those
  per-window measurements.
- The fixture clears colors; it does not create an RT state object, dispatch
  rays, exchange shared render resources with GL, perform application-specific
  postprocessing or run the patched translator's long-vector path on a GPU.
  Those can change resources, load, scheduling and API lifecycle without a
  shader containing a window-geometry operation.

### Next experiments, in order

1. **Localize the coordinate error during one real mode transition.** On the
   failing setup compare restored/maximized, before/after enabling ray tracing,
   with a static known status string/scene. Collect only HWND identifiers,
   parent/owner tree, styles/exstyles, GetWindowRect/GetClientRect mapped to
   screen with ClientToScreen/MapWindowPoints, window region, GetDpiForWindow
   and GetWindowDpiAwarenessContext. Normalize the observer's DPI context so
   virtualized and physical coordinates are not silently mixed. Capture the
   status client image and an unobscured whole-window screen image without
   forcing an extra paint. Use bounded Wine `+win,+message,+opengl,+vulkan`
   output, limited message/API filters, and the existing status trace summary;
   compare actual erase/paint DC origin, clipping and transforms through public
   GDI observations/Wine DC tracing, not only the paint rectangle. The actual
   paint DC matters; a different helper's GetDC is not proof of its state.
   **Supports surface/composition mapping:** clean client image but shifted
   screen image, or a Wine client-surface offset changing by the observed text
   displacement while client geometry stays stable. **Supports GDI/DC mapping:**
   the client image is already wrong and DC origin/transform/clip changes by
   that displacement. Stable public HWND rectangles alone rule out only a
   simple application resize, not hidden surface/DC mapping. If both image
   comparisons contradict this split, retain the uncertainty rather than
   naming a faulty layer.

2. **Measure, then replay the missing window/swapchain lifecycle, without DXR.**
   At public DXGI/WGL/Wine boundaries record only target HWND, creation/resize/
   destroy order, dimensions, buffer count, format, scaling/alpha, present flags,
   reparent/style/region/DPI changes and surface identifiers around that same
   switch. Use existing DXVK/vkd3d informational logging and bounded Wine
   channels; if fields are absent, a subsequent trace-only change in the open
   source API boundary is preferable to inspecting Autodesk code. Do not dump
   shaders or vendor object state. In a later fixture task replay **only the
   observed difference**, prioritizing same-HWND GL plus a new DXGI surface,
   then any actually observed replacement/second chain or resize/reparent
   sequence. Keep workload a clear and retain a matched negative control.
   **Supports a lifecycle trigger:** the replay reproduces the offset with
   no DXR. **Weakens that specific trigger:** the matched replay remains clean;
   it does not rule out all WSI or driver behavior. Do not add arbitrary DComp,
   layered-window or present-mode combinations without evidence they occur.

3. **Separate DXR work from presentation and from the PRIME hang.** If the
   lifecycle replay stays clean, use a small original DXR scene with known
   scalar payload/readback results and fixed HWND/swapchain state; compare
   clear-only versus DXR on the same queue/presentation path, then a measured
   long-vector payload variant with the patched DLLs. The scalar case should
   work with stock and patched vkd3d, allowing a real code-version control.
   **Supports a DXR/workload interaction:** the offset appears only when rays
   run with window/API lifecycle held constant. **Supports an application
   handoff explanation:** clear/lifecycle reproduces it too, or equivalent
   DXR remains clean while the real application transition differs. A negative
   small scene does not exclude every DXR/resource/timing trigger. If the
   existing PRIME mixed-API hang recurs, treat that run as incomplete and first
   collect symbolized Unix stacks of the own fixture's same threads (Wine,
   Mesa, xcb/DRI3/Vulkan WSI); do not infer a deadlock mechanism from the current
   module offsets. No private Autodesk/Microsoft symbols or binary inspection
   are needed for any of these experiments.

None of these experiments is executed or implemented in this documentation-only
task. The next action should be experiment 1, not another broad fixture matrix
or an unmeasured Wine geometry patch.
