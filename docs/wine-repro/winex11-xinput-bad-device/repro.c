/* Original, AI-assisted (Codex), LGPL-2.1-or-later.
 * Own raw-input recipient. Click only its window to exercise XInput events.
 */
#include <windows.h>
#include <stdio.h>

static unsigned int raw_packets, raw_buttons, ordinary_clicks, ticks, errors;
static HANDLE done;

static DWORD WINAPI watchdog(void *event)
{
    if (WaitForSingleObject(event, 35000) == WAIT_OBJECT_0) return 0;
    puts("INCONCLUSIVE watchdog_timeout=1");
    TerminateProcess(GetCurrentProcess(), 2);
    return 2;
}

static LRESULT CALLBACK proc(HWND window, UINT message, WPARAM wp, LPARAM lp)
{
    if (message == WM_INPUT)
    {
        RAWINPUT input;
        UINT size = sizeof(input);
        UINT count = GetRawInputData((HRAWINPUT)lp, RID_INPUT, &input, &size, sizeof(RAWINPUTHEADER));
        if (count == (UINT)-1) { printf("ERROR raw_read=%lu\n", GetLastError()); ++errors; }
        else if (count >= sizeof(RAWINPUTHEADER) + sizeof(RAWMOUSE) && input.header.dwType == RIM_TYPEMOUSE)
        {
            ++raw_packets;
            if (input.data.mouse.usButtonFlags)
            {
                ++raw_buttons;
                printf("RAW button_flags=%04x count=%u\n", input.data.mouse.usButtonFlags, raw_buttons);
            }
        }
        return DefWindowProcW(window, message, wp, lp);
    }
    if (message == WM_LBUTTONDOWN) { printf("CLICK count=%u\n", ++ordinary_clicks); return 0; }
    if (message == WM_PAINT)
    {
        PAINTSTRUCT ps;
        HDC dc = BeginPaint(window, &ps);
        RECT r;
        GetClientRect(window, &r);
        DrawTextW(dc, L"Move the pointer here and click three times.\nOnly this own window; no synthetic SendInput.\nCloses after 20 seconds.",
                  -1, &r, DT_LEFT | DT_TOP | DT_WORDBREAK);
        EndPaint(window, &ps);
        return 0;
    }
    if (message == WM_TIMER)
    {
        printf("ALIVE interval=%u raw_packets=%u raw_buttons=%u ordinary_clicks=%u\n",
               ++ticks, raw_packets, raw_buttons, ordinary_clicks);
        if (ticks == 4) DestroyWindow(window);
        return 0;
    }
    if (message == WM_CLOSE) { DestroyWindow(window); return 0; }
    if (message == WM_DESTROY) { PostQuitMessage(0); return 0; }
    return DefWindowProcW(window, message, wp, lp);
}

int main(void)
{
    WNDCLASSW cls;
    RAWINPUTDEVICE device;
    HANDLE watcher;
    HWND window;
    MSG msg;
    int ret;
    setvbuf(stdout, NULL, _IONBF, 0);
    printf("REPRO topic=winex11-xinput-bad-device bits=%u windows_pid=%lu\n",
           (unsigned int)(sizeof(void *) * 8), GetCurrentProcessId());
    done = CreateEventW(NULL, TRUE, FALSE, NULL);
    if (!done) return 2;
    watcher = CreateThread(NULL, 0, watchdog, done, 0, NULL);
    if (!watcher) { CloseHandle(done); return 2; }
    memset(&cls, 0, sizeof(cls));
    cls.lpfnWndProc = proc;
    cls.hInstance = GetModuleHandleW(NULL);
    cls.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    cls.hCursor = LoadCursorW(NULL, (LPCWSTR)IDC_ARROW);
    cls.lpszClassName = L"WinumRawInput";
    if (!RegisterClassW(&cls)) return 2;
    window = CreateWindowExW(0, cls.lpszClassName, L"Own raw-input fault fixture",
            WS_OVERLAPPEDWINDOW | WS_VISIBLE, 80, 80, 430, 180, NULL, NULL, cls.hInstance, NULL);
    if (!window) return 2;
    memset(&device, 0, sizeof(device));
    device.usUsagePage = 1;
    device.usUsage = 2;
    device.dwFlags = RIDEV_INPUTSINK;
    device.hwndTarget = window;
    if (!RegisterRawInputDevices(&device, 1, sizeof(device)) || !SetTimer(window, 1, 5000, NULL))
    { printf("INCONCLUSIVE raw_registration_or_timer=%lu\n", GetLastError()); DestroyWindow(window); return 2; }
    while ((ret = GetMessageW(&msg, NULL, 0, 0)) > 0)
    { TranslateMessage(&msg); DispatchMessageW(&msg); }
    SetEvent(done);
    WaitForSingleObject(watcher, 3000);
    CloseHandle(watcher);
    CloseHandle(done);
    printf("SUMMARY complete=%d raw_packets=%u raw_buttons=%u ordinary_clicks=%u api_errors=%u\n",
           ticks == 4, raw_packets, raw_buttons, ordinary_clicks, errors);
    return ret == 0 && ticks == 4 && raw_packets && raw_buttons && ordinary_clicks && !errors ? 0 : 2;
}
