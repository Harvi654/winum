# Cross review: urlmon res zone v2 (2026-11-09)

Reviewer: Codex; candidate author: Claude. Reviewed at `54cfb6a`.

Read in full: candidate patch/README/series, production v1 patch, tests-first
res-zones patch, MEASURE source, investigation README, both Windows result
files in windows-2026-11-09 and the production patch page. Read the relevant
Wine 11.20 (`7298a818b79446bae153b8d5aa27235ab3703bf9`) implementations of
get_zone_from_reg, get_zone_from_domains, map_security_uri_to_zone,
map_url_to_zone and CoInternetGetSecurityUrl, and the test unit's initialization
and insertion context. The rest of URLMON and the complete existing security
manager suite were not audited. The vanilla/series values below are the
candidate README's table from run 42569334513, not a new local runtime run.
No proprietary code, symbols or binaries were inspected.

## Implementation and measured matrix

v1 assigned local-machine zone to every res URI reaching this point with
URLZONE_INVALID. The supplied pair shows why this is too broad: it changes
res:/// from native 3 to 0, nonexistent modules from 3 to 0, and res:/res:x
from native -1 to 0. The existing-module case reaches native zone 0 only by
accident; its security URL remains wrong.

v2 limits the special path to a case-insensitive res scheme and a raw URI
starting with res:///. It asks the existing protocol-default registry helper
for the zone (default Internet) instead of assigning local-machine. Other
res spellings retain their domain/scheme path. The literal percent-containing
input fails URI construction before this change, as the native measurement
requires; the patch does not expand Autodesk placeholders.

All ten new test rows' HRESULT/zone/security-URL expectations match our two
Windows outputs. In particular, native res: and res:x return S_OK with
URLZONE_INVALID; they must not be changed to Internet or local-machine by
this fix. The two unknown-scheme controls and invalid bare/empty URLs are
also measured. The existing shell32 module mapping remains unimplemented:
its expected zone and file: security URL are correctly retained under
separate todo_wine checks, while its HRESULT stays an ordinary assertion.
No todo hides the target res:/// result.

The helper frees the raw BSTR and scheme on the new failure branch and frees
the raw BSTR on success. Only the res-specific implementation block changes
relative to v1. Calls after it and the module-to-file mapping remain outside
scope. Protocol-default overrides are respected, but this measurement does
not establish Windows behavior under modified zone policy.

## Findings

- **P1: none identified in the bounded implementation change.** It corrects
  the measured overbroad v1 mapping while retaining the known module-mapping
  limitation explicitly. Native L1 and runtime checks are still required.
- **P2: security-URL output is not asserted present.** Windows returned a
  non-NULL string for every row. The test checks HRESULT but puts every
  string assertion inside `if(secur)`, so S_OK/NULL silently passes that
  part, including the module todo. Add an ordinary `ok(secur != NULL, ...)`
  before guarding its use. This strengthens a measured contract rather than
  inventing an output requirement.
- **P2: the revision header has no dated v2 paragraph.** AGENTS.md requires
  a dated vN paragraph after review. The header explains the behavior change
  but does not identify v2/date. Add that paragraph, including the measured
  scope and remaining module-mapping todo; do not imply L1/L2 already passed.
- **P3: use a string comparison for the file prefix.** The fixed-size
  memcmp reads seven WCHARs without checking the returned string length.
  A short erroneous result should report an assertion, not risk reading
  beyond its allocation. A bounded wide-string comparison expresses the
  intended prefix check and stops at NUL.
- **P3: keep repository attribution/context outside the test body.** The
  test's winum/measurement-path comment is review provenance, suited to the
  patch header. The function itself should follow the surrounding Wine
  tests' style, as already requested in the actctx review.

The module todo is justified by an actual native/Wine difference; its
remaining implementation gap should not be described as complete res URL
compatibility. The supplied Windows values come from the maintainer's
measured policy. This review does not establish equivalence for unmeasured
URI spellings or error paths under missing registry keys.

## Checks and verdict

Replaced v1 with this candidate at its existing position in a separate
pinned Wine 11.20 tree. All 62 production series entries applied with
`--fuzz=0`; no production file or series entry was edited. The separate
tests-first res-zones patch was not also applied: it would duplicate this
function, as the candidate README warns. No build, Windows assertion run,
Wine runtime pass or L2 result is claimed by this source review. The
maintainer's candidates-4 run is left untouched.

Verdict: the bounded implementation is supported by the supplied observations.
Address the output-presence assertion and dated revision header before
promotion, with the small test cleanups above. Native L1 of the actual
assertions, paired URLMON L2 and AutoCAD's icon/startup smoke test remain
open. The corrected zone is 3 rather than v1's 0, so the earlier v1 consumer
success cannot stand in for that new smoke test. Keep the candidate outside
production pending those gates.

## Revision check: bf3c3b9 (2026-11-09)

Read the complete revision diff and updated candidate header/test. The new
ordinary assertion requires a returned security URL for every measured row,
and the header now has its dated v2 paragraph. Both P2 findings are addressed;
the bounded implementation is unchanged. The two P3 cleanups (bounded string
prefix comparison and moving provenance out of the test body) remain minor
review suggestions. All 62 entries, substituting the revised candidate,
applied with --fuzz=0 in a fresh pinned Wine 11.20 tree. No build/runtime or
native assertion result is claimed by this follow-up.

The maintainer has explicitly deferred AutoCAD work because it is not
installed. Record its L4 as deferred, without requesting an installation
for this review. The Wine/native validation gates remain independent and
open. The corrected zone's actual AutoCAD behavior remains unverified;
deferral does not turn v1's old consumer result into evidence for v2.
