/*
 * MEASURE: IXMLDOMParseError getters on Windows, for
 * msxml3-parseerror-filepos v2. The patch's test failed on Windows 10:
 * get_filepos returns S_FALSE (1) for a document without a parse error.
 * This records, for msxml3 and msxml6 documents, the HRESULT and value of
 * get_errorCode, get_line, get_linepos, get_filepos and get_reason in four
 * states: new document, after loadXML("") (what SOLIDWORKS does), after
 * loadXML of malformed XML, after loadXML of valid XML.
 *
 * Build: x86_64-w64-mingw32-gcc / i686-w64-mingw32-gcc -O2 -Wall -o m.exe measure.c -lole32 -loleaut32
 * Text output only. Original, AI-assisted (Claude), LGPL-2.1-or-later.
 */
#define COBJMACROS
#include <windows.h>
#include <ole2.h>
#include <msxml2.h>
#include <stdio.h>

static const CLSID clsid_doc30 = {0xf5078f32,0xc551,0x11d3,{0x89,0xb9,0x00,0x00,0xf8,0x1f,0xe2,0x21}};
static const CLSID clsid_doc60 = {0x88d96a05,0xf192,0x11d4,{0xa6,0x5f,0x00,0x40,0x96,0x32,0x51,0xe5}};
static const IID iid_doc = {0x2933bf81,0x7b36,0x11d2,{0xb2,0x0e,0x00,0xc0,0x4f,0x98,0x3e,0x60}};

static void dump(const char *cls, const char *state, IXMLDOMDocument *doc)
{
    IXMLDOMParseError *err = NULL;
    LONG v;
    BSTR reason = NULL;
    HRESULT hr = IXMLDOMDocument_get_parseError(doc, &err);

    printf("MEASURE class=%s state=%s parseError_hr=0x%08lx", cls, state, hr);
    if (FAILED(hr) || !err) { printf("\n"); return; }
    v = -7; hr = IXMLDOMParseError_get_errorCode(err, &v);  printf(" errorCode_hr=0x%08lx errorCode=0x%08lx", hr, v);
    v = -7; hr = IXMLDOMParseError_get_line(err, &v);       printf(" line_hr=0x%08lx line=%ld", hr, v);
    v = -7; hr = IXMLDOMParseError_get_linepos(err, &v);    printf(" linepos_hr=0x%08lx linepos=%ld", hr, v);
    v = -7; hr = IXMLDOMParseError_get_filepos(err, &v);    printf(" filepos_hr=0x%08lx filepos=%ld", hr, v);
    hr = IXMLDOMParseError_get_filepos(err, NULL);          printf(" filepos_null_hr=0x%08lx", hr);
    hr = IXMLDOMParseError_get_reason(err, &reason);        printf(" reason_hr=0x%08lx reason_len=%u", hr, reason ? SysStringLen(reason) : 0);
    SysFreeString(reason);
    printf("\n");
    IXMLDOMParseError_Release(err);
}

static void run(const char *cls, const CLSID *clsid)
{
    static const struct { const char *state; const WCHAR *xml; } loads[] = {
        { "loadxml_empty", L"" },
        { "loadxml_malformed", L"<a><b></a>" },
        { "loadxml_second_line_error", L"<a>\n  <b>\n</a>" },
        { "loadxml_valid", L"<a><b/></a>" },
    };
    unsigned int i;

    for (i = 0; i <= sizeof(loads) / sizeof(loads[0]); i++)
    {
        IXMLDOMDocument *doc = NULL;
        HRESULT hr = CoCreateInstance(clsid, NULL, CLSCTX_INPROC_SERVER, &iid_doc, (void **)&doc);
        if (FAILED(hr)) { printf("MEASURE class=%s create_hr=0x%08lx\n", cls, hr); return; }
        if (!i) dump(cls, "new", doc);
        else
        {
            VARIANT_BOOL ok = VARIANT_TRUE;
            BSTR xml = SysAllocString(loads[i - 1].xml);
            hr = IXMLDOMDocument_loadXML(doc, xml, &ok);
            printf("LOAD class=%s state=%s hr=0x%08lx success=%d\n", cls, loads[i - 1].state, hr, ok == VARIANT_TRUE);
            SysFreeString(xml);
            dump(cls, loads[i - 1].state, doc);
        }
        IXMLDOMDocument_Release(doc);
    }
}

int main(void)
{
    CoInitialize(NULL);
    printf("arch=%d\n", (int)(sizeof(void *) * 8));
    run("DOMDocument30", &clsid_doc30);
    run("DOMDocument60", &clsid_doc60);
    CoUninitialize();
    puts("SUMMARY done");
    return 0;
}
