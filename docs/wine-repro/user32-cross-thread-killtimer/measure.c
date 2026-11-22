/* Original, AI-assisted (Codex), LGPL-2.1-or-later. */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>

struct cancellation
{
    UINT_PTR id;
    BOOL result;
    DWORD error;
    BOOL create_queue;
};

static unsigned int callbacks;

static void CALLBACK timer_callback(HWND window, UINT message, UINT_PTR id, DWORD tick)
{
    (void)window;
    (void)message;
    (void)id;
    (void)tick;
    ++callbacks;
}

static DWORD WINAPI cancel_timer(void *argument)
{
    struct cancellation *request = argument;
    MSG message;

    if (request->create_queue) PeekMessageW(&message, NULL, 0, 0, PM_NOREMOVE);
    SetLastError(0xdeadbeef);
    request->result = KillTimer(NULL, request->id);
    request->error = GetLastError();
    return 0;
}

static int measure(const char *name, BOOL cross_thread, BOOL create_queue)
{
    struct cancellation request = {0};
    unsigned int messages = 0;
    ULONGLONG deadline;
    HANDLE worker;
    MSG message;

    request.create_queue = create_queue;
    callbacks = 0;
    request.id = SetTimer(NULL, 0, 150, timer_callback);
    if (!request.id)
    {
        fprintf(stderr, "case=%s setup=set_timer_failed\n", name);
        return 1;
    }

    if (cross_thread)
    {
        worker = CreateThread(NULL, 0, cancel_timer, &request, 0, NULL);
        if (!worker)
        {
            KillTimer(NULL, request.id);
            fprintf(stderr, "case=%s setup=create_thread_failed\n", name);
            return 1;
        }
        if (WaitForSingleObject(worker, 2000) != WAIT_OBJECT_0)
        {
            fprintf(stderr, "case=%s setup=worker_wait_failed\n", name);
            ExitProcess(1);
        }
        CloseHandle(worker);
    }
    else cancel_timer(&request);

    deadline = GetTickCount64() + 600;
    while (GetTickCount64() < deadline)
    {
        while (PeekMessageW(&message, NULL, WM_TIMER, WM_TIMER, PM_REMOVE))
        {
            if (message.hwnd || message.wParam != request.id) continue;
            ++messages;
            DispatchMessageW(&message);
        }
        Sleep(5);
    }
    KillTimer(NULL, request.id);
    printf("case=%s kill=%u last_error=0x%08lx timer_messages=%u callbacks=%u\n",
           name, !!request.result, (unsigned long)request.error, messages, callbacks);
    return 0;
}

int main(void)
{
    return measure("same_thread", FALSE, FALSE) |
           measure("cross_thread_no_queue", TRUE, FALSE) |
           measure("cross_thread_with_queue", TRUE, TRUE);
}
