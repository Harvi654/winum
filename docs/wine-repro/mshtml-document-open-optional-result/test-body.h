static void test_doc_open_optional_result(void)
{
    static const char html[] = "<html><head><title>Winum fixture</title></head><body></body></html>";
    unsigned int omit_result;

    for(omit_result = 0; omit_result < 2; omit_result++) {
        IHTMLDocument2 *doc = NULL;
        IPersistStreamInit *persist = NULL;
        IUnknown *identity = NULL;
        IHTMLWindow2 *window = NULL;
        IDispatch *result = NULL;
        IStream *stream = NULL;
        BOOL complete = FALSE;
        BSTR mime = NULL;
        HGLOBAL memory;
        VARIANT omitted;
        DWORD start;
        HRESULT hr;
        void *bytes;

        hr = CoCreateInstance(&CLSID_HTMLDocument, NULL, CLSCTX_INPROC_SERVER | CLSCTX_INPROC_HANDLER,
                &IID_IHTMLDocument2, (void **)&doc);
        ok(hr == S_OK, "Mode %u: CoCreateInstance returned %#lx.\n", omit_result, hr);
        if(FAILED(hr) || !doc)
            goto next_case;
        hr = IHTMLDocument2_QueryInterface(doc, &IID_IUnknown, (void **)&identity);
        ok(hr == S_OK, "Mode %u: IUnknown query returned %#lx.\n", omit_result, hr);
        if(FAILED(hr))
            goto next_case;
        hr = IHTMLDocument2_QueryInterface(doc, &IID_IPersistStreamInit, (void **)&persist);
        ok(hr == S_OK, "Mode %u: IPersistStreamInit query returned %#lx.\n", omit_result, hr);
        if(FAILED(hr))
            goto next_case;

        memory = GlobalAlloc(GMEM_MOVEABLE, sizeof(html) - 1);
        ok(memory != NULL, "Mode %u: GlobalAlloc failed.\n", omit_result);
        if(!memory)
            goto next_case;
        bytes = GlobalLock(memory);
        ok(bytes != NULL, "Mode %u: GlobalLock failed.\n", omit_result);
        if(!bytes) {
            GlobalFree(memory);
            goto next_case;
        }
        memcpy(bytes, html, sizeof(html) - 1);
        GlobalUnlock(memory);
        hr = CreateStreamOnHGlobal(memory, TRUE, &stream);
        ok(hr == S_OK, "Mode %u: CreateStreamOnHGlobal returned %#lx.\n", omit_result, hr);
        if(FAILED(hr)) {
            GlobalFree(memory);
            goto next_case;
        }
        hr = IPersistStreamInit_Load(persist, stream);
        ok(hr == S_OK, "Mode %u: Load returned %#lx.\n", omit_result, hr);
        if(FAILED(hr))
            goto next_case;

        start = GetTickCount();
        do {
            BSTR state = NULL;
            MSG message;

            hr = IHTMLDocument2_get_readyState(doc, &state);
            complete = hr == S_OK && state && !lstrcmpiW(state, L"complete");
            SysFreeString(state);
            if(FAILED(hr) || complete)
                break;
            while(GetTickCount() - start < 5000 && PeekMessageW(&message, NULL, 0, 0, PM_REMOVE)) {
                TranslateMessage(&message);
                DispatchMessageW(&message);
            }
            Sleep(10);
        }while(GetTickCount() - start < 5000);
        ok(hr == S_OK, "Mode %u: get_readyState returned %#lx.\n", omit_result, hr);
        ok(complete, "Mode %u: document did not finish loading.\n", omit_result);
        if(!complete)
            goto next_case;

        hr = IHTMLDocument2_get_parentWindow(doc, &window);
        ok(hr == S_OK, "Mode %u: parentWindow before open returned %#lx.\n", omit_result, hr);
        ok(window != NULL, "Mode %u: no parentWindow before open.\n", omit_result);
        if(FAILED(hr) || !window)
            goto next_case;
        IHTMLWindow2_Release(window);
        window = NULL;
        mime = SysAllocString(L"text/html");
        ok(mime != NULL, "Mode %u: SysAllocString failed.\n", omit_result);
        if(!mime)
            goto next_case;
        VariantInit(&omitted);
        V_VT(&omitted) = VT_ERROR;
        V_ERROR(&omitted) = DISP_E_PARAMNOTFOUND;

        hr = IHTMLDocument2_open(doc, mime, omitted, omitted, omitted, omit_result ? NULL : &result);
        ok(hr == S_OK, "Mode %u: open returned %#lx.\n", omit_result, hr);
        if(!omit_result)
            ok(result != NULL, "No dispatch returned for a supplied result pointer.\n");
        if(SUCCEEDED(hr)) {
            hr = IHTMLDocument2_close(doc);
            ok(hr == S_OK, "Mode %u: close returned %#lx.\n", omit_result, hr);
        }
        hr = IHTMLDocument2_get_parentWindow(doc, &window);
        ok(hr == S_OK, "Mode %u: parentWindow after close returned %#lx.\n", omit_result, hr);
        ok(window != NULL, "Mode %u: no parentWindow after close.\n", omit_result);

next_case:
        if(window) IHTMLWindow2_Release(window);
        if(result) IDispatch_Release(result);
        SysFreeString(mime);
        if(stream) IStream_Release(stream);
        if(persist) IPersistStreamInit_Release(persist);
        if(doc) IHTMLDocument2_Release(doc);
        if(identity) IUnknown_Release(identity);
    }
}
