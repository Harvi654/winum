/* Original, AI-assisted (Codex), LGPL-2.1-or-later.
 * Small DIB writes by a process owning a child of another process's window.
 * An independent process samples the foreign window DC; no vendor components.
 */
#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>

#define CHILD_READY (WM_APP + 7)
static HWND target;
static COLORREF last_color = CLR_INVALID;
static ULONGLONG start;
static unsigned int samples, valid_samples;
static BOOL observer_failed;

static LRESULT CALLBACK proc(HWND window, UINT message, WPARAM wp, LPARAM lp)
{
    if (message == CHILD_READY)
    {
        target = (HWND)wp;
        if (!SetTimer(window, 1, 40, NULL))
        { puts("INCONCLUSIVE timer_failed=1"); observer_failed = TRUE; }
        return 0;
    }
    if (message == WM_TIMER && target && IsWindow(target))
    {
        HDC dc = GetDC(target);
        COLORREF color = dc ? GetPixel(dc, 35, 35) : CLR_INVALID;
        if (dc) ReleaseDC(target, dc);
        ++samples;
        if (color != CLR_INVALID) ++valid_samples;
        if (samples == 1 || color != last_color)
        {
            printf("SAMPLE elapsed_ms=%llu color=%08lx valid=%d\n",
                   (unsigned long long)(GetTickCount64() - start), color, color != CLR_INVALID);
            last_color = color;
        }
        return 0;
    }
    if (message == WM_CLOSE) { DestroyWindow(window); return 0; }
    if (message == WM_DESTROY) { PostQuitMessage(0); return 0; }
    return DefWindowProcW(window, message, wp, lp);
}

static int writer(HWND host, BOOL flush)
{
    BITMAPINFO info;
    DWORD pixels[8 * 8];
    ULONGLONG until;
    unsigned int phase, i;
    MSG msg;
    HDC dc;
    HWND child = CreateWindowExW(0, L"WinumPutImage", L"writer",
            WS_CHILD | WS_VISIBLE, 10, 10, 160, 100, host, NULL, GetModuleHandleW(NULL), NULL);
    if (!child) return 2;
    UpdateWindow(child);
    SendMessageW(host, CHILD_READY, (WPARAM)child, 0);
    /* Finish initial mapping/paint before the idle-only writes. */
    until = GetTickCount64() + 1000;
    while (GetTickCount64() < until)
    {
        while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE)) DispatchMessageW(&msg);
        Sleep(10);
    }
    dc = GetDC(child);
    if (!dc) { DestroyWindow(child); return 2; }
    memset(&info, 0, sizeof(info));
    info.bmiHeader.biSize = sizeof(info.bmiHeader);
    info.bmiHeader.biWidth = 8;
    info.bmiHeader.biHeight = -8;
    info.bmiHeader.biPlanes = 1;
    info.bmiHeader.biBitCount = 32;
    info.bmiHeader.biCompression = BI_RGB;
    for (phase = 0; phase < 6; ++phase)
    {
        DWORD dib_color = phase & 1 ? 0x000000ff : 0x00ff0000;
        int lines;
        for (i = 0; i < ARRAYSIZE(pixels); ++i) pixels[i] = dib_color;
        lines = SetDIBitsToDevice(dc, 32, 32, 8, 8, 0, 0, 0, 8, pixels, &info, DIB_RGB_COLORS);
        if (flush)
        {
            BOOL ret = GdiFlush();
            printf("CONTROL phase=%u gdi_flush=%d\n", phase, ret);
            if (!ret) { ReleaseDC(child, dc); DestroyWindow(child); return 2; }
        }
        printf("DRAW tick=%llu phase=%u color=%08lx lines=%d idle_ms=1200 flush=%d\n",
               (unsigned long long)GetTickCount64(), phase,
               phase & 1 ? RGB(0, 0, 255) : RGB(255, 0, 0), lines, flush);
        if (lines != 8) { ReleaseDC(child, dc); DestroyWindow(child); return 2; }
        /* Deliberately no message pump, DC release, GDI read or next write
         * during this interval. stdout does not make an X request. */
        Sleep(1200);
    }
    {
        BOOL ret = GdiFlush();
        printf("RECOVERY final_gdi_flush=%d\n", ret);
        if (!ret) { ReleaseDC(child, dc); DestroyWindow(child); return 2; }
    }
    Sleep(500);
    ReleaseDC(child, dc);
    DestroyWindow(child);
    return 0;
}

int wmain(int argc, WCHAR **argv)
{
    WNDCLASSW cls;
    WCHAR self[MAX_PATH], command[2 * MAX_PATH];
    PROCESS_INFORMATION pi;
    STARTUPINFOW si;
    ULONGLONG deadline;
    HWND host;
    DWORD code = 2;
    BOOL flush = argc == 2 && !wcscmp(argv[1], L"--gdi-flush");
    int n;
    MSG msg;
    setvbuf(stdout, NULL, _IONBF, 0);
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
    memset(&cls, 0, sizeof(cls));
    cls.lpfnWndProc = proc;
    cls.hInstance = GetModuleHandleW(NULL);
    cls.lpszClassName = L"WinumPutImage";
    cls.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    if (!RegisterClassW(&cls)) return 2;
    if (argc == 4 && !wcscmp(argv[1], L"--writer"))
        return writer((HWND)(uintptr_t)wcstoull(argv[2], NULL, 10), _wtoi(argv[3]));
    if (argc != 1 && !flush) return 2;
    host = CreateWindowExW(0, cls.lpszClassName, L"PutImage: tiny red/blue square",
            WS_OVERLAPPEDWINDOW | WS_VISIBLE | WS_CLIPCHILDREN,
            50, 50, 260, 190, NULL, NULL, cls.hInstance, NULL);
    if (!host) return 2;
    {
        DWORD length = GetModuleFileNameW(NULL, self, ARRAYSIZE(self));
        if (!length || length >= ARRAYSIZE(self)) return 2;
    }
    n = swprintf(command, ARRAYSIZE(command), L"\"%ls\" --writer %llu %d", self,
                 (unsigned long long)(uintptr_t)host, flush);
    if (n < 0 || n >= (int)ARRAYSIZE(command)) return 2;
    memset(&si, 0, sizeof(si));
    memset(&pi, 0, sizeof(pi));
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    si.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    si.hStdError = GetStdHandle(STD_ERROR_HANDLE);
    start = GetTickCount64();
    printf("REPRO topic=winex11-flush-putimage bits=%u start_tick=%llu control_flush=%d\n",
           (unsigned int)(sizeof(void *) * 8), (unsigned long long)start, flush);
    if (!CreateProcessW(NULL, command, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi)) return 2;
    CloseHandle(pi.hThread);
    deadline = start + 20000;
    while (WaitForSingleObject(pi.hProcess, 0) == WAIT_TIMEOUT && IsWindow(host))
    {
        if (GetTickCount64() >= deadline) break;
        MsgWaitForMultipleObjects(1, &pi.hProcess, FALSE, 40, QS_ALLINPUT);
        while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE)) DispatchMessageW(&msg);
    }
    if (WaitForSingleObject(pi.hProcess, 0) != WAIT_OBJECT_0)
    { puts("INCONCLUSIVE timeout_or_window_closed=1"); TerminateProcess(pi.hProcess, 2); }
    WaitForSingleObject(pi.hProcess, 3000);
    GetExitCodeProcess(pi.hProcess, &code);
    CloseHandle(pi.hProcess);
    if (IsWindow(host)) DestroyWindow(host);
    if (!samples || !valid_samples || observer_failed)
    { puts("INCONCLUSIVE no_usable_pixel_observation=1"); return 2; }
    printf("SUMMARY writer_exit=%lu samples=%u valid_samples=%u pixel_timing_requires_comparison=1\n",
           code, samples, valid_samples);
    return (int)code;
}
