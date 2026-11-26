# MSXML Content Center diagnostic overlay

This overlay is built on winum-11.22-3 with the debug patch only.
The MSXML element append candidate is excluded. The debug patch is
outside the release series. The [verified debug overlay](https://github.com/Harvi654/winum/actions/runs/45904679098/artifacts/15158046259)
contains x86_64 and i386 DLLs. The local x86_64 synthetic run exercised
all trace record types and matched all 52 release-baseline cases.

Verify SHA256SUMS and replace both `msxml3.dll` files under their
matching architecture directories below `lib/wine` in a disposable
runtime copy. Launch that copy with `WINEDEBUG=+msxml` and reproduce
Content Center Editor → filter → ISO once. Use the launcher's local `inventor-*.log` or capture stderr in a private
local log. The probe writes `cc_msxml` records:

- `parse_stream` records the parser HRESULT and whether it produced a tree.
- `load` records the public HRESULT, VARIANT_BOOL (NA for
  IPersistStreamInit), and the error stored for get_parseError.
- `root` records the getter HRESULT and root name.
- `append` records the target (document or element) and HRESULT.
- `preview` records the length of the serialized loaded DOM.
  Subsequent `xml` lines give the first 300 UTF-16 code units in chunks.
  The preview is serialized after a successful load, so spelling and
  formatting can differ from the original input stream.

For a local inspection near the crash:

~~~bash
rg 'cc_msxml ' inventor-msxml.local.log | tail -100
~~~

For a shareable status excerpt, omit XML chunks and redact root names:

~~~bash
rg 'cc_msxml (parse_stream|load|root|append|preview) ' inventor-msxml.local.log |
    sed -E 's/ name=.*/ name=<redacted>/' | tail -60
~~~

Keep the captured XML and complete log local. For a Windows follow-up,
describe the relevant XML structure or use a synthetic equivalent.
