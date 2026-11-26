/* Original, AI-assisted (Claude), LGPL-2.1-or-later.
 * MEASURE: EndDialog on windows that are not dialogs.
 *
 * Inventor 2027 calls EndDialog(<its MDI client>, IDCANCEL) after a standard
 * is picked in the Content Center filter; Wine returns FALSE at once
 * ("got invalid window handle") and an MFC C++ exception ends the process
 * soon after. This records, for an MDI client, ordinary child and top-level
 * windows, an owned popup with a disabled owner, a real modeless dialog,
 * a destroyed window and NULL: EndDialog's return value and last error, and
 * the window's visibility, enabled state, focus and its owner's enabled
 * state before and after. One tab-separated line per case. */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <string.h>

static HINSTANCE inst;

static void pump(void)
{
    MSG msg;
    while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE))
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
}

static LRESULT CALLBACK frame_proc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    return DefFrameProcW(hwnd, NULL, msg, wp, lp);
}

static LRESULT CALLBACK plain_proc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    return DefWindowProcW(hwnd, msg, wp, lp);
}

static INT_PTR CALLBACK dlg_proc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    return msg == WM_INITDIALOG;
}

static void report(const char *name, HWND hwnd, HWND owner, BOOL focus_inside)
{
    BOOL vis0 = hwnd ? IsWindowVisible(hwnd) : -1, en0 = hwnd ? IsWindowEnabled(hwnd) : -1;
    BOOL oen0 = owner ? IsWindowEnabled(owner) : -1, exists0 = hwnd ? IsWindow(hwnd) : -1;
    BOOL ret, vis1, en1, oen1, exists1;
    HWND focus0, focus1, active1;
    DWORD err;

    if (focus_inside && hwnd && IsWindow(hwnd)) SetFocus(hwnd);
    pump();
    focus0 = GetFocus();
    SetLastError(0xdeadbeef);
    ret = EndDialog(hwnd, 2);
    err = GetLastError();
    pump();
    vis1 = hwnd ? IsWindowVisible(hwnd) : -1;
    en1 = hwnd ? IsWindowEnabled(hwnd) : -1;
    oen1 = owner ? IsWindowEnabled(owner) : -1;
    exists1 = hwnd ? IsWindow(hwnd) : -1;
    focus1 = GetFocus();
    active1 = GetActiveWindow();
    printf("case\t%s\tret=%d\terr=%lu\texists=%d->%d\tvisible=%d->%d\tenabled=%d->%d\towner_enabled=%d->%d\t"
           "focus_was_window=%d\tfocus_is_window=%d\tactive_is_window=%d\n",
           name, ret, err, exists0, exists1, vis0, vis1, en0, en1, oen0, oen1,
           focus0 == hwnd && hwnd, focus1 == hwnd && hwnd, active1 == hwnd && hwnd);
    fflush(stdout);
}

int main(void)
{
    static const WORD dlg_template[] __attribute__((aligned(4))) =
    {
        /* DLGTEMPLATE: style (2 words), exstyle (2), cdit, x, y, cx, cy;
         * then no menu, the default class, no title */
        LOWORD(WS_POPUP | WS_CAPTION | WS_VISIBLE), HIWORD(WS_POPUP | WS_CAPTION | WS_VISIBLE),
        0, 0, 0, 10, 10, 100, 50, 0, 0, 0
    };
    WNDCLASSW wc;
    CLIENTCREATESTRUCT ccs;
    HWND frame, mdi, child, top, owner, popup, dlg, dead;

    inst = GetModuleHandleW(NULL);
    memset(&wc, 0, sizeof(wc));
    wc.hInstance = inst;
    wc.hbrBackground = (HBRUSH)(COLOR_APPWORKSPACE + 1);
    wc.hCursor = LoadCursorW(NULL, (LPCWSTR)IDC_ARROW);
    wc.lpfnWndProc = frame_proc;
    wc.lpszClassName = L"winum_enddialog_frame";
    RegisterClassW(&wc);
    wc.lpfnWndProc = plain_proc;
    wc.lpszClassName = L"winum_enddialog_plain";
    RegisterClassW(&wc);

    frame = CreateWindowExW(0, L"winum_enddialog_frame", L"winum frame", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                            0, 0, 600, 400, NULL, NULL, inst, NULL);
    memset(&ccs, 0, sizeof(ccs));
    ccs.idFirstChild = 100;
    mdi = CreateWindowExW(WS_EX_CLIENTEDGE, L"MDICLIENT", NULL, WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN,
                          0, 0, 580, 350, frame, (HMENU)1, inst, &ccs);
    pump();
    printf("setup\tframe=%d\tmdi=%d\n", frame != NULL, mdi != NULL);

    report("mdi_client", mdi, frame, FALSE);
    report("mdi_client_focus", mdi, frame, TRUE);

    child = CreateWindowExW(0, L"winum_enddialog_plain", NULL, WS_CHILD | WS_VISIBLE, 10, 10, 50, 50,
                            frame, (HMENU)2, inst, NULL);
    report("plain_child", child, frame, FALSE);

    top = CreateWindowExW(0, L"winum_enddialog_plain", L"top", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                          100, 100, 200, 150, NULL, NULL, inst, NULL);
    pump();
    report("plain_toplevel", top, NULL, FALSE);

    owner = CreateWindowExW(0, L"winum_enddialog_plain", L"owner", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                            150, 150, 200, 150, NULL, NULL, inst, NULL);
    popup = CreateWindowExW(0, L"winum_enddialog_plain", L"popup", WS_POPUP | WS_CAPTION | WS_VISIBLE,
                            200, 200, 150, 100, owner, NULL, inst, NULL);
    EnableWindow(owner, FALSE);
    pump();
    report("owned_popup_owner_disabled", popup, owner, FALSE);

    EnableWindow(owner, FALSE);
    dlg = CreateDialogIndirectParamW(inst, (const DLGTEMPLATE *)dlg_template, owner, dlg_proc, 0);
    pump();
    report("modeless_dialog_owner_disabled", dlg, owner, FALSE);

    dead = CreateWindowExW(0, L"winum_enddialog_plain", NULL, WS_OVERLAPPED, 0, 0, 10, 10, NULL, NULL, inst, NULL);
    DestroyWindow(dead);
    report("destroyed_window", dead, NULL, FALSE);
    report("null", NULL, NULL, FALSE);

    /* After EndDialog on the MDI client: can it still get MDI children? */
    {
        MDICREATESTRUCTW mcs;
        HWND doc;

        memset(&mcs, 0, sizeof(mcs));
        mcs.szClass = L"winum_enddialog_plain";
        mcs.szTitle = L"doc";
        mcs.hOwner = inst;
        mcs.x = mcs.y = mcs.cx = mcs.cy = CW_USEDEFAULT;
        doc = (HWND)SendMessageW(mdi, WM_MDICREATE, 0, (LPARAM)&mcs);
        printf("after\tmdi_visible=%d\tmdi_child_created=%d\n", IsWindowVisible(mdi), doc != NULL);
    }

    DestroyWindow(dlg);
    DestroyWindow(popup);
    DestroyWindow(owner);
    DestroyWindow(top);
    DestroyWindow(frame);
    printf("measure_complete\n");
    return 0;
}
