# Patch status overview

One row per patch of `patches/series`, in series order. Details, dates and
evidence are on each patch's page. Updated 2026-11-06.

Legend: **yes** done and recorded; **partly** done for part of the scope;
**test, not run on Win** the patch has a Wine test that never ran on
Windows 10; **missing** no Wine test exists yet; **CI** waits for the
full-series build; **confirm** the patch has been in daily use for more than
a week, the maintainer still has to confirm it; **-** not recorded.

| # | Patch | DLL | L0 | L1 | L2 | L3 | L4 | L5 | Cross review |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 1 | [combase-stub-activation-context](combase-stub-activation-context.md) | combase | yes | **missing** | - | - | yes | confirm | - |
| 2 | [comctl32-statusbar-erase-background](comctl32-statusbar-erase-background.md) | comctl32 | yes | **missing** | - | - | yes | confirm | - |
| 3 | [comctl32-toolbar-padding-height](comctl32-toolbar-padding-height.md) | comctl32 | yes | **missing** | - | - | yes | confirm | - |
| 4 | [comctl32-toolbar-setrows-wrapable](comctl32-toolbar-setrows-wrapable.md) | comctl32 | yes | **missing** | - | - | yes | confirm | - |
| 5 | [kernelbase-account-domain-sid](kernelbase-account-domain-sid.md) | kernelbase | yes | test, not run on Win | - | - | yes | confirm | - |
| 6 | [msi-transform-null-stream](msi-transform-null-stream.md) | msi | yes | **missing** | - | - | yes | confirm | - |
| 7 | [mspatcha-pa19-32bit-and-large-files](mspatcha-pa19-32bit-and-large-files.md) | mspatcha | yes | **missing** | - | - | yes | confirm | - |
| 8 | [msxml3-domdoc-importnode](msxml3-domdoc-importnode.md) | msxml3 | yes | test, not run on Win | - | - | yes | confirm | - |
| 9 | [ntdll-actctx-threadingmodel-case](ntdll-actctx-threadingmodel-case.md) | ntdll | yes | **missing** | - | - | yes | confirm | - |
| 10 | [ntdll-locate-xstate-feature-enabled](ntdll-locate-xstate-feature-enabled.md) | ntdll | yes | test, not run on Win | - | - | yes | confirm | - |
| 11 | [ole32-storage-thread-lock](ole32-storage-thread-lock.md) | ole32 | yes | test, not run on Win | - | yes | yes | confirm | - |
| 12 | [ole32-transacted-truncate-dirty](ole32-transacted-truncate-dirty.md) | ole32 | yes | test, not run on Win | - | yes | yes | confirm | - |
| 13 | [oleaut32-typedesc-pointee](oleaut32-typedesc-pointee.md) | oleaut32 | yes | **missing** | yes | - | yes | confirm | yes |
| 14 | [oleaut32-typedesc-z-offsets](oleaut32-typedesc-z-offsets.md) | oleaut32 | yes | **missing** | - | - | yes | confirm | - |
| 15 | [riched20-caret-rewrap](riched20-caret-rewrap.md) | riched20 | yes | **missing** | - | - | yes | confirm | - |
| 16 | [riched20-scf-word-insertion-point](riched20-scf-word-insertion-point.md) | riched20 | yes | **missing** | - | - | yes | confirm | - |
| 17 | [riched20-textfont-applytmp](riched20-textfont-applytmp.md) | riched20 | yes | **missing** | - | - | yes | confirm | - |
| 18 | [rpcrt4-ndrstubcall3](rpcrt4-ndrstubcall3.md) | rpcrt4 | yes | **missing** | - | - | yes | confirm | - |
| 19 | [server-mousemove-on-show-hide](server-mousemove-on-show-hide.md) | server | yes | **missing** | - | - | yes | confirm | - |
| 20 | [shell32-folderview-selection](shell32-folderview-selection.md) | shell32 | yes | **missing** | - | - | yes | confirm | - |
| 21 | [shell32-getimage](shell32-getimage.md) | shell32 | yes | test, not run on Win | - | - | yes | confirm | - |
| 22 | [urlmon-maptozone-null-terminate](urlmon-maptozone-null-terminate.md) | urlmon | yes | **missing** | - | - | yes | confirm | - |
| 23 | [urlmon-res-zone-local-machine](urlmon-res-zone-local-machine.md) | urlmon | yes | **missing** | - | - | yes | confirm | - |
| 24 | [wbemprox-timezone](wbemprox-timezone.md) | wbemprox | yes | **missing** | - | - | yes | confirm | - |
| 25 | [winex11-flush-putimage](winex11-flush-putimage.md) | winex11.drv | yes | **missing** | - | - | yes | confirm | - |
| 26 | [winex11-shape-alpha-xwayland](winex11-shape-alpha-xwayland.md) | winex11.drv | yes | **missing** | - | - | yes | confirm | - |
| 27 | [winex11-xinput-bad-device](winex11-xinput-bad-device.md) | winex11.drv | yes | **missing** | - | - | yes | confirm | - |
| 28 | [server-input-attachment-components](server-input-attachment-components.md) | server | yes | **missing** | - | yes | yes | confirm | - |
| 29 | [msxml3-parseerror-filepos](msxml3-parseerror-filepos.md) | msxml3 | yes | test, not run on Win | - | yes | yes | confirm | - |
| 30 | [wininet-http-same-position-seek](wininet-http-same-position-seek.md) | wininet | yes | **missing** | - | yes | yes | confirm | - |
| 31 | [shell32-automation-zip-copy](shell32-automation-zip-copy.md) | shell32 | yes | test, not run on Win | - | yes | yes | confirm | - |
| 32 | [msi-selection-tree-directory-property](msi-selection-tree-directory-property.md) | msi | yes | **missing** | - | yes | yes | confirm | - |
| 33 | [comctl32-header-null-layout](comctl32-header-null-layout.md) | comctl32 | yes | **missing** | - | yes | yes | confirm | - |
| 34 | [msxml3-schema-cache-namespace](msxml3-schema-cache-namespace.md) | msxml3 | yes | yes | yes | yes | yes | confirm | - |
| 35 | [explorer-tasklist-parenting-hosts](explorer-tasklist-parenting-hosts.md) | explorer | yes | **missing** | - | yes | yes | confirm | - |
| 36 | [comctl32-ownerdraw-existing-capture](comctl32-ownerdraw-existing-capture.md) | comctl32 | yes | **missing** | - | yes | yes | confirm | - |
| 37 | [mshtml-document-open-optional-result](mshtml-document-open-optional-result.md) | mshtml | yes | **missing** | - | yes | yes | confirm | - |
| 38 | [winex11-owner-managed-async](winex11-owner-managed-async.md) | winex11.drv | yes | **missing** | - | yes | yes | confirm | - |
| 39 | [glu32-nurbs-curves](glu32-nurbs-curves.md) | glu32 | yes | yes | yes | yes | yes | confirm | - |
| 40 | [win32u-fixed-negative-mat2](win32u-fixed-negative-mat2.md) | win32u | CI | yes | yes | yes | partly | - | - |
| 41 | [win32u-child-topmost-ignore](win32u-child-topmost-ignore.md) | win32u | CI | yes | yes | - | yes | - | yes |
| 42 | [comctl32-themed-checkbox-buffered-paint](comctl32-themed-checkbox-buffered-paint.md) | comctl32 | CI | yes | yes | - | yes | - | yes |
| 43 | [winex11-stale-map-activation](winex11-stale-map-activation.md) | winex11.drv | CI | yes | yes | yes | yes | - | yes |
| 44 | [msi-addlocal-feature-ancestors](msi-addlocal-feature-ancestors.md) | msi | CI | yes | yes | partly | yes | - | yes |
| 45 | [msi-follow-parent-install-level](msi-follow-parent-install-level.md) | msi | CI | yes | yes | partly | yes | - | yes |
| 46 | [msi-setfeaturestate-post-cost-action](msi-setfeaturestate-post-cost-action.md) | msi | CI | yes | yes | partly | yes | - | yes |
| 47 | [msi-setfeaturestate-post-cost-ancestors](msi-setfeaturestate-post-cost-ancestors.md) | msi | CI | yes | yes | partly | yes | - | yes |
| 48 | [msi-setfeaturestate-required-descendants](msi-setfeaturestate-required-descendants.md) | msi | CI | yes | yes | partly | yes | - | yes |
| 49 | [msi-selection-override-ancestors](msi-selection-override-ancestors.md) | msi | CI | yes | yes | partly | yes | - | yes |
| 50 | [msi-required-feature-source-state](msi-required-feature-source-state.md) | msi | CI | yes | yes | partly | yes | - | yes |
| 51 | [msi-required-feature-advertise-state](msi-required-feature-advertise-state.md) | msi | CI | yes | yes | partly | yes | - | yes |
| 52 | [msi-required-feature-absent-state](msi-required-feature-absent-state.md) | msi | CI | yes | yes | partly | yes | - | yes |
| 53 | [msi-costfinalize-reset-feature-requests](msi-costfinalize-reset-feature-requests.md) | msi | CI | yes | yes | partly | yes | - | yes |
| 54 | [msi-post-cost-local-idempotence](msi-post-cost-local-idempotence.md) | msi | CI | yes | yes | partly | yes | - | yes |
| 55 | [msi-disabled-feature-selection](msi-disabled-feature-selection.md) | msi | CI | yes | yes | partly | yes | - | yes |
| 56 | [msi-reject-cyclic-feature-parents](msi-reject-cyclic-feature-parents.md) | msi | CI | yes | yes | partly | yes | - | yes |
| 57 | [msi-costfinalize-feature-depth](msi-costfinalize-feature-depth.md) | msi | CI | yes | yes | partly | yes | - | yes |
| 58 | [msi-ui-localonly-source-state](msi-ui-localonly-source-state.md) | msi | CI | yes | yes | partly | yes | - | yes |
| 59 | [msi-executeaction-ui-selection-transfer](msi-executeaction-ui-selection-transfer.md) | msi | CI | yes | yes | partly | yes | - | yes |
| 60 | [msi-source-directory-component-state](msi-source-directory-component-state.md) | msi | CI | yes | yes | partly | yes | - | yes |
| 61 | [oleaut32-typelib-keep-released](oleaut32-typelib-keep-released.md) | oleaut32 | CI | yes | partly | yes | Inventor | - | yes |
| 62 | [opengl32-window-format-color-bits](opengl32-window-format-color-bits.md) | opengl32 | yes | yes | partly | yes | Inventor | - | yes |

## What is missing, by kind

### L1: a Wine test exists but never ran on Windows 10 (8)

These need only a standalone bundle (like `scripts/build-l1-*.sh`) and one
run in the Windows VM:

- `kernelbase-account-domain-sid`
- `msxml3-domdoc-importnode`
- `ntdll-locate-xstate-feature-enabled`
- `ole32-storage-thread-lock`
- `ole32-transacted-truncate-dirty`
- `shell32-getimage`
- `msxml3-parseerror-filepos`
- `shell32-automation-zip-copy`

### L1: no Wine test yet (29)

Each needs a test written from a Windows measurement (MEASURE program first),
failing on vanilla Wine and passing with the patch. Some cannot have a
normal Wine test (X11 driver, wineserver, Explorer, Xwayland behaviour);
their pages say so and they get an L3 reproducer instead.

- `combase-stub-activation-context`
- `comctl32-statusbar-erase-background`
- `comctl32-toolbar-padding-height`
- `comctl32-toolbar-setrows-wrapable`
- `msi-transform-null-stream`
- `mspatcha-pa19-32bit-and-large-files`
- `ntdll-actctx-threadingmodel-case`
- `oleaut32-typedesc-pointee`
- `oleaut32-typedesc-z-offsets`
- `riched20-caret-rewrap`
- `riched20-scf-word-insertion-point`
- `riched20-textfont-applytmp`
- `rpcrt4-ndrstubcall3`
- `server-mousemove-on-show-hide`
- `shell32-folderview-selection`
- `urlmon-maptozone-null-terminate`
- `urlmon-res-zone-local-machine`
- `wbemprox-timezone`
- `winex11-flush-putimage`
- `winex11-shape-alpha-xwayland`
- `winex11-xinput-bad-device`
- `server-input-attachment-components`
- `wininet-http-same-position-seek`
- `msi-selection-tree-directory-property`
- `comctl32-header-null-layout`
- `explorer-tasklist-parenting-hosts`
- `comctl32-ownerdraw-existing-capture`
- `mshtml-document-open-optional-result`
- `winex11-owner-managed-async`

### L2 for the whole series

One paired run, vanilla 11.20 against the full series, of the test suites
of every touched DLL, x86_64 and i386, replaces the missing per-patch L2
records (planned before the first release).

### L4 for the whole series

One smoke test of the shared build in both programs: Inventor (start, part,
assembly, drawing, save/reopen, restart, 3D PDF, GPU ray tracing on a card
that has it) and SOLIDWORKS (start, part, assembly, drawing, Simulation,
save/reopen, restart, PropertyManager headers and checkboxes, Extrude D1).

### L5

The patches from the Inventor and SOLIDWORKS runtimes have been in daily use
for more than a week; the maintainer confirms that on each page.

### Patch headers

The patches from Autodesk-Inventor-for-Linux and the older SOLIDWORKS
patches have no winum header (`From:`, subject, AI note, what Windows does).
Their pages carry that information for now; headers follow.

