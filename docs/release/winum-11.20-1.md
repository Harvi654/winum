# winum-11.20-1: promotion overview

Prepared 2026-11-11 by Claude for the maintainer's decision. Nothing in
`patches/series` or `docs/patches/` has been changed for this; the
promotion is done only after the maintainer's approval (steps at the
end). Wine 11.20, `WINUM_BUILD=1`, no release published yet.

Sources: `scripts/check-series.sh --report` on `main`, the candidate
READMEs, the cross reviews in `docs/wine-repro/cross-review-*`, the L1
records (`docs/wine-repro/batch1-measure/l1-*`, `l1-all-2` to `l1-all-5`)
and the L2 runs of `test-run/all-1` to `all-5` (`l2-all-*`, `l1-all-*`).
The latest paired L2 with every candidate (all-5, run 42889340446) has one
regression, a `test_ClipCursor` check that fails in vanilla Wine too (3 of
6 local runs); all-4 had only the known `ntdll.i386.time` timing check.
"L2 ok" below means: no new failure in the touched DLL's tests and the
neighbouring suites in that run.

Today: 41 of 62 entries carry a valid marker, 21 do not. Every one of the
21 has a candidate or (urlmon NULL terminator) a test in a candidate.

## Status

### The 21 entries without a marker (they need a candidate promoted)

| # | Patch | Candidate (replaces the file of the same name) | Review | L1 (Windows 10) | L2 | L4 | Proposed marker | Open |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 2 | comctl32-statusbar-erase-background | `comctl32-statusbar-v2`: incremental (`0001-status-background-tests.patch`, `0002-status-background-v2.patch` on top of the production file) - to be merged into one file | Claude, approve (2026-11-07) | 2026-11-07, v5 180/0, v6 360/0, both arch | ok (all-1..5, `comctl32:status` 1025 -> 1401) | Inventor: status bar (all-1) | l1 | merge into one patch file; P2 of the review documented, not fixed |
| 3 | comctl32-toolbar-padding-height | `comctl32-toolbar-padding-height-v5` | Codex, approve v5 | batch1-6, all-3..5 (`comctl32:toolbar`), both arch | ok | Inventor palette (all-1, all-5) | l1 | none; the button size difference found on 2026-11-10 was setrows (fixed there) |
| 4 | comctl32-toolbar-setrows-wrapable | `comctl32-toolbar-setrows-v5` | Codex, approve (9b96bc7); v4 has no review, replaced | all-5: every setrows check passed, both arch (one unrelated x86_64 failure in the old `test_WM_PAINT`, outside messages) | ok (all-5) | Inventor palette as on Windows (all-5) | l1 | 4 vertical cases differ from Windows (3+4 groups, 4 rows); Wine's width wrap of vertical toolbars before `TB_SETROWS` |
| 6 | msi-transform-null-stream | `msi-transform-null-stream-v2` | Claude, approve | 43/0, both arch (2026-11-09) | ok (`msi:db`) | Inventor 2026.4 update (v1 code, unchanged) | l1 | workaround wider than measured (any failed `OpenStream`) |
| 9 | ntdll-actctx-threadingmodel-case | `ntdll-actctx-threadingmodel-case-v2` | Claude, approve + re-read of the follow-up (2026-11-11) | all-2: `kernel32:actctx` 3863/0, both arch | ok (kernel32 in g3 since all-2) | Stress Analysis runs (v1 parser, unchanged) | l1 | missing-attribute defaults and invalid-value rejection stay todo_wine |
| 13 | oleaut32-typedesc-pointee | `oleaut32-typedesc-pointee-v3` | Claude, approve | 2026-11-09, both arch | ok (`oleaut32:typelib`) | Design Accelerator (all-1) | l1 | - |
| 14 | oleaut32-typedesc-z-offsets | `oleaut32-typedesc-z-offsets-v2` | Claude, approve | 40095/0, both arch | ok (4 expected size todos) | Design Accelerator (all-1) | l1 | writer leaves record size 0 (separate gap) |
| 15 | riched20-caret-rewrap | `riched20-caret-rewrap-v2` | Codex, approve | batch1-6, both arch | ok | dimension text editing (all-1) | l1 | - |
| 16 | riched20-scf-word-insertion-point | `riched20-scf-word-insertion-point-v5` | Codex, approve v5 (70e3c1d) | batch1-6, both arch | ok | GD&T symbols survive repeated edits (all-1) | l1 | - |
| 17 | riched20-textfont-applytmp | `riched20-textfont-applytmp-v6` | Codex, approve v6 | batch1-6, both arch | ok | repeated spell check, no black text (all-1) | l1 | second unread temp cycle todo_wine |
| 19 | server-mousemove-on-show-hide | `server-mousemove-on-show-hide-v5` | Codex, approve (2c8870f) | all-4: two runs, both arch, 0 failures | ok (all-4, all-5) | sketch value updates at once with 32 ms (all-2) | l1 | hidden-window move todo_wine; counts saturate at 16 (P3); host input can add moves (P3) |
| 20 | shell32-folderview-selection | `shell32-folderview-selection-v2` | Claude, approve | 357/0, both arch | ok (`shell32:selection` 357/0) | click/double-click (v1) | l1 | unmeasured NULL/invalid PIDL branches |
| 22 | urlmon-maptozone-null-terminate | none; its URLs (`x`, `""`, `unknownscheme:`) are in the test of `urlmon-res-zone-v2` | production patch, no candidate | through `urlmon:sec_mgr` (all-2, 692/0) | ok | AutoCAD, deferred | l1, page names the test in `urlmon-res-zone-local-machine.patch` | confirm in the page that those URLs reach the fixed copy path; no own test |
| 23 | urlmon-res-zone-local-machine | `urlmon-res-zone-v2` (includes the tests-first test) | Codex, approve after follow-up (bf3c3b9) | all-2: `urlmon:sec_mgr` 692/0, both arch | ok | AutoCAD, deferred | l1 | module res: URL todo_wine; `patches/tests-first/series` entry is removed with it |
| 24 | wbemprox-timezone | `wbemprox-timezone-v3` | Claude, approve v3 (2026-11-11) | v2 body 104/0 (v3 differs only in skip()) | ok (83/0, all-2..5) | Inventor runs (v1) | l1 | no value coverage in the UTC CI; unmeasured DST modes |
| 28 | server-input-attachment-components | `server-input-attachment-components-v5` | Claude, approve | 16719/0, both arch | ok (`user32:attachment` 12039, 16 todo; neighbouring suites) | Inventor smoke (all-1); SOLIDWORKS deferred | l1 | absolute inactive-state view not available in CI |
| 29 | msxml3-parseerror-filepos | `msxml3-parseerror-filepos-v2` | Codex: approve bounded change, **P2 required before promotion** | all-2: `msxml3:domdoc` 36894/0 (v2 body) | ok | SOLIDWORKS, deferred | l1 after v3 | **blocker**: v3 must add the `get_errorCode` check for the empty string (both classes, measured S_OK/0xc00ce558) and fix the P3 wording; then L1 of `msxml3:domdoc` again |
| 30 | wininet-http-same-position-seek | `wininet-same-position-seek-v2` | Claude, approve | 200/0, both arch | ok (`wininet:http`) | SOLIDWORKS, deferred | l1 | chunked/close-delimited seeks todo_wine |
| 33 | comctl32-header-null-layout | `comctl32-header-null-layout-v2` | Codex, approve | batch1, both arch | ok | Inventor smoke; SOLIDWORKS deferred | l1 | - |
| 36 | comctl32-ownerdraw-existing-capture | `comctl32-ownerdraw-existing-capture-v2` | Codex, approve | batch1, both arch | ok | Inventor smoke; SOLIDWORKS deferred | l1 | - |
| 37 | mshtml-document-open-optional-result | `mshtml-document-open-optional-result-v2` | Claude, approve | 34/0, both arch | ok with pinned Gecko since all-2 (`mshtml:dom` 44107 -> 44140) | SOLIDWORKS Simulation, deferred | l1 | loaded state only in the test |

### The 41 entries that already pass `check-series.sh`

No candidate is needed; they stay as they are. Two have a newer
candidate that may be promoted with them (see notes).

| # | Patch | Marker | Note |
| --- | --- | --- | --- |
| 1 | combase-stub-activation-context | known-deviation (2026-11-10) | candidate `combase-stub-activation-context-v2` blocked; production stays |
| 5 | kernelbase-account-domain-sid | l1 | |
| 7 | mspatcha-pa19-32bit-and-large-files | l3-only (2026-11-10) | optional: replace with `mspatcha-signature-v2` (adds the signature test; Claude approve; L1 50/0; L2 50/0); marker unchanged |
| 8 | msxml3-domdoc-importnode | l1 | |
| 10 | ntdll-locate-xstate-feature-enabled | l1 | |
| 11 | ole32-storage-thread-lock | l1 | |
| 12 | ole32-transacted-truncate-dirty | l1 | |
| 18 | rpcrt4-ndrstubcall3 | l3-only (2026-11-09) | |
| 21 | shell32-getimage | l1 | |
| 25 | winex11-flush-putimage | l3-only | |
| 26 | winex11-shape-alpha-xwayland | l3-only | |
| 27 | winex11-xinput-bad-device | l3-only | |
| 31 | shell32-automation-zip-copy | l1 | |
| 32 | msi-selection-tree-directory-property | l3-only, temporary (2026-11-10) | native UI measurement to be redesigned |
| 34 | msxml3-schema-cache-namespace | l1 | |
| 35 | explorer-tasklist-parenting-hosts | l3-only | |
| 38 | winex11-owner-managed-async | l3-only | |
| 39 | glu32-nurbs-curves | l1 | |
| 40 | win32u-fixed-negative-mat2 | l1 | |
| 41 | win32u-child-topmost-ignore | l1 | |
| 42 | comctl32-themed-checkbox-buffered-paint | l1 | |
| 43 | winex11-stale-map-activation | l1 | |
| 44-60 | msi-addlocal-feature-ancestors ... msi-source-directory-component-state (17 patches) | l1 | |
| 61 | oleaut32-typelib-keep-released | l1 | |
| 62 | opengl32-window-format-color-bits | l1 | |

## Before the promotion

1. **parseerror v3** (blocker): add the empty-string `get_errorCode`
   assertion for both classes and the P3 wording; Codex re-reads; one
   VM visit for `msxml3:domdoc` (both architectures).
2. Maintainer: approve this overview (or strike entries); decide whether
   mspatcha's file is replaced by the signature-test candidate.

## Promotion steps (after approval, one commit on `main`)

1. Copy each candidate file over its production file in `patches/` (table
   above). For the status bar: produce one patch = production +
   `0001` + `0002`, checked to give the same tree at `--fuzz=0`.
2. Remove `urlmon-tests-res-zones.patch` from `patches/tests-first/series`
   (the test moves into `urlmon-res-zone-local-machine.patch`).
3. Set the marker in each of the 21 pages (`<!-- winum-test: l1 -->`),
   update their validation table (L1/L2/L4 with dates and runs, as above)
   and history; mark the candidate folders as promoted (nothing deleted).
4. `WINUM_BUILD`: stays 1 - nothing was published under 11.20-1, so the
   first release is `winum-11.20-1`; raise it only if the maintainer
   prefers a fresh number.
5. Check: every series entry applies with `--fuzz=0` on the official
   11.20 archive (both architectures build: L0); `bash -n scripts/*.sh`;
   `scripts/check-series.sh` **without** `--report` must exit 0 (62 of
   62).
6. A final `test-run/` of the promoted series alone (no substitutions)
   for one paired L2, and a Build Wine of `main` for a last Inventor
   smoke test.
7. Maintainer: signed tag `winum-11.20-1` and `gh release create
   --verify-tag`; the release workflow adds the source archive.

## After the release

- Consumers: pin `winum-11.20-1` and its SHA-256 in
  Autodesk-Inventor-for-Linux (and SOLIDWORKS-for-Linux when work there
  resumes).
- Deferred L4: SOLIDWORKS (server input, parseerror, wininet, header,
  owner-draw, mshtml, msi tree) and AutoCAD (urlmon x2).
- Open measurements: msi selection tree (temporary l3-only); combase
  conforming replacement (known deviation); mspatcha decoder fixtures
  (l3-only); Wine's width wrap of vertical toolbars before `TB_SETROWS`;
  the 3+4/4-rows setrows case; wbemprox values outside standard time.
- Wine bug reports (words only, AGENTS.md) for the fixes with a clean
  synthetic test, starting with toolbar `TB_SETROWS`/button size and the
  show/hide mouse move.
- L5: a week of normal use of the release.
