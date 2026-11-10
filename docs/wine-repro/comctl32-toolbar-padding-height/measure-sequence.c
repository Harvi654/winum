/* Original, AI-assisted (Claude), LGPL-2.1-or-later.
 * MEASURE: when does a toolbar without strings take its button size from
 * TB_SETPADDING? Inventor's symbol palette (trace, 2026-11-10) does, on a
 * CCS_VERT | TBSTYLE_WRAPABLE toolbar: TB_SETBITMAPSIZE 16x15, then 32x16,
 * TB_SETPADDING(4, 0), TB_SETROWS(12, TRUE), then resizes. The earlier
 * MEASUREs kept 23x22 without strings whatever the padding, but never
 * changed the bitmap size. This program runs that sequence and variants
 * (no bitmap size change, size changed only by the image list, padding
 * before or between the size changes, buttons added late) for comctl32 5
 * and 6, each sequence in a child process with a time limit, and prints
 * the button size, padding, first item and layout after every step.
 * Own windows, own image lists; no vendor files. */
#include "../batch1-measure/comctl-support.h"

typedef HIMAGELIST (WINAPI *il_create_fn)(int, int, UINT, int, int);
typedef int (WINAPI *il_add_fn)(HIMAGELIST, HBITMAP, HBITMAP);
typedef BOOL (WINAPI *il_destroy_fn)(HIMAGELIST);

static il_create_fn il_create;
static il_add_fn il_add;
static il_destroy_fn il_destroy;
static HWND parent;

/* steps: 1 = TB_SETBITMAPSIZE 16x15 (the default size), 2 = TB_SETBITMAPSIZE
 * 32x16, i = TB_SETIMAGELIST 16x16 list, I = TB_SETIMAGELIST 32x16 list,
 * a = TB_ADDBUTTONS (47 in groups 12+10+10+8+7), p = TB_SETPADDING(4, 0),
 * r = TB_SETROWS(12, TRUE), w = width 200, z = TB_AUTOSIZE,
 * b = TB_SETBUTTONSIZE 23x22, B = TB_SETBUTTONSIZE 40x24, S = TB_SETBUTTONSIZE 1x1,
 * m = the 47 buttons one by one, each with its own 32x16 bitmap through
 * TB_ADDBITMAP (separators through TB_ADDBUTTONS only), n = TB_SETINDENT 0.
 * Inventor's complete palette sequence (second trace, 2026-11-10) is
 * "inventor-exact"; the value of its TB_SETINDENT is not in the trace. */
static const struct { const char *name, *steps; } sequences[] =
{
    { "inventor", "12Iaprwz" },
    { "inventor-buttons-late", "12Iparwz" },
    { "padding-first", "p12Iarwz" },
    { "padding-between", "1p2Iarwz" },
    { "no-size-change", "1iaprwz" },
    { "imagelist-only-32", "Iaprwz" },
    { "imagelist-only-16", "iaprwz" },
    { "changed-and-back", "121iaprwz" },
    { "inventor-no-setrows", "12Iapwz" },
    { "inventor-exact", "1b2Bmpnrwz" },
    { "exact-no-padding", "1b2Bmnrwz" },
    { "exact-no-setrows", "1b2Bmpnwz" },
    { "exact-padding-before-buttonsize", "1b2pBmnrwz" },
    { "exact-buttonsize-minimum", "1b2Smpnrwz" },
    { "exact-no-buttonsize", "12mpnrwz" },
};

static const int groups[] = { 12, 10, 10, 8, 7, 0 };

static HIMAGELIST make_images(int cx, int cy)
{
    BITMAPINFO bmi;
    HIMAGELIST himl;
    HBITMAP bitmap;
    void *bits;
    HDC hdc;

    memset(&bmi, 0, sizeof(bmi));
    bmi.bmiHeader.biSize = sizeof(bmi.bmiHeader);
    bmi.bmiHeader.biWidth = cx * 4;
    bmi.bmiHeader.biHeight = cy;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 24;
    hdc = GetDC(NULL);
    bitmap = CreateDIBSection(hdc, &bmi, DIB_RGB_COLORS, &bits, NULL, 0);
    ReleaseDC(NULL, hdc);
    if (!bitmap) { b3_error("CreateDIBSection", GetLastError()); return NULL; }
    memset(bits, 0x80, ((cx * 4 * 3 + 3) & ~3) * cy);
    himl = il_create(cx, cy, ILC_COLOR24, 4, 0);
    if (!himl) b3_error("ImageList_Create", GetLastError());
    else if (il_add(himl, bitmap, NULL) != 0) b3_error("ImageList_Add", ERROR_INVALID_DATA);
    DeleteObject(bitmap);
    return himl;
}

static HBITMAP make_bitmap(int cx, int cy)
{
    BITMAPINFO bmi;
    HBITMAP bitmap;
    void *bits;
    HDC hdc;

    memset(&bmi, 0, sizeof(bmi));
    bmi.bmiHeader.biSize = sizeof(bmi.bmiHeader);
    bmi.bmiHeader.biWidth = cx;
    bmi.bmiHeader.biHeight = cy;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 24;
    hdc = GetDC(NULL);
    bitmap = CreateDIBSection(hdc, &bmi, DIB_RGB_COLORS, &bits, NULL, 0);
    ReleaseDC(NULL, hdc);
    if (!bitmap) b3_error("CreateDIBSection", GetLastError());
    else memset(bits, 0x80, ((cx * 3 + 3) & ~3) * cy);
    return bitmap;
}

/* Inventor's way: one TB_ADDBITMAP (a bitmap handle, hInst NULL) and one
 * TB_ADDBUTTONS per button. */
static void add_one_by_one(HWND tb, TBBUTTON *buttons, int count, HBITMAP *bitmaps)
{
    TBADDBITMAP add;
    TBBUTTON button;
    int i;
    LRESULT index;

    for (i = 0; i < count; i++)
    {
        button = buttons[i];
        if (!(button.fsStyle & BTNS_SEP))
        {
            bitmaps[i] = make_bitmap(32, 16);
            add.hInst = NULL;
            add.nID = (UINT_PTR)bitmaps[i];
            index = SendMessageW(tb, TB_ADDBITMAP, 1, (LPARAM)&add);
            if (index < 0) b3_error("TB_ADDBITMAP", ERROR_INVALID_DATA);
            button.iBitmap = (int)index;
        }
        if (!SendMessageW(tb, TB_ADDBUTTONSW, 1, (LPARAM)&button)) b3_error("TB_ADDBUTTONS_1", ERROR_INVALID_DATA);
    }
}

static int fill_buttons(TBBUTTON *buttons)
{
    int n = 0, g, i;

    for (g = 0; groups[g]; g++)
    {
        if (g)
        {
            memset(&buttons[n], 0, sizeof(buttons[n]));
            buttons[n++].fsStyle = BTNS_SEP;
        }
        for (i = 0; i < groups[g]; i++, n++)
        {
            memset(&buttons[n], 0, sizeof(buttons[n]));
            buttons[n].iBitmap = n % 4;
            buttons[n].idCommand = 100 + n;
            buttons[n].fsState = TBSTATE_ENABLED;
            buttons[n].fsStyle = BTNS_BUTTON;
            buttons[n].iString = -1;
        }
    }
    return n;
}

static void print_state(HWND tb, const char *label, char step)
{
    int count = SendMessageW(tb, TB_BUTTONCOUNT, 0, 0), i, in_row = 0;
    DWORD size = SendMessageW(tb, TB_GETBUTTONSIZE, 0, 0), pad = SendMessageW(tb, TB_GETPADDING, 0, 0);
    char layout[512] = "", part[24];
    LONG top = LONG_MIN;
    TBBUTTON button;
    RECT rc, item0 = { -1, -1, -1, -1 }, client;

    for (i = 0; i < count; i++)
    {
        memset(&button, 0, sizeof(button));
        SendMessageW(tb, TB_GETBUTTON, i, (LPARAM)&button);
        if (!SendMessageW(tb, TB_GETITEMRECT, i, (LPARAM)&rc)) { b3_error("TB_GETITEMRECT", ERROR_INVALID_DATA); continue; }
        if (!i) item0 = rc;
        if (rc.top != top)
        {
            if (top != LONG_MIN)
            {
                if (in_row) snprintf(part, sizeof(part), "%d ", in_row);
                else snprintf(part, sizeof(part), "S ");
                strncat(layout, part, sizeof(layout) - strlen(layout) - 1);
            }
            top = rc.top;
            in_row = 0;
        }
        if (!(button.fsStyle & BTNS_SEP)) in_row++;
    }
    if (top != LONG_MIN)
    {
        if (in_row) snprintf(part, sizeof(part), "%d", in_row);
        else snprintf(part, sizeof(part), "S");
        strncat(layout, part, sizeof(layout) - strlen(layout) - 1);
    }
    GetClientRect(tb, &client);
    printf("STEP %s %c button=%u,%u pad=%u,%u item0=%ld,%ld,%ld,%ld rows=%d client=%ldx%ld layout=[%s]\n",
           label, step, LOWORD(size), HIWORD(size), LOWORD(pad), HIWORD(pad), item0.left, item0.top,
           item0.right, item0.bottom, (int)SendMessageW(tb, TB_GETROWS, 0, 0), client.right, client.bottom, layout);
}

static void run_sequence(int version, unsigned int s)
{
    HIMAGELIST himl16 = make_images(16, 16), himl32 = make_images(32, 16);
    TBBUTTON buttons[64];
    HBITMAP bitmaps[64] = { 0 };
    int count = fill_buttons(buttons), i;
    char label[64];
    const char *step;
    HWND tb;
    RECT rc;

    snprintf(label, sizeof(label), "v=%d seq=%s", version, sequences[s].name);
    tb = CreateWindowExW(0, TOOLBARCLASSNAMEW, NULL, WS_CHILD | WS_VISIBLE | CCS_NORESIZE | CCS_NOPARENTALIGN |
                         CCS_NODIVIDER | TBSTYLE_FLAT | TBSTYLE_WRAPABLE | CCS_VERT,
                         0, 0, 300, 400, parent, NULL, GetModuleHandleW(NULL), NULL);
    if (!tb || !himl16 || !himl32) { b3_error("Setup", GetLastError()); return; }
    SendMessageW(tb, TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);
    print_state(tb, label, '-');
    for (step = sequences[s].steps; *step; step++)
    {
        switch (*step)
        {
        case '1': SendMessageW(tb, TB_SETBITMAPSIZE, 0, MAKELPARAM(16, 15)); break;
        case '2': SendMessageW(tb, TB_SETBITMAPSIZE, 0, MAKELPARAM(32, 16)); break;
        case 'i': SendMessageW(tb, TB_SETIMAGELIST, 0, (LPARAM)himl16); break;
        case 'I': SendMessageW(tb, TB_SETIMAGELIST, 0, (LPARAM)himl32); break;
        case 'a':
            if (!SendMessageW(tb, TB_ADDBUTTONSW, count, (LPARAM)buttons)) b3_error("TB_ADDBUTTONS", ERROR_INVALID_DATA);
            break;
        case 'p': SendMessageW(tb, TB_SETPADDING, 0, MAKELPARAM(4, 0)); break;
        case 'r': SendMessageW(tb, TB_SETROWS, MAKEWPARAM(12, TRUE), (LPARAM)&rc); break;
        case 'w': SetWindowPos(tb, NULL, 0, 0, 200, 600, SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOMOVE); break;
        case 'z': SendMessageW(tb, TB_AUTOSIZE, 0, 0); break;
        case 'b': SendMessageW(tb, TB_SETBUTTONSIZE, 0, MAKELPARAM(23, 22)); break;
        case 'B': SendMessageW(tb, TB_SETBUTTONSIZE, 0, MAKELPARAM(40, 24)); break;
        case 'S': SendMessageW(tb, TB_SETBUTTONSIZE, 0, MAKELPARAM(1, 1)); break;
        case 'm': add_one_by_one(tb, buttons, count, bitmaps); break;
        case 'n': SendMessageW(tb, TB_SETINDENT, 0, 0); break;
        default: continue;
        }
        print_state(tb, label, *step);
    }
    DestroyWindow(tb);
    for (i = 0; i < count; i++) if (bitmaps[i]) DeleteObject(bitmaps[i]);
    il_destroy(himl16);
    il_destroy(himl32);
}

static int child_main(char **argv)
{
    struct cc cc;
    int version = atoi(argv[2]);
    unsigned int s = atoi(argv[3]);

    child_start();
    if (s >= ARRAYSIZE(sequences)) return 2;
    parent = CreateWindowExW(0, L"STATIC", L"measure", WS_OVERLAPPEDWINDOW, 0, 0, 640, 700,
                             NULL, NULL, GetModuleHandleW(NULL), NULL);
    if (!parent) { b3_error("CreateParent", GetLastError()); return 1; }
    if (cc_load(&cc, version))
    {
        il_create = (il_create_fn)(void *)cc_proc(&cc, "ImageList_Create");
        il_add = (il_add_fn)(void *)cc_proc(&cc, "ImageList_Add");
        il_destroy = (il_destroy_fn)(void *)cc_proc(&cc, "ImageList_Destroy");
        if (il_create && il_add && il_destroy) run_sequence(version, s);
    }
    cc_unload(&cc);
    DestroyWindow(parent);
    return b3_errors ? 1 : 0;
}

int main(int argc, char **argv)
{
    WCHAR args[64];
    char label[96];
    unsigned int s;
    int version, failed = 0;
    DWORD code;

    if (argc == 4 && !strcmp(argv[1], "seq")) return child_main(argv);
    if (!b3_start("comctl32-toolbar-padding-sequence", 600000)) return 2;
    printf("SYSTEM cxedge=%d cyedge=%d\n", GetSystemMetrics(SM_CXEDGE), GetSystemMetrics(SM_CYEDGE));
    for (version = 5; version <= 6; version++)
        for (s = 0; s < ARRAYSIZE(sequences); s++)
        {
            swprintf(args, ARRAYSIZE(args), L"seq %d %u", version, s);
            snprintf(label, sizeof(label), "v=%d seq=%s steps=%s", version, sequences[s].name, sequences[s].steps);
            code = run_child(label, args, 10000);
            if (code) failed++;
        }
    /* children that timed out or failed count as errors of the measurement */
    printf("CHILDREN_FAILED %d\n", failed);
    if (failed) b3_error("ChildFailed", failed);
    return b3_finish();
}
