# shell32-autocomplete-responsive.patch

<!-- winum-test: known-deviation: Windows 10 calls the measured providers on a separate STA while v2 runs batches on the edit STA to preserve COM ownership and restore AutoCAD filename editing (maintainer decision 2026-11-19) -->

On 2026-11-19, maintainer @Harvi654 approved retaining v2 with this
deviation and pausing the compatibility work. V2 remains a candidate on
`topic/shell32-autocomplete-measure`. The full build is on the validation
branch `test-run/shell32-autocomplete-responsive-2`. The follow-up below
stays open until a conforming patch replaces v2.

[Runtime patch and AutoCAD reproduction](../../patches/candidates/shell32-autocomplete-responsive/README.md)

## Problem and fix

AutoCAD 2027 with `FILEDIA=1` could freeze when the user edited a filename
in Save As. Wine 11.22's `autocomplete_text()` called `enumerate_strings()`
in the edit message handler and waited for the full list before yielding
to further input. The Wine trace showed repeated `IsNetDrive` calls during
enumeration.

V2 uses the edit window's timer to call `Reset`, `Expand` and `Next`
after the input handler returns. Each slice yields between provider
calls after up to 64 `Next` calls or about 8 ms of elapsed work. A
single call can exceed that interval. V2 discards obsolete requests
when the edit text or directory changes or the edit closes. It publishes
the finished list for the current request. Review of v1 led to v2 freeing
a request when a callback unwinds and checking the edit state after
reentrant messages from autoappend.

In the paired AutoCAD trial, the released shell32 froze during filename
editing and required termination. With v2, the maintainer could edit
the filename, cancel the dialog, continue drawing and exit normally.
The separate CoreCLR failure while accepting Open or Save As with
`FILEDIA=0` was reproduced with both builds.
[AutoCAD results](../wine-repro/shell32-autocomplete-timing/README.md#autocad-2027-follow-up)

## Known deviation and risk

Windows 10 x86_64 and i686 called `Next` on a separate STA for both the
plain in-process and free-threaded synthetic providers. In the held-call
trials, replacing text, disabling autocomplete and destroying the edit
all returned before the first `Next` did. Replacement showed the new
prefix. Disable and destroy prevented further enumeration during the
observation.

V2 invokes the provider on the edit STA to preserve COM apartment
ownership. It yields between calls. A slow or blocked `Reset`, `Expand`
or `Next` still blocks the edit thread until that call returns. In the
held-call fixture, edit actions ran only after the hold timed out under
v2. Large lists can still take time to populate.

The explicit standard COM control routed the plain provider's call back
to the original STA. Its free-threaded counterpart ran on the MTA
worker. Windows autocomplete also accepted a standard proxy whose
provider was owned by a separate STA. That provider received zero
`Reset` and `Next` calls during the three-second observation. The probe
exited with code 1 for the missing callback. The result leaves both the
worker's HRESULT and the behavior of other proxy configurations open.
[Windows MEASURE results](../wine-repro/shell32-autocomplete-timing/README.md#follow-up-on-com-apartments)

## Validation of v2

All recorded runs below are from 2026-11-19. The tested v2 runtime patch
has SHA-256 `12a8151e995a4411ace1fdd7b25d4706ed3bca3e372d4e9200fd1e6c360f8ba1`.

| Level | Evidence |
| --- | --- |
| L0 | Full Wine 11.22 series plus v2 built for x86_64 and i386 with `--fuzz=0` at `48bc33e`. [Build 44616982879](https://github.com/Harvi654/winum/actions/runs/44616982879) |
| L1 | The original callback-thread test passed on Windows 10. The extended test at `d02aa0b` ran 34 checks per architecture with zero failures or skips and exit code 0. Under v2, the x86_64 replay had three expected Wine todos covering the callback thread, disable and destroy. [Extended L1](../wine-repro/shell32-autocomplete-timing/README.md#l1-result-for-the-extended-lifecycle-test) |
| L2 | At `dca2338`, 21 shell32 units completed for each variant and architecture against the released 11.22-1 runtime. Zero new failure locations. The autocomplete unit ran 14 fewer callback assertions per architecture. This pairing used the original callback-thread test. [Run 44592013573](https://github.com/Harvi654/winum/actions/runs/44592013573) |
| L3 | Synthetic slow-source replay established that v2 returned from `WM_CHAR` without inline `Next`. A reentrant replacement discarded the old request. The later held-call probe reproduced the remaining delay and the proxy difference. [Measurement and fixtures](../wine-repro/shell32-autocomplete-timing/README.md) |
| L4 | AutoCAD 2027 passed the `FILEDIA=1` filename edit and Cancel comparison. Inventor 2027 and 2026 passed Open, Place, Save As, reopen and restart on copies of their prefixes. The runs used the v2 DLLs from `12294c2` with winum 11.22-1. [Tested runtime bundle](https://github.com/Harvi654/winum/actions/runs/44562084665) |
| L5 | A week of normal use is pending. |
| Cross review | V2 incorporates the independent review findings on v1. Final confirmation of the version to be added to the series is pending. |

## Rebuild and resume

Fetch the topic and test-run branches in a winum checkout to recover the
source and workflows after Actions artifacts expire.

```bash
git fetch origin topic/shell32-autocomplete-measure \
    test-run/shell32-autocomplete-responsive-1 \
    test-run/shell32-autocomplete-responsive-2
```

- **Exact v2 runtime.** `12294c2` contains the candidate workflow and
  patch for the released 11.22-1 base. `48bc33e` contains the full
  validation series and build scripts. To rebuild the full artifact,
  check out `48bc33e` in a separate worktree and run
  `bash scripts/build-wine.sh dist` with the build dependencies listed
  in the workflow and sufficient disk space. Verify the supplied SHA-256
  sums when downloading an artifact.
- **Paired Wine baseline.** `dca2338` contains
  `scripts/test-shell32-autocomplete-l2.sh` and the workflow that compares
  both architectures with and without v2. Use this runner as the starting
  point for a new pairing against the same base series.
- **Windows reference.** The topic branch retains the MEASURE source,
  MinGW build commands and text summaries. `d02aa0b` contains the
  extended Wine assertions and `scripts/build-l1-shell32-autocomplete-thread.sh`.
  The generated bundles include `.bat` runners for the offline Windows
  10 VM. Return only the generated text results.
  [Build and run instructions](../wine-repro/shell32-autocomplete-timing/README.md#build)

### Open follow-up for Windows compatibility

1. Start with `IAutoComplete2_fnInit`, `autocomplete_text()`,
   `enum_timer_proc()` and `enumerate_slice()` in
   `dlls/shell32/autocomplete.c`. Use the plain and free-threaded
   fixtures, the explicit marshaling controls and the separate STA proxy
   to test any proposed provider transfer. Ordinary COM marshaling can
   send `Next` back to the edit STA. A raw `IEnumString *` from an
   arbitrary provider may require its original apartment. Establish
   provider ownership and callback delivery before choosing a worker
   design. Measure any new provider case in the offline VM using our
   own fixtures and public interfaces.
2. Keep window updates on the edit thread. A new design needs to return
   control to the edit during an outstanding provider call and discard
   that call's result when text changes, autocomplete is disabled, or
   the edit is destroyed. Check request and COM-reference lifetimes
   through reentrancy and teardown. Check `Reset` and `IACList::Expand`
   when changing directories as well.
3. Use the three existing Wine todos as regression targets. Require the
   measured off-edit-thread callback and the disable and destroy actions
   to pass without those todos. Replay the replacement and separate-proxy
   MEASURE cases on the new code. Preserve the proven AutoCAD filename
   edit and Cancel behavior and both Inventor smoke paths. Repeat L0,
   the paired L2 run and L4 for the revised runtime. Record independent
   review and L5 for the version proposed for `patches/series`.

## Upstream report

The public MEASURE and Wine tests reproduce the edit-thread behavior
with an in-process test provider. A Wine bug report can cite the input
dispatch, callback placement and lifecycle observations and use the
synthetic test to show the difference. The candidate is AI-assisted and
has no upstream submission.
