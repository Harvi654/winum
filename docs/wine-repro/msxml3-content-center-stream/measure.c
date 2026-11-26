/* Original MEASURE program, AI-assisted (Codex), LGPL-2.1-or-later. */
#define COBJMACROS
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <ole2.h>
#include <oleauto.h>
#include <ocidl.h>
#include <initguid.h>
#include <msxml2.h>
#include <stdio.h>

static const WCHAR declaration[] = L"<?xml version=\"1.0\" encoding=\"UTF-16\"?>";
static const WCHAR root_text[] = L"<root><item/></root>";

enum load_method { variant_stream, persist_stream, fresh, load_xml };

static void print_string(BSTR value)
{
    UINT i;

    if (!value)
    {
        printf("NULL");
        return;
    }
    putchar('"');
    for (i = 0; i < SysStringLen(value); ++i)
    {
        unsigned int ch = value[i];
        if (ch == '"' || ch == '\\') printf("\\%c", (int)ch);
        else if (ch >= 32 && ch < 127) putchar((int)ch);
        else printf("\\u%04x", ch);
    }
    putchar('"');
}

static HRESULT make_stream(BOOL bom, BOOL decl, BOOL root, IStream **out)
{
    static const WCHAR marker = 0xfeff;
    LARGE_INTEGER offset = {{0, 0}};
    IStream *stream = NULL;
    HRESULT hr = CreateStreamOnHGlobal(NULL, TRUE, &stream);

    *out = NULL;
    if (FAILED(hr)) return hr;
    if (bom) hr = IStream_Write(stream, &marker, sizeof(marker), NULL);
    if (SUCCEEDED(hr) && decl)
        hr = IStream_Write(stream, declaration, sizeof(declaration) - sizeof(WCHAR), NULL);
    if (SUCCEEDED(hr) && root)
        hr = IStream_Write(stream, root_text, sizeof(root_text) - sizeof(WCHAR), NULL);
    if (SUCCEEDED(hr)) hr = IStream_Seek(stream, offset, STREAM_SEEK_SET, NULL);
    if (FAILED(hr)) IStream_Release(stream);
    else *out = stream;
    return hr;
}

static void inspect_error(IXMLDOMDocument *doc)
{
    IXMLDOMParseError *error = NULL;
    HRESULT hr, code_hr = E_UNEXPECTED, reason_hr = E_UNEXPECTED;
    BSTR reason = NULL;
    LONG code = 0;

    hr = IXMLDOMDocument_get_parseError(doc, &error);
    if (error)
    {
        code_hr = IXMLDOMParseError_get_errorCode(error, &code);
        reason_hr = IXMLDOMParseError_get_reason(error, &reason);
        IXMLDOMParseError_Release(error);
    }
    printf("parseError=0x%08lx code_hr=0x%08lx code=0x%08lx reason_hr=0x%08lx reason=",
           (unsigned long)hr, (unsigned long)code_hr, (unsigned long)code, (unsigned long)reason_hr);
    print_string(reason);
    putchar('\n');
    SysFreeString(reason);
}

static void inspect_document(IXMLDOMDocument *doc, const char *stage)
{
    IXMLDOMNodeList *list = NULL;
    LONG length = -1, i;
    HRESULT hr, length_hr = E_UNEXPECTED;
    BSTR xml = NULL;

    hr = IXMLDOMDocument_get_childNodes(doc, &list);
    if (list) length_hr = IXMLDOMNodeList_get_length(list, &length);
    printf("%s childNodes=0x%08lx length_hr=0x%08lx length=%ld nodes=",
           stage, (unsigned long)hr, (unsigned long)length_hr, (long)length);
    for (i = 0; list && i < length; ++i)
    {
        IXMLDOMNode *node = NULL;
        DOMNodeType type = NODE_INVALID;
        BSTR name = NULL;

        hr = IXMLDOMNodeList_get_item(list, i, &node);
        printf("%s[%ld:item=0x%08lx", i ? "," : "", (long)i, (unsigned long)hr);
        if (node)
        {
            HRESULT type_hr = IXMLDOMNode_get_nodeType(node, &type);
            HRESULT name_hr = IXMLDOMNode_get_nodeName(node, &name);
            printf(" type=%d/0x%08lx name=", (int)type, (unsigned long)type_hr);
            print_string(name);
            printf("/0x%08lx", (unsigned long)name_hr);
            SysFreeString(name);
            IXMLDOMNode_Release(node);
        }
        putchar(']');
    }
    putchar('\n');
    if (list) IXMLDOMNodeList_Release(list);

    hr = IXMLDOMDocument_get_xml(doc, &xml);
    printf("%s xml_hr=0x%08lx xml=", stage, (unsigned long)hr);
    print_string(xml);
    putchar('\n');
    SysFreeString(xml);
}

static BOOL run_case(REFCLSID clsid, unsigned int version, enum load_method method,
                     BOOL bom, BOOL decl, BOOL root, BOOL append_to_root)
{
    static const char *const methods[] = {"load_unknown", "persist_load", "fresh", "load_xml"};
    IXMLDOMProcessingInstruction *pi = NULL;
    IXMLDOMElement *element = NULL;
    IXMLDOMDocument *doc = NULL;
    IPersistStreamInit *persist = NULL;
    IRunnableObject *runnable = NULL;
    IXMLDOMNode *node = NULL, *appended = NULL;
    IStream *stream = NULL;
    BSTR target = NULL, data = NULL, name = NULL;
    VARIANT_BOOL loaded = 123;
    HRESULT hr, name_hr, stream_hr;
    BOOL complete = FALSE;

    printf("case dom=%u method=%s bom=%d decl=%d root=%d append_to=%s\n",
           version, methods[method], bom, decl, root, append_to_root ? "root" : "document");
    hr = CoCreateInstance(clsid, NULL, CLSCTX_INPROC_SERVER, &IID_IXMLDOMDocument, (void **)&doc);
    printf("create=0x%08lx\n", (unsigned long)hr);
    if (!doc) goto done;
    hr = IXMLDOMDocument_put_async(doc, VARIANT_FALSE);
    printf("async=0x%08lx\n", (unsigned long)hr);
    target = SysAllocString(L"xml");
    data = SysAllocString(L"version='1.0' encoding='UTF-16'");
    if (!target || !data) goto done;
    hr = IXMLDOMDocument_createProcessingInstruction(doc, target, data, &pi);
    printf("createPI=0x%08lx present=%d\n", (unsigned long)hr, !!pi);
    if (pi)
    {
        hr = IXMLDOMProcessingInstruction_QueryInterface(pi, &IID_IXMLDOMNode, (void **)&node);
        printf("pi_as_node=0x%08lx\n", (unsigned long)hr);
    }

    if (method == variant_stream || method == persist_stream)
    {
        stream_hr = make_stream(bom, decl, root, &stream);
        printf("stream=0x%08lx\n", (unsigned long)stream_hr);
        if (!stream) goto done;
        hr = IStream_QueryInterface(stream, &IID_IRunnableObject, (void **)&runnable);
        printf("stream_runnable=0x%08lx\n", (unsigned long)hr);
    }

    if (method == variant_stream)
    {
        VARIANT source;
        VariantInit(&source);
        V_VT(&source) = VT_UNKNOWN;
        V_UNKNOWN(&source) = (IUnknown *)stream;
        hr = IXMLDOMDocument_load(doc, source, &loaded);
        printf("load=0x%08lx success=%d\n", (unsigned long)hr, (int)loaded);
    }
    else if (method == persist_stream)
    {
        hr = IXMLDOMDocument_QueryInterface(doc, &IID_IPersistStreamInit, (void **)&persist);
        printf("persist_qi=0x%08lx\n", (unsigned long)hr);
        if (persist)
        {
            hr = IPersistStreamInit_Load(persist, stream);
            printf("load=0x%08lx success=NA\n", (unsigned long)hr);
        }
    }
    else if (method == load_xml)
    {
        BSTR text = SysAllocString(L"<?xml version=\"1.0\" encoding=\"UTF-16\"?><root><item/></root>");
        if (!text) goto done;
        hr = IXMLDOMDocument_loadXML(doc, text, &loaded);
        SysFreeString(text);
        printf("load=0x%08lx success=%d\n", (unsigned long)hr, (int)loaded);
    }

    inspect_error(doc);
    hr = IXMLDOMDocument_get_documentElement(doc, &element);
    name_hr = element ? IXMLDOMElement_get_nodeName(element, &name) : E_UNEXPECTED;
    printf("documentElement=0x%08lx present=%d name_hr=0x%08lx name=",
           (unsigned long)hr, !!element, (unsigned long)name_hr);
    print_string(name);
    putchar('\n');
    inspect_document(doc, "before");

    if (!node || (append_to_root && !element))
        printf("append=skipped\n");
    else
    {
        if (append_to_root) hr = IXMLDOMElement_appendChild(element, node, &appended);
        else hr = IXMLDOMDocument_appendChild(doc, node, &appended);
        printf("append=0x%08lx result_present=%d\n", (unsigned long)hr, !!appended);
    }
    inspect_document(doc, "after");
    complete = TRUE;

done:
    SysFreeString(name);
    SysFreeString(data);
    SysFreeString(target);
    if (appended) IXMLDOMNode_Release(appended);
    if (element) IXMLDOMElement_Release(element);
    if (persist) IPersistStreamInit_Release(persist);
    if (runnable) IRunnableObject_Release(runnable);
    if (stream) IStream_Release(stream);
    if (node) IXMLDOMNode_Release(node);
    if (pi) IXMLDOMProcessingInstruction_Release(pi);
    if (doc) IXMLDOMDocument_Release(doc);
    printf("case_complete=%d\n", complete);
    return complete;
}

int main(void)
{
    static const CLSID *const classes[] = {&CLSID_DOMDocument30, &CLSID_DOMDocument60};
    unsigned int c, method, bits, total = 0, failed = 0;
    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    printf("measure=msxml3-content-center-stream bits=%u coinit=0x%08lx\n",
           (unsigned int)(sizeof(void *) * 8), (unsigned long)hr);
    if (FAILED(hr)) return 1;
    for (c = 0; c < sizeof(classes) / sizeof(classes[0]); ++c)
    {
        unsigned int version = c ? 60 : 30;

        for (method = variant_stream; method <= persist_stream; ++method)
        {
            for (bits = 0; bits < 8; ++bits)
            {
                BOOL bom = !!(bits & 1), decl = !!(bits & 2), root = !!(bits & 4);

                ++total;
                if (!run_case(classes[c], version, method, bom, decl, root, FALSE)) ++failed;
                if (root)
                {
                    ++total;
                    if (!run_case(classes[c], version, method, bom, decl, root, TRUE)) ++failed;
                }
            }
        }
        for (method = fresh; method <= load_xml; ++method)
        {
            ++total;
            if (!run_case(classes[c], version, method, FALSE, method == load_xml, method == load_xml, FALSE))
                ++failed;
        }
    }
    CoUninitialize();
    printf("measure_complete cases=%u failed=%u\n", total, failed);
    return failed ? 1 : 0;
}
