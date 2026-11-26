/* Original, AI-assisted (Claude), LGPL-2.1-or-later.
 * MEASURE: RichEdit EM_SETTARGETDEVICE line width and paragraph alignment.
 *
 * Inventor's Format Text dialog (a RICHEDIT50W window) sends
 * EM_SETTARGETDEVICE with a non-NULL DC and a line width of 42211 twips,
 * inserts one character with EM_SETTEXTEX(ST_SELECTION) and centres the
 * paragraph. Under Wine the character is laid out about 1400 pixels to the
 * right and cannot be scrolled to. This program records, for both
 * RichEdit classes, several window styles, reference DCs, line widths and
 * alignments, where Windows places the character (EM_POSFROMCHAR), the
 * formatting rectangle, the horizontal scroll range and the line count.
 * Each case uses a new control. No values are assumed; one tab-separated
 * line per case. */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <richedit.h>
#include <stdio.h>
#include <string.h>

static const struct { const char *name; const WCHAR *dll, *cls; } classes[] =
{
    { "riched20", L"riched20.dll", L"RichEdit20W" },
    { "msftedit", L"msftedit.dll", L"RICHEDIT50W" },
};

static const struct { const char *name; DWORD style; } styles[] =
{
    { "plain", 0 },
    { "scroll_disablenoscroll", WS_HSCROLL | WS_VSCROLL | ES_DISABLENOSCROLL },
    { "scroll_auto", WS_HSCROLL | WS_VSCROLL | ES_AUTOHSCROLL | ES_AUTOVSCROLL },
};

static const struct { const char *name; WORD value; } aligns[] =
{
    { "left", PFA_LEFT },
    { "center", PFA_CENTER },
    { "right", PFA_RIGHT },
};

static const LPARAM widths[] = { 5000, 42211, 100000 };

enum dc_kind { DC_NONE, DC_NULL, DC_SCREEN, DC_MEMORY, DC_METAFILE, DC_PRINTER };
static const char *dc_names[] = { "none", "null", "screen", "memory", "metafile", "printer" };
static HDC dcs[6];

static HWND parent;
static int cases_done, cases_failed;

static void pump(void)
{
    MSG msg;
    while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE))
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
}

static LRESULT CALLBACK parent_proc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    return DefWindowProcW(hwnd, msg, wp, lp);
}

static void set_alignment(HWND edit, WORD align)
{
    PARAFORMAT2 pf;

    SendMessageW(edit, EM_SETSEL, 0, -1);
    memset(&pf, 0, sizeof(pf));
    pf.cbSize = sizeof(pf);
    pf.dwMask = PFM_ALIGNMENT;
    pf.wAlignment = align;
    SendMessageW(edit, EM_SETPARAFORMAT, 0, (LPARAM)&pf);
    SendMessageW(edit, EM_SETSEL, 0, 0);
}

/* order 0: target device, text, alignment (Inventor's order in the trace);
 * order 1: text, alignment, target device. */
static void run_case(int c, int s, enum dc_kind kind, LPARAM width, int a, int order)
{
    static const WCHAR text[] = L"A";
    SETTEXTEX st = { ST_SELECTION, 1200 };
    SCROLLINFO si;
    POINTL pos = { -99999, -99999 };
    POINT scroll = { 0, 0 };
    RECT fmt, client;
    LRESULT target_ret = -1, lines;
    BOOL have_scroll;
    HWND edit;

    if (kind == DC_PRINTER && !dcs[DC_PRINTER])
        return;

    edit = CreateWindowExW(0, classes[c].cls, L"",
                           WS_CHILD | WS_VISIBLE | WS_BORDER | ES_MULTILINE | ES_WANTRETURN | styles[s].style,
                           10, 10, 610, 165, parent, (HMENU)1, GetModuleHandleW(NULL), NULL);
    if (!edit)
    {
        printf("error\t%s\t%s\tCreateWindowEx failed %lu\n", classes[c].name, styles[s].name, GetLastError());
        cases_failed++;
        return;
    }

    if (order == 1)
    {
        SendMessageW(edit, EM_SETTEXTEX, (WPARAM)&st, (LPARAM)text);
        set_alignment(edit, aligns[a].value);
    }
    if (kind != DC_NONE)
        target_ret = SendMessageW(edit, EM_SETTARGETDEVICE, (WPARAM)dcs[kind], width);
    if (order == 0)
    {
        SendMessageW(edit, EM_SETTEXTEX, (WPARAM)&st, (LPARAM)text);
        set_alignment(edit, aligns[a].value);
    }

    UpdateWindow(edit);
    pump();

    SendMessageW(edit, EM_POSFROMCHAR, (WPARAM)&pos, 0);
    SendMessageW(edit, EM_GETRECT, 0, (LPARAM)&fmt);
    GetClientRect(edit, &client);
    SendMessageW(edit, EM_GETSCROLLPOS, 0, (LPARAM)&scroll);
    lines = SendMessageW(edit, EM_GETLINECOUNT, 0, 0);
    memset(&si, 0, sizeof(si));
    si.cbSize = sizeof(si);
    si.fMask = SIF_ALL;
    have_scroll = GetScrollInfo(edit, SB_HORZ, &si);

    printf("case\t%s\t%s\t%s\t%ld\t%s\torder%d\tret=%ld\tpos=%ld,%ld\tclient=%ld,%ld\t"
           "rect=%ld,%ld,%ld,%ld\thscroll=%d:%d,%d,%u,%d\tscrollpos=%ld,%ld\tlines=%ld\tvisible=%d\n",
           classes[c].name, styles[s].name, dc_names[kind], kind == DC_NONE ? 0L : (long)width,
           aligns[a].name, order, (long)target_ret, pos.x, pos.y, client.right, client.bottom,
           fmt.left, fmt.top, fmt.right, fmt.bottom,
           have_scroll, si.nMin, si.nMax, si.nPage, si.nPos, scroll.x, scroll.y, (long)lines,
           pos.x >= 0 && pos.x < client.right);
    fflush(stdout);

    DestroyWindow(edit);
    pump();
    cases_done++;
}

int main(void)
{
    static const LPARAM null_widths[] = { 0, 1, 5000, 42211 };
    WNDCLASSW wc;
    HDC screen;
    DWORD printer_error;
    int c, s, a, w, order, k;

    memset(&wc, 0, sizeof(wc));
    wc.lpfnWndProc = parent_proc;
    wc.hInstance = GetModuleHandleW(NULL);
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wc.lpszClassName = L"winum_target_device_parent";
    RegisterClassW(&wc);
    parent = CreateWindowExW(0, wc.lpszClassName, L"winum measure", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                             0, 0, 700, 260, NULL, NULL, wc.hInstance, NULL);
    if (!parent)
    {
        printf("error\tparent window %lu\n", GetLastError());
        return 1;
    }
    pump();

    screen = GetDC(NULL);
    dcs[DC_SCREEN] = screen;
    dcs[DC_MEMORY] = CreateCompatibleDC(screen);
    dcs[DC_METAFILE] = CreateEnhMetaFileW(screen, NULL, NULL, NULL);
    /* Present on a default Windows 10 install; the default printer's name is not reported. */
    dcs[DC_PRINTER] = CreateDCW(L"WINSPOOL", L"Microsoft Print to PDF", NULL, NULL);
    printer_error = dcs[DC_PRINTER] ? 0 : GetLastError();

    printf("screen\tlogpixels=%d,%d\thorzres=%d\n", GetDeviceCaps(screen, LOGPIXELSX),
           GetDeviceCaps(screen, LOGPIXELSY), GetDeviceCaps(screen, HORZRES));
    printf("metafile\tavailable=%d\n", dcs[DC_METAFILE] != NULL);
    if (dcs[DC_PRINTER])
        printf("printer\tavailable=1\tlogpixels=%d,%d\thorzres=%d\tphysicalwidth=%d\n",
               GetDeviceCaps(dcs[DC_PRINTER], LOGPIXELSX), GetDeviceCaps(dcs[DC_PRINTER], LOGPIXELSY),
               GetDeviceCaps(dcs[DC_PRINTER], HORZRES), GetDeviceCaps(dcs[DC_PRINTER], PHYSICALWIDTH));
    else
        printf("printer\tavailable=0\terror=%lu\n", printer_error);
    printf("columns\tclass\tstyle\tdc\twidth\talign\torder\tEM_SETTARGETDEVICE\tEM_POSFROMCHAR(0)\t"
           "client\tEM_GETRECT\tGetScrollInfo(SB_HORZ) ok:min,max,page,pos\tEM_GETSCROLLPOS\tEM_GETLINECOUNT\t"
           "char 0 inside client\n");

    for (c = 0; c < 2; c++)
    {
        if (!LoadLibraryW(classes[c].dll))
        {
            printf("error\t%s\tLoadLibrary %lu\n", classes[c].name, GetLastError());
            cases_failed++;
            continue;
        }
        for (s = 0; s < 3; s++)
            for (order = 0; order < 2; order++)
                for (a = 0; a < 3; a++)
                {
                    run_case(c, s, DC_NONE, 0, a, order);
                    for (w = 0; w < 4; w++)
                        run_case(c, s, DC_NULL, null_widths[w], a, order);
                    for (k = DC_SCREEN; k <= DC_PRINTER; k++)
                        for (w = 0; w < 3; w++)
                            run_case(c, s, k, widths[w], a, order);
                }
    }

    if (dcs[DC_PRINTER]) DeleteDC(dcs[DC_PRINTER]);
    if (dcs[DC_METAFILE]) DeleteEnhMetaFile(CloseEnhMetaFile(dcs[DC_METAFILE]));
    DeleteDC(dcs[DC_MEMORY]);
    ReleaseDC(NULL, screen);
    DestroyWindow(parent);
    printf("measure_complete cases=%d failed=%d\n", cases_done, cases_failed);
    return cases_failed ? 1 : 0;
}
