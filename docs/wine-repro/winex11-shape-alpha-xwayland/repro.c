/* Original, AI-assisted (Codex), LGPL-2.1-or-later.
 * Own hidden-first shaped dialog and ordinary-class control for Xwayland.
 */
#include <windows.h>
#include <stdio.h>
#include <wchar.h>

static HWND popup;
static unsigned int phase;
static BOOL ordinary, late_region, failed;

static DWORD WINAPI watchdog(void *done)
{
    if (WaitForSingleObject(done, 40000) == WAIT_OBJECT_0) return 0;
    puts("INCONCLUSIVE watchdog_timeout=1");
    TerminateProcess(GetCurrentProcess(), 2);
    return 2;
}

static void paint(HWND window, COLORREF color)
{
    PAINTSTRUCT ps;
    HDC dc = BeginPaint(window, &ps);
    HBRUSH brush = CreateSolidBrush(color);
    RECT rect;
    GetClientRect(window, &rect);
    if (dc && brush) FillRect(dc, &rect, brush);
    else failed = TRUE;
    if (brush) DeleteObject(brush);
    EndPaint(window, &ps);
}

static BOOL set_shape(unsigned int step)
{
    HRGN region = NULL, extra;
    int ret;
    if (step != 2)
    {
        region = CreateRectRgn(0, 0, 160, 80);
        extra = CreateRectRgn(step == 1 ? 80 : 0, 80, step == 1 ? 160 : 80, 160);
        if (!region || !extra)
        {
            if (region) DeleteObject(region);
            if (extra) DeleteObject(extra);
            return FALSE;
        }
        ret = CombineRgn(region, region, extra, RGN_OR);
        DeleteObject(extra);
        if (ret == ERROR) { DeleteObject(region); return FALSE; }
    }
    ret = SetWindowRgn(popup, region, TRUE);
    if (!ret && region) DeleteObject(region);
    printf("SHAPE phase=%u form=%s visible=%d ret=%d error=%lu\n", step,
           step == 2 ? "rectangle" : step == 1 ? "missing-bottom-left" : "missing-bottom-right",
           IsWindowVisible(popup), ret, ret ? 0 : GetLastError());
    if (ret) { InvalidateRect(popup, NULL, TRUE); UpdateWindow(popup); }
    return ret != 0;
}

static INT_PTR CALLBACK dialog_proc(HWND window, UINT message, WPARAM wp, LPARAM lp)
{
    (void)wp; (void)lp;
    if (message == WM_INITDIALOG) return FALSE;
    if (message == WM_ERASEBKGND) return TRUE;
    if (message == WM_PAINT) { paint(window, RGB(210, 45, 70)); return TRUE; }
    if (message == WM_LBUTTONDOWN) { printf("CLICK target=popup phase=%u\n", phase); return TRUE; }
    return FALSE;
}

static LRESULT CALLBACK ordinary_proc(HWND window, UINT message, WPARAM wp, LPARAM lp)
{
    if (message == WM_ERASEBKGND) return TRUE;
    if (message == WM_PAINT) { paint(window, RGB(210, 45, 70)); return 0; }
    if (message == WM_LBUTTONDOWN) { printf("CLICK target=popup phase=%u\n", phase); return 0; }
    return DefWindowProcW(window, message, wp, lp);
}

static LRESULT CALLBACK host_proc(HWND window, UINT message, WPARAM wp, LPARAM lp)
{
    if (message == WM_PAINT) { paint(window, RGB(30, 180, 90)); return 0; }
    if (message == WM_LBUTTONDOWN) { printf("CLICK target=owner phase=%u\n", phase); return 0; }
    if (message == WM_TIMER)
    {
        if (++phase == 4) { DestroyWindow(window); return 0; }
        if (!set_shape(phase)) { failed = TRUE; DestroyWindow(window); }
        return 0;
    }
    if (message == WM_CLOSE) { failed = phase < 4; DestroyWindow(window); return 0; }
    if (message == WM_DESTROY) { PostQuitMessage(0); return 0; }
    return DefWindowProcW(window, message, wp, lp);
}

int wmain(int argc, WCHAR **argv)
{
    struct { DLGTEMPLATE dialog; WORD menu, cls, title; } data;
    WNDCLASSW cls;
    POINT position = { 50, 50 };
    WCHAR actual_class[32] = {0};
    HANDLE done, watcher;
    DWORD style, exstyle;
    RECT bounds;
    HWND owner;
    int i, ret;
    MSG message;
    setvbuf(stdout, NULL, _IONBF, 0);
    for (i = 1; i < argc; ++i)
    {
        if (!wcscmp(argv[i], L"--ordinary")) ordinary = TRUE;
        else if (!wcscmp(argv[i], L"--late-region")) late_region = TRUE;
        else return 2;
    }
    done = CreateEventW(NULL, TRUE, FALSE, NULL);
    if (!done) return 2;
    watcher = CreateThread(NULL, 0, watchdog, done, 0, NULL);
    if (!watcher) { CloseHandle(done); return 2; }
    memset(&cls, 0, sizeof(cls));
    cls.lpfnWndProc = host_proc;
    cls.hInstance = GetModuleHandleW(NULL);
    cls.hCursor = LoadCursorW(NULL, (LPCWSTR)IDC_ARROW);
    cls.lpszClassName = L"WinumShapeOwner";
    if (!RegisterClassW(&cls)) return 2;
    cls.lpfnWndProc = ordinary_proc;
    cls.lpszClassName = L"WinumOrdinaryShape";
    if (!RegisterClassW(&cls)) return 2;
    owner = CreateWindowExW(0, L"WinumShapeOwner", L"Shape L3: green owner, red shaped popup",
            WS_OVERLAPPEDWINDOW | WS_VISIBLE, 60, 60, 360, 310,
            NULL, NULL, cls.hInstance, NULL);
    if (!owner || !ClientToScreen(owner, &position)) return 2;
    if (ordinary)
        popup = CreateWindowExW(WS_EX_TOOLWINDOW, L"WinumOrdinaryShape", L"Winum own ordinary popup",
                WS_POPUP, position.x, position.y, 160, 160, owner, NULL, cls.hInstance, NULL);
    else
    {
        memset(&data, 0, sizeof(data));
        data.dialog.style = WS_POPUP;
        data.dialog.dwExtendedStyle = WS_EX_TOOLWINDOW;
        data.dialog.cx = data.dialog.cy = 80;
        popup = CreateDialogIndirectParamW(cls.hInstance, &data.dialog, owner, dialog_proc, 0);
        if (popup)
        {
            SetWindowTextW(popup, L"Winum own shaped dialog");
            if (!SetWindowPos(popup, NULL, position.x, position.y, 160, 160, SWP_NOZORDER | SWP_NOACTIVATE))
            { DestroyWindow(owner); return 2; }
        }
    }
    if (!popup) { DestroyWindow(owner); return 2; }
    style = GetWindowLongW(popup, GWL_STYLE);
    exstyle = GetWindowLongW(popup, GWL_EXSTYLE);
    if (!GetClassNameW(popup, actual_class, ARRAYSIZE(actual_class)) ||
        wcscmp(actual_class, ordinary ? L"WinumOrdinaryShape" : L"#32770") ||
        GetWindow(popup, GW_OWNER) != owner || !(style & WS_POPUP) ||
        (style & WS_CAPTION) == WS_CAPTION || (style & WS_VISIBLE) ||
        (exstyle & (WS_EX_LAYERED | WS_EX_NOREDIRECTIONBITMAP)) ||
        !GetWindowRect(popup, &bounds) || bounds.right - bounds.left != 160 ||
        bounds.bottom - bounds.top != 160)
    { puts("INCONCLUSIVE target_preconditions=0"); DestroyWindow(owner); return 2; }
    printf("REPRO topic=winex11-shape-alpha-xwayland bits=%u ordinary=%d late_region=%d "
           "dialog_class=%d owner_ok=%d style=%08lx exstyle=%08lx\n",
           (unsigned int)(8 * sizeof(void *)), ordinary, late_region,
           !wcscmp(actual_class, L"#32770"), GetWindow(popup, GW_OWNER) == owner,
           (DWORD)GetWindowLongW(popup, GWL_STYLE), (DWORD)GetWindowLongW(popup, GWL_EXSTYLE));
    if (late_region)
    {
        ShowWindow(popup, SW_SHOWNOACTIVATE);
        UpdateWindow(popup);
        if (!IsWindowVisible(popup)) { DestroyWindow(owner); return 2; }
    }
    if (!set_shape(0)) { DestroyWindow(owner); return 2; }
    if (!late_region) ShowWindow(popup, SW_SHOWNOACTIVATE);
    if (!IsWindowVisible(popup)) { DestroyWindow(owner); return 2; }
    if (!SetTimer(owner, 1, 5000, NULL)) { DestroyWindow(owner); return 2; }
    while ((ret = GetMessageW(&message, NULL, 0, 0)) > 0)
    {
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }
    SetEvent(done);
    WaitForSingleObject(watcher, 3000);
    CloseHandle(watcher);
    CloseHandle(done);
    printf("SUMMARY complete=%d api_failed=%d visual_check_required=1\n", phase == 4, failed || ret < 0);
    return !failed && ret == 0 && phase == 4 ? 0 : 2;
}
