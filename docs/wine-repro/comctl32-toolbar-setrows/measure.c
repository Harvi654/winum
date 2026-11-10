/* Original, AI-assisted (Claude), LGPL-2.1-or-later.
 * MEASURE: TB_SETROWS with TBSTYLE_WRAPABLE and separator groups,
 * comctl32 v5 and v6. For comctl32-toolbar-setrows-wrapable, which Inventor
 * needs for its symbol palette (CCS_VERT | TBSTYLE_WRAPABLE, 47 buttons in
 * 5 separator groups, TB_SETROWS(12)). Own windows, own image list; no
 * vendor files.
 *
 * Per case: TB_SETROWS(rows, larger) on a fresh toolbar, then TB_GETROWS,
 * the RECT TB_SETROWS returned, the indices with TBSTATE_WRAP, and the
 * layout read from TB_GETITEMRECT: buttons per row in order, a separator
 * shown as S (its own row) or s (on a row with buttons). */
#include "../batch1-measure/comctl-support.h"

typedef HIMAGELIST (WINAPI *il_create_fn)(int, int, UINT, int, int);
typedef int (WINAPI *il_add_fn)(HIMAGELIST, HBITMAP, HBITMAP);
typedef BOOL (WINAPI *il_destroy_fn)(HIMAGELIST);

static il_create_fn il_create;
static il_add_fn il_add;
static il_destroy_fn il_destroy;
static HWND parent;

static const struct { const char *name; DWORD style; } styles[] =
{
    { "wrap", TBSTYLE_WRAPABLE },
    { "flatwrap", TBSTYLE_FLAT | TBSTYLE_WRAPABLE },
    { "vertwrap", TBSTYLE_FLAT | TBSTYLE_WRAPABLE | CCS_VERT },
    { "nowrap", TBSTYLE_FLAT },
};

/* group sizes, 0 ends the list; separators go between groups */
static const struct { const char *name; int groups[8]; } shapes[] =
{
    { "g9", { 9, 0 } },
    { "g3-4", { 3, 4, 0 } },
    { "g1-6-2", { 1, 6, 2, 0 } },
    { "g12-10-10-8-7", { 12, 10, 10, 8, 7, 0 } },
};

static const int row_counts[] = { 1, 2, 3, 4, 5, 6, 8, 10, 12, 15, 20 };

static HIMAGELIST make_images(void)
{
    BITMAPINFO bmi;
    HIMAGELIST himl;
    HBITMAP bitmap;
    void *bits;
    HDC hdc;

    memset(&bmi, 0, sizeof(bmi));
    bmi.bmiHeader.biSize = sizeof(bmi.bmiHeader);
    bmi.bmiHeader.biWidth = 64;
    bmi.bmiHeader.biHeight = 16;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 24;
    hdc = GetDC(NULL);
    bitmap = CreateDIBSection(hdc, &bmi, DIB_RGB_COLORS, &bits, NULL, 0);
    ReleaseDC(NULL, hdc);
    if (!bitmap) { b3_error("CreateDIBSection", GetLastError()); return NULL; }
    memset(bits, 0x80, 64 * 16 * 3);
    himl = il_create(16, 16, ILC_COLOR24, 4, 0);
    if (!himl) b3_error("ImageList_Create", GetLastError());
    else if (il_add(himl, bitmap, NULL) != 0) b3_error("ImageList_Add", ERROR_INVALID_DATA);
    DeleteObject(bitmap);
    return himl;
}

static int fill_buttons(TBBUTTON *buttons, int max, const int *groups)
{
    int n = 0, g, i, id = 100;

    for (g = 0; groups[g]; g++)
    {
        if (g && n < max)
        {
            memset(&buttons[n], 0, sizeof(buttons[n]));
            buttons[n].fsStyle = BTNS_SEP;
            buttons[n].idCommand = 0;
            n++;
        }
        for (i = 0; i < groups[g] && n < max; i++, n++)
        {
            memset(&buttons[n], 0, sizeof(buttons[n]));
            buttons[n].iBitmap = n % 4;
            buttons[n].idCommand = id++;
            buttons[n].fsState = TBSTATE_ENABLED;
            buttons[n].fsStyle = BTNS_BUTTON;
            buttons[n].iString = -1;
        }
    }
    return n;
}

static HWND make_toolbar(DWORD style, HIMAGELIST himl, const TBBUTTON *buttons, int count, BOOL zero_padding)
{
    HWND tb = CreateWindowExW(0, TOOLBARCLASSNAMEW, NULL,
                              WS_CHILD | WS_VISIBLE | CCS_NORESIZE | CCS_NOPARENTALIGN | CCS_NODIVIDER | style,
                              0, 0, 300, 400, parent, NULL, GetModuleHandleW(NULL), NULL);
    if (!tb) { b3_error("CreateToolbar", GetLastError()); return NULL; }
    SendMessageW(tb, TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);
    if (zero_padding) SendMessageW(tb, TB_SETPADDING, 0, MAKELPARAM(0, 0));
    SendMessageW(tb, TB_SETIMAGELIST, 0, (LPARAM)himl);
    if (!SendMessageW(tb, TB_ADDBUTTONSW, count, (LPARAM)buttons)) b3_error("TB_ADDBUTTONS", ERROR_INVALID_DATA);
    return tb;
}

/* Rows by item top; a separator alone on its row is S, beside buttons s. */
static void print_layout(HWND tb)
{
    int count = SendMessageW(tb, TB_BUTTONCOUNT, 0, 0), i, in_row = 0, row_sep = 0;
    char wraps[512] = "", layout[512] = "", part[24];
    LONG top = LONG_MIN;
    TBBUTTON button;
    RECT rc;

    for (i = 0; i < count; i++)
    {
        memset(&button, 0, sizeof(button));
        SendMessageW(tb, TB_GETBUTTON, i, (LPARAM)&button);
        if (button.fsState & TBSTATE_WRAP)
        {
            snprintf(part, sizeof(part), "%s%d", wraps[0] ? "," : "", i);
            strncat(wraps, part, sizeof(wraps) - strlen(wraps) - 1);
        }
        if (!SendMessageW(tb, TB_GETITEMRECT, i, (LPARAM)&rc)) { b3_error("TB_GETITEMRECT", ERROR_INVALID_DATA); continue; }
        if (rc.top != top)
        {
            if (top != LONG_MIN)
            {
                if (in_row) snprintf(part, sizeof(part), "%d%s ", in_row, row_sep ? "s" : "");
                else snprintf(part, sizeof(part), "S ");
                strncat(layout, part, sizeof(layout) - strlen(layout) - 1);
            }
            top = rc.top;
            in_row = 0;
            row_sep = 0;
        }
        if (button.fsStyle & BTNS_SEP) row_sep = 1;
        else in_row++;
    }
    if (top != LONG_MIN)
    {
        if (in_row) snprintf(part, sizeof(part), "%d%s", in_row, row_sep ? "s" : "");
        else snprintf(part, sizeof(part), "S");
        strncat(layout, part, sizeof(layout) - strlen(layout) - 1);
    }
    GetClientRect(tb, &rc);
    printf(" getrows=%d client=%ldx%ld wrap=[%s] layout=[%s]\n", (int)SendMessageW(tb, TB_GETROWS, 0, 0),
           rc.right, rc.bottom, wraps, layout);
}

static void run_case(int version, int style, int shape, int rows, BOOL larger, HIMAGELIST himl)
{
    TBBUTTON buttons[64];
    int count = fill_buttons(buttons, 64, shapes[shape].groups);
    HWND tb = make_toolbar(styles[style].style, himl, buttons, count, FALSE);
    RECT rc;

    if (!tb) return;
    SetRect(&rc, -1, -1, -1, -1);
    SendMessageW(tb, TB_SETROWS, MAKEWPARAM(rows, larger), (LPARAM)&rc);
    printf("CASE v=%d style=%s shape=%s rows=%d larger=%d rect=%ld,%ld,%ld,%ld", version, styles[style].name,
           shapes[shape].name, rows, larger, rc.left, rc.top, rc.right, rc.bottom);
    print_layout(tb);
    DestroyWindow(tb);
}

/* Inventor's sequence as far as known: zero padding, TB_SETROWS(12), the
 * toolbar resized to the returned rectangle, buttons replaced one by one,
 * later resizes. */
static void run_palette(int version, BOOL larger, HIMAGELIST himl)
{
    TBBUTTON buttons[64], button;
    int count = fill_buttons(buttons, 64, shapes[3].groups);
    HWND tb = make_toolbar(styles[2].style, himl, buttons, count, TRUE);
    RECT rc;

    if (!tb) return;
    SetRect(&rc, -1, -1, -1, -1);
    SendMessageW(tb, TB_SETROWS, MAKEWPARAM(12, larger), (LPARAM)&rc);
    printf("PALETTE v=%d larger=%d step=setrows rect=%ld,%ld,%ld,%ld", version, larger, rc.left, rc.top, rc.right, rc.bottom);
    print_layout(tb);
    SetWindowPos(tb, NULL, 0, 0, rc.right - rc.left, rc.bottom - rc.top, SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOMOVE);
    printf("PALETTE v=%d larger=%d step=resize-to-rect", version, larger);
    print_layout(tb);
    memset(&button, 0, sizeof(button));
    SendMessageW(tb, TB_GETBUTTON, 5, (LPARAM)&button);
    SendMessageW(tb, TB_DELETEBUTTON, 5, 0);
    SendMessageW(tb, TB_INSERTBUTTONW, 5, (LPARAM)&button);
    printf("PALETTE v=%d larger=%d step=replace-5", version, larger);
    print_layout(tb);
    SetWindowPos(tb, NULL, 0, 0, 500, 300, SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOMOVE);
    printf("PALETTE v=%d larger=%d step=resize-500x300", version, larger);
    print_layout(tb);
    SendMessageW(tb, TB_AUTOSIZE, 0, 0);
    printf("PALETTE v=%d larger=%d step=autosize", version, larger);
    print_layout(tb);
    SetWindowPos(tb, NULL, 0, 0, 60, 600, SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOMOVE);
    printf("PALETTE v=%d larger=%d step=resize-60x600", version, larger);
    print_layout(tb);
    DestroyWindow(tb);
}

static void measure_version(int version)
{
    HIMAGELIST himl;
    struct cc cc;
    unsigned int st, sh, r;
    int larger;

    if (cc_load(&cc, version))
    {
        il_create = (il_create_fn)(void *)cc_proc(&cc, "ImageList_Create");
        il_add = (il_add_fn)(void *)cc_proc(&cc, "ImageList_Add");
        il_destroy = (il_destroy_fn)(void *)cc_proc(&cc, "ImageList_Destroy");
        if (il_create && il_add && il_destroy && (himl = make_images()))
        {
            for (st = 0; st < ARRAYSIZE(styles); st++)
                for (sh = 0; sh < ARRAYSIZE(shapes); sh++)
                    for (r = 0; r < ARRAYSIZE(row_counts); r++)
                        for (larger = 0; larger < 2; larger++)
                            run_case(version, st, sh, row_counts[r], larger, himl);
            for (larger = 0; larger < 2; larger++) run_palette(version, larger, himl);
            il_destroy(himl);
        }
    }
    cc_unload(&cc);
}

int main(void)
{
    if (!b3_start("comctl32-toolbar-setrows", 120000)) return 2;
    parent = CreateWindowExW(0, L"STATIC", L"measure", WS_OVERLAPPEDWINDOW, 0, 0, 640, 700,
                             NULL, NULL, GetModuleHandleW(NULL), NULL);
    if (!parent) { b3_error("CreateParent", GetLastError()); return b3_finish(); }
    printf("SYSTEM cxedge=%d cyedge=%d\n", GetSystemMetrics(SM_CXEDGE), GetSystemMetrics(SM_CYEDGE));
    measure_version(5);
    measure_version(6);
    DestroyWindow(parent);
    return b3_finish();
}
