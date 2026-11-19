# winex11-unmanaged-before-map (candidate)

A captionless popup that is shown without activation stays a managed X
window in Wine, because every overlapped window gets `WS_CAPTION` at
creation and winex11 only ever switches a window *to* managed. DevExpress
menus in Vault Explorer 2027 (dark theme) are created that way and then
restyled to `WS_POPUP | WS_CLIPSIBLINGS` before their first show; under
KWin the managed menu ends up below KWin's own override-redirect shadow
window: the menu opens, but is invisible. The patch lets a window that was
never mapped become unmanaged when `X11DRV_WindowPosChanged` decides on
`SWP_SHOWWINDOW`. Original code, AI-assisted (Claude), LGPL-2.1-or-later.
Not submitted upstream. Contributed from
`Harvi654/Autodesk-Inventor-for-Linux` PR #8 (Vault), where the maintainer
asked for it as a separate winum candidate.

Not in `patches/series`. Applies after the five winex11 patches of the
series (`winex11-owner-managed-async`, `winex11-stale-map-activation`
included); checked on Wine 11.22 with `git apply --check`, 2026-11-17.

## What it changes

- `x11drv_win_data.was_mapped`: set on the first map request in
  `window_set_wm_state()`.
- `window_set_managed()`: switching to unmanaged is allowed while the
  window was never mapped (ICCCM: `override_redirect` may change on a
  withdrawn window; no `WM_STATE` change can be pending). The ERR for a
  mapped window is unchanged.
- `X11DRV_WindowPosChanged()`: on `SWP_SHOWWINDOW`, a managed window that
  was never mapped is re-evaluated with `is_window_managed()`. The desktop
  window, embedded windows and `Managed=N` are excluded.

v2 (2026-11-07), after a cross review by a second model: `was_mapped`
replaces the check of `wm_state_serial` and the pending WM_STATE, which
also matched a window that had been shown, hidden and withdrawn; the
exclusions above were added.

## Test

No `ok()` test: nothing visible to Win32 changes, only the X window's
override-redirect attribute, which user32's tests cannot see. The
reproducer `docs/wine-repro/winex11-unmanaged-menus/` creates the three
windows the way the menus are created and asks the X server (libX11
through ctypes) whether each is override-redirect or listed in
`_NET_CLIENT_LIST`. So this would be `l3-only` with that reason, the
maintainer's call.

## Validation (Wine 11.20, the Inventor project's `wine-11.20-6` series)

| Level | Result |
| --- | --- |
| L0 | builds in the Inventor project's Wine workflow (`wine-11.20-6` plus this patch) |
| L2 | `user32_test.exe win` and `msg`, twice each, unpatched vs patched: the same `win` lines fail with and without it (10689/10690 hit test through a transparent window, one varying focus block), `msg` clean in both. Table in the reproducer README. |
| L3 | reproducer: `menu` and `menu-layered` override-redirect with the patch, managed without; `caption` managed in both |
| L4 | Vault Explorer 2027 dark theme: every menu and context menu visible, also on a second open (contributor, KDE Plasma 6 / Xwayland, NVIDIA, 2026-11-07). Inventor 2027 unchanged in daily use since. |

## Open

- L2 on Wine 11.22 with the winum series (the runs above were on 11.20
  with the Inventor project's series, which is now archived).
- The maintainer's L4 in Inventor, and in SOLIDWORKS when it is worked on
  again (the DLL is shared).
- Whether a Wine bug report is worth filing: the behaviour is reproducible
  with the synthetic program alone.
