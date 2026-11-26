/* Original MEASURE program, AI-assisted (Codex), LGPL-2.1-or-later. */
#define COBJMACROS
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <ole2.h>
#include <oleauto.h>
#include <initguid.h>
#include <msxml2.h>
#include <stdio.h>

struct fixture
{
    const char *name;
    const WCHAR *xml;
    const WCHAR *query;
};

static const struct fixture fixtures[] =
{
    {"empty", L"<group><keep/></group>", L"./string"},
    {"one", L"<group><string id='1'/><keep/></group>", L"./string"},
    {"several", L"<group><string id='1'/><keep/><string id='2'/><string id='3'/></group>", L"./string"},
    {"different_parents", L"<group><left><string id='1'/></left><right><string id='2'/><keep/></right></group>",
     L".//string"},
    {"ancestor_and_descendant", L"<group><string id='1'><string id='2'/></string><string id='3'/></group>",
     L".//string"},
};

static void print_string(BSTR text)
{
    unsigned int i;

    if (!text)
    {
        printf("NULL");
        return;
    }
    putchar('"');
    for (i = 0; i < SysStringLen(text); ++i)
    {
        unsigned int ch = text[i];
        if (ch == '"' || ch == '\\') printf("\\%c", (int)ch);
        else if (ch >= 32 && ch < 127) putchar((int)ch);
        else printf("\\u%04x", ch);
    }
    putchar('"');
}

static HRESULT select_nodes(IXMLDOMElement *root, BSTR query, BOOL dispatch, IXMLDOMSelection **selection)
{
    HRESULT hr, qi = E_UNEXPECTED;

    *selection = NULL;
    if (dispatch)
    {
        WCHAR *name = L"selectNodes";
        DISPPARAMS params = {0};
        VARIANT arg, value;
        DISPID id;

        VariantInit(&value);
        V_VT(&arg) = VT_BSTR;
        V_BSTR(&arg) = query;
        params.rgvarg = &arg;
        params.cArgs = 1;
        hr = IXMLDOMElement_GetIDsOfNames(root, &IID_NULL, &name, 1, LOCALE_USER_DEFAULT, &id);
        printf("get_dispid=0x%08lx\n", (unsigned long)hr);
        if (SUCCEEDED(hr))
        {
            hr = IXMLDOMElement_Invoke(root, id, &IID_NULL, LOCALE_USER_DEFAULT,
                                       DISPATCH_METHOD, &params, &value, NULL, NULL);
            if (SUCCEEDED(hr) && V_VT(&value) == VT_DISPATCH && V_DISPATCH(&value))
                qi = IDispatch_QueryInterface(V_DISPATCH(&value), &IID_IXMLDOMSelection, (void **)selection);
        }
        printf("selectNodes=0x%08lx dispatch_type=%u selection_qi=0x%08lx\n",
               (unsigned long)hr, V_VT(&value), (unsigned long)qi);
        VariantClear(&value);
    }
    else
    {
        IXMLDOMNodeList *list = NULL;

        hr = IXMLDOMElement_selectNodes(root, query, &list);
        if (list)
        {
            qi = IXMLDOMNodeList_QueryInterface(list, &IID_IXMLDOMSelection, (void **)selection);
            IXMLDOMNodeList_Release(list);
        }
        printf("selectNodes=0x%08lx selection_qi=0x%08lx\n", (unsigned long)hr, (unsigned long)qi);
    }
    return FAILED(hr) ? hr : qi;
}

static void inspect_node(const char *operation, HRESULT hr, IXMLDOMNode *node)
{
    IXMLDOMNode *parent = NULL;
    HRESULT xml_hr = E_UNEXPECTED, parent_hr = E_UNEXPECTED;
    BSTR xml = NULL;

    if (node)
    {
        xml_hr = IXMLDOMNode_get_xml(node, &xml);
        parent_hr = IXMLDOMNode_get_parentNode(node, &parent);
    }
    printf("%s=0x%08lx node=%d parent_hr=0x%08lx parent=%d xml_hr=0x%08lx xml=",
           operation, (unsigned long)hr, !!node, (unsigned long)parent_hr,
           !!parent, (unsigned long)xml_hr);
    print_string(xml);
    putchar('\n');
    SysFreeString(xml);
    if (parent) IXMLDOMNode_Release(parent);
    if (node) IXMLDOMNode_Release(node);
}

static void inspect_state(IXMLDOMElement *root, IXMLDOMSelection *selection, const char *stage)
{
    IXMLDOMNodeList *children = NULL;
    HRESULT child_hr, length_hr = E_UNEXPECTED, selection_hr, xml_hr;
    LONG child_count = -1, selected = -1;
    BSTR xml = NULL;

    child_hr = IXMLDOMElement_get_childNodes(root, &children);
    if (children)
    {
        length_hr = IXMLDOMNodeList_get_length(children, &child_count);
        IXMLDOMNodeList_Release(children);
    }
    selection_hr = IXMLDOMSelection_get_length(selection, &selected);
    xml_hr = IXMLDOMElement_get_xml(root, &xml);
    printf("%s childNodes=0x%08lx child_length_hr=0x%08lx child_length=%ld selection_length_hr=0x%08lx "
           "selection_length=%ld xml_hr=0x%08lx xml=", stage,
           (unsigned long)child_hr, (unsigned long)length_hr, child_count,
           (unsigned long)selection_hr, selected, (unsigned long)xml_hr);
    print_string(xml);
    putchar('\n');
    SysFreeString(xml);
}

static void inspect_next(IXMLDOMSelection *selection, const char *label)
{
    IXMLDOMNode *node = NULL;
    HRESULT hr = IXMLDOMSelection_nextNode(selection, &node);

    inspect_node(label, hr, node);
}

static BOOL run_case(REFCLSID clsid, unsigned int version, const struct fixture *fixture,
                     BOOL dispatch, BOOL remove_next, BOOL advance)
{
    IXMLDOMDocument *doc = NULL;
    IXMLDOMElement *root = NULL, *added = NULL;
    IXMLDOMSelection *selection = NULL, *new_selection = NULL;
    ISupportErrorInfo *support = NULL;
    BSTR xml = NULL, query = NULL, name = NULL;
    VARIANT_BOOL loaded = VARIANT_FALSE;
    HRESULT hr;
    BOOL complete = FALSE;

    printf("case dom=%u fixture=%s path=%s operation=%s cursor=%s\n", version, fixture->name,
           dispatch ? "dispatch" : "nodelist_qi", remove_next ? "removeNext" : "removeAll",
           advance ? "advanced" : "start");
    hr = CoCreateInstance(clsid, NULL, CLSCTX_INPROC_SERVER, &IID_IXMLDOMDocument, (void **)&doc);
    printf("create=0x%08lx\n", (unsigned long)hr);
    if (!doc) goto done;
    xml = SysAllocString(fixture->xml);
    query = SysAllocString(fixture->query);
    name = SysAllocString(L"string");
    if (!xml || !query || !name) goto done;
    hr = IXMLDOMDocument_loadXML(doc, xml, &loaded);
    printf("loadXML=0x%08lx result=%d\n", (unsigned long)hr, (int)loaded);
    if (hr != S_OK || loaded != VARIANT_TRUE) goto done;
    hr = IXMLDOMDocument_get_documentElement(doc, &root);
    printf("documentElement=0x%08lx\n", (unsigned long)hr);
    if (!root) goto done;
    if (FAILED(select_nodes(root, query, dispatch, &selection)) || !selection) goto done;

    hr = IXMLDOMSelection_QueryInterface(selection, &IID_ISupportErrorInfo, (void **)&support);
    printf("support_error_info=0x%08lx present=%d\n", (unsigned long)hr, !!support);
    if (support)
    {
        hr = ISupportErrorInfo_InterfaceSupportsErrorInfo(support, &IID_IXMLDOMSelection);
        printf("supports_selection=0x%08lx\n", (unsigned long)hr);
        hr = ISupportErrorInfo_InterfaceSupportsErrorInfo(support, &IID_IXMLDOMNodeList);
        printf("supports_nodelist=0x%08lx\n", (unsigned long)hr);
        ISupportErrorInfo_Release(support);
    }

    inspect_state(root, selection, "before");
    if (advance) inspect_next(selection, "advance");
    if (remove_next)
    {
        IXMLDOMNode *node = NULL;
        hr = IXMLDOMSelection_removeNext(selection, &node);
        inspect_node("removeNext", hr, node);
    }
    else
    {
        hr = IXMLDOMSelection_removeAll(selection);
        printf("removeAll=0x%08lx\n", (unsigned long)hr);
    }

    inspect_state(root, selection, "after");
    inspect_next(selection, "next_after_remove");
    hr = IXMLDOMSelection_reset(selection);
    printf("reset=0x%08lx\n", (unsigned long)hr);
    inspect_next(selection, "next_after_reset");

    hr = IXMLDOMDocument_createElement(doc, name, &added);
    if (SUCCEEDED(hr) && added)
        hr = IXMLDOMElement_appendChild(root, (IXMLDOMNode *)added, NULL);
    printf("append_fresh_match=0x%08lx\n", (unsigned long)hr);
    inspect_state(root, selection, "old_selection_after_append");
    hr = IXMLDOMSelection_reset(selection);
    printf("reset_after_append=0x%08lx\n", (unsigned long)hr);
    inspect_next(selection, "old_next_after_append");
    puts("fresh_selection:");
    if (SUCCEEDED(select_nodes(root, query, dispatch, &new_selection)) && new_selection)
        inspect_state(root, new_selection, "fresh_selection_after_append");
    complete = !!new_selection;

done:
    if (new_selection) IXMLDOMSelection_Release(new_selection);
    if (added) IXMLDOMElement_Release(added);
    if (selection) IXMLDOMSelection_Release(selection);
    if (root) IXMLDOMElement_Release(root);
    if (doc) IXMLDOMDocument_Release(doc);
    SysFreeString(name);
    SysFreeString(query);
    SysFreeString(xml);
    printf("case_complete=%d\n", complete);
    return complete;
}

int main(void)
{
    static const CLSID *const classes[] = {&CLSID_DOMDocument30, &CLSID_DOMDocument60};
    unsigned int c, f, mode, total = 0, failed = 0;
    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    printf("measure=msxml3-selection-removeall bits=%u coinit=0x%08lx\n",
           (unsigned int)(sizeof(void *) * 8), (unsigned long)hr);
    if (FAILED(hr)) return 1;
    for (c = 0; c < sizeof(classes) / sizeof(classes[0]); ++c)
        for (f = 0; f < sizeof(fixtures) / sizeof(fixtures[0]); ++f)
            for (mode = 0; mode < 8; ++mode)
            {
                ++total;
                if (!run_case(classes[c], c ? 60 : 30, &fixtures[f],
                              !!(mode & 1), !!(mode & 2), !!(mode & 4)))
                    ++failed;
            }
    CoUninitialize();
    printf("measure_complete cases=%u failed=%u\n", total, failed);
    return failed != 0;
}
