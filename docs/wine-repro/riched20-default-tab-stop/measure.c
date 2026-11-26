/* Original MEASURE program, AI-assisted (Codex), LGPL-2.1-or-later. */

#define COBJMACROS
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <ole2.h>
#include <richedit.h>
#include <richole.h>
#include <tom.h>
#include <limits.h>
#include <stdio.h>

/* MinGW's uuid archive does not supply IID_ITextDocument. */
static const IID text_document_iid =
    {0x8cc497c0, 0xa1df, 0x11ce, {0x80, 0x98, 0x00, 0xaa, 0x00, 0x47, 0xbe, 0x5d}};

static const WCHAR text[] = L"A\tB\tC";

static void measure_positions(HWND hwnd, const char *name, const char *step)
{
    static const int offsets[] = {0, 2, 4};
    size_t i;

    for (i = 0; i < ARRAYSIZE(offsets); ++i)
    {
        POINTL pt = {LONG_MIN, LONG_MIN};
        LRESULT ret = SendMessageW(hwnd, EM_POSFROMCHAR, (WPARAM)&pt, offsets[i]);
        printf("class=%s step=%s event=position char=%d ret=%lld x=%ld y=%ld\n",
               name, step, offsets[i], (long long)ret, pt.x, pt.y);
    }
}

static void measure_para(HWND hwnd, const char *name, const char *step)
{
    PARAFORMAT2 format = {0};
    LRESULT ret;
    int i;

    format.cbSize = sizeof(format);
    ret = SendMessageW(hwnd, EM_GETPARAFORMAT, 0, (LPARAM)&format);
    printf("class=%s step=%s event=para ret=0x%08lx mask=0x%08lx indent=%ld tab_count=%d tabs=",
           name, step, (unsigned long)ret, (unsigned long)format.dwMask,
           format.dxStartIndent, format.cTabCount);
    for (i = 0; i < ARRAYSIZE(format.rgxTabs); ++i)
        printf("%s%ld", i ? "," : "", format.rgxTabs[i]);
    putchar('\n');
}

static void measure_get(ITextDocument *document, const char *name, const char *step)
{
    float value = -12345.0f;
    HRESULT hr = ITextDocument_GetDefaultTabStop(document, &value);

    printf("class=%s step=%s event=get hr=0x%08lx points=%.9g\n",
           name, step, (unsigned long)hr, (double)value);
}

static void measure_options(HWND hwnd, const char *name)
{
    static const DWORD inputs[] = {0, IMF_AUTOFONT};
    LRESULT before, ret, after;
    size_t i;

    before = SendMessageW(hwnd, EM_GETLANGOPTIONS, 0, 0);
    printf("class=%s event=language_initial flags=0x%08lx\n", name, (unsigned long)before);
    for (i = 0; i < ARRAYSIZE(inputs); ++i)
    {
        ret = SendMessageW(hwnd, EM_SETLANGOPTIONS, 0, inputs[i]);
        after = SendMessageW(hwnd, EM_GETLANGOPTIONS, 0, 0);
        printf("class=%s event=language_set input=0x%08lx ret=%lld read=0x%08lx\n",
               name, (unsigned long)inputs[i], (long long)ret, (unsigned long)after);
    }
    ret = SendMessageW(hwnd, EM_SETLANGOPTIONS, 0, before);
    after = SendMessageW(hwnd, EM_GETLANGOPTIONS, 0, 0);
    printf("class=%s event=language_restore ret=%lld read=0x%08lx\n",
           name, (long long)ret, (unsigned long)after);

    before = SendMessageW(hwnd, EM_GETTYPOGRAPHYOPTIONS, 0, 0);
    ret = SendMessageW(hwnd, EM_SETTYPOGRAPHYOPTIONS,
                       TO_ADVANCEDTYPOGRAPHY, TO_ADVANCEDTYPOGRAPHY);
    after = SendMessageW(hwnd, EM_GETTYPOGRAPHYOPTIONS, 0, 0);
    printf("class=%s event=advanced_typography before=0x%08lx set=0x%08lx mask=0x%08lx ret=%lld read=0x%08lx\n",
           name, (unsigned long)before, (unsigned long)TO_ADVANCEDTYPOGRAPHY,
           (unsigned long)TO_ADVANCEDTYPOGRAPHY, (long long)ret, (unsigned long)after);
}

static BOOL set_text(HWND hwnd)
{
    return SetWindowTextW(hwnd, L"") && SetWindowTextW(hwnd, text);
}

static BOOL measure_control(const WCHAR *dll, const WCHAR *class_name, const char *name)
{
    static const struct
    {
        const char *name;
        float points;
    } cases[] =
    {
        {"set_36", 36.0f},
        {"set_fractional", 18.5f},
        {"set_zero", 0.0f},
        {"set_negative", -12.0f},
        {"set_large", 100000000.0f}
    };
    ITextDocument *document = NULL;
    IRichEditOle *richole = NULL;
    BOOL complete = FALSE;
    HWND hwnd = NULL;
    HMODULE module;
    RECT rect = {0};
    HDC dc;
    HRESULT hr;
    LRESULT available;
    size_t i;

    module = LoadLibraryExW(dll, NULL, LOAD_LIBRARY_SEARCH_SYSTEM32);
    if (!module)
    {
        printf("class=%s setup=load_failed error=%lu\n", name, GetLastError());
        return FALSE;
    }
    hwnd = CreateWindowExW(0, class_name, L"", WS_POPUP | ES_MULTILINE, 0, 0,
                           1600, 400, NULL, NULL, GetModuleHandleW(NULL), NULL);
    if (!hwnd)
    {
        printf("class=%s setup=create_failed error=%lu\n", name, GetLastError());
        goto done;
    }
    available = SendMessageW(hwnd, EM_GETOLEINTERFACE, 0, (LPARAM)&richole);
    if (!available || !richole)
    {
        printf("class=%s setup=get_ole_failed ret=%lld\n", name, (long long)available);
        goto done;
    }
    hr = richole->lpVtbl->QueryInterface(richole, &text_document_iid, (void **)&document);
    if (FAILED(hr) || !document)
    {
        printf("class=%s setup=query_document_failed hr=0x%08lx\n", name, (unsigned long)hr);
        goto done;
    }
    dc = GetDC(hwnd);
    SendMessageW(hwnd, EM_GETRECT, 0, (LPARAM)&rect);
    printf("class=%s setup=ready dpi_x=%d text_left=%ld text_right=%ld\n",
           name, dc ? GetDeviceCaps(dc, LOGPIXELSX) : 0, rect.left, rect.right);
    if (dc) ReleaseDC(hwnd, dc);

    measure_get(document, name, "new");
    hr = ITextDocument_GetDefaultTabStop(document, NULL);
    printf("class=%s step=new event=get_null hr=0x%08lx\n", name, (unsigned long)hr);
    measure_para(hwnd, name, "new");
    if (!set_text(hwnd)) goto text_failed;
    measure_positions(hwnd, name, "before_set");
    measure_para(hwnd, name, "before_set");

    for (i = 0; i < ARRAYSIZE(cases); ++i)
    {
        const char *step = cases[i].name;
        hr = ITextDocument_SetDefaultTabStop(document, cases[i].points);
        printf("class=%s step=%s event=set points=%.9g hr=0x%08lx\n",
               name, step, (double)cases[i].points, (unsigned long)hr);
        measure_get(document, name, step);
        measure_positions(hwnd, name, step);
        measure_para(hwnd, name, step);
        if (!set_text(hwnd)) goto text_failed;
        printf("class=%s step=%s event=reinserted_text\n", name, step);
        measure_positions(hwnd, name, step);
        measure_para(hwnd, name, step);
    }

    /* Use a separate fresh control so the language baseline precedes edits. */
    ITextDocument_Release(document);
    document = NULL;
    richole->lpVtbl->Release(richole);
    richole = NULL;
    DestroyWindow(hwnd);
    hwnd = CreateWindowExW(0, class_name, L"", WS_POPUP | ES_MULTILINE, 0, 0,
                           1600, 400, NULL, NULL, GetModuleHandleW(NULL), NULL);
    if (!hwnd)
    {
        printf("class=%s setup=create_options_control_failed error=%lu\n", name, GetLastError());
        goto done;
    }
    measure_options(hwnd, name);
    complete = TRUE;
    goto done;

text_failed:
    printf("class=%s setup=set_window_text_failed error=%lu\n", name, GetLastError());
done:
    if (document) ITextDocument_Release(document);
    if (richole) richole->lpVtbl->Release(richole);
    if (hwnd) DestroyWindow(hwnd);
    FreeLibrary(module);
    printf("class=%s complete=%d\n", name, complete);
    return complete;
}

int main(void)
{
    BOOL legacy, modern;
    HRESULT hr;

    setvbuf(stdout, NULL, _IONBF, 0);
    printf("measure=riched20-default-tab-stop bits=%u\n", (unsigned int)(sizeof(void *) * CHAR_BIT));
    hr = OleInitialize(NULL);
    if (FAILED(hr))
    {
        printf("setup=ole_initialize_failed hr=0x%08lx\n", (unsigned long)hr);
        return 1;
    }

    legacy = measure_control(L"riched20.dll", L"RichEdit20W", "RichEdit20W");
    modern = measure_control(L"msftedit.dll", L"RICHEDIT50W", "RICHEDIT50W");
    OleUninitialize();

    printf("measure_complete=%d\n", legacy && modern);
    return legacy && modern ? 0 : 1;
}
