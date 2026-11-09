# Server input attachment components v3: foreground setup, round 1

Candidate only; replaces v2 in an experimental build. Production
patches/series, the production patch and its marker remain unchanged.
The server/queue.c and server/window.c hunks are byte-identical to v2.
Claude's conditional approval of v2 covers that implementation; the
changed test needs its own review and validation.

## Why this revision

Batch4-1 (42695491325) finished the test but retained 2377 i386 and
2370 x86_64 ordinary active/focus failures. The first mismatch already
occurs before any attachment, also in vanilla under Xvfb. The old helper
checked command delivery, not successful foreground activation.

Per the maintainer's decision, this is **foreground setup round 1 of at
most three**. Known unrelated full-series failures (mousemove v1's mouse
sequence, toolbar v1, urlmon v1, ntdll i386 exception) are not investigated
here. Their attribution is recorded in the investigation's L2 directory.

## Changed setup and unchanged assertions

Create the four own windows hidden, then show without incidental
activation. Before any explicit edge or child exists, seed each independent
queue's active/focus state and process pending events within a bound.
Explicit graphs use their literal measured isolated row, including the
special first case (A active, B/C/D NULL); child cases start with the four
independent own windows active/focused. This establishes a prerequisite;
the test no longer assesses automatic initial activation.

Setup-only ACTIVATE_START retries ordinary show/foreground/active/focus
calls and drains pending events. Check actual foreground, visibility and
every worker's GetGUIThreadInfo active/focus/capture state. An unreached
initial state emits a named skip and cleans up before any edge or child.
Transport/API failures remain hard failures. No synthetic input or
attachment to a foreign thread is used to gain foreground permission.

Once a graph exists, ACTIVATE retains the original one-shot call order,
with only passive result observations added. Failed activation is a hard
failure/abort, never a skip. All later state assertions, graph descriptions,
592 literal checkpoints, 52 detach results/errors and existing narrow todos
are unchanged. No post-attachment state is used to repair observed state.

ATTACHMENT_COVERAGE reports completed/skipped separately for all 44
attempts. A skip is not completed coverage. The native wrapper rejects
any skipped scenario (exit 2) and has a six-minute watchdog for the added
bounded preparation. An all-skipped Wine test is unvalidated even if its
standard test summary returns success.

Seeding changes setup history. A fresh native L1 of this exact body must
confirm that the unchanged later Windows expectations still apply. V2's
native pass does not transfer to v3; do not change the oracle to Wine values.

## Validation

- Full-series --fuzz=0: 62 entries with v3 substituted.
- Native x86_64/i686 compile: -Wall -Wextra -Werror; exact-body parity
  checked by scripts/build-l1-input-components.sh.
- Focused feature-disabled build: wineserver and both full user32 test
  executables link. No cloud Wine execution or full graphical L0 claim.
- Larger-code second-model advisory: no remaining concrete blocker or
  recognizable copied code found in the bounded review (not proof of
  originality). Its post-attachment activation and visibility suggestions
  were incorporated. This is not a promotion cross review.
- **V3 native L1 pending:** require 44 completed, zero skipped/failures and
  exit 0 on both architectures. The Wine-unit native variant must also
  have zero todos/flaky/skips. Original v2 evidence is retained.
- **V3 paired L2 pending:** require 44 completed, zero skipped and zero
  ordinary series failures per architecture before claiming validation.
  Verify vanilla still demonstrates the target difference. Expected Wine
  todos remain 12. Incomplete coverage must not be presented as a pass.
- Review of changed setup, applicable L4 and normal-use checks pending;
  SOLIDWORKS L4 remains deferred under AGENTS.md.

If setup remains unreliable under Xvfb, proceed to the maintainer's second
option (separate foreground-independent checks), then only if needed the
third (keep dependent coverage in native L1). Do not spend the three rounds
changing unrelated patches or adding broad todos.

## Native bundle

```bash
export INPUT_L1="$HOME/.cache/winum-input-v3-l1-$(date +%Y%m%d-%H%M%S)"
cd "$HOME/winum"
git pull --ff-only
bash scripts/build-l1-input-components.sh "$INPUT_L1"
```

The default builder/list now target this candidate and the investigation's
v3/ sources. The v2 body/wrapper remain at their original locations.
Alternatively use the main Build MEASURE bundle workflow with list input
`docs/wine-repro/batch4-measure/l1.list`; no batch-specific workflow is added.

Extract into a fresh offline Windows VM directory, run run-measure.bat
without touching input, and return result-input-components-l1-x86_64.txt,
result-input-components-l1-i686.txt and exitcodes.txt. For the paired
run's Wine-unit L1 bundle return result-user32-attachment-*.txt and
exitcodes.txt. Check the coverage line as well as the failure summary;
a skipped native setup is incomplete L1.

## Retained open points

Reparenting again after SetParent(child,NULL), including a third thread's
host, remains unmeasured. The server implementation is unchanged.
Foreground dependence is now an explicit prerequisite/coverage gate, not
assumed solved before revised native and paired results arrive.


## Returned round-1 result (2026-11-10)

Run 42713756358, series coverage returned: 44 completed, 0 skipped on both
architectures, 10235 checks and the expected 12 todos. Ordinary failures
fell to 301 (i386) / 305 (x86_64), at active/focus assertions. The first
is already after joined-probe-B of the isolated nonedge graph, observer A;
no explicit attachment exists there. This is not a failed initial-state
skip. See the investigation's l2-v3-2026-11-10 directory for original data.
Round 1 made progress but did not pass. Proceed to the requested separation
of component checks and foreground-dependent absolute state; preserve the
literal native observations and do not add broad todos. Native v3 L1 is
not assumed passed.
