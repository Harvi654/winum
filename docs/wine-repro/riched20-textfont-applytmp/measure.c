/* Original, AI-assisted (Claude), LGPL-2.1-or-later.
 * MEASURE: TOM ITextFont::Reset(tomApplyTmp) followed by property changes
 * and Reset(tomApplyNow) / Reset(tomApplyLater), on a range of an own
 * windowed RichEdit, riched20 and msftedit. Reports the HRESULTs, what the
 * font object returns and what EM_GETCHARFORMAT shows in the text. */
#define COBJMACROS
#include "../batch1-measure/comctl-support.h"
#include <ole2.h>
#include <richedit.h>
#include <richole.h>
#include <tom.h>

#define TOM_APPLYTMP 4 /* tomApplyTmp, not in every tom.h */

static const GUID iid_itextdocument =
    { 0x8cc497c0, 0xa1df, 0x11ce, { 0x80, 0x98, 0x00, 0xaa, 0x00, 0x47, 0xbe, 0x5d } };

static const struct { const char *name; const WCHAR *dll, *cls; } dlls[] =
{
    { "riched20", L"riched20.dll", L"RichEdit20W" },
    { "msftedit", L"msftedit.dll", L"RICHEDIT50W" },
};

/* applytmp_now: Reset(tomApplyTmp), set, Reset(tomApplyNow);
 * applytmp_stale: as applytmp_now, but the text's colour is changed with
 *   EM_SETCHARFORMAT after the font object was taken;
 * applytmp_only: Reset(tomApplyTmp), set, no further Reset;
 * applytmp_empty: Reset(tomApplyTmp), Reset(tomApplyNow), nothing set;
 * applytmp_later: Reset(tomApplyTmp), set, Reset(tomApplyLater), Reset(tomApplyNow);
 * applylater_now: control, Reset(tomApplyLater), set, Reset(tomApplyNow);
 * direct: control, set without any Reset */
static const char *sequences[] =
{
    "applytmp_now", "applytmp_stale", "applytmp_only", "applytmp_empty",
    "applytmp_later", "applylater_now", "direct",
};

static HWND edit;
static const WCHAR text[] = L"abc def ghi";

static void dump_text(const char *seq, const char *when)
{
    CHARFORMAT2W cf;
    LONG i, len = lstrlenW(text);
    printf("TEXT seq=%s when=%s", seq, when);
    for (i = 0; i < len; i++)
    {
        memset(&cf, 0, sizeof(cf));
        cf.cbSize = sizeof(cf);
        SendMessageW(edit, EM_SETSEL, i, i + 1);
        SendMessageW(edit, EM_GETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);
        printf(" %ld:u%d/c%06lx%s/b%d", i, (cf.dwMask & CFM_UNDERLINETYPE) ? cf.bUnderlineType : -1,
               cf.crTextColor & 0xffffff, (cf.dwEffects & CFE_AUTOCOLOR) ? "a" : "",
               !!(cf.dwEffects & CFE_BOLD));
    }
    printf("\n");
}

static void dump_font(ITextFont *font, const char *seq, const char *when)
{
    LONG underline = -1, color = -1, bold = -1;
    HRESULT hr1, hr2, hr3;
    hr1 = ITextFont_GetUnderline(font, &underline);
    hr2 = ITextFont_GetForeColor(font, &color);
    hr3 = ITextFont_GetBold(font, &bold);
    printf("FONT seq=%s when=%s underline=%08lx:%ld forecolor=%08lx:%08lx bold=%08lx:%ld\n", seq, when,
           (DWORD)hr1, underline, (DWORD)hr2, (DWORD)color, (DWORD)hr3, bold);
}

static void setup_text(void)
{
    CHARFORMAT2W cf;
    SendMessageW(edit, WM_SETTEXT, 0, (LPARAM)text);
    memset(&cf, 0, sizeof(cf));
    cf.cbSize = sizeof(cf);
    cf.dwMask = CFM_COLOR | CFM_BOLD | CFM_UNDERLINETYPE;
    cf.crTextColor = RGB(0, 128, 0);
    SendMessageW(edit, EM_SETCHARFORMAT, SCF_ALL, (LPARAM)&cf);
    SendMessageW(edit, EM_SETSEL, 4, 7);
    cf.crTextColor = RGB(255, 0, 0);
    cf.dwEffects = CFE_BOLD;
    SendMessageW(edit, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);
}

static void set_props(ITextFont *font, const char *seq)
{
    HRESULT hr1 = ITextFont_SetUnderline(font, tomWave);
    HRESULT hr2 = ITextFont_SetForeColor(font, RGB(0, 0, 255));
    HRESULT hr3 = ITextFont_SetBold(font, tomFalse);
    printf("SET seq=%s underline_hr=%08lx forecolor_hr=%08lx bold_hr=%08lx\n", seq, (DWORD)hr1, (DWORD)hr2, (DWORD)hr3);
}

static void run_sequence(ITextDocument *doc, unsigned int s)
{
    const char *seq = sequences[s];
    ITextRange *range = NULL;
    ITextFont *font = NULL;
    CHARFORMAT2W cf;
    HRESULT hr;

    setup_text();
    hr = ITextDocument_Range(doc, 4, 7, &range);
    if (SUCCEEDED(hr)) hr = ITextRange_GetFont(range, &font);
    if (FAILED(hr) || !font) { b3_error("GetFont", (DWORD)hr); if (range) ITextRange_Release(range); return; }
    printf("SEQUENCE seq=%s\n", seq);
    dump_font(font, seq, "taken");
    if (s == 1)
    {
        /* the text changes after the font object was taken */
        memset(&cf, 0, sizeof(cf));
        cf.cbSize = sizeof(cf);
        cf.dwMask = CFM_COLOR;
        cf.crTextColor = RGB(128, 0, 128);
        SendMessageW(edit, EM_SETSEL, 4, 7);
        SendMessageW(edit, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);
        dump_font(font, seq, "text_changed");
    }
    if (s <= 4) printf("RESET seq=%s mode=applytmp hr=%08lx\n", seq, (DWORD)ITextFont_Reset(font, TOM_APPLYTMP));
    if (s == 5) printf("RESET seq=%s mode=applylater hr=%08lx\n", seq, (DWORD)ITextFont_Reset(font, tomApplyLater));
    if (s != 3)
    {
        set_props(font, seq);
        dump_font(font, seq, "set");
        dump_text(seq, "set");
    }
    if (s == 4)
    {
        printf("RESET seq=%s mode=applylater hr=%08lx\n", seq, (DWORD)ITextFont_Reset(font, tomApplyLater));
        dump_font(font, seq, "applylater");
        dump_text(seq, "applylater");
    }
    if (s != 2 && s != 6)
    {
        printf("RESET seq=%s mode=applynow hr=%08lx\n", seq, (DWORD)ITextFont_Reset(font, tomApplyNow));
        dump_font(font, seq, "applynow");
        dump_text(seq, "applynow");
    }
    ITextFont_Release(font);
    ITextRange_Release(range);
    /* a fresh font object on the same range */
    hr = ITextDocument_Range(doc, 4, 7, &range);
    if (SUCCEEDED(hr)) hr = ITextRange_GetFont(range, &font);
    if (SUCCEEDED(hr) && font)
    {
        dump_font(font, seq, "fresh");
        ITextFont_Release(font);
    }
    if (range) ITextRange_Release(range);
}

/* tomApplyTmp on a font that is not attached to a range */
static void detached(ITextDocument *doc)
{
    ITextRange *range = NULL;
    ITextFont *font = NULL, *dup = NULL;
    HRESULT hr;

    setup_text();
    hr = ITextDocument_Range(doc, 4, 7, &range);
    if (SUCCEEDED(hr)) hr = ITextRange_GetFont(range, &font);
    if (SUCCEEDED(hr) && font) hr = ITextFont_GetDuplicate(font, &dup);
    if (FAILED(hr) || !dup) { b3_error("GetDuplicate", (DWORD)hr); }
    else
    {
        printf("RESET seq=detached mode=applytmp hr=%08lx\n", (DWORD)ITextFont_Reset(dup, TOM_APPLYTMP));
        printf("RESET seq=detached mode=applynow hr=%08lx\n", (DWORD)ITextFont_Reset(dup, tomApplyNow));
        ITextFont_Release(dup);
    }
    if (font) ITextFont_Release(font);
    if (range) ITextRange_Release(range);
}

int main(void)
{
    ITextDocument *doc;
    IRichEditOle *ole;
    unsigned int d, s;
    HWND parent;
    HRESULT hr;

    if (!b3_start("riched20-textfont-applytmp", 60000)) return 2;
    printf("LEGEND per character: u<bUnderlineType>/c<crTextColor as BBGGRR>[a=CFE_AUTOCOLOR]/b<bold>\n");
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
            for (s = 0; s < ARRAYSIZE(sequences); s++) run_sequence(doc, s);
            detached(doc);
            ITextDocument_Release(doc);
        }
        ole->lpVtbl->Release(ole);
        DestroyWindow(parent);
    }
    return b3_finish();
}
