/* Original, AI-assisted (Claude), LGPL-2.1-or-later.
 * MEASURE (follow-up for textfont-applytmp, cross review P3): what one font
 * object does after a tomApplyTmp cycle - a second tomApplyNow, changes
 * after the cycle with and without a new Reset, tomCacheParms afterwards.
 * riched20 (RichEdit20W) and msftedit (RICHEDIT50W); own windows only. */
#define COBJMACROS
#include "../batch3-measure/measure-support.h"
#include <ole2.h>
#include <richedit.h>
#include <richole.h>
#include <tom.h>

#define TOM_APPLYTMP 4

static const GUID iid_itextdocument =
    { 0x8cc497c0, 0xa1df, 0x11ce, { 0x80, 0x98, 0x00, 0xaa, 0x00, 0x47, 0xbe, 0x5d } };
static const struct { const char *name; const WCHAR *dll, *cls; } dlls[] =
{
    { "riched20", L"riched20.dll", L"RichEdit20W" },
    { "msftedit", L"msftedit.dll", L"RICHEDIT50W" },
};
static HWND edit;

static void text_state(const char *seq, const char *when)
{
    CHARFORMAT2W cf;
    memset(&cf, 0, sizeof(cf));
    cf.cbSize = sizeof(cf);
    SendMessageW(edit, EM_SETSEL, 4, 7);
    SendMessageW(edit, EM_GETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);
    printf("TEXT seq=%s when=%s mask_color=%d color=%06lx mask_bold=%d bold=%d\n", seq, when,
           !!(cf.dwMask & CFM_COLOR), cf.crTextColor & 0xffffff, !!(cf.dwMask & CFM_BOLD), !!(cf.dwEffects & CFE_BOLD));
}

static void font_state(ITextFont *font, const char *seq, const char *when)
{
    LONG color = -1, bold = -1;
    HRESULT hr1 = ITextFont_GetForeColor(font, &color), hr2 = ITextFont_GetBold(font, &bold);
    printf("FONT seq=%s when=%s color=%08lx:%08lx bold=%08lx:%ld\n", seq, when, (DWORD)hr1, (DWORD)color, (DWORD)hr2, bold);
}

static void reset(ITextFont *font, LONG mode, const char *seq, const char *name)
{
    printf("RESET seq=%s mode=%s hr=%08lx\n", seq, name, (DWORD)ITextFont_Reset(font, mode));
}

static void setup(void)
{
    CHARFORMAT2W cf;
    SendMessageW(edit, WM_SETTEXT, 0, (LPARAM)L"abc def ghi");
    memset(&cf, 0, sizeof(cf));
    cf.cbSize = sizeof(cf);
    cf.dwMask = CFM_COLOR | CFM_BOLD;
    cf.crTextColor = RGB(0, 128, 0);
    SendMessageW(edit, EM_SETCHARFORMAT, SCF_ALL, (LPARAM)&cf);
    SendMessageW(edit, EM_SETSEL, 4, 7);
    cf.crTextColor = RGB(255, 0, 0);
    cf.dwEffects = CFE_BOLD;
    SendMessageW(edit, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);
}

/* now_now: tmp, set blue, now, now;
 * set_after: tmp, set blue, now, set not bold (no Reset);
 * tmp_again: tmp, set blue, now, tmp, set not bold, now;
 * cache_after: tmp, set blue, now, cacheparms, read, trackparms */
static const char *sequences[] = { "now_now", "set_after", "tmp_again", "cache_after" };

static void run(ITextDocument *doc, unsigned int s)
{
    const char *seq = sequences[s];
    ITextRange *range = NULL;
    ITextFont *font = NULL;
    HRESULT hr;

    setup();
    hr = ITextDocument_Range(doc, 4, 7, &range);
    if (SUCCEEDED(hr)) hr = ITextRange_GetFont(range, &font);
    if (FAILED(hr) || !font) { b3_error("GetFont", (DWORD)hr); if (range) ITextRange_Release(range); return; }
    printf("SEQUENCE seq=%s\n", seq);
    reset(font, TOM_APPLYTMP, seq, "applytmp");
    printf("SET seq=%s color_hr=%08lx\n", seq, (DWORD)ITextFont_SetForeColor(font, RGB(0, 0, 255)));
    reset(font, tomApplyNow, seq, "applynow");
    font_state(font, seq, "now1");
    text_state(seq, "now1");
    switch (s)
    {
    case 0:
        reset(font, tomApplyNow, seq, "applynow");
        break;
    case 1:
        printf("SET seq=%s bold_hr=%08lx\n", seq, (DWORD)ITextFont_SetBold(font, tomFalse));
        break;
    case 2:
        reset(font, TOM_APPLYTMP, seq, "applytmp");
        printf("SET seq=%s bold_hr=%08lx\n", seq, (DWORD)ITextFont_SetBold(font, tomFalse));
        reset(font, tomApplyNow, seq, "applynow");
        break;
    case 3:
        reset(font, tomCacheParms, seq, "cacheparms");
        font_state(font, seq, "cached");
        reset(font, tomTrackParms, seq, "trackparms");
        break;
    }
    font_state(font, seq, "end");
    text_state(seq, "end");
    ITextFont_Release(font);
    ITextRange_Release(range);
}

int main(void)
{
    ITextDocument *doc;
    IRichEditOle *ole;
    unsigned int d, s;
    HWND parent;
    HRESULT hr;

    if (!b3_start("riched20-textfont-followup", 60000)) return 2;
    for (d = 0; d < ARRAYSIZE(dlls); d++)
    {
        printf("DLL %s\n", dlls[d].name);
        if (!LoadLibraryW(dlls[d].dll)) { b3_error("LoadLibrary", GetLastError()); continue; }
        parent = CreateWindowExW(0, L"STATIC", L"measure", WS_OVERLAPPEDWINDOW, 0, 0, 400, 200,
                                 NULL, NULL, GetModuleHandleW(NULL), NULL);
        edit = CreateWindowExW(0, dlls[d].cls, L"", WS_CHILD | WS_VISIBLE | ES_MULTILINE,
                               0, 0, 380, 150, parent, (HMENU)1, GetModuleHandleW(NULL), NULL);
        if (!parent || !edit) { b3_error("CreateWindow", GetLastError()); continue; }
        ole = NULL;
        doc = NULL;
        if (!SendMessageW(edit, EM_GETOLEINTERFACE, 0, (LPARAM)&ole) || !ole)
        { b3_error("EM_GETOLEINTERFACE", 0); DestroyWindow(parent); continue; }
        hr = ole->lpVtbl->QueryInterface(ole, &iid_itextdocument, (void **)&doc);
        if (FAILED(hr)) b3_error("QI_ITextDocument", (DWORD)hr);
        else
        {
            for (s = 0; s < ARRAYSIZE(sequences); s++) run(doc, s);
            ITextDocument_Release(doc);
        }
        ole->lpVtbl->Release(ole);
        DestroyWindow(parent);
    }
    return b3_finish();
}
