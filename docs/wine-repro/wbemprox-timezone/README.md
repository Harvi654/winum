# Win32_TimeZone: native WMI versus Win32 MEASURE

Original, AI-assisted (Codex), LGPL-2.1-or-later. Expected native field values,
types and relationships remain unknown. The production patch/page were read
in full, together with the relevant public WMI query setup/enumeration tests.
No production patch, validation marker or series changes.

The program queries only local ROOT/CIMV2 Win32_TimeZone using public COM/WMI
APIs. It snapshots GetTimeZoneInformation before and after, enumerates the
class and repeats with a Bias-equality filter using the API snapshot. Each
row reports Get HRESULT, VARIANT type, CIM type and numeric value alongside
the corresponding Win32 field. Names report only lengths/presence/equality
to Win32 names; Caption/Description report lengths, without printing their
content. No user, host, process, hardware inventory or general WMI dump is
queried. System timezone settings are never changed.

All 21 properties added by the patch are observed. Equality to Win32 is an
observation, not a presumption: native WMI may represent names or transition
fields differently. This allows the later test to be based on returned
relationships instead of the VM's hard-coded timezone or locale. The final
snapshot-unchanged flag detects a settings/transition change during the run;
if it is false, do not derive comparisons from that run without explaining it.

COM security and proxy setup are process-local. Each enumerator Next has a
five-second limit; at most eight objects per query are inspected, and a
45-second independent process watchdog covers initialization and shutdown.
All returned VARIANTs, BSTRs and COM references are released. Normal setup,
timeout and bound errors produce nonzero exit. Zero exit does not imply an
assertion L1 pass or that a query returned an object: require complete row
observations and an unchanged API snapshot before deriving native values.

Both MinGW architectures build with -Wall -Wextra -Werror. The timezone
entry is in both bundle lists. Return result-timezone-x86_64.txt,
result-timezone-i686.txt and exitcodes.txt from the one final batch-3 VM run,
including any incomplete process. Only text returns from the offline VM.
No vendor files, symbols or native implementation internals are read.
Native acceptance, assertions and runtime gates remain open; raw logs will
not be committed. A requested second-model advisory earlier in this batch
could not run because of its usage limit; no independent review is claimed
for this source.

## Windows observations (2026-11-09)

x86_64/i686 text matches apart from bitness, both exit 0, api_errors=0.
The all query returns exactly one complete object, then S_FALSE/no object.
The Win32 snapshot is unchanged. This provides observations for the current
VM timezone, not hard-coded timezone-independent constants for a test.

- Bias: WMI -480 versus Win32 +480, VT_I4/CIM_SINT32. Thus copying the
  Win32 Bias unchanged contradicts this measured native case.
- DaylightBias: -60 in both, VT_I4/CIM_SINT32.
- StandardBias: 0 in both, VT_I4/**CIM_UINT32**. The patch declares SINT32,
  which disagrees with the observed class metadata even when the value is 0.
- Both sets of transition Day/Hour/Minute/Month/Second/Year fields equal
  the Win32 snapshot and use VT_I4/CIM_UINT32. DayOfWeek equals the API and
  uses VT_UI1/CIM_UINT8. All Get calls return S_OK.
- StandardName and DaylightName are present BSTR/CIM_STRING and equal their
  corresponding Win32 names (each length 21 in this run). The names are not
  printed or recorded as fixed expected text.
- Caption and Description are present BSTR/CIM_STRING, each length 38;
  the production patch's empty strings disagree with these observations.
  Matching lengths alone do not establish matching text.

The Bias=Win32-Bias filtered query returns zero rows. It is not a positive
row-filter control when native WMI uses the opposite sign, so do not turn
that filter into a presumed one-row assertion. A native-derived test must
express the observed relations/types and document the locale/settings
limits; it must not adopt the patch's unchanged Bias or signed StandardBias
as its oracle. Correcting those differences would be candidate work after
the current MSI assertion gate, not a production change in this measurement
record. No L1 pass or marker upgrade is inferred from MEASURE; raw logs are
summarized rather than committed.
