# Patch status overview

One row per patch of `patches/series`, in series order. Details, dates and
evidence are on each patch's page. L1 column updated 2026-11-11 for
winum-11.20-1 from the page markers (`scripts/check-series.sh`: 62 of 62);
the other columns are those of 2026-11-06 and the pages are authoritative.
The earlier version of this file is `docs/archive/patches-overview-2026-11-06.md`.

Legend: **yes** done and recorded; **partly** done for part of the scope;
**test, not run on Win** the patch has a Wine test that never ran on
Windows 10; **missing** no Wine test exists yet; **L3 only** / **known deviation** the
maintainer's recorded exception (reason on the page); **confirm** the patch has been in daily use for more than
a week, the maintainer still has to confirm it; **-** not recorded.

| # | Patch | DLL | L0 | L1 | L2 | L3 | L4 | L5 | Cross review |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 1 | [combase-stub-activation-context](combase-stub-activation-context.md) | combase | yes | known deviation | - | - | yes | confirm | - |
| 2 | [comctl32-statusbar-erase-background](comctl32-statusbar-erase-background.md) | comctl32 | yes | yes | - | - | yes | confirm | - |
| 3 | [comctl32-toolbar-padding-height](comctl32-toolbar-padding-height.md) | comctl32 | yes | yes | - | - | yes | confirm | - |
| 4 | [comctl32-toolbar-setrows-wrapable](comctl32-toolbar-setrows-wrapable.md) | comctl32 | yes | yes | - | - | yes | confirm | - |
| 5 | [kernelbase-account-domain-sid](kernelbase-account-domain-sid.md) | kernelbase | yes | yes | - | - | yes | confirm | - |
| 6 | [msi-transform-null-stream](msi-transform-null-stream.md) | msi | yes | yes | - | - | yes | confirm | - |
| 7 | [mspatcha-pa19-32bit-and-large-files](mspatcha-pa19-32bit-and-large-files.md) | mspatcha | yes | L3 only | - | - | yes | confirm | - |
| 8 | [msxml3-domdoc-importnode](msxml3-domdoc-importnode.md) | msxml3 | yes | yes | - | - | yes | confirm | - |
| 9 | [ntdll-actctx-threadingmodel-case](ntdll-actctx-threadingmodel-case.md) | ntdll | yes | yes | - | - | yes | confirm | - |
| 10 | [ntdll-locate-xstate-feature-enabled](ntdll-locate-xstate-feature-enabled.md) | ntdll | yes | yes | - | - | yes | confirm | - |
| 11 | [ole32-storage-thread-lock](ole32-storage-thread-lock.md) | ole32 | yes | yes | - | yes | yes | confirm | - |
| 12 | [ole32-transacted-truncate-dirty](ole32-transacted-truncate-dirty.md) | ole32 | yes | yes | - | yes | yes | confirm | - |
| 13 | [oleaut32-typedesc-pointee](oleaut32-typedesc-pointee.md) | oleaut32 | yes | yes | yes | - | yes | confirm | yes |
| 14 | [oleaut32-typedesc-z-offsets](oleaut32-typedesc-z-offsets.md) | oleaut32 | yes | yes | - | - | yes | confirm | - |
| 15 | [riched20-caret-rewrap](riched20-caret-rewrap.md) | riched20 | yes | yes | - | - | yes | confirm | - |
| 16 | [riched20-scf-word-insertion-point](riched20-scf-word-insertion-point.md) | riched20 | yes | yes | - | - | yes | confirm | - |
| 17 | [riched20-textfont-applytmp](riched20-textfont-applytmp.md) | riched20 | yes | yes | - | - | yes | confirm | - |
| 18 | [rpcrt4-ndrstubcall3](rpcrt4-ndrstubcall3.md) | rpcrt4 | yes | L3 only | - | - | yes | confirm | - |
| 19 | [server-mousemove-on-show-hide](server-mousemove-on-show-hide.md) | server | yes | yes | - | - | yes | confirm | - |
| 20 | [shell32-folderview-selection](shell32-folderview-selection.md) | shell32 | yes | yes | - | - | yes | confirm | - |
| 21 | [shell32-getimage](shell32-getimage.md) | shell32 | yes | yes | - | - | yes | confirm | - |
| 22 | [urlmon-maptozone-null-terminate](urlmon-maptozone-null-terminate.md) | urlmon | yes | L3 only | - | - | yes | confirm | - |
| 23 | [urlmon-res-zone-local-machine](urlmon-res-zone-local-machine.md) | urlmon | yes | yes | - | - | yes | confirm | - |
| 24 | [wbemprox-timezone](wbemprox-timezone.md) | wbemprox | yes | yes | - | - | yes | confirm | - |
| 25 | [winex11-flush-putimage](winex11-flush-putimage.md) | winex11.drv | yes | L3 only | - | - | yes | confirm | - |
| 26 | [winex11-shape-alpha-xwayland](winex11-shape-alpha-xwayland.md) | winex11.drv | yes | L3 only | - | - | yes | confirm | - |
| 27 | [winex11-xinput-bad-device](winex11-xinput-bad-device.md) | winex11.drv | yes | L3 only | - | - | yes | confirm | - |
| 28 | [server-input-attachment-components](server-input-attachment-components.md) | server | yes | yes | - | yes | yes | confirm | - |
| 29 | [msxml3-parseerror-filepos](msxml3-parseerror-filepos.md) | msxml3 | yes | yes | - | yes | yes | confirm | - |
| 30 | [wininet-http-same-position-seek](wininet-http-same-position-seek.md) | wininet | yes | yes | - | yes | yes | confirm | - |
| 31 | [shell32-automation-zip-copy](shell32-automation-zip-copy.md) | shell32 | yes | yes | - | yes | yes | confirm | - |
| 32 | [msi-selection-tree-directory-property](msi-selection-tree-directory-property.md) | msi | yes | L3 only | - | yes | yes | confirm | - |
| 33 | [comctl32-header-null-layout](comctl32-header-null-layout.md) | comctl32 | yes | yes | - | yes | yes | confirm | - |
| 34 | [msxml3-schema-cache-namespace](msxml3-schema-cache-namespace.md) | msxml3 | yes | yes | yes | yes | yes | confirm | - |
| 35 | [explorer-tasklist-parenting-hosts](explorer-tasklist-parenting-hosts.md) | explorer | yes | L3 only | - | yes | yes | confirm | - |
| 36 | [comctl32-ownerdraw-existing-capture](comctl32-ownerdraw-existing-capture.md) | comctl32 | yes | yes | - | yes | yes | confirm | - |
| 37 | [mshtml-document-open-optional-result](mshtml-document-open-optional-result.md) | mshtml | yes | yes | - | yes | yes | confirm | - |
| 38 | [winex11-owner-managed-async](winex11-owner-managed-async.md) | winex11.drv | yes | L3 only | - | yes | yes | confirm | - |
| 39 | [glu32-nurbs-curves](glu32-nurbs-curves.md) | glu32 | yes | yes | yes | yes | yes | confirm | - |
| 40 | [win32u-fixed-negative-mat2](win32u-fixed-negative-mat2.md) | win32u | yes | yes | yes | yes | partly | - | - |
| 41 | [win32u-child-topmost-ignore](win32u-child-topmost-ignore.md) | win32u | yes | yes | yes | - | yes | - | yes |
| 42 | [comctl32-themed-checkbox-buffered-paint](comctl32-themed-checkbox-buffered-paint.md) | comctl32 | yes | yes | yes | - | yes | - | yes |
| 43 | [winex11-stale-map-activation](winex11-stale-map-activation.md) | winex11.drv | yes | yes | yes | yes | yes | - | yes |
| 44 | [msi-addlocal-feature-ancestors](msi-addlocal-feature-ancestors.md) | msi | yes | yes | yes | partly | yes | - | yes |
| 45 | [msi-follow-parent-install-level](msi-follow-parent-install-level.md) | msi | yes | yes | yes | partly | yes | - | yes |
| 46 | [msi-setfeaturestate-post-cost-action](msi-setfeaturestate-post-cost-action.md) | msi | yes | yes | yes | partly | yes | - | yes |
| 47 | [msi-setfeaturestate-post-cost-ancestors](msi-setfeaturestate-post-cost-ancestors.md) | msi | yes | yes | yes | partly | yes | - | yes |
| 48 | [msi-setfeaturestate-required-descendants](msi-setfeaturestate-required-descendants.md) | msi | yes | yes | yes | partly | yes | - | yes |
| 49 | [msi-selection-override-ancestors](msi-selection-override-ancestors.md) | msi | yes | yes | yes | partly | yes | - | yes |
| 50 | [msi-required-feature-source-state](msi-required-feature-source-state.md) | msi | yes | yes | yes | partly | yes | - | yes |
| 51 | [msi-required-feature-advertise-state](msi-required-feature-advertise-state.md) | msi | yes | yes | yes | partly | yes | - | yes |
| 52 | [msi-required-feature-absent-state](msi-required-feature-absent-state.md) | msi | yes | yes | yes | partly | yes | - | yes |
| 53 | [msi-costfinalize-reset-feature-requests](msi-costfinalize-reset-feature-requests.md) | msi | yes | yes | yes | partly | yes | - | yes |
| 54 | [msi-post-cost-local-idempotence](msi-post-cost-local-idempotence.md) | msi | yes | yes | yes | partly | yes | - | yes |
| 55 | [msi-disabled-feature-selection](msi-disabled-feature-selection.md) | msi | yes | yes | yes | partly | yes | - | yes |
| 56 | [msi-reject-cyclic-feature-parents](msi-reject-cyclic-feature-parents.md) | msi | yes | yes | yes | partly | yes | - | yes |
| 57 | [msi-costfinalize-feature-depth](msi-costfinalize-feature-depth.md) | msi | yes | yes | yes | partly | yes | - | yes |
| 58 | [msi-ui-localonly-source-state](msi-ui-localonly-source-state.md) | msi | yes | yes | yes | partly | yes | - | yes |
| 59 | [msi-executeaction-ui-selection-transfer](msi-executeaction-ui-selection-transfer.md) | msi | yes | yes | yes | partly | yes | - | yes |
| 60 | [msi-source-directory-component-state](msi-source-directory-component-state.md) | msi | yes | yes | yes | partly | yes | - | yes |
| 61 | [oleaut32-typelib-keep-released](oleaut32-typelib-keep-released.md) | oleaut32 | yes | yes | partly | yes | Inventor | - | yes |
| 62 | [opengl32-window-format-color-bits](opengl32-window-format-color-bits.md) | opengl32 | yes | yes | partly | yes | Inventor | - | yes |

## What is missing, by kind

Since winum-11.20-1 every patch of the series has a Wine test that passed on
Windows 10 or a recorded exception (`check-series.sh`, 62 of 62). The open
measurements and follow-ups are listed in `docs/release/winum-11.20-1.md`
("After the release") and on each page. The list of 2026-11-06 is in the
archived copy named above.
