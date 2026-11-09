# Server input attachment components v2

Candidate only. Production patches/series, the production patch and its
missing-test marker are unchanged. At entry 37 substitute this directory's
server-input-attachment-components.patch for the production file.

## Native basis and change from v1

The Windows 10 x86_64/i686 output in
docs/wine-repro/server-input-attachment-components/windows-2026-11-09/
comes from bundle run 42659893206 (`e0421f4`). Both runs completed 44
scenarios with zero harness errors and exit 0; their bodies match apart
from bitness. All records were checked for personal/machine identifiers
before committing. See that directory's README and SHA256SUMS.

Explicit removal is consistent with preserving remaining reference
components: chains, bridges, cycles, duplicates/opposite edges and
nonedges, both detach directions and active anchors. The test retains
literal passive observations as well as controlled focus probes; it does
not generate expected output from a BFS or union-find model.

The implicit sequences expose an important limit in v1. With child-A and
child-B owned by C under hosts A/B, Windows SetParent(child-B,NULL) retains
the A/B/C association, including after child-B is subsequently destroyed.
Vanilla preserves B's active A there; v1's component split would newly
clear B's active state. V2 keeps the existing association when reparenting
to the desktop, so it preserves that already-compatible field and the
remaining host/child focus together. The existing v1 graph walk, active
input-object retention and allocation rollback are otherwise unchanged.
Other reparent destinations and detach-all are unchanged.

Directly destroying child-B, without unparenting first, is different on
Windows: B separates while A/C retain child-A focus. Wine's direct
destruction does not remove that implicit edge; fixing that separate path
is outside v2. Only B's active/focus assertions after direct destruction
are todo_wine. Surviving A/C state, capture, the unparent case and all
explicit graph targets are ordinary assertions. The self-detach error
(Windows invalid parameter, Wine access denied) is another narrow todo;
its false return remains ordinary. No target regression is hidden.

## Wine test and native assertion build

The patch adds dlls/user32/tests/attachment.c and its Makefile entry.
The same body is in test-body.h, with 592 measured state checkpoints and
52 detach result/error records. The standalone wrapper treats all
Wine-only todos as ordinary assertions on Windows.

```bash
export INPUT_L1="$HOME/.cache/winum-input-l1-$(date +%Y%m%d-%H%M%S)"
cd "$HOME/winum"
git pull
bash scripts/build-l1-input-components.sh "$INPUT_L1"
```

That builder verifies byte parity with the candidate before compiling both
architectures using -Wall -Wextra -Werror, and hashes binaries, body,
wrapper and manifest. For the main Build MEASURE bundle workflow, supply
`docs/wine-repro/batch4-measure/l1.list` as its list input; no new workflow
is needed. The older batch-specific workflow is left unchanged.

In the offline Windows VM run run-measure.bat from a fresh extracted
directory without touching mouse or keyboard. Return the two
result-input-components-l1-*.txt files and exitcodes.txt. A native pass
needs zero failures, 44 complete scenarios and exit 0 on both architectures.
The prior measurement's exit 0 is not substituted for that check.

## Validation and remaining scope

- Native MEASURE: both architectures complete; values above.
- Standalone compile: both architectures, -Wall -Wextra -Werror.
- Full-series --fuzz=0 and focused server/test compilation: recorded in
  the investigation README after final verification; not a full graphical
  L0 claim.
- Native assertion L1: pending.
- Paired L2: [run 42695491325](https://github.com/Harvi654/winum/actions/runs/42695491325),
  own test-run/batch4-1 at 36b639e. Started 2026-11-09; result pending.
  Both variants were applied with --fuzz=0 before pushing: their new test
  bytes match the candidate exactly, only series has the implementation.
  The g1 series artifact includes user32:attachment for native L1.
- Larger-code second-model advisory: literal data was independently
  checked against both native files, with no value mismatches; no obvious
  fixture blocker or recognizable copied code was identified in the
  bounded comparison (not proof of originality). Formal cross review for
  promotion still belongs to the other assistant.
- L4: SOLIDWORKS deferred under AGENTS.md; other applicable consumer smoke
  and normal-use validation have not been performed for v2.

The broad observation table also checks creation/activation neighbors.
Exact-body native L1 is needed to establish its stability; a failure must
be investigated rather than changing Windows values to the Wine result.
Unmeasured: different desktops/DPI, cross-process behavior, repeated
reparent histories, callbacks and allocation failure through Windows.
The old native graph shim is not a Windows oracle. In particular, this
measurement does not establish a universal implicit-attachment lifetime
rule beyond the tested early/late and direct/unparent sequences.
