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

static void expand_and_print(const char *source, const char *fixture, const char *label,
                             ITextRange *range, LONG unit, bool use_delta)
{
    const LONG sentinel = 0x12345678;
    LONG start = -1, end = -1, out_start = -1, out_end = -1, delta = sentinel;
    HRESULT hr, start_hr, end_hr;

    range->GetStart(&start);
    range->GetEnd(&end);
    hr = range->Expand(unit, use_delta ? &delta : nullptr);
    start_hr = range->GetStart(&out_start);
    end_hr = range->GetEnd(&out_end);
    printf("source=%s fixture=%s case=%s unit=%ld in=%ld,%ld hr=0x%08lx start=%ld end=%ld",
           source, fixture, label, unit, start, end, (unsigned long)hr, out_start, out_end);
    if (use_delta) printf(" delta=%ld", delta);
    else printf(" delta=NULL");
    printf(" getter_hr=0x%08lx,0x%08lx\n", (unsigned long)start_hr, (unsigned long)end_hr);
}

static bool measure_matrix(const char *source, ITextDocument *doc, LONG story_end,
                           bool narrow = false)
{
    const struct
    {
        const char *label;
        LONG start, end;
    } cases[] =
    {
        {"inside_first", 2, 5},
        {"across_paragraphs", 5, 23},
        {"whole_first", 0, 18},
        {"whole_second", 18, 31},
        {"exact_end_boundary", 6, 18},
        {"collapsed_inside", 7, 7},
        {"collapsed_at_boundary", 18, 18},
        {"explicit_final_cr", 35, 36},
        {"collapsed_final_mark", story_end - 1, story_end - 1},
        {"final_mark", story_end - 1, story_end},
        {"collapsed_story_end", story_end, story_end},
    };
    const bool delta_modes[] = {false, true};
    const LONG units[] = {tomCharacter, tomWord, tomSentence, tomParagraph, tomLine};
    const char *fixture = narrow ? "narrow" : "paragraphs";

    for (LONG unit : units)
    {
        if (narrow && unit != tomLine) continue;
        for (const auto &item : cases)
        {
            for (bool use_delta : delta_modes)
            {
                ITextRange *range = nullptr;
                HRESULT hr = doc->Range(item.start, item.end, &range);
                if (FAILED(hr))
                {
                    printf("source=%s fixture=%s case=%s setup=range_failed hr=0x%08lx\n",
                           source, fixture, item.label, (unsigned long)hr);
                    return false;
                }
                expand_and_print(source, fixture, item.label, range, unit, use_delta);
                range->Release();
            }
        }
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
    if (duplicate)
    {
        expand_and_print(source, "short", "duplicate", duplicate, tomParagraph, false);
        duplicate->Release();
    }
    if (range) range->Release();
    return SUCCEEDED(hr);
}

static bool measure_document(const char *source, ITextDocument *doc, HWND hwnd)
{
    ITextRange *story = nullptr;
    LONG end = -1;
    HRESULT hr = write_text(doc, L"Alpha beta. Next!\rGamma delta.\rLast\r");

    if (SUCCEEDED(hr)) hr = doc->Range(0, 0, &story);
    if (SUCCEEDED(hr)) hr = story->Expand(tomStory, nullptr);
    if (SUCCEEDED(hr)) hr = story->GetEnd(&end);
    if (story) story->Release();
    printf("source=%s fixture=paragraphs event=setup hr=0x%08lx story_end=%ld\n",
           source, (unsigned long)hr, end);
    if (FAILED(hr) || end < 36 || !measure_matrix(source, doc, end)) return false;

    if (hwnd)
    {
        if (!SetWindowPos(hwnd, nullptr, 0, 0, 55, 200, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE))
            return false;
        if (!measure_matrix(source, doc, end, true)) return false;
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
    if (doc) success = measure_document(source, doc, hwnd);
    if (doc) doc->Release();
    if (ole) ole->Release();
    if (hwnd) DestroyWindow(hwnd);
    printf("source=%s complete=%d\n", source, success);
    return success;
}

static void measure_services(const char *source, HMODULE module)
{
    ITextDocument *doc = nullptr;
    IUnknown *services = nullptr;
    PCreateTextServices create;
    HRESULT hr;

    if (!module || !(create = (PCreateTextServices)GetProcAddress(module, "CreateTextServices")))
    {
        printf("source=%s event=unavailable\n", source);
        return;
    }
    HDC dc = GetDC(nullptr);
    if (!dc)
    {
        printf("source=%s event=dc_failed error=%lu\n", source, GetLastError());
        return;
    }
    measure_host host(dc);
    hr = create(nullptr, &host, &services);
    printf("source=%s event=create hr=0x%08lx\n", source, (unsigned long)hr);
    if (SUCCEEDED(hr) && services)
    {
        hr = services->QueryInterface(document_iid, (void **)&doc);
        printf("source=%s event=document hr=0x%08lx\n", source, (unsigned long)hr);
    }
    bool success = doc && measure_document(source, doc, nullptr);
    if (doc) doc->Release();
    if (services) services->Release();
    ReleaseDC(nullptr, dc);
    printf("source=%s complete=%d\n", source, success);
}

int main()
{
    HRESULT hr = OleInitialize(nullptr);
    setvbuf(stdout, nullptr, _IONBF, 0);
    printf("measure=riched-expand bits=%u\n", (unsigned)(8 * sizeof(void *)));
    if (FAILED(hr))
    {
        printf("setup=ole_initialize_failed hr=0x%08lx\n", (unsigned long)hr);
        return 1;
    }
    HMODULE old_module = LoadLibraryExW(L"riched20.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
    HMODULE new_module = LoadLibraryExW(L"msftedit.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
    bool old_ok = measure_window("RichEdit20W", old_module, L"RichEdit20W");
    bool new_ok = measure_window("RICHEDIT50W", new_module, L"RICHEDIT50W");
    measure_services("riched20_services", old_module);
    measure_services("msftedit_services", new_module);
    if (new_module) FreeLibrary(new_module);
    if (old_module) FreeLibrary(old_module);
    OleUninitialize();
    printf("measure_complete=%d\n", old_ok && new_ok);
    return old_ok && new_ok ? 0 : 1;
}
