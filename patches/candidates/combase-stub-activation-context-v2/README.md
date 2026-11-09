# COM stub activation-context v2: blocked conformance candidate

Original, AI-assisted (Codex), LGPL-2.1-or-later. Not submitted upstream.
**BLOCKED:** the native observations do not support the production patch's
first-marshaling capture policy for this standard-marshaled IClassFactory.
This candidate adds a regression test; it does not supply a conforming fix.
Do not promote it or infer a Wine runtime pass from its build results.

Replace the production combase-stub-activation-context.patch in a private
recipe with this file; do not apply both. All production changes to
combase_private.h, rpc.c and stubmanager.c are unchanged. V2 adds
combase/tests/stub_actctx.c and a Makefile entry. No production patch, series
entry or test marker is changed.

## Complete native oracle (2026-11-09)

The maintainer's offline Windows 10 x86_64/i686 MEASURE outputs match apart
from bitness. Both exit 0 with eight scenarios, 48 actual callbacks, no setup
errors and complete=1. Every client call reached exactly one server callback,
returned the probe's chosen E_NOTIMPL and NULL output, and preserved the
client's marker mask. Own context controls verified A=1, B=2, and baseline=0.
The [evidence page](../../../docs/wine-repro/combase-stub-activation-context/README.md)
records the fixture and aggregate results; raw logs are not committed.

| Server apartment | Caller with no test context | Caller with A | Caller with B |
| --- | ---: | ---: | ---: |
| STA | 0 | 0 | 0 |
| MTA | 0 | 1 | 2 |

These callback masks hold for all four object scenarios and both marshaled
streams. In particular, constructing under A, first marshaling under B and
second marshaling under A did not select a persistent B marker. STA callbacks
ran on the creator thread, MTA callbacks on neither the creator nor client;
CoGetApartmentType reported MAINSTA/MTA respectively. The test asserts the
observable context masks, not a native propagation mechanism or a rule for
all COM interfaces. IClassFactory has a call_as proxy/stub; other interface
and out-of-process paths still need their own evidence.

## Why the production policy is not validated

The existing patch captures GetCurrentActCtx when a stub manager is made and
activates it in rpc_execute_call. In dispatch_rpc it copies that saved handle
into params. That policy selects first-marshal A/B in the MTA cases, while
native callbacks in this fixture follow the caller's current phase.

There is a source-path distinction: ClientRpcChannelBuffer_GetBuffer clears
params and sets up a local STA shortcut without copying the saved actctx.
The root patch assigns it only in dispatch_rpc, so the local STA shortcut
may retain native-like mask 0. Do not predict a local STA regression merely
from the first-marshal comment. Actual Wine behavior still needs a paired
runtime comparison; none is claimed here.

The full ordinary-ok regression is retained. No todos or changed expected
values hide the mismatch, and no unmeasured replacement propagation scheme
is implemented. A passing Windows L1 alone will not make this a passing
production Wine fix. Keep the existing CAD workaround unchanged until the
scope, paired runtime evidence, independent review and CAD validation justify
a correction. Native L1 is recorded below; the production marker stays
missing because the production patch still has no integrated test.

## Native assertion L1

```sh
bash scripts/build-l1-com-stub-actctx.sh /path/to/NEW-output
```

The builder checks candidate/test-body.h parity, compiles both static MinGW
EXEs with -Wall -Wextra -Werror, and packages the shared body, native harness,
license and SHA256SUMS. It downloads nothing. Both native assertion and Wine
harnesses call exactly that body; the native reporter uses interlocked
counters for assertions from different threads. The scenario watchdog and
COM lifetime controls are inherited from the completed MEASURE. Trace output
is suppressed by the native harness; failed assertions remain visible.

Copy the complete directory into a fresh directory in the offline Windows 10
VM and run run-measure.bat. Return result-com-stub-actctx-l1-x86_64.txt,
result-com-stub-actctx-l1-i686.txt and exitcodes.txt. Acceptance requires both
exits 0 and a completed SUMMARY checks=... failures=0 with no FAIL lines.
This is the new assertion test, not a repeat of the successful MEASURE.

The main bundle list and topic l1.list also contain com-stub-actctx-l1. The
generic builder does not check candidate/body parity; recheck equality after
future changes or use the dedicated builder before accepting evidence.

## Preparation validation

- Complete Windows MEASURE: both architectures, 2026-11-09. Native assertion
  L1 passed separately, as recorded below; MEASURE was not counted as L1.
- Both standalone native assertion EXEs compile with MinGW GCC 14.2 and
  -Wall -Wextra -Werror. Dedicated bundle checksums verify.
- All 62 patches apply with --fuzz=0 when this replaces the production entry.
  All three resulting combase implementation files match production byte for
  byte, so only the test and its Makefile entry are added.
- Both combase DLL and complete combase_test.exe targets link (x86_64/i386).
  Source: upstream wine-11.20 tag, configuration
  --enable-archs=x86_64,i386 --without-x --without-wayland --without-freetype.
  This is focused compile validation, not an official-archive full-feature
  L0 build or runtime comparison.
- Formal cross review, paired Wine comparison and consumer L4 remain open.
  The test currently cannot be claimed to pass with the production fix.


A second-model read-only advisory read the complete native outputs, candidate,
shared body, native harness and builder, and relevant open Wine marshaling,
class-factory and activation-context code. It confirmed exact oracle/body
parity and balanced COM/handle ownership, and checked the local STA shortcut
separately from MTA dispatch. An explicit winuser.h dependency is included for
the message pump. No conspicuous copied block was identified; this is not
proof of originality, formal cross review or a Wine runtime pass.


## Windows assertion L1 (2026-11-09)

Both offline Windows 10 runs (x86_64/i686) completed **291 checks, 0 failures**,
no FAIL lines, with exit 0. This establishes L1 for the candidate's shared
Wine test body, including all 48 callback observations and client restoration
checks. Raw logs are not committed. The candidate stays BLOCKED: a Windows
pass validates the test, not the production capture policy. Paired Wine
runtime comparison, independent review and consumer validation remain open.
