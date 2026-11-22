/* Original, AI-assisted (Codex), LGPL-2.1-or-later. */
#define COBJMACROS
#include <windows.h>
#include <oleauto.h>
#include <msxml2.h>
#include <stdio.h>

/* The CLSID_DOMDocument2 used by Wine's domdoc test. Some MinGW
 * headers alias it without providing its definition. */
static const CLSID dom_document = {0xf6d90f11, 0x9c73, 0x11d3, {0xb3, 0x2e, 0x00, 0xc0, 0x4f, 0x99, 0x0b, 0xb4}};

static int same_identity(IXMLDOMNode *left, IXMLDOMNode *right)
{
    IUnknown *a = NULL, *b = NULL;
    int same = -1;

    if (left && right &&
        SUCCEEDED(IXMLDOMNode_QueryInterface(left, &IID_IUnknown, (void **)&a)) &&
        SUCCEEDED(IXMLDOMNode_QueryInterface(right, &IID_IUnknown, (void **)&b)))
        same = a == b;
    if (b) IUnknown_Release(b);
    if (a) IUnknown_Release(a);
    return same;
}

static int measure_case(BOOL document, BOOL insert, BOOL alias, BOOL output)
{
    IXMLDOMNode *container = NULL, *node = NULL, *second = NULL, *reference;
    IXMLDOMNode *returned = NULL, *parent = NULL, *last = NULL;
    IXMLDOMDocument *doc = NULL;
    IXMLDOMElement *root = NULL;
    IXMLDOMNodeList *list = NULL;
    BSTR source, before = NULL, name = NULL, after = NULL;
    VARIANT ref;
    VARIANT_BOOL loaded;
    HRESULT hr, parent_hr, last_hr, name_hr = E_FAIL;
    long count = -1;
    int intact, setup_ok = 0;
    long index = document ? 0 : 1;
    const WCHAR *target_name = document ? L"root" : L"target";
    const char *operation = insert ? "insertBefore" : "replaceChild";

    hr = CoCreateInstance(&dom_document, NULL, CLSCTX_INPROC_SERVER,
                          &IID_IXMLDOMDocument, (void **)&doc);
    if (FAILED(hr)) goto done;
    source = SysAllocString(L"<root><first/><target/></root>");
    if (!source)
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }
    hr = IXMLDOMDocument_loadXML(doc, source, &loaded);
    SysFreeString(source);
    if (hr != S_OK || loaded != VARIANT_TRUE) goto done;
    if (document)
        hr = IXMLDOMDocument_QueryInterface(doc, &IID_IXMLDOMNode, (void **)&container);
    else if ((hr = IXMLDOMDocument_get_documentElement(doc, &root)) == S_OK && root)
        hr = IXMLDOMElement_QueryInterface(root, &IID_IXMLDOMNode, (void **)&container);
    if (hr != S_OK || !container ||
        (hr = IXMLDOMNode_get_childNodes(container, &list)) != S_OK || !list ||
        (hr = IXMLDOMNodeList_get_item(list, index, &node)) != S_OK || !node ||
        (hr = IXMLDOMNodeList_get_item(list, index, &second)) != S_OK || !second ||
        (hr = IXMLDOMNode_get_xml(container, &before)) != S_OK)
        goto done;

    setup_ok = 1;
    reference = alias ? second : node;
    printf("%s parent=%s handles=%s out=%d same_interface=%d same_identity=%d\n", operation,
           document ? "document" : "element", alias ? "reacquired" : "same", output, node == reference, same_identity(node, reference));
    if (insert)
    {
        VariantInit(&ref);
        V_VT(&ref) = VT_DISPATCH;
        V_DISPATCH(&ref) = (IDispatch *)reference;
        hr = IXMLDOMNode_insertBefore(container, node, ref, output ? &returned : NULL);
    }
    else
        hr = IXMLDOMNode_replaceChild(container, node, reference, output ? &returned : NULL);
    printf("call hr=%08lx returned=%d input_identity=%d reference_identity=%d\n",
           (unsigned long)hr, returned != NULL, same_identity(returned, node),
           same_identity(returned, reference));

    parent_hr = IXMLDOMNode_get_parentNode(node, &parent);
    last_hr = IXMLDOMNode_get_lastChild(container, &last);
    if (last_hr == S_OK && last) name_hr = IXMLDOMNode_get_nodeName(last, &name);
    intact = parent_hr == S_OK && parent && name_hr == S_OK && name && !lstrcmpW(name, target_name);
    printf("tree parent_hr=%08lx parent_present=%d parent_identity=%d last_hr=%08lx last_is_target=%d\n",
           (unsigned long)parent_hr, parent != NULL, same_identity(parent, container),
           (unsigned long)last_hr, name_hr == S_OK && name && !lstrcmpW(name, target_name));
    /* The broken Wine child list can loop forever on a full traversal. */
    if (intact)
    {
        HRESULT length_hr = IXMLDOMNodeList_get_length(list, &count);
        HRESULT xml_hr = IXMLDOMNode_get_xml(container, &after);

        printf("walk length_hr=%08lx child_count=%ld xml_hr=%08lx xml_equal=%d\n",
               (unsigned long)length_hr, count, (unsigned long)xml_hr,
               xml_hr == S_OK && after && before && !lstrcmpW(before, after));
    }
    else
        puts("walk skipped after tree check");

done:
    if (!setup_ok) printf("%s parent=%s handles=%s out=%d setup_hr=%08lx\n", operation,
                          document ? "document" : "element", alias ? "reacquired" : "same", output, (unsigned long)hr);
    SysFreeString(after);
    SysFreeString(name);
    SysFreeString(before);
    if (last) IXMLDOMNode_Release(last);
    if (parent) IXMLDOMNode_Release(parent);
    if (returned) IXMLDOMNode_Release(returned);
    if (second) IXMLDOMNode_Release(second);
    if (node) IXMLDOMNode_Release(node);
    if (list) IXMLDOMNodeList_Release(list);
    if (container) IXMLDOMNode_Release(container);
    if (root) IXMLDOMElement_Release(root);
    if (doc) IXMLDOMDocument_Release(doc);
    return setup_ok ? 0 : 1;
}

static DWORD WINAPI watchdog(void *done)
{
    if (WaitForSingleObject(done, 20000) != WAIT_OBJECT_0)
    {
        puts("timeout");
        TerminateProcess(GetCurrentProcess(), 2);
    }
    return 0;
}

int main(void)
{
    HANDLE done, thread;
    HRESULT hr;
    int document, insert, alias, output, status = 0;

    setvbuf(stdout, NULL, _IONBF, 0);
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
    hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    if (FAILED(hr))
    {
        printf("CoInitializeEx hr=%08lx\n", (unsigned long)hr);
        return 2;
    }
    done = CreateEventW(NULL, TRUE, FALSE, NULL);
    thread = done ? CreateThread(NULL, 0, watchdog, done, 0, NULL) : NULL;
    if (!thread)
    {
        if (done) CloseHandle(done);
        CoUninitialize();
        return 2;
    }

    printf("MEASURE msxml3-same-node bits=%u\n", (unsigned)(sizeof(void *) * 8));
    for (document = 0; document < 2; ++document)
        for (insert = 0; insert < 2; ++insert)
            for (alias = 0; alias < 2; ++alias)
                for (output = 0; output < 2; ++output)
                    status |= measure_case(document, insert, alias, output);

    CoUninitialize();
    SetEvent(done);
    WaitForSingleObject(thread, INFINITE);
    CloseHandle(thread);
    CloseHandle(done);
    return status;
}
