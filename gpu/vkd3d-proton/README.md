# vkd3d-proton patches

Not part of winum's Wine. vkd3d-proton (Direct3D 12 on Vulkan) is a
separate component that the Inventor installer pins (3.0.1); these
patches are tried here for Inventor 2027's GPU ray tracing before a home
for patched DXVK/vkd3d-proton builds is decided.

- `dxil-spirv-ray-payload-long-vector.patch`: long vectors in ray
  payloads become arrays (see the patch header).
- `tests/`: synthetic ray tracing shaders and their DXIL. `v1`-`v5` are
  original (winum, LGPL-2.1-or-later); `matrix-payload.hlsl` is the fixture
  from Autodesk-Inventor-for-Linux PR #3 (krokosik). The `.dxil` files were
  compiled with Microsoft's DXC v1.8.2505.1 (`-T lib_6_3`); Arch's
  directx-shader-compiler 1.10 aborted on these library targets.

Results (2026-11-06, dxil-spirv of vkd3d-proton 3.0.1 built natively,
`--debug-all-entry-points --validate`): all six shaders are rejected by
spirv-val without the patch ("Illegal number of components (12) for
TypeVector") and validate with it.

`.github/workflows/build-vkd3d.yml` repeats that check and builds the
Windows DLLs as an artifact. Validation so far (2026-11-06): Inventor 2027
renders with GPU ray tracing on an RX 6400 with these DLLs and the Wine
pixel format fix (docs/wine-repro/opengl32-pixel-format-alpha/README.md).
Open: a cross review of the port by an assistant that did not write it;
publishing the DLLs with a winum release (a second archive next to Wine,
pinned by SHA-256 in the installers).
