# MSXML3 same-node validation for winum PR #4

[PR #4](https://github.com/Harvi654/winum/pull/4), reviewed at
`126157d`, supplies a Wine regression test and an Inventor 2027
reproducer. The contributor ran Wine 11.20 x86_64 with and without
the patch. The Inventor sketch preview completed with the patched
runtime and the TrueView profile stayed at about 21 KB. The PR
contains no Windows result or runnable offline VM bundle.

## Offline Windows 10

Use the [MEASURE artifact](https://github.com/Harvi654/winum/actions/runs/44529214420/artifacts/14532423469).
On Linux, extract the artifact and run `sha256sum -c SHA256SUMS` in
the resulting folder. Copy the folder into the offline Windows 10
snapshot and double-click `run-measure.bat`. Return `exitcodes.txt`,
`result-msxml3-same-node-x86_64.txt` and
`result-msxml3-same-node-i686.txt`. The fixture uses a synthetic DOM
and writes no user data or paths to its results.

The two operations each get a fresh tree. The program records the
HRESULT and returned COM identity with and without an output pointer.
It uses both the same interface and two requests for the same
underlying child. The parent is either an element or the document
whose child is the root. It records parent, last child, length and
XML equality. These lines are observations for deciding the expected
test values. A watchdog bounds execution if a child-list walk hangs.

To rebuild the standalone bundle with MinGW:

```bash
export MSXML3_MEASURE_OUT="$PWD/msxml3-same-node-vm"
bash scripts/build-measure-bundle.sh "$MSXML3_MEASURE_OUT" \
    docs/wine-repro/msxml3-same-node-measure/measure.list
(cd "$MSXML3_MEASURE_OUT" && sha256sum -c SHA256SUMS)
```

The exact contributor test is also built for x86_64 and i386 by the
[test job](https://github.com/Harvi654/winum/actions/runs/44528763253).
Its `msxml3-same-node-l1` artifact contains `run-tests.bat`. The
standalone MEASURE establishes Windows behavior. L1 on the Wine test
is recorded after that test reflects the measurements.

## Windows 10 result (2026-11-19)

The maintainer ran the offline bundle from `81a3a91`. x86_64 and
i686 both exited 0. All 16 case records are identical after the
architecture banner.

| Parent | Operation | Return | Tree after the call |
| --- | --- | --- | --- |
| Element | replaceChild | `S_OK` | Two children, same last child, parent and XML |
| Element | insertBefore | `S_OK` | Two children, same last child, parent and XML |
| Document | replaceChild on the root element | `S_OK` | One child, same root, parent and XML |
| Document | insertBefore on the root element | `E_FAIL` | One child, same root, parent and XML |

For successful calls with an output pointer, the returned object has the
child's COM identity. With separately retrieved interfaces, Windows gave
different `IXMLDOMNode` pointers for the same child and the same
`IUnknown` identity. Wine uses independent wrapper identities for
element accesses. The revised regression test checks that the return
matches the relevant input wrapper and verifies the same tree outcome.
Leaving the insertBefore same-child guard after the type check preserves
the measured `E_FAIL` for the root. The runtime hunks are unchanged.
This is the MEASURE result. L1 for the revised Wine test is pending.

## AutoCAD 2027 comparison on Arch

Use the installed winum 11.22-1 for the baseline and the
[PR #4 runtime artifact](https://github.com/Harvi654/winum/actions/runs/44528762825/artifacts/14533394848)
for the candidate. Build `aeb7a59` completed on Wine 11.22 with WoW64. The baseline Wine patch set is identical to
`winum-11.22-1`. The test branch adds only the contributor's MSXML
patch to the normal runtime. The candidate tarball uses the same
filename as the stable build, so unpack it in a separate directory.

Run this from your winum checkout:

```bash
export MSXML_BASE="$HOME/.local/share/winum/winum-11.22-1-x86_64"
export MSXML_BUILD_DIR="$(mktemp -d "$HOME/.winum-msxml3-pr4.XXXXXXXX")"
export MSXML_BUILD_NAME=winum-11.22-1-x86_64
export MSXML_CANDIDATE="$MSXML_BUILD_DIR/$MSXML_BUILD_NAME"

(
    set -eu
    gh run download 44528762825 --repo Harvi654/winum \
        -n winum-11.22-1-binary -D "$MSXML_BUILD_DIR"
    cd "$MSXML_BUILD_DIR"
    sha256sum -c "$MSXML_BUILD_NAME.tar.xz.sha256"
    tar -xJf "$MSXML_BUILD_NAME.tar.xz"
    grep -E '^commit:[[:space:]]+aeb7a59$|msxml3-replacechild-same-node\.patch$' \
        "$MSXML_CANDIDATE/BUILDINFO"
    grep -Eq '^commit:[[:space:]]+aeb7a59$' "$MSXML_CANDIDATE/BUILDINFO"
    grep -q 'msxml3-replacechild-same-node.patch' "$MSXML_CANDIDATE/BUILDINFO"
)
```

Close AutoCAD, then check free space and the prefix size. Make two
copies beside your regular prefix on the same filesystem. Each
copy can need the full prefix size on filesystems without reflinks.
The copies contain sign-in data.

```bash
export ACAD_SOURCE="$HOME/.wine-autocad2027"
export ACAD_BASE_PREFIX="$HOME/.wine-autocad2027.msxml3-base"
export ACAD_PR4_PREFIX="$HOME/.wine-autocad2027.msxml3-pr4"
df -h "$HOME" "$ACAD_SOURCE"
du -sh -- "$ACAD_SOURCE"

(
    set -eu
    umask 077
    test -d "$ACAD_SOURCE/drive_c"
    for prefix in "$ACAD_BASE_PREFIX" "$ACAD_PR4_PREFIX"; do
        if [ -e "$prefix" ] || [ -L "$prefix" ]; then
            printf '%s\n' 'A test prefix already exists. Check it before copying.' >&2
            exit 1
        fi
    done
    cp -aT --reflink=auto -- "$ACAD_SOURCE" "$ACAD_BASE_PREFIX"
    cp -aT --reflink=auto -- "$ACAD_SOURCE" "$ACAD_PR4_PREFIX"
)
```

Run the same operation in each copy. Reproduce the earlier affected
startup or profile action if you remember it. Also check a disposable
drawing, save/reopen, close AutoCAD, and restart. Do not reset the
profile before the first paired test. If the existing launcher
normally starts prerequisites, keep those settings for both runs.
The standalone watcher starts `acad.exe` via the selected Wine.

```bash
git fetch origin topic/msxml3-same-node-measure
export ACAD_PROFILE_WATCH="$(mktemp "$HOME/.acad-profile-watch.XXXXXXXX.py")"
git show FETCH_HEAD:docs/wine-repro/msxml3-same-node-measure/watch-autocad.py \
    > "$ACAD_PROFILE_WATCH"
python3 -I "$ACAD_PROFILE_WATCH" "$ACAD_BASE_PREFIX" "$MSXML_BASE"
python3 -I "$ACAD_PROFILE_WATCH" "$ACAD_PR4_PREFIX" "$MSXML_CANDIDATE"
```

To use the local AutoCAD launcher instead, give the watcher
`--launcher "$HOME/autocad-on-linux/scripts/launch-autocad.sh"`. It runs
the launcher from its project root and exports the selected Wine,
wineserver and test prefix. If the launcher has its own prefix or Wine
selector, configure it to use those same paths. Use `--log-dir` with a
fresh log directory if the launcher redirects Wine traces there. A
missing MSXML load trace leaves the DLL selection unverified.

The watcher reports size counts for `Profile.aws`, the largest size,
growth from the starting state, the exit code and selected MSXML
trace counts. It samples every 0.75 s. At 64 MiB additional growth it
requests shutdown of that copy's wineserver. Ctrl+C does the same
for a hang. No full paths or raw traces are written. Return only the
two summaries and say what happened in the UI. A native MSXML load
does not exercise the patched Wine DLL. If the baseline does not
reproduce the issue, the comparison is a smoke test. If it does
reproduce, the same file name and a stack in MSXML parsing are still
insufficient to identify the same write loop. Compare the behavior,
the trace counts and the size change.

## Review and merge gates

- The guard compares the underlying Wine DOM object and explains the
  observed TrueView child-list cycle. Each mutating call should be
  tested on a separate tree. The submitted test currently runs
  insertBefore after replaceChild on a potentially damaged tree.
- At the document root, node_insert_before checks child type before
  the new same-node guard. The existing single-element check sees the
  root already in the document and can reject inserting it before
  itself. The Windows fixture confirms that this check must remain
  before the guard.
- A generated `__pycache__/check.cpython-311.pyc` from another topic
  appears in the PR diff and should be excluded. The current commit
  author uses the contributor's handle and GitHub noreply address.
- Before promotion into `patches/series`: measure Windows, match and
  rerun the final regression tests on both Windows architectures,
  record L0/L2 on the full Wine 11.22 series, run the maintainer's
  Inventor preview and smoke checks and record this AutoCAD comparison.
  Add a patch page with the L1 marker once earned. The release checker
  expects the standard patch header. Preserve contributor attribution
  in the page and history. The PR's normal build did not include the
  candidate, which is outside its series.
