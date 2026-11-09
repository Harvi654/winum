/* Original, AI-assisted (Claude), LGPL-2.1-or-later.
 * MEASURE: toolbar button sizes after TB_SETPADDING, comctl32 v5 and v6.
 * Own windows, own image list and strings only; no vendor files. */
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
    { "plain", 0 },
    { "flat", TBSTYLE_FLAT },
    { "list", TBSTYLE_LIST },
    { "flatlist", TBSTYLE_FLAT | TBSTYLE_LIST },
    { "vertwrap", TBSTYLE_FLAT | TBSTYLE_WRAPABLE | CCS_VERT },
};

/* img: 16x16 image list, no text; imgtext: image list and text;
 * text: no image list, I_IMAGENONE and text; none: no image list, no text */
static const char *contents[] = { "img", "imgtext", "text", "none" };

static const struct { int set; WORD cx, cy; } paddings[] =
{
    { 0, 0, 0 }, { 1, 0, 0 }, { 1, 1, 1 }, { 1, 2, 2 }, { 1, 3, 3 }, { 1, 4, 4 },
    { 1, 7, 6 }, { 1, 8, 8 }, { 1, 10, 10 }, { 1, 0, 10 }, { 1, 10, 0 },
    { 1, 16, 16 }, { 1, 0xffff, 0xffff },
};

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

static void observe(HWND toolbar, const char *tag, const char *step)
{
    DWORD pad = (DWORD)SendMessageW(toolbar, TB_GETPADDING, 0, 0);
    DWORD size = (DWORD)SendMessageW(toolbar, TB_GETBUTTONSIZE, 0, 0);
    RECT r0 = { -1, -1, -1, -1 }, r1 = { -1, -1, -1, -1 }, wr;
    TBMETRICS metrics;
    BOOL ok0, ok1;

    ok0 = (BOOL)SendMessageW(toolbar, TB_GETITEMRECT, 0, (LPARAM)&r0);
    ok1 = (BOOL)SendMessageW(toolbar, TB_GETITEMRECT, 1, (LPARAM)&r1);
    GetWindowRect(toolbar, &wr);
    memset(&metrics, 0, sizeof(metrics));
    metrics.cbSize = sizeof(metrics);
    metrics.dwMask = TBMF_PAD | TBMF_BARPAD | TBMF_BUTTONSPACING;
    SendMessageW(toolbar, TB_GETMETRICS, 0, (LPARAM)&metrics);
    printf("SIZE %s step=%s pad=%u,%u button=%u,%u item0=%d:%ld,%ld,%ld,%ld item1=%d:%ld,%ld "
           "window=%ld,%ld metrics=%d,%d,%d,%d,%d,%d\n",
           tag, step, LOWORD(pad), HIWORD(pad), LOWORD(size), HIWORD(size),
           ok0, r0.left, r0.top, r0.right, r0.bottom, ok1, r1.left, r1.top,
           wr.right - wr.left, wr.bottom - wr.top, metrics.cxPad, metrics.cyPad,
           metrics.cxBarPad, metrics.cyBarPad, metrics.cxButtonSpacing, metrics.cyButtonSpacing);
}

static void set_padding(HWND toolbar, const char *tag, int p)
{
    DWORD old;
    if (!paddings[p].set) return;
    old = (DWORD)SendMessageW(toolbar, TB_SETPADDING, 0, MAKELPARAM(paddings[p].cx, paddings[p].cy));
    printf("SETPAD %s old=%u,%u\n", tag, LOWORD(old), HIWORD(old));
}

static void run_case(const struct cc *cc, int s, int c, int p, int after)
{
    TBBUTTON buttons[4];
    HIMAGELIST himl = NULL;
    HWND toolbar;
    char tag[16];
    LRESULT first = -1;
    int i;

    static int id;
    char setpad[16] = "default";

    if (paddings[p].set) snprintf(setpad, sizeof(setpad), "%u,%u", paddings[p].cx, paddings[p].cy);
    snprintf(tag, sizeof(tag), "id=%d", ++id);
    toolbar = CreateWindowExW(0, TOOLBARCLASSNAMEW, NULL, WS_CHILD | WS_VISIBLE | styles[s].style,
                              0, 0, 0, 0, parent, (HMENU)1, GetModuleHandleW(NULL), NULL);
    if (!toolbar) { b3_error("CreateToolbar", GetLastError()); return; }
    SendMessageW(toolbar, TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);
    printf("CASE %s v=%d style=%s content=%s order=%s setpad=%s ccm_version=%ld class_module=%d\n", tag,
           cc->version, styles[s].name, contents[c], after ? "after" : "before", setpad,
           (long)SendMessageW(toolbar, CCM_GETVERSION, 0, 0),
           (HMODULE)GetClassLongPtrW(toolbar, GCLP_HMODULE) == cc->module);
    observe(toolbar, tag, "created");
    if (!after) { set_padding(toolbar, tag, p); observe(toolbar, tag, "padded"); }

    if (c == 0 || c == 1)
    {
        himl = make_images();
        SendMessageW(toolbar, TB_SETIMAGELIST, 0, (LPARAM)himl);
    }
    if (c == 1 || c == 2)
        first = SendMessageW(toolbar, TB_ADDSTRINGW, 0, (LPARAM)L"Ab\0Cd\0Ef\0Gh\0");
    memset(buttons, 0, sizeof(buttons));
    for (i = 0; i < 4; i++)
    {
        buttons[i].iBitmap = (c == 2) ? I_IMAGENONE : i;
        buttons[i].idCommand = 100 + i;
        buttons[i].fsState = TBSTATE_ENABLED;
        buttons[i].fsStyle = BTNS_BUTTON;
        buttons[i].iString = (first >= 0) ? first + i : -1;
    }
    if (!SendMessageW(toolbar, TB_ADDBUTTONSW, 4, (LPARAM)buttons)) b3_error("TB_ADDBUTTONSW", GetLastError());
    observe(toolbar, tag, "buttons");
    SendMessageW(toolbar, TB_AUTOSIZE, 0, 0);
    observe(toolbar, tag, "autosize");
    if (after)
    {
        set_padding(toolbar, tag, p);
        observe(toolbar, tag, "padded");
        SendMessageW(toolbar, TB_AUTOSIZE, 0, 0);
        observe(toolbar, tag, "padded_autosize");
    }
    /* recalculations Wine does in TOOLBAR_CalcToolbar */
    SendMessageW(toolbar, TB_SETBITMAPSIZE, 0, MAKELPARAM(16, 16));
    observe(toolbar, tag, "setbitmapsize");
    SendMessageW(toolbar, TB_SETBUTTONSIZE, 0, MAKELPARAM(1, 1));
    observe(toolbar, tag, "setbuttonsize1");
    SendMessageW(toolbar, TB_ADDBUTTONSW, 1, (LPARAM)&buttons[0]);
    observe(toolbar, tag, "addbutton");
    DestroyWindow(toolbar);
    if (himl) il_destroy(himl);
}

static void run_all(const struct cc *cc)
{
    unsigned int s, c, p;
    int after;
    for (s = 0; s < ARRAYSIZE(styles); s++)
        for (c = 0; c < ARRAYSIZE(contents); c++)
            for (p = 0; p < ARRAYSIZE(paddings); p++)
                for (after = 0; after < 2; after++)
                    run_case(cc, s, c, p, after);
}

static void measure_version(int version)
{
    struct cc cc;
    if (cc_load(&cc, version))
    {
        il_create = (il_create_fn)(void *)cc_proc(&cc, "ImageList_Create");
        il_add = (il_add_fn)(void *)cc_proc(&cc, "ImageList_Add");
        il_destroy = (il_destroy_fn)(void *)cc_proc(&cc, "ImageList_Destroy");
        if (il_create && il_add && il_destroy) run_all(&cc);
    }
    cc_unload(&cc);
}

int main(void)
{
    TEXTMETRICW tm;
    HFONT font;
    HDC hdc;

    if (!b3_start("comctl32-toolbar-padding-height", 120000)) return 2;
    parent = CreateWindowExW(0, L"STATIC", L"measure", WS_OVERLAPPEDWINDOW, 0, 0, 640, 400,
                             NULL, NULL, GetModuleHandleW(NULL), NULL);
    if (!parent) { b3_error("CreateParent", GetLastError()); return b3_finish(); }
    hdc = GetDC(parent);
    font = GetStockObject(DEFAULT_GUI_FONT);
    SelectObject(hdc, font);
    GetTextMetricsW(hdc, &tm);
    printf("SYSTEM cxedge=%d cyedge=%d dpi_y=%d gui_font_height=%ld\n", GetSystemMetrics(SM_CXEDGE),
           GetSystemMetrics(SM_CYEDGE), GetDeviceCaps(hdc, LOGPIXELSY), tm.tmHeight);
    ReleaseDC(parent, hdc);

    measure_version(5);
    measure_version(6);
    DestroyWindow(parent);
    return b3_finish();
}
