/* Original, AI-assisted (Claude), LGPL-2.1-or-later.
 * MEASURE (follow-up for toolbar-padding-height v3): TB_SETPADDING with -1
 * after another padding and with -1 in one direction only, and button sizes
 * after TB_SETBITMAPSIZE with a different size, with and without strings,
 * padding set before or after the buttons. comctl32 v5 and v6; own windows,
 * own image list and strings only. */
#include "../batch1-measure/comctl-support.h"

typedef HIMAGELIST (WINAPI *il_create_fn)(int, int, UINT, int, int);
typedef int (WINAPI *il_add_fn)(HIMAGELIST, HBITMAP, HBITMAP);
typedef BOOL (WINAPI *il_destroy_fn)(HIMAGELIST);

static il_create_fn il_create;
static il_add_fn il_add;
static il_destroy_fn il_destroy;
static HWND parent;

static HWND new_toolbar(void)
{
    HWND toolbar = CreateWindowExW(0, TOOLBARCLASSNAMEW, NULL, WS_CHILD | WS_VISIBLE, 0, 0, 0, 0,
                                   parent, (HMENU)1, GetModuleHandleW(NULL), NULL);
    if (!toolbar) b3_error("CreateToolbar", GetLastError());
    else SendMessageW(toolbar, TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);
    return toolbar;
}

static void padding_cases(int version)
{
    static const struct { WORD cx, cy; } first[] = { { 3, 4 }, { 0, 0 }, { 16, 16 } };
    static const struct { WORD cx, cy; } second[] = { { 0xffff, 0xffff }, { 0xffff, 5 }, { 5, 0xffff }, { 0xfffe, 0xfffe } };
    unsigned int f, s;
    HWND toolbar;
    DWORD old, now;

    for (f = 0; f < ARRAYSIZE(first); f++)
        for (s = 0; s < ARRAYSIZE(second); s++)
        {
            if (!(toolbar = new_toolbar())) return;
            SendMessageW(toolbar, TB_SETPADDING, 0, MAKELPARAM(first[f].cx, first[f].cy));
            old = (DWORD)SendMessageW(toolbar, TB_SETPADDING, 0, MAKELPARAM(second[s].cx, second[s].cy));
            now = (DWORD)SendMessageW(toolbar, TB_GETPADDING, 0, 0);
            printf("PAD v=%d first=%u,%u second=%u,%u old=%u,%u now=%u,%u\n", version, first[f].cx, first[f].cy,
                   second[s].cx, second[s].cy, LOWORD(old), HIWORD(old), LOWORD(now), HIWORD(now));
            DestroyWindow(toolbar);
        }
}

static void bitmap_cases(int version, HBITMAP bitmap)
{
    static const struct { WORD cx, cy; } sizes[] = { { 16, 16 }, { 20, 20 }, { 16, 24 }, { 24, 16 }, { 16, 15 } };
    static const struct { int set; WORD cx, cy; } paddings[] = { { 0 }, { 1, 0, 0 }, { 1, 0, 10 }, { 1, 10, 0 } };
    unsigned int z, p, strings, after;
    TBBUTTON buttons[2];
    HIMAGELIST himl;
    HWND toolbar;
    LRESULT str;
    DWORD a, b;
    int i;

    for (strings = 0; strings < 2; strings++)
        for (after = 0; after < 2; after++)
            for (p = 0; p < ARRAYSIZE(paddings); p++)
                for (z = 0; z < ARRAYSIZE(sizes); z++)
                {
                    if (!(toolbar = new_toolbar())) return;
                    if (paddings[p].set && !after)
                        SendMessageW(toolbar, TB_SETPADDING, 0, MAKELPARAM(paddings[p].cx, paddings[p].cy));
                    himl = il_create(16, 16, ILC_COLOR24, 2, 0);
                    if (himl) il_add(himl, bitmap, NULL);
                    SendMessageW(toolbar, TB_SETIMAGELIST, 0, (LPARAM)himl);
                    str = strings ? SendMessageW(toolbar, TB_ADDSTRINGW, 0, (LPARAM)L"Ab\0Cd\0") : -1;
                    memset(buttons, 0, sizeof(buttons));
                    for (i = 0; i < 2; i++)
                    {
                        buttons[i].iBitmap = i;
                        buttons[i].idCommand = 100 + i;
                        buttons[i].fsState = TBSTATE_ENABLED;
                        buttons[i].iString = str >= 0 ? str + i : -1;
                    }
                    SendMessageW(toolbar, TB_ADDBUTTONSW, 2, (LPARAM)buttons);
                    if (paddings[p].set && after)
                        SendMessageW(toolbar, TB_SETPADDING, 0, MAKELPARAM(paddings[p].cx, paddings[p].cy));
                    a = (DWORD)SendMessageW(toolbar, TB_GETBUTTONSIZE, 0, 0);
                    SendMessageW(toolbar, TB_SETBITMAPSIZE, 0, MAKELPARAM(sizes[z].cx, sizes[z].cy));
                    b = (DWORD)SendMessageW(toolbar, TB_GETBUTTONSIZE, 0, 0);
                    printf("BMP v=%d strings=%u order=%s pad=%s%u,%u bitmap=%u,%u before=%u,%u after=%u,%u\n",
                           version, strings, after ? "after" : "before", paddings[p].set ? "" : "default:",
                           paddings[p].cx, paddings[p].cy, sizes[z].cx, sizes[z].cy,
                           LOWORD(a), HIWORD(a), LOWORD(b), HIWORD(b));
                    DestroyWindow(toolbar);
                    if (himl) il_destroy(himl);
                }
}

static void measure_version(int version, HBITMAP bitmap)
{
    struct cc cc;
    if (cc_load(&cc, version))
    {
        il_create = (il_create_fn)(void *)cc_proc(&cc, "ImageList_Create");
        il_add = (il_add_fn)(void *)cc_proc(&cc, "ImageList_Add");
        il_destroy = (il_destroy_fn)(void *)cc_proc(&cc, "ImageList_Destroy");
        padding_cases(version);
        if (il_create && il_add && il_destroy) bitmap_cases(version, bitmap);
    }
    cc_unload(&cc);
}

int main(void)
{
    HBITMAP bitmap;
    HDC hdc;

    if (!b3_start("comctl32-toolbar-padding-followup", 120000)) return 2;
    parent = CreateWindowExW(0, L"STATIC", L"measure", WS_OVERLAPPEDWINDOW, 0, 0, 640, 400,
                             NULL, NULL, GetModuleHandleW(NULL), NULL);
    if (!parent) { b3_error("CreateParent", GetLastError()); return b3_finish(); }
    hdc = GetDC(NULL);
    bitmap = CreateCompatibleBitmap(hdc, 32, 16);
    printf("SYSTEM cxedge=%d dpi_y=%d\n", GetSystemMetrics(SM_CXEDGE), GetDeviceCaps(hdc, LOGPIXELSY));
    ReleaseDC(NULL, hdc);
    measure_version(5, bitmap);
    measure_version(6, bitmap);
    DeleteObject(bitmap);
    DestroyWindow(parent);
    return b3_finish();
}
