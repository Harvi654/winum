# Patch status overview

One row per patch of `patches/series` (winum-11.22-2, 73 patches), in
series order. Details, dates and evidence are on each patch's page, which
is authoritative; this table is updated with every release (last:
winum-11.22-2, 2026-11-23). The L1 column follows the page markers that
`scripts/check-series.sh` checks (73 of 73: L1 62, L3 only 9, known
deviation 2). "Since" is the first winum release with the patch. Earlier
versions of this file: `docs/archive/patches-overview-2026-11-06.md` and
the git history.

Legend: **yes** done and recorded; **partly** done for part of the scope;
**L3 only** / **known deviation** the maintainer's recorded exception
(reason on the page); **Inventor** L4 in Inventor only (the other program
deferred, AGENTS.md); **confirm** the patch has been in daily use for more
than a week, the maintainer still has to confirm L5; **-** not recorded or
not planned for the patch.

| # | Patch | DLL | Since | L0 | L1 | L2 | L3 | L4 | L5 | Cross review |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 1 | [combase-stub-activation-context](combase-stub-activation-context.md) | combase | 11.20-1 | yes | known deviation | - | - | yes | confirm | - |
| 2 | [comctl32-statusbar-erase-background](comctl32-statusbar-erase-background.md) | comctl32 | 11.20-1 | yes | yes | - | - | yes | confirm | - |
| 3 | [comctl32-toolbar-padding-height](comctl32-toolbar-padding-height.md) | comctl32 | 11.20-1 | yes | yes | - | - | yes | confirm | - |
| 4 | [comctl32-toolbar-setrows-wrapable](comctl32-toolbar-setrows-wrapable.md) | comctl32 | 11.20-1 | yes | yes | - | - | yes | confirm | - |
| 5 | [kernelbase-account-domain-sid](kernelbase-account-domain-sid.md) | kernelbase | 11.20-1 | yes | yes | - | - | yes | confirm | - |
| 6 | [msi-transform-null-stream](msi-transform-null-stream.md) | msi | 11.20-1 | yes | yes | - | - | yes | confirm | - |
| 7 | [mspatcha-pa19-32bit-and-large-files](mspatcha-pa19-32bit-and-large-files.md) | mspatcha | 11.20-1 | yes | L3 only | - | - | yes | confirm | - |
| 8 | [msxml3-domdoc-importnode](msxml3-domdoc-importnode.md) | msxml3 | 11.20-1 | yes | yes | - | - | yes | confirm | - |
| 9 | [ntdll-actctx-threadingmodel-case](ntdll-actctx-threadingmodel-case.md) | ntdll | 11.20-1 | yes | yes | - | - | yes | confirm | - |
| 10 | [ntdll-locate-xstate-feature-enabled](ntdll-locate-xstate-feature-enabled.md) | ntdll | 11.20-1 | yes | yes | - | - | yes | confirm | - |
| 11 | [ole32-storage-thread-lock](ole32-storage-thread-lock.md) | ole32 | 11.20-1 | yes | yes | - | yes | yes | confirm | - |
| 12 | [ole32-transacted-truncate-dirty](ole32-transacted-truncate-dirty.md) | ole32 | 11.20-1 | yes | yes | - | yes | yes | confirm | - |
| 13 | [oleaut32-typedesc-pointee](oleaut32-typedesc-pointee.md) | oleaut32 | 11.20-1 | yes | yes | yes | - | yes | confirm | yes |
| 14 | [oleaut32-typedesc-z-offsets](oleaut32-typedesc-z-offsets.md) | oleaut32 | 11.20-1 | yes | yes | - | - | yes | confirm | - |
| 15 | [riched20-caret-rewrap](riched20-caret-rewrap.md) | riched20 | 11.20-1 | yes | yes | - | - | yes | confirm | - |
| 16 | [riched20-scf-word-insertion-point](riched20-scf-word-insertion-point.md) | riched20 | 11.20-1 | yes | yes | - | - | yes | confirm | - |
| 17 | [riched20-textfont-applytmp](riched20-textfont-applytmp.md) | riched20 | 11.20-1 | yes | yes | - | - | yes | confirm | - |
| 18 | [rpcrt4-ndrstubcall3](rpcrt4-ndrstubcall3.md) | rpcrt4 | 11.20-1 | yes | L3 only | - | - | yes | confirm | - |
| 19 | [server-mousemove-on-show-hide](server-mousemove-on-show-hide.md) | server | 11.20-1 | yes | yes | - | - | yes | confirm | - |
| 20 | [shell32-folderview-selection](shell32-folderview-selection.md) | shell32 | 11.20-1 | yes | yes | - | - | yes | confirm | - |
| 21 | [shell32-getimage](shell32-getimage.md) | shell32 | 11.20-1 | yes | yes | - | - | yes | confirm | - |
| 22 | [urlmon-maptozone-null-terminate](urlmon-maptozone-null-terminate.md) | urlmon | 11.20-1 | yes | L3 only | - | - | yes | confirm | - |
| 23 | [urlmon-res-zone-local-machine](urlmon-res-zone-local-machine.md) | urlmon | 11.20-1 | yes | yes | - | - | yes | confirm | - |
| 24 | [wbemprox-timezone](wbemprox-timezone.md) | wbemprox | 11.20-1 | yes | yes | - | - | yes | confirm | - |
| 25 | [winex11-flush-putimage](winex11-flush-putimage.md) | winex11.drv | 11.20-1 | yes | L3 only | - | - | yes | confirm | - |
| 26 | [winex11-shape-alpha-xwayland](winex11-shape-alpha-xwayland.md) | winex11.drv | 11.20-1 | yes | L3 only | - | - | yes | confirm | - |
| 27 | [winex11-xinput-bad-device](winex11-xinput-bad-device.md) | winex11.drv | 11.20-1 | yes | L3 only | - | - | yes | confirm | - |
| 28 | [server-input-attachment-components](server-input-attachment-components.md) | server | 11.20-1 | yes | yes | - | yes | yes | confirm | - |
| 29 | [msxml3-parseerror-filepos](msxml3-parseerror-filepos.md) | msxml3 | 11.20-1 | yes | yes | - | yes | yes | confirm | - |
| 30 | [wininet-http-same-position-seek](wininet-http-same-position-seek.md) | wininet | 11.20-1 | yes | yes | - | yes | yes | confirm | - |
| 31 | [shell32-automation-zip-copy](shell32-automation-zip-copy.md) | shell32 | 11.20-1 | yes | yes | - | yes | yes | confirm | - |
| 32 | [msi-selection-tree-directory-property](msi-selection-tree-directory-property.md) | msi | 11.20-1 | yes | L3 only | - | yes | yes | confirm | - |
| 33 | [comctl32-header-null-layout](comctl32-header-null-layout.md) | comctl32 | 11.20-1 | yes | yes | - | yes | yes | confirm | - |
| 34 | [msxml3-schema-cache-namespace](msxml3-schema-cache-namespace.md) | msxml3 | 11.20-1 | yes | yes | yes | yes | yes | confirm | - |
| 35 | [explorer-tasklist-parenting-hosts](explorer-tasklist-parenting-hosts.md) | explorer | 11.20-1 | yes | L3 only | - | yes | yes | confirm | - |
| 36 | [comctl32-ownerdraw-existing-capture](comctl32-ownerdraw-existing-capture.md) | comctl32 | 11.20-1 | yes | yes | - | yes | yes | confirm | - |
| 37 | [mshtml-document-open-optional-result](mshtml-document-open-optional-result.md) | mshtml | 11.20-1 | yes | yes | - | yes | yes | confirm | - |
| 38 | [winex11-owner-managed-async](winex11-owner-managed-async.md) | winex11.drv | 11.20-1 | yes | L3 only | - | yes | yes | confirm | - |
| 39 | [glu32-nurbs-curves](glu32-nurbs-curves.md) | glu32 | 11.20-1 | yes | yes | yes | yes | yes | confirm | - |
| 40 | [win32u-fixed-negative-mat2](win32u-fixed-negative-mat2.md) | win32u | 11.20-1 | yes | yes | yes | yes | partly | confirm | - |
| 41 | [win32u-child-topmost-ignore](win32u-child-topmost-ignore.md) | win32u | 11.20-1 | yes | yes | yes | - | yes | confirm | yes |
| 42 | [comctl32-themed-checkbox-buffered-paint](comctl32-themed-checkbox-buffered-paint.md) | comctl32 | 11.20-1 | yes | yes | yes | - | yes | confirm | yes |
| 43 | [winex11-stale-map-activation](winex11-stale-map-activation.md) | winex11.drv | 11.20-1 | yes | yes | yes | yes | yes | confirm | yes |
| 44 | [msi-addlocal-feature-ancestors](msi-addlocal-feature-ancestors.md) | msi | 11.20-1 | yes | yes | yes | partly | yes | confirm | yes |
| 45 | [msi-follow-parent-install-level](msi-follow-parent-install-level.md) | msi | 11.20-1 | yes | yes | yes | partly | yes | confirm | yes |
| 46 | [msi-setfeaturestate-post-cost-action](msi-setfeaturestate-post-cost-action.md) | msi | 11.20-1 | yes | yes | yes | partly | yes | confirm | yes |
| 47 | [msi-setfeaturestate-post-cost-ancestors](msi-setfeaturestate-post-cost-ancestors.md) | msi | 11.20-1 | yes | yes | yes | partly | yes | confirm | yes |
| 48 | [msi-setfeaturestate-required-descendants](msi-setfeaturestate-required-descendants.md) | msi | 11.20-1 | yes | yes | yes | partly | yes | confirm | yes |
| 49 | [msi-selection-override-ancestors](msi-selection-override-ancestors.md) | msi | 11.20-1 | yes | yes | yes | partly | yes | confirm | yes |
| 50 | [msi-required-feature-source-state](msi-required-feature-source-state.md) | msi | 11.20-1 | yes | yes | yes | partly | yes | confirm | yes |
| 51 | [msi-required-feature-advertise-state](msi-required-feature-advertise-state.md) | msi | 11.20-1 | yes | yes | yes | partly | yes | confirm | yes |
| 52 | [msi-required-feature-absent-state](msi-required-feature-absent-state.md) | msi | 11.20-1 | yes | yes | yes | partly | yes | confirm | yes |
| 53 | [msi-costfinalize-reset-feature-requests](msi-costfinalize-reset-feature-requests.md) | msi | 11.20-1 | yes | yes | yes | partly | yes | confirm | yes |
| 54 | [msi-post-cost-local-idempotence](msi-post-cost-local-idempotence.md) | msi | 11.20-1 | yes | yes | yes | partly | yes | confirm | yes |
| 55 | [msi-disabled-feature-selection](msi-disabled-feature-selection.md) | msi | 11.20-1 | yes | yes | yes | partly | yes | confirm | yes |
| 56 | [msi-reject-cyclic-feature-parents](msi-reject-cyclic-feature-parents.md) | msi | 11.20-1 | yes | yes | yes | partly | yes | confirm | yes |
| 57 | [msi-costfinalize-feature-depth](msi-costfinalize-feature-depth.md) | msi | 11.20-1 | yes | yes | yes | partly | yes | confirm | yes |
| 58 | [msi-ui-localonly-source-state](msi-ui-localonly-source-state.md) | msi | 11.20-1 | yes | yes | yes | partly | yes | confirm | yes |
| 59 | [msi-executeaction-ui-selection-transfer](msi-executeaction-ui-selection-transfer.md) | msi | 11.20-1 | yes | yes | yes | partly | yes | confirm | yes |
| 60 | [msi-source-directory-component-state](msi-source-directory-component-state.md) | msi | 11.20-1 | yes | yes | yes | partly | yes | confirm | yes |
| 61 | [oleaut32-typelib-keep-released](oleaut32-typelib-keep-released.md) | oleaut32 | 11.20-1 | yes | yes | partly | yes | Inventor | confirm | yes |
| 62 | [opengl32-window-format-color-bits](opengl32-window-format-color-bits.md) | opengl32 | 11.20-1 | yes | yes | partly | yes | Inventor | confirm | yes |
| 63 | [ole32-createfromfile-package](ole32-createfromfile-package.md) | ole32 | 11.20-2 | yes | yes | yes | yes | yes | confirm | yes |
| 64 | [ole32-storage-localdocs](ole32-storage-localdocs.md) | ole32 | 11.20-2 | yes | yes | yes | yes | yes | confirm | yes |
| 65 | [ole32-propset-null-strings](ole32-propset-null-strings.md) | ole32 | 11.20-2 | yes | yes | yes | yes | yes | confirm | yes |
| 66 | [packager-save-view-advise](packager-save-view-advise.md) | packager | 11.20-2 | yes | yes | yes | yes | yes | confirm | yes |
| 67 | [shell32-parentrelativeforui](shell32-parentrelativeforui.md) | shell32 | 11.22-2 | yes | yes | yes | - | yes | - | yes |
| 68 | [shell32-label-edit-enter](shell32-label-edit-enter.md) | shell32 | 11.22-2 | yes | yes | yes | - | yes | - | yes |
| 69 | [shell32-background-menu](shell32-background-menu.md) | shell32 | 11.22-2 | yes | yes | yes | - | yes | - | yes |
| 70 | [shell32-autocomplete-responsive](shell32-autocomplete-responsive.md) | shell32 | 11.22-2 | yes | known deviation | yes | yes | yes | - | yes |
| 71 | [msxml3-replacechild-same-node](msxml3-replacechild-same-node.md) | msxml3 | 11.22-2 | yes | yes | yes | yes | yes | - | yes |
| 72 | [ucrtbase-cxx-exception-size](ucrtbase-cxx-exception-size.md) | ucrtbase | 11.22-2 | yes | yes | yes | - | yes | - | yes |
| 73 | [server-cross-thread-killtimer](server-cross-thread-killtimer.md) | server | 11.22-2 | yes | yes | yes | yes | yes | - | yes |

## By release

| Release | Date | Patches | New in it |
| --- | --- | --- | --- |
| [winum-11.20-1](../release/winum-11.20-1.md) | 2026-11-11 | 62 | rows 1-62: the Inventor project's former `wine-11.20-6` series, the SOLIDWORKS fixes, 17 MSI feature-selection fixes, typelib cache, OpenGL pixel format |
| [winum-11.20-2](../release/winum-11.20-2.md) | 2026-11-12 | 66 | rows 63-66: Import CAD Reference Model (ole32, packager), contributed by @krokosik |
| [winum-11.22-1](../release/winum-11.22-1.md) | 2026-11-16 | 66 | the same 66 on Wine 11.22 |
| [winum-11.22-2](../release/winum-11.22-2.md) | 2026-11-23 | 73 | rows 67-73: Inventor's Open/Save dialogs (shell32), msxml3 same node (@Lukas10999), AutoCAD Save As (ucrtbase) and "Connection Lost" (server) |

## What is missing, by kind

Every patch of the series has a Wine test that passed on Windows 10 or a
recorded exception (`check-series.sh`, 73 of 73). L5 is the maintainer's
confirmation after a week of use; rows marked **confirm** wait for it.
SOLIDWORKS and AutoCAD L4 are recorded as deferred where the program was
not being worked on. Open measurements and follow-ups are on each page and
under "Open after the release" in the release notes; the list of
2026-11-06 is in the archived copy named above.
