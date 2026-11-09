# HTTP same-position seek: loopback Windows MEASURE

Original, AI-assisted (Codex), LGPL-2.1-or-later. Native seek results, body
continuity and request counts are unmeasured. Production patch/page and
Wine's HTTP set-file-pointer/read paths and related seek tests were read;
this is an original bounded server/matrix, not copied response fixtures.
No production patch, marker or series changes.

An own Winsock server binds 127.0.0.1 at an OS-selected port and serves the
same generated 16384-byte body using Content-Length, chunked framing or
HTTP/1.0 close-delimiting. It sends Connection: close and no compression.
Each framing has four fresh request handles: no-seek control, FILE_CURRENT
distance zero after reading 17 bytes, FILE_BEGIN to that same position, and
the current-position call with NO_CACHE_WRITE. The last case is a separate
negative/control observation, not presumed to allow seeking.

InternetOpen uses DIRECT access; cookies, authentication and UI are disabled.
No DNS or external server, credentials, proxy or application is needed.
Each case has a distinct URL under the own ephemeral listener. After closing
its handles the program attempts deletion of only that URL's cache entry;
failure is not interpreted as a seek result and may leave that own cache
entry. No unrelated cache content is enumerated or deleted.

Outputs report the actual seek position/last error, read status, byte count,
full-body comparison including the already-read prefix, EOF, and the own
server's request-count delta. Last error after a successful seek may retain
the sentinel; record it without treating it as an API failure. BODY error
is separate and nonzero only for a failed body read. Comparing request counts
can distinguish a no-op from a second request; no outcome is hard-coded.

No-seek bodies must reach EOF with the complete original content, and every
prefix must match. Failed controls/setup invalidate the observation. Other
seek/body outcomes are measured, not asserted. Require all 12 CASE_END rows
and SUMMARY complete=1 with no API errors before selecting native values.
This does not cover TLS, Range responses, gzip, asynchronous WinINet,
persistent connections, redirects or large media downloads.

Socket and WinINet timeouts bound operations. The actor owns accepted
sockets; the main thread signals stop and joins it before closing listener
state or WSACleanup. A 60-second independent watchdog covers all 12 cases
and cleanup, terminating only this process on a stall. Both architectures
build with -Wall -Wextra -Werror. The final batch list has http-same-seek;
return both result-http-same-seek-*.txt and exitcodes.txt even on failure.
If the loopback control cannot run, retain its failure rather than inferring
a WinINet contract. Only text returns; raw logs will not be committed. Native
assertion L1 and Wine runtime gates remain pending.

## Windows observations (2026-11-09)

Both architectures match apart from bitness, exit 0, api_errors=0; all 12
cases finish and the server observes 12 requests. Every prefix matches and
every final body has 16384 correct bytes, successful reads and EOF, including
after rejected seek calls. Every individual case uses exactly one request.

| Framing / mode | Seek position | Last error | Body preserved | Requests |
| --- | --- | --- | --- | --- |
| Content-Length, cached, FILE_CURRENT+0 | 17 | 0 | yes | 1 |
| Content-Length, cached, FILE_BEGIN=17 | 17 | 0 | yes | 1 |
| Content-Length, NO_CACHE_WRITE, FILE_CURRENT+0 | INVALID_SET_FILE_POINTER | 12016 | yes | 1 |
| Chunked, cached FILE_CURRENT/FILE_BEGIN or NO_CACHE_WRITE FILE_CURRENT | INVALID_SET_FILE_POINTER | 12016 | yes | 1 |
| Close-delimited, cached FILE_CURRENT/FILE_BEGIN or NO_CACHE_WRITE FILE_CURRENT | INVALID_SET_FILE_POINTER | 12016 | yes | 1 |

12016 is ERROR_INTERNET_INVALID_OPERATION. Each no-seek control also reads
all original bytes with one request. The cached Content-Length cases are
therefore a usable successful-no-op contract, and unsupported seek attempts
must preserve subsequent body reading too. The measurements do not justify
a broad claim that every HTTP framing allows same-position seeking. A future
Wine test must distinguish the target cached-length case from independent
chunked/close support differences, rather than marking a successful target
as todo or treating unsupported native seeks as success. This is MEASURE,
not native assertion L1. Production code/series/marker remain unchanged;
raw logs are summarized rather than committed.

## Windows-derived assertion candidate

The [v2 candidate](../../../patches/candidates/wininet-same-position-seek-v2/README.md)
keeps the one-line implementation fix unchanged and adds the 12 native
observations to Wine's existing http.c test unit. Its original independent
loopback actor preserves the measured headers/body/API matrix; only the
private cache URL gains a PID/tick suffix. The entire helper/test block is
checked by scripts/build-l1-http-same-seek.sh against the standalone source
before compilation. Full successful native execution should report 200
checks, zero failures, 12 completed cases and 12 server-observed requests.

Cached-length seek results, all body/EOF comparisons and request counts
are ordinary assertions. Wine-only todos are confined to the separate
cached chunked/close-delimited seek-rejection gap; the native wrapper treats
those checks as ordinary too. No assertion guesses NO_CACHE_WRITE FILE_BEGIN
or broader HTTP modes. The shared 45-second cooperative budget and native
60-second watchdog bound fixture execution. Exact-body native L1 and paired
Wine runtime still need observation; production page/marker/series are
unchanged pending validation and cross review.

## Windows exact-body assertion L1 (2026-11-09)

Both x86_64 and i686 assertion outputs report 200 checks, zero failures,
12 completed cases, 12 server-observed requests and exit 0 (96d3ca3 sources,
offline Windows 10). All native controls run as ordinary assertions,
including checks that have Wine-only todos in the integrated test. The
candidate's native L1 passes for this complete measured matrix.

The native pass does not establish Wine's before/after runtime results or
broader HTTP support. Paired wininet L2, formal cross review and consumer
validation remain open. The candidate is not promoted and production series
and marker stay unchanged. Raw output and Windows binaries are not committed.
