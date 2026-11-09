/* Original, AI-assisted (Claude), LGPL-2.1-or-later.
 * MEASURE: HDM_LAYOUT with a NULL HDLAYOUT and with NULL members, comctl32
 * v5 and v6. Each case runs in its own child process, so a crash is an
 * exit code. Own windows only. */
#include "../batch1-measure/comctl-support.h"

static const char *layouts[] = { "valid", "null_layout", "null_prc", "null_pwpos", "null_both" };
static const struct { const char *name; DWORD style; } styles[] =
{
    { "buttons", HDS_BUTTONS },
    { "hidden", HDS_BUTTONS | HDS_HIDDEN },
};

static int child(int version, unsigned int s, unsigned int l, int unicode)
{
    HDLAYOUT layout, *param = &layout;
    WINDOWPOS pos;
    RECT rc;
    HWND parent, header;
    struct cc cc;
    LRESULT ret;
    HDITEMW item;

    child_start();
    if (!cc_load(&cc, version)) return 4;
    parent = CreateWindowExW(0, L"STATIC", L"measure", WS_OVERLAPPEDWINDOW, 0, 0, 400, 300,
                             NULL, NULL, GetModuleHandleW(NULL), NULL);
    header = CreateWindowExW(0, WC_HEADERW, NULL, WS_CHILD | WS_VISIBLE | styles[s].style,
                             0, 0, 300, 30, parent, (HMENU)1, GetModuleHandleW(NULL), NULL);
    if (!parent || !header) { printf("ERROR operation=CreateWindow code=%08lx\n", GetLastError()); return 5; }
    memset(&item, 0, sizeof(item));
    item.mask = HDI_TEXT | HDI_WIDTH;
    item.cxy = 80;
    item.pszText = (WCHAR *)L"Ab";
    SendMessageW(header, HDM_INSERTITEMW, 0, (LPARAM)&item);

    SetRect(&rc, 5, 7, 205, 107);
    memset(&pos, 0xcc, sizeof(pos));
    layout.prc = (l == 2 || l == 4) ? NULL : &rc;
    layout.pwpos = (l == 3 || l == 4) ? NULL : &pos;
    if (l == 1) param = NULL;

    SetLastError(0xdeadbeef);
    printf("CALL v=%d style=%s layout=%s unicode=%d\n", version, styles[s].name, layouts[l], unicode);
    ret = unicode ? SendMessageW(header, HDM_LAYOUT, 0, (LPARAM)param)
                  : SendMessageA(header, HDM_LAYOUT, 0, (LPARAM)param);
    printf("RETURN ret=%ld error=%08lx\n", (long)ret, GetLastError());
    printf("STATE rc=%ld,%ld,%ld,%ld pos_after=%p,x=%d,y=%d,cx=%d,cy=%d,flags=%08x\n",
           rc.left, rc.top, rc.right, rc.bottom, (void *)pos.hwndInsertAfter,
           pos.x, pos.y, pos.cx, pos.cy, pos.flags);
    /* the control still works afterwards */
    printf("AFTER count=%ld\n", (long)SendMessageW(header, HDM_GETITEMCOUNT, 0, 0));
    DestroyWindow(header);
    DestroyWindow(parent);
    cc_unload(&cc);
    return 0;
}

int wmain(int argc, WCHAR **argv)
{
    unsigned int s, l;
    int version, unicode;
    WCHAR args[64];
    char label[96];

    if (argc == 5)
        return child(_wtoi(argv[1]), _wtoi(argv[2]), _wtoi(argv[3]), _wtoi(argv[4]));

    if (!b3_start("comctl32-header-null-layout", 180000)) return 2;
    for (version = 5; version <= 6; version++)
        for (s = 0; s < ARRAYSIZE(styles); s++)
            for (l = 0; l < ARRAYSIZE(layouts); l++)
                for (unicode = 0; unicode < 2; unicode++)
                {
                    swprintf(args, ARRAYSIZE(args), L"%d %u %u %d", version, s, l, unicode);
                    snprintf(label, sizeof(label), "v=%d style=%s layout=%s unicode=%d",
                             version, styles[s].name, layouts[l], unicode);
                    run_child(label, args, 20000);
                }
    return b3_finish();
}
