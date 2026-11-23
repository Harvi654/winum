/*
 * A menu the way DevExpress (Vault 2027) makes one: an overlapped window
 * (which always gets WS_CAPTION at creation), its caption dropped while it is
 * still hidden, then shown without activation. On Windows and on a fixed Wine
 * it is shown as a popup; on Wine 11.20 the X window stays managed by the
 * window manager.
 *
 * Original synthetic reproducer, written with AI assistance; LGPL 2.1 or
 * later. No Autodesk, DevExpress or Microsoft code or binaries were used.
 *
 * Prints one line per window: "<case> <X window id>", then keeps the windows
 * open for a few seconds so check.sh can look at them.
 */
#include <windows.h>
#include <stdio.h>

static LRESULT CALLBACK proc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    if (msg == WM_PAINT)
    {
        PAINTSTRUCT ps;
        HDC dc = BeginPaint(hwnd, &ps);
        FillRect(dc, &ps.rcPaint, GetStockObject(GRAY_BRUSH));
        EndPaint(hwnd, &ps);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

static void show(const char *name, DWORD ex_style, DWORD new_style, int x)
{
    HWND hwnd = CreateWindowExW(ex_style, L"repro", L"repro", WS_CLIPSIBLINGS,
                                x, 100, 200, 300, NULL, NULL, NULL, NULL);
    DWORD created = GetWindowLongW(hwnd, GWL_STYLE);

    if (new_style)
    {
        SetWindowLongW(hwnd, GWL_STYLE, new_style);
        SetWindowPos(hwnd, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER |
                     SWP_NOACTIVATE | SWP_FRAMECHANGED);
    }
    SetWindowPos(hwnd, HWND_TOPMOST, x, 100, 200, 300, SWP_SHOWWINDOW | SWP_NOACTIVATE);
    UpdateWindow(hwnd);
    printf("%s 0x%lx created %08lx shown %08lx\n", name,
           (unsigned long)(ULONG_PTR)GetPropA(hwnd, "__wine_x11_whole_window"),
           created, GetWindowLongW(hwnd, GWL_STYLE));
}

int main(void)
{
    WNDCLASSW wc = {0};
    DWORD end;
    MSG msg;

    wc.lpfnWndProc = proc;
    wc.lpszClassName = L"repro";
    wc.hCursor = LoadCursorW(NULL, (LPCWSTR)IDC_ARROW);
    RegisterClassW(&wc);

    /* control: keeps its caption, stays managed everywhere */
    show("caption", 0, 0, 100);
    /* the menu: caption dropped before the first show */
    show("menu", WS_EX_TOOLWINDOW, WS_POPUP | WS_CLIPSIBLINGS, 350);
    /* the same with WS_EX_LAYERED, as most DevExpress menus have it */
    show("menu-layered", WS_EX_TOOLWINDOW | WS_EX_LAYERED, WS_POPUP | WS_CLIPSIBLINGS, 600);
    fflush(stdout);

    for (end = GetTickCount() + 4000; GetTickCount() < end; Sleep(20))
        while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE)) DispatchMessageW(&msg);
    return 0;
}
