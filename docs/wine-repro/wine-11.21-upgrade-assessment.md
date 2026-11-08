# Wine 11.21 upgrade assessment

2026-11-08, Codex. Assessment only: the production version and patch series
remain unchanged. No proprietary code, shaders or binaries were inspected.

## Sources and result

- winum `0479bc9`, `wine/version.env`: Wine 11.20, build 1; 62 production
  patches in `patches/series`.
- Wine public mirror, `wine-11.20`:
  `7298a818b79446bae153b8d5aa27235ab3703bf9`.
- Wine public mirror, `wine-11.21`:
  `2528aff26208257fdbf980666a4f0e66d47563ce`.
- The maintainer supplied the public 11.21 release notes. Source comparison:
  `git diff wine-11.20 wine-11.21` in the public Wine mirror.

All **62 production patches apply sequentially to clean 11.21 source with
GNU patch `--fuzz=0`**, including an independent dry run before each application.
Some hunks require line offsets, which are not fuzzy matching. This establishes
source applicability, **not L0**: no 11.21 build, Wine test suite or CAD runtime
test was performed in this assessment.

After the full production series, the two comctl32 status-bar v2 candidate
patches also apply with `--fuzz=0`. The status-bar DC trace patch applies in a
separate check after the production series. Neither belongs in the production
series merely because it applies.

## Changes that may help

| Upstream change | Relevance and limits |
| --- | --- |
| OpenGL drawable ownership, framebuffer sizing and remote swapchain infrastructure in win32u | Relevant to investigating child surfaces and resize transitions. This is the most interesting area for our post-ray-tracing window redraw issue, but there is no demonstrated fix. The new remote `present_opengl_swapchain()` only validates the object and returns success; this infrastructure must not be described as completed cross-process presentation support. |
| Framebuffer resize handling, including separate read/draw framebuffer attachments | Potentially relevant to EGL rendering problems. Our successful Inventor configuration uses GLX (`UseEGL=N`), so the reported EGL black-screen fix is not evidence that our GLX issue is fixed. |
| `GL_MESA_map_buffer_client_pointer` WoW64 buffer mapping | Can improve 32-bit OpenGL programs running through WoW64 when the host driver exposes this extension. It is not a general speedup for 64-bit Inventor, and extension availability on the older Intel driver must be measured. |
| oleaut32 float-to-decimal normalization boundary | A concrete .NET Framework 4.8 rounding fix (bug 60553). Useful for the native .NET 4.8 direction; separate from our typelib performance patch. No evidence that it fixes dimension input or repainting. |
| Service-account logon, token ownership, username handling; additional WMI service properties and ChangeStartMode | Worth testing with licensing and Identity Manager services. No Autodesk-specific result yet, and these changes do not replace our account-domain SID or timezone fixes. |
| WinHTTP response buffer increased to 64 KB | Fixes truncation of long headers, including signed download redirects. Helps Windows components using WinHTTP; it does not directly change a Linux Python/curl downloader. |
| PKCS#7 authenticated attribute ordering and certificate IP SAN checks | General signature and HTTPS compatibility improvements. No measured installer benefit yet. |
| Menu-bar layout recomputation, 64-bit dialog return values, ComboBoxEx null-text crash, tooltip rectangle adjustment | Useful UI correctness fixes. No evidence that they resolve the Inventor ribbon displacement, Vault shadow/menu issue or native Wayland input problems. |

The release also changes ntdll address reservation, thread allocation, SEH,
WoW64 and build tools. These increase the importance of checking both PE
architectures and service startup rather than testing only a 64-bit GL fixture.

## What this does not fix by inspection

There is no 11.20-to-11.21 source change in `winex11.drv/xrender.c`,
`win32u/dce.c` or `comctl32/status.c`. We therefore have no direct upstream
replacement for our status-bar patch or the measured XRender workaround.
Indirect effects through OpenGL surfaces remain possible and need an A/B test.
Keep the working per-application `ClientSideWithRender=N` setting until that
test demonstrates otherwise. See the [status-bar investigation](comctl32-statusbar-erase/README.md).

The small winewayland changes are not evidence that WebView, dialog input or
restore/maximize behaviour now works. Continue with Wine's X11 driver through
XWayland for the working Inventor setup. Retest native Wayland separately;
do not combine an upstream upgrade and a backend switch into one comparison.

Wine's new `d3d12!D3D12GetInterface` is a stub returning `E_NOINTERFACE`, not a
replacement for vkd3d-proton or its ray payload patch. Native d3d12/d3d12core
and DXVK dxgi remain separate pinned components. Similarly, the WineD3D Vulkan
swapchain-handle reset is not automatically a fix for the DXVK/vkd3d path.

The macOS driver changes do not apply to a MacBook booted into Arch Linux.
Wine cannot add hardware DXR support to Intel HD 4000. That machine is useful
for GDI, window transitions, OpenGL and basic supported Vulkan tests; it is
not the GPU ray tracing validation machine.

## Overlap with our patches

Eleven patches touch files that upstream also changed. File overlap does not
mean semantic conflict or supersession:

| Local patch | Upstream overlap and review result |
| --- | --- |
| `comctl32-toolbar-padding-height` | `toolbar.c`; upstream mouse capture ordering is separate from our padding calculation. |
| `comctl32-toolbar-setrows-wrapable` | Same file; row wrapping fix still applies, with line offsets. |
| `kernelbase-account-domain-sid` | Only its advapi32 test file overlaps. `kernelbase/security.c` is unchanged upstream. |
| `msxml3-domdoc-importnode` | Node helpers, private header and tests changed. Upstream `domdoc_importNode` remains a stub. |
| `wbemprox-timezone` | `builtin.c` has service-property/method changes; timezone implementation remains distinct. |
| `msxml3-parseerror-filepos` | Test file changed elsewhere; local fix still applies. |
| `wininet-http-same-position-seek` | Upstream cache/error handling differs from our seek-position guard. |
| `comctl32-ownerdraw-existing-capture` | Upstream keyboard capture ordering differs from our mouse owner-draw capture guard. |
| `win32u-child-topmost-ignore` | `window.c` changed elsewhere; local child topmost handling still applies. |
| `comctl32-themed-checkbox-buffered-paint` | Button keyboard capture changes do not replace checkbox background handling. |
| `opengl32-window-format-color-bits` | `wgl.c` adds thread-detach handling; pixel-format selection is not replaced. OpenGL tests changed too. |

No patch removal is justified by this comparison. In particular, MSI sources
and `oleaut32/typelib.c` are unchanged upstream. Clean application alone would
not establish that all 62 fixes remain necessary or behave correctly.

## Proposed migration

1. Prepare a separate topic branch and isolated build directories. Pin the
   official 11.21 archive and verify its SHA-256 before extraction. The normal
   builder currently records the downloaded archive's hash rather than checking
   it against an expected value; that distinction needs addressing for a
   verified upgrade recipe. The DC trace builder has an explicit 11.20 commit,
   archive hash and version checks: changing `version.env` alone cannot update
   it safely. Never mix individual 11.21 DLLs into the 11.20 bundle.
2. Build the complete series for x86_64 and i386, with `--fuzz=0`, using the
   normal WoW64 configuration. Run paired vanilla-11.21/patched-11.21 DLL tests,
   prioritizing the overlapping files and neighbouring server/user32/win32u/X11
   suites. Reuse the Windows measurements; do not change expected results to
   agree with a newer Wine. Candidate status-bar v2 remains a separate decision.
3. On a copied test prefix, keep vkd3d-proton, DXVK, GPU, GLX and application
   settings fixed. Compare 11.20 and 11.21: fresh maximized window without RT;
   RT rendering while maximized; Restore Down then maximize; actual minimize
   then restore. Record status text, white edges and other UI displacement.
   First retain `ClientSideWithRender=N`; test its default separately in a
   controlled run. Check the WebView home page after closing Options, including
   whether a click is still needed to refresh it.
4. Complete Inventor part/assembly/drawing/PDF/save/reopen/restart tests,
   service startup/shutdown and the other consumer's shared-DLL smoke tests.
   Use the Intel MacBook for additional GL/GDI coverage. System Wine 11.21 is
   a useful vanilla baseline there, but is not a patched winum build.
5. Promote only after the evidence is recorded, then publish a new immutable
   release through the maintainer and update consumers' version/hash pins.
   Keep the working old bundle and original prefix available during testing.

Installing Arch's system Wine does not itself replace the private gpurt-v3
Wine bundle. Which one runs depends on the launcher and its selected paths.
Do not open the production prefix with system Wine merely to check a version.

## Review scope

Read in full: supplied release notes, production series, repository rules,
the upstream diffs for overlapping files, win32u OpenGL drawable/framebuffer
changes, user32 dialog return handling, win32u menu layout, Wine's d3d12 stub,
WinHTTP buffer sizing and oleaut32 normalization. Inspected the relevant local
fixes and source paths to distinguish overlap from replacement.

Sampled rather than fully audited: the remaining upstream changes (including
the complete WoW64 mapping and d3dkmt implementation), all local patch bodies
as a set, and the build/release infrastructure. This is an upgrade assessment,
not a complete semantic cross review of every change in Wine 11.21.
