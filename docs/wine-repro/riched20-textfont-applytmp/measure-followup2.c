/* Original, AI-assisted (Claude), LGPL-2.1-or-later.
 * MEASURE (second follow-up for textfont-applytmp): a second tomApplyTmp
 * cycle, with the font and the text read after every step, on the same
 * font object and on a new one; and a first cycle whose change is bold.
 * msftedit (RICHEDIT50W) and, for comparison, riched20; own windows only. */
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

static void step(ITextFont *font, const char *seq, const char *when)
{
    font_state(font, seq, when);
    text_state(seq, when);
}

/* same: one font object, two cycles (blue, then not bold);
 * new_font: the second cycle on a new font object of the same range;
 * bold_first: one cycle whose change is not bold;
 * twice_tmp: tomApplyTmp twice before the change */
static const char *sequences[] = { "same", "new_font", "bold_first", "twice_tmp" };

static void run(ITextDocument *doc, unsigned int s)
{
    const char *seq = sequences[s];
    ITextRange *range = NULL;
    ITextFont *font = NULL, *font2 = NULL;
    HRESULT hr;

    setup();
    hr = ITextDocument_Range(doc, 4, 7, &range);
    if (SUCCEEDED(hr)) hr = ITextRange_GetFont(range, &font);
    if (FAILED(hr) || !font) { b3_error("GetFont", (DWORD)hr); if (range) ITextRange_Release(range); return; }
    printf("SEQUENCE seq=%s\n", seq);
    if (s == 2)
    {
        reset(font, TOM_APPLYTMP, seq, "applytmp");
        printf("SET seq=%s bold_hr=%08lx\n", seq, (DWORD)ITextFont_SetBold(font, tomFalse));
        step(font, seq, "set");
        reset(font, tomApplyNow, seq, "applynow");
        step(font, seq, "now");
    }
    else
    {
        reset(font, TOM_APPLYTMP, seq, "applytmp");
        if (s == 3) reset(font, TOM_APPLYTMP, seq, "applytmp");
        printf("SET seq=%s color_hr=%08lx\n", seq, (DWORD)ITextFont_SetForeColor(font, RGB(0, 0, 255)));
        step(font, seq, "set1");
        reset(font, tomApplyNow, seq, "applynow");
        step(font, seq, "now1");
        if (s == 1)
        {
            hr = ITextRange_GetFont(range, &font2);
            if (FAILED(hr) || !font2) { b3_error("GetFont2", (DWORD)hr); font2 = NULL; }
        }
        if (s <= 1)
        {
            ITextFont *f = font2 ? font2 : font;
            reset(f, TOM_APPLYTMP, seq, "applytmp2");
            step(f, seq, "tmp2");
            printf("SET seq=%s bold_hr=%08lx\n", seq, (DWORD)ITextFont_SetBold(f, tomFalse));
            step(f, seq, "set2");
            reset(f, tomApplyNow, seq, "applynow2");
            step(f, seq, "now2");
        }
    }
    if (font2) ITextFont_Release(font2);
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

    if (!b3_start("riched20-textfont-followup2", 60000)) return 2;
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
