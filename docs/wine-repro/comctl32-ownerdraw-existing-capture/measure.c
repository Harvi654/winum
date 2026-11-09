/* Original, AI-assisted (Claude), LGPL-2.1-or-later.
 * MEASURE: mouse capture of a button that already holds (or does not hold)
 * the capture when it gets WM_LBUTTONDOWN, user32 buttons (v5) and comctl32
 * v6 buttons. Messages are sent directly to own windows; no real input. */
#include "../batch1-measure/comctl-support.h"

static HWND parent, other, button;
static WNDPROC button_proc;
static int capture_in_down, clicked, drawitems;
static const char *stage = "";

static const char *who(HWND hwnd)
{
    if (!hwnd) return "none";
    if (hwnd == button) return "button";
    if (hwnd == parent) return "parent";
    if (hwnd == other) return "other";
    return "foreign";
}

static LRESULT CALLBACK subclass_proc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    LRESULT ret;
    switch (msg)
    {
    case WM_CAPTURECHANGED:
        printf("MSG stage=%s to=button msg=WM_CAPTURECHANGED new=%s capture=%s\n", stage, who((HWND)lp), who(GetCapture()));
        break;
    case WM_LBUTTONDOWN:
        printf("MSG stage=%s to=button msg=WM_LBUTTONDOWN capture=%s\n", stage, who(GetCapture()));
        if (capture_in_down)
        {
            HWND prev = SetCapture(hwnd);
            printf("SUBCLASS_SETCAPTURE prev=%s capture=%s\n", who(prev), who(GetCapture()));
        }
        break;
    case WM_LBUTTONUP:
        printf("MSG stage=%s to=button msg=WM_LBUTTONUP capture=%s\n", stage, who(GetCapture()));
        break;
    case BM_SETSTATE:
        printf("MSG stage=%s to=button msg=BM_SETSTATE wparam=%d\n", stage, (int)wp);
        break;
    }
    ret = CallWindowProcW(button_proc, hwnd, msg, wp, lp);
    if (msg == WM_LBUTTONDOWN || msg == WM_LBUTTONUP)
        printf("RETURNED stage=%s msg=%s capture=%s state=%04lx\n", stage,
               msg == WM_LBUTTONDOWN ? "WM_LBUTTONDOWN" : "WM_LBUTTONUP", who(GetCapture()),
               (DWORD)SendMessageW(hwnd, BM_GETSTATE, 0, 0));
    return ret;
}

static LRESULT CALLBACK parent_proc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    if (msg == WM_COMMAND && (HWND)lp == button)
    {
        printf("MSG stage=%s to=parent msg=WM_COMMAND code=%u\n", stage, HIWORD(wp));
        if (HIWORD(wp) == BN_CLICKED) clicked++;
    }
    if (msg == WM_DRAWITEM)
    {
        const DRAWITEMSTRUCT *dis = (const DRAWITEMSTRUCT *)lp;
        drawitems++;
        printf("MSG stage=%s to=parent msg=WM_DRAWITEM action=%04x selected=%d\n", stage,
               dis->itemAction, !!(dis->itemState & ODS_SELECTED));
        return TRUE;
    }
    if (msg == WM_CAPTURECHANGED)
        printf("MSG stage=%s to=%s msg=WM_CAPTURECHANGED new=%s\n", stage, who(hwnd), who((HWND)lp));
    return DefWindowProcW(hwnd, msg, wp, lp);
}

static const struct { const char *name; DWORD style; } types[] =
{
    { "ownerdraw", BS_OWNERDRAW },
    { "pushbutton", BS_PUSHBUTTON },
    { "autocheckbox", BS_AUTOCHECKBOX },
};
/* none: nobody holds the capture; self_down: the subclass calls SetCapture
 * in WM_LBUTTONDOWN before the button; self_before: the button holds the
 * capture before the message; other: another window of ours holds it */
static const char *precaptures[] = { "none", "self_down", "self_before", "other" };

static void run_case(int version, unsigned int t, unsigned int p)
{
    char label[96];
    HWND prev;

    snprintf(label, sizeof(label), "v=%d type=%s pre=%s", version, types[t].name, precaptures[p]);
    button = CreateWindowExW(0, WC_BUTTONW, L"Ab", WS_CHILD | WS_VISIBLE | types[t].style,
                             10, 10, 60, 24, parent, (HMENU)7, GetModuleHandleW(NULL), NULL);
    if (!button) { b3_error("CreateButton", GetLastError()); return; }
    button_proc = (WNDPROC)SetWindowLongPtrW(button, GWLP_WNDPROC, (LONG_PTR)subclass_proc);
    UpdateWindow(parent);
    clicked = drawitems = 0;
    capture_in_down = (p == 1);
    ReleaseCapture();
    stage = "setup";
    printf("CASE %s foreground=%d\n", label, GetForegroundWindow() == parent);
    if (p == 2)
    {
        prev = SetCapture(button);
        printf("PRE_SETCAPTURE target=button prev=%s capture=%s\n", who(prev), who(GetCapture()));
    }
    if (p == 3)
    {
        prev = SetCapture(other);
        printf("PRE_SETCAPTURE target=other prev=%s capture=%s\n", who(prev), who(GetCapture()));
    }
    stage = "down";
    SendMessageW(button, WM_LBUTTONDOWN, MK_LBUTTON, MAKELPARAM(5, 5));
    printf("AFTER_DOWN %s capture=%s state=%04lx\n", label, who(GetCapture()),
           (DWORD)SendMessageW(button, BM_GETSTATE, 0, 0));
    stage = "up";
    SendMessageW(button, WM_LBUTTONUP, 0, MAKELPARAM(5, 5));
    printf("AFTER_UP %s capture=%s state=%04lx clicked=%d drawitems=%d\n", label, who(GetCapture()),
           (DWORD)SendMessageW(button, BM_GETSTATE, 0, 0), clicked, drawitems);
    stage = "cleanup";
    ReleaseCapture();
    DestroyWindow(button);
    button = NULL;
}

/* neighbour: SetCapture on a plain window that already holds the capture */
static void plain_repeat(void)
{
    HWND prev;
    ReleaseCapture();
    stage = "plain";
    prev = SetCapture(other);
    printf("PLAIN first prev=%s capture=%s\n", who(prev), who(GetCapture()));
    prev = SetCapture(other);
    printf("PLAIN repeat prev=%s capture=%s\n", who(prev), who(GetCapture()));
    prev = SetCapture(parent);
    printf("PLAIN switch prev=%s capture=%s\n", who(prev), who(GetCapture()));
    ReleaseCapture();
    printf("PLAIN released capture=%s\n", who(GetCapture()));
}

static void measure_version(int version)
{
    unsigned int t, p;
    struct cc cc;
    if (cc_load(&cc, version))
    {
        plain_repeat();
        for (t = 0; t < ARRAYSIZE(types); t++)
            for (p = 0; p < ARRAYSIZE(precaptures); p++)
                run_case(version, t, p);
    }
    cc_unload(&cc);
}

int main(void)
{
    WNDCLASSW wc;

    if (!b3_start("comctl32-ownerdraw-existing-capture", 60000)) return 2;
    memset(&wc, 0, sizeof(wc));
    wc.lpfnWndProc = parent_proc;
    wc.hInstance = GetModuleHandleW(NULL);
    wc.hCursor = LoadCursorW(NULL, (const WCHAR *)IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = L"WinumCaptureParent";
    if (!RegisterClassW(&wc)) { b3_error("RegisterClassW", GetLastError()); return b3_finish(); }
    parent = CreateWindowExW(0, wc.lpszClassName, L"measure", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                             50, 50, 300, 200, NULL, NULL, wc.hInstance, NULL);
    other = CreateWindowExW(0, wc.lpszClassName, L"other", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                            400, 50, 200, 150, NULL, NULL, wc.hInstance, NULL);
    if (!parent || !other) { b3_error("CreateWindow", GetLastError()); return b3_finish(); }
    SetForegroundWindow(parent);
    UpdateWindow(parent);
    measure_version(5);
    measure_version(6);
    DestroyWindow(other);
    DestroyWindow(parent);
    return b3_finish();
}
