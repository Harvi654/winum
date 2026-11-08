/*
 * MEASURE: when does Windows send WM_MOUSEMOVE to the window under the
 * cursor after other windows are shown, hidden, created, destroyed or
 * moved? For server-mousemove-on-show-hide, whose generated moves make
 * Wine's own comctl32 button and user32 mouse tests fail intermittently.
 *
 * The cursor is put over the centre of window A. For each scenario the
 * program does one action, then pumps messages for 400 ms and prints every
 * WM_MOUSEMOVE it received (which window, how many ms after the action).
 *
 * Build: x86_64-w64-mingw32-gcc / i686-w64-mingw32-gcc -O2 -Wall -o m.exe measure.c -luser32 -lgdi32
 * Do not touch mouse or keyboard while it runs (about 15 s).
 * Text output only. Original, AI-assisted (Claude), LGPL-2.1-or-later.
 */
#include <windows.h>
#include <stdio.h>

#define MAXREC 64
static struct { HWND hwnd; DWORD ms; LPARAM lp; } rec[MAXREC];
static int nrec;
static DWORD t0;
static HWND a, b_away, b_under, c_away, c_under;

static const char *tag(HWND h)
{
    if (h == a) return "A";
    if (h == b_away) return "B_away";
    if (h == b_under) return "B_under";
    if (h == c_away) return "C_away";
    if (h == c_under) return "C_under";
    return "other";
}

static LRESULT CALLBACK proc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    if (msg == WM_MOUSEMOVE && nrec < MAXREC)
    {
        rec[nrec].hwnd = hwnd;
        rec[nrec].ms = GetTickCount() - t0;
        rec[nrec].lp = lp;
        nrec++;
    }
    return DefWindowProcA(hwnd, msg, wp, lp);
}

static void pump(DWORD ms)
{
    DWORD end = GetTickCount() + ms;
    MSG m;
    while ((LONG)(end - GetTickCount()) > 0)
    {
        while (PeekMessageA(&m, 0, 0, 0, PM_REMOVE)) { TranslateMessage(&m); DispatchMessageA(&m); }
        MsgWaitForMultipleObjects(0, NULL, FALSE, 10, QS_ALLINPUT);
    }
}

static void report(const char *name)
{
    POINT pt;
    int i;
    GetCursorPos(&pt);
    printf("SCENARIO %-26s moves=%d cursor=%ld,%ld under=%s", name, nrec, pt.x, pt.y, tag(WindowFromPoint(pt)));
    for (i = 0; i < nrec; i++)
        printf(" [%s %lums]", tag(rec[i].hwnd), rec[i].ms);
    printf("\n");
}

#define RUN(name, action) do { pump(300); nrec = 0; t0 = GetTickCount(); action; pump(400); report(name); } while (0)

static HWND child(int x, int y)
{
    return CreateWindowA("measure_mm", NULL, WS_CHILD, x, y, 40, 40, a, NULL, NULL, NULL);
}

int main(void)
{
    WNDCLASSA wc = {0};
    RECT r;
    POINT centre, cc;
    HWND tmp;
    int i;

    wc.lpfnWndProc = proc;
    wc.hInstance = GetModuleHandleA(NULL);
    wc.hCursor = LoadCursorA(NULL, (LPCSTR)IDC_ARROW);
    wc.hbrBackground = GetStockObject(WHITE_BRUSH);
    wc.lpszClassName = "measure_mm";
    RegisterClassA(&wc);

    a = CreateWindowA("measure_mm", "A", WS_OVERLAPPEDWINDOW | WS_VISIBLE, 100, 100, 420, 320, NULL, NULL, NULL, NULL);
    GetClientRect(a, &r);
    centre.x = r.right / 2; centre.y = r.bottom / 2;
    cc = centre;
    c_away = child(10, 10);
    c_under = child(cc.x - 20, cc.y - 20);
    ClientToScreen(a, &centre);
    b_away = CreateWindowExA(WS_EX_TOOLWINDOW, "measure_mm", "B_away", WS_POPUP, 700, 500, 100, 80, NULL, NULL, NULL, NULL);
    b_under = CreateWindowExA(WS_EX_TOOLWINDOW, "measure_mm", "B_under", WS_POPUP, centre.x - 30, centre.y - 30, 60, 60, NULL, NULL, NULL, NULL);
    SetForegroundWindow(a);
    SetCursorPos(centre.x, centre.y);
    pump(800);
    printf("arch=%d centre=%ld,%ld\n", (int)(sizeof(void *) * 8), centre.x, centre.y);

    RUN("baseline", (void)0);
    RUN("cursor_set_same_pos", SetCursorPos(centre.x, centre.y));
    RUN("show_toplevel_away", ShowWindow(b_away, SW_SHOWNOACTIVATE));
    RUN("hide_toplevel_away", ShowWindow(b_away, SW_HIDE));
    RUN("show_toplevel_under", ShowWindow(b_under, SW_SHOWNOACTIVATE));
    RUN("hide_toplevel_under", ShowWindow(b_under, SW_HIDE));
    RUN("show_child_away", ShowWindow(c_away, SW_SHOWNA));
    RUN("hide_child_away", ShowWindow(c_away, SW_HIDE));
    RUN("show_child_under", ShowWindow(c_under, SW_SHOWNA));
    RUN("hide_child_under", ShowWindow(c_under, SW_HIDE));
    RUN("burst_child_away_x5", for (i = 0; i < 5; i++) { ShowWindow(c_away, SW_SHOWNA); ShowWindow(c_away, SW_HIDE); });
    RUN("move_toplevel_away", SetWindowPos(b_away, NULL, 720, 520, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE));
    RUN("create_child_away_visible", tmp = CreateWindowA("measure_mm", NULL, WS_CHILD | WS_VISIBLE, 10, 60, 40, 40, a, NULL, NULL, NULL));
    RUN("destroy_child_away", DestroyWindow(tmp));
    RUN("create_child_under_visible", tmp = CreateWindowA("measure_mm", NULL, WS_CHILD | WS_VISIBLE, cc.x - 20, cc.y - 20, 40, 40, a, NULL, NULL, NULL));
    RUN("destroy_child_under", DestroyWindow(tmp));
    RUN("invalidate_update_A", InvalidateRect(a, NULL, TRUE); UpdateWindow(a));
    SetCursorPos(5, 5);
    pump(300);
    RUN("cursor_off_A_show_away", ShowWindow(b_away, SW_SHOWNOACTIVATE));
    RUN("cursor_off_A_hide_away", ShowWindow(b_away, SW_HIDE));
    puts("SUMMARY done");
    DestroyWindow(b_under);
    DestroyWindow(b_away);
    DestroyWindow(a);
    return 0;
}
