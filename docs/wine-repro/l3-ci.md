# Shared L3 builds and local desktop collection

The `Build L3 comparison runtimes` workflow builds the **current production
patches/series**, not unpromoted candidates. All four jobs use the same
repository commit, official SHA-256-verified Wine archive and Arch package
recipe, with x86_64/i386 WoW64 support and --fuzz=0:

- `l3-full`: full series, shared by all comparisons and RPC probes;
- `l3-without-putimage`, `l3-without-shape`, `l3-without-xinput`: each
  omits exactly its named winex11 patch in a disposable worktree;
- `l3-fixtures`: unchanged winex11 programs/helpers and the RPC programs
  compiled by the MEASURE builder using `rpcrt4-ndrstubcall3/l3.list`.

Each runtime artifact carries the binary tarball/checksum, corresponding
patched source, exact series, omitted name and commit. SHA256SUMS covers
the payloads. Artifacts are retained for 30 days, without a tag or release.
The normal main series is never edited. Push a new `test-run/l3-*` branch
from the reviewed main commit to launch one run; alternatively dispatch
`build-l3.yml` on the desired commit's branch. Do not reuse another author's
branch. No Wine or GUI fixture executes in this CI workflow.

## One local command

After all five jobs succeed, in a fresh terminal in the normal Arch/KDE
Wayland session, from an updated winum checkout:

```bash
bash scripts/run-l3-local.sh RUN_ID
```

Needs `gh` authenticated via its stored login (`gh auth login` beforehand
if needed), Python 3, tar/xz, coreutils, distribution Wine/wineserver and
the normal runtime libraries used by the project's Arch Wine build. The
runner unsets GH_TOKEN before every gh operation by removing it from its
process environment. No token is requested in chat. Allow several GB of
download/extraction space; no compiler or local Wine build is needed.

It checks the run completed successfully, all five commits match its SHA,
checksums validate and each omitted list differs by exactly its one patch.
Private prefixes force the X11 driver. Only fixture-specific invocations
load the observation/fault helpers; no application prefix is used.
All Wine invocations of fixtures have an external 60-second guard. Raw
output stays temporary; only filtered records, exit codes and entered visual
observations enter `l3-summary-RUN_ID.txt` (existing summaries are refused).
Matching private wineservers are stopped before switching variants and on
exit; temporary runtimes and prefixes are removed. Interrupted summaries
lack `COLLECTION complete=1` and must not be called complete.

PutImage: keep host visible and pointer outside; both render modes run
plain, explicit-flush and observed controls. Shape: click red and missing
quadrants during each phase, then enter transparent/opaque and click-target
observations at the prompt. XInput: physically move inside and click three
times for each observe/open/map run. RPC: the three MEASURE programs run
on both architectures with full-series and distribution Wine. The latter
is only the RPC reference, not an unpatched winex11 control.

A completed collection is **not** an automated pass. Preserve each fixture's
README criteria: XPUT/flush attribution, actual ARGB selection and compositor
observations, same-PID hook/fault coverage, healthy raw/click controls and
RPC exports/results. Missing paths, no input, early close or timeout remain
inconclusive. Do not upload raw logs or treat the CI build as desktop L3/L4.

## Preparation verification (2026-11-11)

Shell syntax and workflow YAML parsing passed. All four private omission
recipes were prepared and checked without building Wine. A host-only mock
run exercised 40 fixture invocations, ten observation prompts, GH_TOKEN
removal, shutdown/collection flow, and rejection of a corrupt artifact
before prefix setup. This is orchestration validation, not Wine L3.
The cloud host lacks MinGW and X11 development headers; actual fixture
compilation and all runtime builds are delegated to the CI jobs with the
listed dependencies. No desktop reproducer or CAD program ran in cloud.
