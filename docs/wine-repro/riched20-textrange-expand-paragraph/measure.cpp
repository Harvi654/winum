/* Original MEASURE program, AI-assisted (Codex), LGPL-2.1-or-later. */
#define UNICODE
#define _UNICODE
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <ole2.h>
#include <oleauto.h>
#include <imm.h>
#include <richedit.h>
#include <richole.h>
#include <tom.h>
#include <textserv.h>
#include <stdio.h>

static const IID document_iid =
    {0x8cc497c0, 0xa1df, 0x11ce, {0x80, 0x98, 0x00, 0xaa, 0x00, 0x47, 0xbe, 0x5d}};
static const IID host_iid =
    {0x13e670f4, 0x1a5a, 0x11cf, {0xab, 0xeb, 0x00, 0xaa, 0x00, 0xb6, 0x5e, 0xa1}};

class measure_host : public ITextHost
{
    ULONG refs = 1;
    HDC dc;
    CHARFORMATW font = {};
    PARAFORMAT paragraph = {};

public:
    explicit measure_host(HDC screen) : dc(screen)
    {
        font.cbSize = sizeof(font);
        font.dwMask = CFM_FACE | CFM_SIZE | CFM_CHARSET;
        font.yHeight = 200;
        font.bCharSet = DEFAULT_CHARSET;
        lstrcpyW(font.szFaceName, L"Arial");
        paragraph.cbSize = sizeof(paragraph);
        paragraph.dwMask = PFM_ALIGNMENT;
        paragraph.wAlignment = PFA_LEFT;
    }

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void **out) override
    {
        *out = nullptr;
        if (!IsEqualIID(iid, IID_IUnknown) && !IsEqualIID(iid, host_iid)) return E_NOINTERFACE;
        *out = static_cast<ITextHost *>(this);
        AddRef();
        return S_OK;
    }
    ULONG STDMETHODCALLTYPE AddRef() override { return ++refs; }
    ULONG STDMETHODCALLTYPE Release() override { return --refs; }

    HDC TxGetDC() override { return dc; }
    INT TxReleaseDC(HDC) override { return 1; }
    BOOL TxShowScrollBar(INT, BOOL) override { return FALSE; }
    BOOL TxEnableScrollBar(INT, INT) override { return FALSE; }
    BOOL TxSetScrollRange(INT, LONG, INT, BOOL) override { return FALSE; }
    BOOL TxSetScrollPos(INT, INT, BOOL) override { return FALSE; }
    void TxInvalidateRect(LPCRECT, BOOL) override {}
    void TxViewChange(BOOL) override {}
    BOOL TxCreateCaret(HBITMAP, INT, INT) override { return FALSE; }
    BOOL TxShowCaret(BOOL) override { return FALSE; }
    BOOL TxSetCaretPos(INT, INT) override { return FALSE; }
    BOOL TxSetTimer(UINT, UINT) override { return FALSE; }
    void TxKillTimer(UINT) override {}
    void TxScrollWindowEx(INT, INT, LPCRECT, LPCRECT, HRGN, LPRECT, UINT) override {}
    void TxSetCapture(BOOL) override {}
    void TxSetFocus() override {}
    void TxSetCursor(HCURSOR, BOOL) override {}
    BOOL TxScreenToClient(LPPOINT) override { return TRUE; }
    BOOL TxClientToScreen(LPPOINT) override { return TRUE; }
    HRESULT TxActivate(LONG *old) override { if (old) *old = 0; return S_OK; }
    HRESULT TxDeactivate(LONG) override { return S_OK; }
    HRESULT TxGetClientRect(LPRECT rect) override
    {
        SetRect(rect, 0, 0, 320, 200);
        return S_OK;
    }
    HRESULT TxGetViewInset(LPRECT rect) override { SetRectEmpty(rect); return S_OK; }
    HRESULT TxGetCharFormat(const CHARFORMATW **format) override { *format = &font; return S_OK; }
    HRESULT TxGetParaFormat(const PARAFORMAT **format) override { *format = &paragraph; return S_OK; }
    COLORREF TxGetSysColor(int index) override { return GetSysColor(index); }
    HRESULT TxGetBackStyle(TXTBACKSTYLE *style) override { *style = TXTBACK_OPAQUE; return S_OK; }
    HRESULT TxGetMaxLength(DWORD *length) override { *length = 32767; return S_OK; }
    HRESULT TxGetScrollBars(DWORD *bars) override { *bars = 0; return S_OK; }
    HRESULT TxGetPasswordChar(WCHAR *ch) override { *ch = 0; return S_OK; }
    HRESULT TxGetAcceleratorPos(LONG *pos) override { *pos = -1; return S_OK; }
    HRESULT TxGetExtent(LPSIZEL extent) override
    {
        extent->cx = MulDiv(320, 2540, GetDeviceCaps(dc, LOGPIXELSX));
        extent->cy = MulDiv(200, 2540, GetDeviceCaps(dc, LOGPIXELSY));
        return S_OK;
    }
    HRESULT OnTxCharFormatChange(const CHARFORMATW *) override { return S_OK; }
    HRESULT OnTxParaFormatChange(const PARAFORMAT *) override { return S_OK; }
    HRESULT TxGetPropertyBits(DWORD mask, DWORD *bits) override
    {
        *bits = mask & (TXTBIT_RICHTEXT | TXTBIT_MULTILINE | TXTBIT_WORDWRAP);
        return S_OK;
    }
    HRESULT TxNotify(DWORD, void *) override { return S_OK; }
    HIMC TxImmGetContext() override { return nullptr; }
    void TxImmReleaseContext(HIMC) override {}
    HRESULT TxGetSelectionBarWidth(LONG *width) override { *width = 0; return S_OK; }
};

static HRESULT write_text(ITextDocument *doc, const WCHAR *text)
{
    ITextRange *range = nullptr;
    BSTR string = SysAllocString(text);
    HRESULT hr;

    if (!string) return E_OUTOFMEMORY;
    hr = doc->Range(0, 0, &range);
    if (SUCCEEDED(hr))
    {
        hr = range->Expand(tomStory, nullptr);
        if (SUCCEEDED(hr)) hr = range->SetText(string);
        range->Release();
    }
    SysFreeString(string);
    return hr;
}

enum operation { expand, start_of, end_of, move, move_start, move_end };

static bool measure_call(const char *source, const char *fixture, const char *label, ITextRange *range,
                         operation op, LONG argument, bool use_delta)
{
    static const char *const names[] = {"Expand", "StartOf", "EndOf", "Move", "MoveStart", "MoveEnd"};
    LONG start = -1, end = -1, out_start = -1, out_end = -1, delta = 0x12345678;
    LONG *out = use_delta ? &delta : nullptr;
    HRESULT start_hr = range->GetStart(&start), end_hr = range->GetEnd(&end), hr;

    if (FAILED(start_hr) || FAILED(end_hr))
    {
        printf("source=%s fixture=%s case=%s setup=input_bounds_failed getter_hr=0x%08lx,0x%08lx\n",
               source, fixture, label, (unsigned long)start_hr, (unsigned long)end_hr);
        return false;
    }

    switch (op)
    {
    case expand: hr = range->Expand(tomParagraph, out); break;
    case start_of: hr = range->StartOf(tomParagraph, argument, out); break;
    case end_of: hr = range->EndOf(tomParagraph, argument, out); break;
    case move: hr = range->Move(tomParagraph, argument, out); break;
    case move_start: hr = range->MoveStart(tomParagraph, argument, out); break;
    case move_end: hr = range->MoveEnd(tomParagraph, argument, out); break;
    default: return false;
    }

    start_hr = range->GetStart(&out_start);
    end_hr = range->GetEnd(&out_end);
    printf("source=%s fixture=%s case=%s op=%s arg=%ld in=%ld,%ld hr=0x%08lx start=%ld end=%ld",
           source, fixture, label, names[op], argument, start, end, (unsigned long)hr, out_start, out_end);
    if (use_delta) printf(" delta=%ld", delta);
    else printf(" delta=NULL");
    printf(" getter_hr=0x%08lx,0x%08lx\n", (unsigned long)start_hr, (unsigned long)end_hr);
    return SUCCEEDED(start_hr) && SUCCEEDED(end_hr);
}

static bool measure_range(const char *source, const char *fixture, const char *label,
                          ITextDocument *doc, LONG start, LONG end, operation op, LONG argument)
{
    for (unsigned int use_delta = 0; use_delta < 2; ++use_delta)
    {
        ITextRange *range = nullptr;
        HRESULT hr = doc->Range(start, end, &range);

        if (FAILED(hr))
        {
            printf("source=%s fixture=%s case=%s setup=range_failed hr=0x%08lx\n",
                   source, fixture, label, (unsigned long)hr);
            return false;
        }
        bool success = measure_call(source, fixture, label, range, op, argument, use_delta != 0);
        range->Release();
        if (!success) return false;
    }
    return true;
}

static bool measure_matrix(const char *source, const char *fixture, ITextDocument *doc,
                           LONG text_length, LONG story_end, LONG second_start)
{
    static const LONG counts[] = {-32, -2, -1, 0, 1, 2, 32};
    static const LONG extend_modes[] = {tomMove, tomExtend};
    static const operation edges[] = {start_of, end_of};
    static const operation moves[] = {move, move_start, move_end};
    const bool has_second = second_start > 0;
    const LONG first_end = has_second ? second_start : text_length;
    const struct
    {
        const char *label;
        LONG start, end;
        bool requires_second;
    } cases[] =
    {
        {"start_caret", 0, 0, false},
        {"inside_first", 1, first_end > 5 ? 5 : first_end, false},
        {"inside_caret", 1, 1, false},
        {"end_at_boundary", 1, first_end, true},
        {"whole_first", 0, first_end, true},
        {"across_paragraphs", 1, second_start + 2, true},
        {"second_caret", second_start, second_start, true},
        {"inside_second", second_start + 2, second_start + 5, true},
        {"last_explicit_character", text_length - 1, text_length, false},
        {"text_end_caret", text_length, text_length, false},
        {"implicit_final_mark", text_length, story_end, false},
        {"story_end_requested", story_end, story_end, false},
    };

    for (const auto &item : cases)
    {
        if (item.requires_second && !has_second) continue;
        if (!measure_range(source, fixture, item.label, doc, item.start, item.end, expand, 0))
            return false;
        for (operation op : edges)
            for (LONG extend : extend_modes)
                if (!measure_range(source, fixture, item.label, doc, item.start, item.end, op, extend))
                    return false;
        for (operation op : moves)
            for (LONG count : counts)
                if (!measure_range(source, fixture, item.label, doc, item.start, item.end, op, count))
                    return false;
    }
    return true;
}

static bool measure_short_sequence(const char *source, ITextDocument *doc)
{
    ITextRange *range = nullptr, *duplicate = nullptr;
    HRESULT hr = write_text(doc, L"-\r");
    HRESULT expand_hr = E_UNEXPECTED, start_hr = E_UNEXPECTED, end_hr = E_UNEXPECTED;

    if (SUCCEEDED(hr)) hr = doc->Range(0, 0, &range);
    if (SUCCEEDED(hr)) expand_hr = hr = range->Expand(tomStory, nullptr);
    if (SUCCEEDED(hr)) start_hr = hr = range->SetStart(0);
    if (SUCCEEDED(hr)) end_hr = hr = range->SetEnd(2);
    if (SUCCEEDED(hr)) hr = range->GetDuplicate(&duplicate);
    printf("source=%s fixture=short event=sequence expand_story=0x%08lx set_start=0x%08lx set_end=0x%08lx duplicate=0x%08lx\n",
           source, (unsigned long)expand_hr, (unsigned long)start_hr, (unsigned long)end_hr, (unsigned long)hr);
    bool success = duplicate &&
        measure_call(source, "short_sequence", "duplicate", duplicate, expand, 0, false) &&
        measure_call(source, "short_sequence", "after_expand", duplicate, move_end, 1, false);
    if (duplicate) duplicate->Release();
    if (range) range->Release();
    return success;
}

static bool measure_document(const char *source, ITextDocument *doc)
{
    static const struct
    {
        const char *name;
        const WCHAR *text;
        LONG second_start;
    } fixtures[] =
    {
        {"short", L"-\r", 0},
        {"one_paragraph", L"Alpha beta. Next!", 0},
        {"two_paragraphs", L"Alpha beta. Next!\rGamma delta.", 18},
        {"two_trailing_cr", L"Alpha beta. Next!\rGamma delta.\r", 18},
        {"original", L"Alpha beta. Next!\rGamma delta.\rLast\r", 18},
    };

    for (const auto &fixture : fixtures)
    {
        ITextRange *story = nullptr;
        LONG end = -1, length = lstrlenW(fixture.text);
        HRESULT hr = write_text(doc, fixture.text);

        if (SUCCEEDED(hr)) hr = doc->Range(0, 0, &story);
        if (SUCCEEDED(hr)) hr = story->Expand(tomStory, nullptr);
        if (SUCCEEDED(hr)) hr = story->GetEnd(&end);
        if (story) story->Release();
        printf("source=%s fixture=%s event=setup hr=0x%08lx text_length=%ld story_end=%ld\n",
               source, fixture.name, (unsigned long)hr, length, end);
        if (FAILED(hr) || end < length || !measure_matrix(source, fixture.name, doc, length, end, fixture.second_start))
            return false;
    }
    return measure_short_sequence(source, doc);
}

static bool measure_window(const char *source, HMODULE module, const WCHAR *class_name)
{
    ITextDocument *doc = nullptr;
    IRichEditOle *ole = nullptr;
    HWND hwnd = nullptr;
    HRESULT hr = E_FAIL;
    bool success = false;

    if (module)
        hwnd = CreateWindowExW(0, class_name, L"", WS_POPUP | ES_MULTILINE, 0, 0, 320, 200,
                               nullptr, nullptr, GetModuleHandleW(nullptr), nullptr);
    if (hwnd && SendMessageW(hwnd, EM_GETOLEINTERFACE, 0, (LPARAM)&ole) && ole)
        hr = ole->QueryInterface(document_iid, (void **)&doc);
    printf("source=%s event=document hr=0x%08lx\n", source, (unsigned long)hr);
    if (doc) success = measure_document(source, doc);
    if (doc) doc->Release();
    if (ole) ole->Release();
    if (hwnd) DestroyWindow(hwnd);
    printf("source=%s complete=%d\n", source, success);
    return success;
}

static bool measure_services(const char *source, HMODULE module)
{
    ITextDocument *doc = nullptr;
    IUnknown *services = nullptr;
    PCreateTextServices create;
    HRESULT hr;

    if (!module || !(create = (PCreateTextServices)GetProcAddress(module, "CreateTextServices")))
    {
        printf("source=%s event=unavailable\n", source);
        return false;
    }
    HDC dc = GetDC(nullptr);
    if (!dc)
    {
        printf("source=%s event=dc_failed error=%lu\n", source, GetLastError());
        return false;
    }
    measure_host host(dc);
    hr = create(nullptr, &host, &services);
    printf("source=%s event=create hr=0x%08lx\n", source, (unsigned long)hr);
    if (SUCCEEDED(hr) && services)
    {
        hr = services->QueryInterface(document_iid, (void **)&doc);
        printf("source=%s event=document hr=0x%08lx\n", source, (unsigned long)hr);
    }
    bool success = doc && measure_document(source, doc);
    if (doc) doc->Release();
    if (services) services->Release();
    ReleaseDC(nullptr, dc);
    printf("source=%s complete=%d\n", source, success);
    return success;
}

int main()
{
    HRESULT hr = OleInitialize(nullptr);
    setvbuf(stdout, nullptr, _IONBF, 0);
    printf("measure=riched-paragraph-navigation bits=%u\n", (unsigned)(8 * sizeof(void *)));
    if (FAILED(hr))
    {
        printf("setup=ole_initialize_failed hr=0x%08lx\n", (unsigned long)hr);
        return 1;
    }
    HMODULE old_module = LoadLibraryExW(L"riched20.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
    HMODULE new_module = LoadLibraryExW(L"msftedit.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
    bool old_ok = measure_window("RichEdit20W", old_module, L"RichEdit20W");
    bool new_ok = measure_window("RICHEDIT50W", new_module, L"RICHEDIT50W");
    bool old_services_ok = measure_services("riched20_services", old_module);
    bool new_services_ok = measure_services("msftedit_services", new_module);
    bool complete = old_ok && new_ok && old_services_ok && new_services_ok;
    if (new_module) FreeLibrary(new_module);
    if (old_module) FreeLibrary(old_module);
    OleUninitialize();
    printf("measure_complete=%d\n", complete);
    return complete ? 0 : 1;
}
