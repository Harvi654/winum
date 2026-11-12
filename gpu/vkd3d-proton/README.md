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
Cross review (Codex, 2026-11-07,
`docs/wine-repro/cross-review-2026-11-07-vkd3d-ray-payload.md`): the
component-wise bridge is sound for the long-vector copy path the tests
cover. Fixed after the review: the workflow now requires the unpatched
converter to reject every fixture with the long-vector diagnostic (P2-1).
Open:
- P2-2, inherited from 3.0.1, not caused by the patch: a `float2x2`
  payload copy still emits an illegal aggregate `OpBitcast`; needs its own
  fix and test.
- P3: `v1` and `v2` compile to identical DXIL; no fixture covers TraceRay,
  CallShader, custom hit attributes, nested arrays, 16/64-bit or integer
  leaves (the reviewer's extra probes pass but are not committed); no GPU
  readback test; the CI images and packages are not pinned.
- P3 (review 2026-11-12, `docs/wine-repro/review-2026-11-12-vkd3d-ray-payload.md`):
  no fixture reaches the `OpCompositeInsert` branch (more than 13
  constituents, e.g. a `float4x4` payload member).

Further L4: an RTX 5080 (tester, 2026-11-07) and an RX 9060 XT (RDNA 4,
maintainer, 2026-11-12). License of the patch: MIT, like the dxil-spirv
files it changes (the header said LGPL until 2026-11-12); the DLLs are
LGPL-2.1-or-later like vkd3d-proton.

## Release

Released: `vkd3d-proton-3.0.1-winum-1` (2026-11-12, run 43227933384),
`vkd3d-proton-3.0.1-winum-1.tar.zst` SHA-256
`7d20a226388830a268943c6d0845e0e51f0569fd540df0e2f0153e31a838c8b7`. L4 with
the released DLLs (maintainer, RX 9060 XT, copy of the Inventor 2027 prefix,
archive checked with `sha256sum -c` and `gh attestation verify`): GPU ray
tracing panel GPU, High renders. The DLLs differ bit for bit from the
earlier test artifact (version string, rebuilt), hence this second L4.

`version.env` names the upstream release (`VKD3D_VERSION`, `VKD3D_TAG`,
`VKD3D_COMMIT`) and winum's build number (`VKD3D_WINUM_BUILD`, raised
whenever a patch here changes). The maintainer makes the signed tag
`vkd3d-proton-<VKD3D_VERSION>-winum-<VKD3D_WINUM_BUILD>` on `main`
(`git tag -s`, then `git push origin <tag>`); `build-vkd3d.yml` checks that
the tag matches `version.env` and that no release of that name exists,
runs the shader check, builds, and publishes the release with:

- `vkd3d-proton-<version>-winum-<build>.tar.zst`: `x64/` and `x86/` with
  `d3d12.dll` and `d3d12core.dll` (the layout of upstream's release archive,
  so the Inventor installer unpacks it the same way), `BUILDINFO.txt`
  (upstream commit, patch hashes, winum commit, tool versions) and
  `SHA256SUMS` of the DLLs;
- `vkd3d-proton-<version>-winum-<build>-source.tar.xz`: the complete
  source (upstream tag with submodules, patches applied, the patches,
  `README.txt` with the build command);
- a `.sha256` for each archive and a signed build provenance attestation
  (`gh attestation verify FILE -R Harvi654/winum`).

It is not marked "Latest" (that is the Wine release). A published release is
never replaced; a change is a new build number. The installers pin the
archive by URL and SHA-256 (Autodesk-Inventor-for-Linux: `VKD3D_*` in
`scripts/lib/common.sh`, `nix/inventor.nix`).
