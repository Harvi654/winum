# Wine 11.23 rebase audit (2026-11-27)

Status: paused before patch changes. This is an audit of the proposed
`winum-11.23-1`.

The WineHQ `wine-11.23.tar.xz.sign` verified on 2026-11-27 using Alexandre
Julliard's key with the exact fingerprint
`DA23579A74D4AD9AF9D3F945CEFAC8EAAF17519D`. GnuPG reported the same
primary and signing fingerprint in VALIDSIG. SHA-256 of the verified
tarball is `b3eeee8837383b3eefb65e5b95851a2568d1dc54702cb100207dd0082b1a227f`.

Compared the [11.22 and 11.23 tags](https://github.com/wine-mirror/wine/compare/wine-11.22...wine-11.23)
(301 commits), and byte-compared the 155 file paths addressed by the
80 current series patches and four promotion candidates in the verified
source archives. 23 paths changed upstream. The sparse-tree audit applied
each patch in sequence using GNU patch with `--fuzz=0`; after a failed
dry run, it skipped that patch before trying the rest. All 84 applied to
11.22. Five failed on 11.23. This is an application audit without a build.

## Changes that need a decision

| Patch | What changed in upstream 11.23 |
| --- | --- |
| `server-mousemove-on-show-hide` | [Window styles in shared memory](https://github.com/wine-mirror/wine/commit/5a938d967ed6a781ce80817bac36543a3336a5fd) and [the cursor update condition](https://github.com/wine-mirror/wine/commit/1e722e8d7ec43536d75af6b10d5bf8d3df323164) rewrote the exact branch in `server/window.c:set_window_pos` where winum adds a delayed move. 11.23 uses `old_visible && is_visible(win)` for window motion. That branch has no delayed show/hide arm. |
| `server-input-attachment-components` | [Avoid return-void call patterns](https://github.com/wine-mirror/wine/commit/e8f4226ac56aa9bd8bc3ff37f0a2ce90bee4582e) edits `detach_thread_input` where winum handles a disconnected input graph. Upstream sets the invalid-parameter error separately and returns for count zero or greater than one. The graph split is still a TODO upstream. |
| `msi-costfinalize-reset-feature-requests` and `msi-costfinalize-feature-depth` | [Keep installed components in their registered location](https://github.com/wine-mirror/wine/commit/34c80fd64e16d29b8564fed0c1c52168e68c9d7a) changes `ACTION_CostFinalize` in both patch areas. It reads installed component state earlier, then uses the registered locations before resolving target directories. The fresh-costing action reset and feature-depth bound remain separate winum behavior. |
| `server-token-integrity-level` | The generated server protocol changed from 979 to 988. The patch still expects to bump 979 to 980. Upstream [changes the admin token owner and default DACL](https://github.com/wine-mirror/wine/commit/c18d47f2ba14dada220e1a1b9107c353fb6b34c1), a separate behavior from token integrity. |
| `ntdll-locate-xstate-feature-enabled` (applies) | The ARM64 and XState work [changes the config used by the same API family](https://github.com/wine-mirror/wine/commit/02ca3c39147ff0807469084a0e0085d016abf8f1) and [threads it through the size and offset helpers](https://github.com/wine-mirror/wine/commit/a8e830956a6964671b9e8b12fd9c0ea0d4358d34). The winum guard in `RtlLocateExtendedFeature2` still applies. This needs compatibility review with the new config path. |

## Requested DLLs

- **riched20:** upstream [removed explicit image bases](https://github.com/wine-mirror/wine/commit/73fa7117cbc7f45a13c9fc24ae37b1e7bdd9b304) from build rules, with no changes in the RichEdit implementation files touched by the candidates. Tab stop, TOM paragraph navigation v3, and target-device scroll all applied in their proposed order.
- **ole32, msxml3, combase, wbemdisp, msado15:** no upstream commits in these directories between the tags. The ole32 RevokeDragDrop v2 patch applied in the sparse-tree audit. No upstream replacement for the four approved candidates was identified.
- **kernelbase:** upstream changes cover yearly time zone caching, an enclave API stub, image-base settings, and StrCmpLogicalW. None touched the kernelbase files in the series. The adjacent [wbemprox owner SID implementation](https://github.com/wine-mirror/wine/commit/589a301a390517b2d3d4b68e26b22dd32376e198) touched the same source file as the timezone patch, which still applied.
- **server and win32u:** upstream changes cover foreground key repeats, window styles in shared memory, DPI and fractional scaling, layered window repaint, remote OpenGL/Vulkan presentation, registry path casing, and ANSI message translation. The rows above identify failed hunks. The ClearType and cross-thread KillTimer patches applied in the audit.

## Other changed target paths

The full intersection with the promotion and existing series patches is
below. A changed file alone does not imply a patch conflict.

- `dlls/advapi32/tests/security.c` — kernelbase-account-domain-sid.patch, ntdll-token-integrity-level.patch
- `dlls/msi/action.c` — msi-addlocal-feature-ancestors.patch, msi-follow-parent-install-level.patch, msi-setfeaturestate-required-descendants.patch, msi-selection-override-ancestors.patch, msi-required-feature-source-state.patch, msi-costfinalize-reset-feature-requests.patch, msi-disabled-feature-selection.patch, msi-reject-cyclic-feature-parents.patch, msi-costfinalize-feature-depth.patch, msi-executeaction-ui-selection-transfer.patch, msi-source-directory-component-state.patch
- `dlls/msi/tests/db.c` — msi-transform-null-stream.patch
- `dlls/ntdll/exception.c` — ntdll-locate-xstate-feature-enabled.patch
- `dlls/ntdll/tests/exception.c` — ntdll-locate-xstate-feature-enabled.patch
- `dlls/ntdll/unix/security.c` — ntdll-token-integrity-level.patch
- `dlls/user32/tests/win.c` — win32u-child-topmost-ignore.patch, winex11-stale-map-activation.patch
- `dlls/wbemprox/builtin.c` — wbemprox-timezone.patch
- `dlls/win32u/window.c` — win32u-child-topmost-ignore.patch
- `dlls/winex11.drv/event.c` — winex11-stale-map-activation.patch
- `dlls/winex11.drv/window.c` — winex11-shape-alpha-xwayland.patch, winex11-owner-managed-async.patch, winex11-stale-map-activation.patch
- `dlls/winex11.drv/x11drv.h` — winex11-shape-alpha-xwayland.patch, winex11-stale-map-activation.patch
- `dlls/winex11.drv/xrender.c` — winex11-flush-putimage.patch
- `include/wine/server_protocol.h` — server-token-integrity-level.patch
- `server/process.c` — server-cross-thread-killtimer.patch
- `server/process.h` — server-cross-thread-killtimer.patch
- `server/protocol.def` — server-token-integrity-level.patch
- `server/queue.c` — server-mousemove-on-show-hide.patch, server-input-attachment-components.patch, server-cross-thread-killtimer.patch
- `server/request_handlers.h` — server-token-integrity-level.patch
- `server/request_trace.h` — server-token-integrity-level.patch
- `server/token.c` — server-token-integrity-level.patch
- `server/user.h` — server-mousemove-on-show-hide.patch
- `server/window.c` — server-mousemove-on-show-hide.patch, server-input-attachment-components.patch

## Release gates

- The existing 80-entry `scripts/check-series.sh` check passes.
- The 11.22 combined overlay [smoke run 45880891853](https://github.com/Harvi654/winum/actions/runs/45880891853) passed in Inventor, AutoCAD, and Revit on the maintainer's machine.
- The fifth candidate, msxml3-selection-removeall v2 for Content Center #45, passed Inventor L4, [paired L2](https://github.com/Harvi654/winum/actions/runs/45907358527), and maintainer review. Its AutoCAD and Revit L4 remain open. Its release test must drop the removeAll todo marker if promoted.
- A complete 11.23 series, both-architecture L0, paired L2, and the release PR are pending resolution of the upstream overlaps. Inventor pins must come from the signed published winum release.

### Resolution approved 2026-11-27

The maintainer approved a mechanical rebase of the five failed patches.
The revised source series passes zero-fuzz application. The existing
XState fix is retained unchanged. See the
[release candidate notes](winum-11.23-1.md) for validation and the MSI
maintenance changes that the L4 plan covers.
