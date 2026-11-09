# Win32_TimeZone v2: measured properties and conformance test

Original, AI-assisted (Codex), LGPL-2.1-or-later. Candidate only; the
production patch, its test marker and patches/series are unchanged.

## Windows basis and implementation changes

The original 21-property MEASURE and the label-source follow-up both
completed on offline Windows 10 x86_64/i686 on 2026-11-09, with matching
architecture outputs. See the [native observations](../../../docs/wine-repro/wbemprox-timezone/README.md).
The stable sample is standard time, automatic daylight adjustment enabled,
StandardBias zero. The observed Bias is -480 versus Win32 base +480;
StandardBias is VT_I4/CIM_UINT32. Names/transition fields match Win32.
Caption and Description each equal the complete raw registry Display value,
not either Win32 name or the indirect MUI_Display text. Neither localization
API attempt resolved a source in this sample.

The candidate retains the class and corrects three measured differences:
negate the base Bias, declare/store StandardBias as UINT32, and obtain the
two labels from the current dynamic timezone's raw Display registry value.
Each label has independent dynamic ownership. All four row strings are
released if allocation fails or the query filter rejects the row. Failed
GetTimeZoneInformation aborts the fill instead of reading uninitialized data.
The label reader checks key/path bounds and registry type/length, and owns
the copied string; it never returns an alias of temporary storage.

Empty labels remain the fallback when the current key/Display is unavailable.
That fallback is preserved behavior, not a newly measured Windows result.
Successful MUI localization, custom zones, disabled automatic DST, nonzero
standard adjustment and active daylight time remain validation gaps. In
particular, the native sample cannot distinguish negated base Bias from
negated standard/active totals. Choosing negated base is the smallest
candidate correction for the observed case, not proof of the other modes.
Do not promote this candidate as covering those unknown cases.

## Wine test and L1 bundle

A new dlls/wbemprox/tests/timezone.c unit queries just Win32_TimeZone. It
requires the class, one object/end of enumeration and all 21 observed
property CIM/VARIANT types (including non-NULL strings). It asserts values
against the public Win32 snapshot only when that snapshot is unchanged,
both APIs report standard time, automatic DST is enabled and StandardBias
is zero. Display equality additionally requires a readable, terminated
REG_SZ source. No native string, timezone name or registry path is printed.

Outside the measured value conditions it prints a skip and still checks
class/types/presence; missing Display similarly marks the omitted label
comparison. This is partial coverage, not a full value-test pass. Native
L1 acceptance requires the measured fixture conditions and zero skips.
The raw Display relation has only been measured in the returned setup;
cross-locale/MUI behavior still needs a separate observation and review.

All target assertions are ordinary ok(); no todo hides the absent vanilla
class or the previous patch's wrong metadata/values. The predicted failure
on vanilla and success with this candidate need observed runtime results.
The test changes no timezone, registry value or other system setting.
COM, BSTR, VARIANT, enumerator/object and registry resources are released.
The stable snapshot brackets the query; independently editing Display during
the test is outside the measured conditions and is not detected by the
timezone API snapshot alone. Do not change settings while it runs.

```sh
bash scripts/build-l1-timezone.sh /path/to/NEW-output
```

The builder verifies the entire named helper/function block against the
candidate test, then compiles the unchanged body in the native wrapper for
x86_64/i686 with -Wall -Wextra -Werror. No BEGIN/END markers are inserted in
Wine source. The EXEs carry the common original asInvoker/Windows-10
manifest and an independent 45-second process watchdog. Enumeration calls
are individually bounded to five seconds. A watchdog exit 2 is not a pass.

Run run-measure.bat in a fresh offline Windows 10 folder with the same
timezone settings as the measurements. Return result-timezone-l1-x86_64.txt,
result-timezone-l1-i686.txt and exitcodes.txt, even on failure. Full successful
execution in those conditions should report 104 checks, zero failures and
zero skips, exit 0 each. Native assertion execution is pending. The main
measure list also contains this entry; use the dedicated builder for parity.

## Application and checks

Substitute this patch for wbemprox-timezone.patch at its existing series
position, retaining every other production entry. Do not apply both versions
or edit patches/series. The full 62-entry series applies with --fuzz=0 using
the candidate on upstream Wine 11.20 commit
7298a818b79446bae153b8d5aa27235ab3703bf9, source archive SHA-256
9f516d63e9f814f4dad70ce0a1943211b8a6236008ac3025cf58004091c661db.
The final candidate was strictly reapplied over those unchanged neighbours.
Applied test-body parity and both strict standalone builds pass. Focused
Wine builds link complete wbemprox.dll and wbemprox_test.exe PE targets for
x86_64 and i386, including the new timezone unit. Configure used
--enable-archs=x86_64,i386 --without-x --without-wayland --without-freetype;
this is focused compilation, not a full graphical Wine L0 build or runtime.
The new unit uses the GUID definitions already supplied by query.c, avoiding
duplicate definitions in the combined test executable. No Wine runtime or
native assertion pass is inferred from compilation. Bundle checksums pass.

A second-model read-only advisory read the complete original candidate,
test body, harness and builder against the measurement results and open
Wine table ownership code. It found the packed record/column types aligned,
distinct string ownership and OOM/filter cleanup consistent, no blocking
defect and no conspicuous copied nontrivial block. Suggested defensive key/
path and registry-source bounds checks are included. This limited advisory
is not an originality certificate, native L1 or formal cross review.

Native exact-body assertion L1, paired wbemprox L2, observed vanilla/patched
runtime, Claude's formal cross review, the unmeasured timezone modes and
consumer validation remain open. No production marker upgrade is claimed.
