/* Original, AI-assisted (Claude), LGPL-2.1-or-later.
 * MEASURE: caret and selection when the caret is placed right after a text
 * change that needs a rewrap (TOM ITextRange::SetText, then Select,
 * EM_SETSEL, EM_EXSETSEL, focus or ScrollIntoView), riched20 and msftedit.
 * Each case runs in its own child process. Own windows and text only. */
#define COBJMACROS
#include "../batch1-measure/comctl-support.h"
#include <ole2.h>
#include <richedit.h>
#include <richole.h>
#include <tom.h>

static const GUID iid_itextdocument =
    { 0x8cc497c0, 0xa1df, 0x11ce, { 0x80, 0x98, 0x00, 0xaa, 0x00, 0x47, 0xbe, 0x5d } };

static const struct { const char *name; const WCHAR *dll, *cls; } dlls[] =
{
    { "riched20", L"riched20.dll", L"RichEdit20W" },
    { "msftedit", L"msftedit.dll", L"RICHEDIT50W" },
};
/* tom_replace: ITextRange::SetText over the whole text; tom_append: an
 * ITextRange at the end; replacesel: EM_REPLACESEL (a message path) */
static const char *changes[] = { "tom_replace", "tom_append", "replacesel" };
static const char *triggers[] = { "range_select", "em_setsel", "em_exsetsel", "setfocus", "scrollintoview", "none" };

static const WCHAR initial[] = L"abc";
static const WCHAR long_text[] = L"alpha beta gamma delta epsilon zeta eta theta iota kappa";

static void report(HWND edit, LONG pos, const char *when)
{
    CHARRANGE sel = { -1, -1 };
    POINTL pt = { -1, -1 };
    POINT caret = { -1, -1 };
    LONG lines, line, first;
    BOOL caret_ok;

    SendMessageW(edit, EM_EXGETSEL, 0, (LPARAM)&sel);
    lines = (LONG)SendMessageW(edit, EM_GETLINECOUNT, 0, 0);
    line = (LONG)SendMessageW(edit, EM_EXLINEFROMCHAR, 0, pos);
    first = (LONG)SendMessageW(edit, EM_LINEINDEX, line, 0);
    SendMessageW(edit, EM_POSFROMCHAR, (WPARAM)&pt, pos);
    caret_ok = GetCaretPos(&caret);
    printf("STATE when=%s sel=%ld,%ld textlen=%ld lines=%ld pos=%ld line=%ld line_start=%ld "
           "posfromchar=%ld,%ld caret_ok=%d caret=%ld,%ld caret_matches=%d focus=%d\n",
           when, sel.cpMin, sel.cpMax, (long)GetWindowTextLengthW(edit), lines, pos, line, first,
           pt.x, pt.y, caret_ok, caret.x, caret.y, caret.x == pt.x && caret.y == pt.y,
           GetFocus() == edit);
}

static int child(unsigned int d, unsigned int c, unsigned int t)
{
    ITextDocument *doc = NULL;
    IRichEditOle *ole = NULL;
    ITextRange *range = NULL;
    HWND parent, edit, other;
    CHARRANGE cr;
    BSTR text;
    LONG pos;
    HRESULT hr;

    child_start();
    printf("CALL dll=%s change=%s trigger=%s\n", dlls[d].name, changes[c], triggers[t]);
    if (!LoadLibraryW(dlls[d].dll)) { printf("ERROR operation=LoadLibrary code=%08lx\n", GetLastError()); return 4; }
    parent = CreateWindowExW(0, L"STATIC", L"measure", WS_OVERLAPPEDWINDOW | WS_VISIBLE, 50, 50, 300, 300,
                             NULL, NULL, GetModuleHandleW(NULL), NULL);
    edit = CreateWindowExW(0, dlls[d].cls, initial, WS_CHILD | WS_VISIBLE | ES_MULTILINE | WS_BORDER,
                           10, 10, 120, 200, parent, (HMENU)1, GetModuleHandleW(NULL), NULL);
    other = CreateWindowExW(0, L"EDIT", L"", WS_CHILD | WS_VISIBLE, 150, 10, 60, 20,
                            parent, (HMENU)2, GetModuleHandleW(NULL), NULL);
    if (!parent || !edit || !other) { printf("ERROR operation=CreateWindow code=%08lx\n", GetLastError()); return 5; }
    SetForegroundWindow(parent);
    SetFocus(t == 3 ? other : edit);
    UpdateWindow(parent);
    if (!SendMessageW(edit, EM_GETOLEINTERFACE, 0, (LPARAM)&ole) || !ole)
    { printf("ERROR operation=EM_GETOLEINTERFACE code=0\n"); return 6; }
    hr = ole->lpVtbl->QueryInterface(ole, &iid_itextdocument, (void **)&doc);
    if (FAILED(hr)) { printf("ERROR operation=QI_ITextDocument code=%08lx\n", (DWORD)hr); return 7; }
    report(edit, 0, "initial");

    /* the change */
    text = SysAllocString(long_text);
    if (c == 0) hr = ITextDocument_Range(doc, 0, 3, &range);
    else if (c == 1) hr = ITextDocument_Range(doc, 3, 3, &range);
    else hr = S_OK;
    if (FAILED(hr)) { printf("ERROR operation=Range code=%08lx\n", (DWORD)hr); return 8; }
    if (c < 2)
    {
        hr = ITextRange_SetText(range, text);
        printf("SETTEXT hr=%08lx\n", (DWORD)hr);
    }
    else
    {
        SendMessageW(edit, EM_SETSEL, 0, -1);
        SendMessageW(edit, EM_REPLACESEL, FALSE, (LPARAM)long_text);
    }
    SysFreeString(text);
    report(edit, 0, "changed");

    /* place the caret inside the text, on a wrapped line */
    pos = 23;
    switch (t)
    {
    case 0:
        if (range) ITextRange_Release(range);
        range = NULL;
        hr = ITextDocument_Range(doc, pos, pos, &range);
        if (SUCCEEDED(hr)) hr = ITextRange_Select(range);
        printf("TRIGGER hr=%08lx\n", (DWORD)hr);
        break;
    case 1:
        printf("TRIGGER ret=%ld\n", (long)SendMessageW(edit, EM_SETSEL, pos, pos));
        break;
    case 2:
        cr.cpMin = cr.cpMax = pos;
        printf("TRIGGER ret=%ld\n", (long)SendMessageW(edit, EM_EXSETSEL, 0, (LPARAM)&cr));
        break;
    case 3:
        SetFocus(edit);
        printf("TRIGGER focus=%d\n", GetFocus() == edit);
        break;
    case 4:
        if (range) ITextRange_Release(range);
        range = NULL;
        hr = ITextDocument_Range(doc, pos, pos, &range);
        if (SUCCEEDED(hr)) hr = ITextRange_ScrollIntoView(range, tomStart);
        printf("TRIGGER hr=%08lx\n", (DWORD)hr);
        break;
    default:
        printf("TRIGGER none\n");
        break;
    }
    if (t >= 3)
    {
        CHARRANGE cur;
        SendMessageW(edit, EM_EXGETSEL, 0, (LPARAM)&cur);
        pos = cur.cpMin;
    }
    report(edit, pos, "triggered");
    UpdateWindow(edit);
    report(edit, pos, "painted");
    if (range) ITextRange_Release(range);
    ITextDocument_Release(doc);
    ole->lpVtbl->Release(ole);
    DestroyWindow(parent);
    return 0;
}

int wmain(int argc, WCHAR **argv)
{
    unsigned int d, c, t;
    WCHAR args[32];
    char label[96];

    if (argc == 4) return child(_wtoi(argv[1]), _wtoi(argv[2]), _wtoi(argv[3]));
    if (!b3_start("riched20-caret-rewrap", 240000)) return 2;
    for (d = 0; d < ARRAYSIZE(dlls); d++)
        for (c = 0; c < ARRAYSIZE(changes); c++)
            for (t = 0; t < ARRAYSIZE(triggers); t++)
            {
                swprintf(args, ARRAYSIZE(args), L"%u %u %u", d, c, t);
                snprintf(label, sizeof(label), "dll=%s change=%s trigger=%s", dlls[d].name, changes[c], triggers[t]);
                run_child(label, args, 20000);
            }
    return b3_finish();
}
