/* Original Win32 measurement, AI-assisted (Codex), LGPL-2.1-or-later. */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>

enum { width = 180, height = 150, target_x = 320, target_y = 70 };
enum { paint_queue = 1, stop_worker };
static const char class_name[] = "WinumStaleBitsMeasure";

struct counts
{
    unsigned paint, erase, nccalc;
    RECT before[3], after[3];
    UINT nccalc_result;
    BOOL has_rects;
};

struct pane
{
    HWND hwnd;
    BOOL root, red_origin;
    COLORREF color;
    CRITICAL_SECTION lock;
    struct counts counts;
};

struct worker
{
    const char *name;
    struct pane *container;
    HWND parent;
    BOOL visible;
    HANDLE thread, ready, request, done;
    LONG command;
};

struct scenario
{
    const char *name;
    int cover; /* -1: no ribbon; 0/half/full: ribbon at the corresponding overlap */
    BOOL warm, hidden, redraw, intermediate, threaded;
    UINT move_flags;
};

static void fill(HDC dc, const RECT *rect, COLORREF color)
{
    SetDCBrushColor(dc, color);
    FillRect(dc, rect, GetStockObject(DC_BRUSH));
}

static LRESULT CALLBACK pane_proc(HWND window, UINT message, WPARAM wp, LPARAM lp)
{
    struct pane *pane = (struct pane *)GetWindowLongPtrA(window, GWLP_USERDATA);
    if (message == WM_NCCREATE)
    {
        pane = ((CREATESTRUCTA *)lp)->lpCreateParams;
        SetWindowLongPtrA(window, GWLP_USERDATA, (LONG_PTR)pane);
    }
    if (!pane) return DefWindowProcA(window, message, wp, lp);
    switch (message)
    {
    case WM_PAINT:
    {
        PAINTSTRUCT ps = {0};
        HDC dc;
        RECT rect;
        EnterCriticalSection(&pane->lock);
        ++pane->counts.paint;
        LeaveCriticalSection(&pane->lock);
        dc = BeginPaint(window, &ps);
        if (dc && GetClientRect(window, &rect))
        {
            fill(dc, &rect, pane->color);
            if (pane->root && pane->red_origin)
            {
                SetRect(&rect, 0, 0, width, height);
                fill(dc, &rect, RGB(255, 0, 0));
            }
        }
        EndPaint(window, &ps);
        return 0;
    }
    case WM_ERASEBKGND:
        EnterCriticalSection(&pane->lock);
        ++pane->counts.erase;
        LeaveCriticalSection(&pane->lock);
        return 1;
    case WM_NCCALCSIZE:
    {
        NCCALCSIZE_PARAMS *params = wp ? (NCCALCSIZE_PARAMS *)lp : NULL;
        RECT original[3];
        LRESULT result;
        if (params) CopyMemory(original, params->rgrc, sizeof(original));
        result = DefWindowProcA(window, message, wp, lp);
        EnterCriticalSection(&pane->lock);
        ++pane->counts.nccalc;
        if (params)
        {
            CopyMemory(pane->counts.before, original, sizeof(original));
            CopyMemory(pane->counts.after, params->rgrc, sizeof(original));
            pane->counts.nccalc_result = (UINT)result;
            pane->counts.has_rects = TRUE;
        }
        LeaveCriticalSection(&pane->lock);
        return result;
    }
    default:
        return DefWindowProcA(window, message, wp, lp);
    }
}

static BOOL record_call(const char *name, const char *api, BOOL ok)
{
    if (!ok) printf("case=%s error=%s last_error=%lu\n", name, api, GetLastError());
    return ok;
}

static HWND make_child(struct pane *pane, HWND parent, DWORD style, int y, int w)
{
    return CreateWindowExA(0, class_name, "", WS_CHILD | style, 0, y, w, height,
                           parent, NULL, GetModuleHandleA(NULL), pane);
}

static void drain(const char *name)
{
    MSG msg;
    unsigned count = 0;
    while (count < 5000 && PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE))
    {
        if (msg.message != WM_QUIT) DispatchMessageW(&msg);
        ++count;
    }
    if (count == 5000) printf("case=%s error=queue_drain_limit\n", name);
}

static DWORD WINAPI worker_proc(void *arg)
{
    struct worker *worker = arg;
    MSG msg;
    DWORD status;
    worker->container->hwnd = make_child(worker->container, worker->parent,
                                         WS_CLIPCHILDREN | WS_CLIPSIBLINGS |
                                         (worker->visible ? WS_VISIBLE : 0), 0, width);
    record_call(worker->name, "CreateWindowEx(container worker)", !!worker->container->hwnd);
    if (!SetEvent(worker->ready)) return 1;
    for (;;)
    {
        status = MsgWaitForMultipleObjectsEx(1, &worker->request, INFINITE, QS_SENDMESSAGE,
                                            MWMO_INPUTAVAILABLE);
        if (status == WAIT_OBJECT_0)
        {
            if (InterlockedCompareExchange(&worker->command, 0, 0) == stop_worker) break;
            drain(worker->name);
            if (!SetEvent(worker->done)) break;
        }
        else if (status == WAIT_OBJECT_0 + 1)
            PeekMessageW(&msg, NULL, 0, 0, PM_NOREMOVE | PM_QS_SENDMESSAGE);
        else break;
    }
    if (worker->container->hwnd)
        record_call(worker->name, "DestroyWindow(container worker)", DestroyWindow(worker->container->hwnd));
    return 0;
}

/* Sent messages can complete across threads while posted paint remains queued. */
static void wait_without_paint(const char *name, const char *step, HANDLE event)
{
    ULONGLONG deadline = GetTickCount64() + 5000;
    MSG msg;
    DWORD status = WAIT_TIMEOUT;
    ULONGLONG now;
    while ((now = GetTickCount64()) < deadline)
    {
        status = MsgWaitForMultipleObjectsEx(1, &event, (DWORD)(deadline - now),
                                            QS_SENDMESSAGE, MWMO_INPUTAVAILABLE);
        if (status == WAIT_OBJECT_0) return;
        if (status != WAIT_OBJECT_0 + 1) break;
        PeekMessageW(&msg, NULL, 0, 0, PM_NOREMOVE | PM_QS_SENDMESSAGE);
    }
    printf("case=%s error=%s wait_result=%lu last_error=%lu\n", name, step, status, GetLastError());
    fflush(stdout);
    /* A worker may still own a window pointing into this case's stack. */
    ExitProcess(0);
}

static void pump_case(const char *name, struct worker *worker)
{
    drain(name);
    if (worker->thread)
    {
        InterlockedExchange(&worker->command, paint_queue);
        if (!record_call(name, "SetEvent(drain)", SetEvent(worker->request))) return;
        wait_without_paint(name, "worker drain", worker->done);
        drain(name);
    }
}

static void print_rect(const RECT *rect)
{
    printf("%ld,%ld,%ld,%ld", rect->left, rect->top, rect->right, rect->bottom);
}

static DWORD pixel(HDC dc, int x, int y)
{
    COLORREF color = dc ? GetPixel(dc, x, y) : CLR_INVALID;
    if (color == CLR_INVALID) return 0xffffffff;
    return ((DWORD)GetRValue(color) << 16) | ((DWORD)GetGValue(color) << 8) | GetBValue(color);
}

static void observe(const char *name, const char *step, HWND parent, struct pane *container)
{
    POINT client = {0, 0};
    RECT frame = {0}, rect = {0}, update = {0}, bounds = {0};
    struct counts counts;
    HWND window = container->hwnd;
    BOOL frame_ok, origin_ok, rect_ok, has_update;
    HRGN region;
    int region_type = ERROR, left_in_update = -1, right_in_update = -1;
    DWORD source_left = 0xffffffff, source_right = 0xffffffff;
    DWORD dest_left = 0xffffffff, dest_right = 0xffffffff;
    HDC dc;

    frame_ok = record_call(name, "GetWindowRect(top)", GetWindowRect(parent, &frame));
    origin_ok = record_call(name, "ClientToScreen(top)", ClientToScreen(parent, &client));
    dc = GetDCEx(parent, NULL, DCX_WINDOW | DCX_CACHE);
    record_call(name, "GetDCEx(top)", !!dc);
    if (dc && frame_ok && origin_ok)
    {
        int x = client.x - frame.left;
        int y = client.y - frame.top;
        GdiFlush();
        source_left = pixel(dc, x + width / 4, y + height / 2);
        source_right = pixel(dc, x + 3 * width / 4, y + height / 2);
        dest_left = pixel(dc, x + target_x + width / 4, y + target_y + height / 2);
        dest_right = pixel(dc, x + target_x + 3 * width / 4, y + target_y + height / 2);
    }
    if (dc) record_call(name, "ReleaseDC(top)", ReleaseDC(parent, dc) != 0);
    rect_ok = record_call(name, "GetWindowRect(container)", GetWindowRect(window, &rect));
    has_update = GetUpdateRect(window, &update, FALSE);
    region = CreateRectRgn(0, 0, 0, 0);
    if (record_call(name, "CreateRectRgn", !!region))
    {
        region_type = GetUpdateRgn(window, region, FALSE);
        if (record_call(name, "GetUpdateRgn", region_type != ERROR))
        {
            GetRgnBox(region, &bounds);
            left_in_update = PtInRegion(region, width / 4, height / 2);
            right_in_update = PtInRegion(region, 3 * width / 4, height / 2);
        }
        DeleteObject(region);
    }
    EnterCriticalSection(&container->lock);
    counts = container->counts;
    LeaveCriticalSection(&container->lock);

    printf("case=%s step=%s source=%08lx,%08lx dest=%08lx,%08lx update=%d:", name, step,
           source_left, source_right, dest_left, dest_right, has_update);
    print_rect(&update);
    printf(" region=%d:", region_type);
    print_rect(&bounds);
    printf(" region_samples=%d,%d paint=%u erase=%u nccalc=%u nccalc_true=%d",
           left_in_update, right_in_update, counts.paint, counts.erase, counts.nccalc, counts.has_rects);
    if (counts.has_rects)
    {
        unsigned i;
        printf(" nccalc_ret=%04x nccalc_in=", counts.nccalc_result);
        for (i = 0; i < 3; ++i) { if (i) putchar('/'); print_rect(&counts.before[i]); }
        printf(" nccalc_out=");
        for (i = 0; i < 3; ++i) { if (i) putchar('/'); print_rect(&counts.after[i]); }
    }
    printf(" rect_ok=%d window=", rect_ok);
    print_rect(&rect);
    putchar('\n');
}

static void change_pos(const char *name, const char *step, HWND window, int x, int y, UINT flags)
{
    BOOL ok = SetWindowPos(window, NULL, x, y, width, height, flags);
    DWORD error = ok ? 0 : GetLastError();
    printf("case=%s op=%s flags=%04x ok=%d last_error=%lu\n", name, step, flags, ok, error);
}

static void run_case(const struct scenario *test)
{
    const UINT move = SWP_NOZORDER | SWP_NOACTIVATE;
    const UINT frame = move | SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED;
    struct pane top = {0}, middle = {0}, container = {0}, ribbon = {0};
    struct worker worker = {0};
    HWND parent;

    top.root = TRUE;
    top.red_origin = test->cover < 0;
    top.color = middle.color = RGB(0, 128, 0);
    container.color = RGB(0, 0, 255);
    ribbon.color = RGB(255, 0, 0);
    InitializeCriticalSection(&top.lock);
    InitializeCriticalSection(&middle.lock);
    InitializeCriticalSection(&container.lock);
    InitializeCriticalSection(&ribbon.lock);

    top.hwnd = CreateWindowExA(WS_EX_TOPMOST, class_name, "Winum stale bits MEASURE",
                              WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_VISIBLE,
                              32, 32, 570, 360, NULL, NULL, GetModuleHandleA(NULL), &top);
    if (!record_call(test->name, "CreateWindowEx(top)", !!top.hwnd)) goto cleanup;
    record_call(test->name, "UpdateWindow(top)", UpdateWindow(top.hwnd));
    drain(test->name);
    parent = top.hwnd;
    if (test->intermediate)
    {
        middle.hwnd = make_child(&middle, top.hwnd, WS_VISIBLE | WS_CLIPCHILDREN, 0, width);
        if (!record_call(test->name, "CreateWindowEx(intermediate)", !!middle.hwnd)) goto cleanup;
        parent = middle.hwnd;
    }

    if (test->threaded)
    {
        worker.name = test->name;
        worker.container = &container;
        worker.parent = parent;
        worker.visible = !test->hidden;
        worker.ready = CreateEventA(NULL, FALSE, FALSE, NULL);
        worker.request = CreateEventA(NULL, FALSE, FALSE, NULL);
        worker.done = CreateEventA(NULL, FALSE, FALSE, NULL);
        if (!record_call(test->name, "CreateEvent(worker)", worker.ready && worker.request && worker.done))
            goto cleanup;
        worker.thread = CreateThread(NULL, 0, worker_proc, &worker, 0, NULL);
        if (!record_call(test->name, "CreateThread", !!worker.thread)) goto cleanup;
        wait_without_paint(test->name, "worker create", worker.ready);
    }
    else
        container.hwnd = make_child(&container, parent, WS_CLIPCHILDREN | WS_CLIPSIBLINGS |
                                    (test->hidden ? 0 : WS_VISIBLE), 0, width);
    if (!record_call(test->name, "CreateWindowEx(container)", !!container.hwnd)) goto cleanup;

    if (test->cover >= 0)
    {
        ribbon.hwnd = make_child(&ribbon, top.hwnd, WS_VISIBLE | WS_CLIPSIBLINGS,
                                 test->cover ? 0 : height + 12, test->cover ? test->cover : width);
        if (!record_call(test->name, "CreateWindowEx(ribbon)", !!ribbon.hwnd)) goto cleanup;
        /* Explicit Z order without touching the other thread's paint queue. */
        record_call(test->name, "SetWindowPos(ribbon top)",
                    SetWindowPos(ribbon.hwnd, HWND_TOP, 0, 0, 0, 0,
                                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE));
        record_call(test->name, "UpdateWindow(ribbon)", UpdateWindow(ribbon.hwnd));
    }
    if (test->warm) pump_case(test->name, &worker);
    observe(test->name, "initial", top.hwnd, &container);
    if (test->redraw)
    {
        SendMessageA(container.hwnd, WM_SETREDRAW, FALSE, 0);
        observe(test->name, "redraw_off", top.hwnd, &container);
    }
    change_pos(test->name, test->intermediate ? "move_parent" : "move",
               test->intermediate ? middle.hwnd : container.hwnd,
               target_x, target_y, move | test->move_flags);
    observe(test->name, "after_move", top.hwnd, &container);
    if (test->hidden)
    {
        change_pos(test->name, "show", container.hwnd, 0, 0, move | SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
        observe(test->name, "after_show", top.hwnd, &container);
    }
    if (test->redraw)
    {
        SendMessageA(container.hwnd, WM_SETREDRAW, TRUE, 0);
        observe(test->name, "redraw_on", top.hwnd, &container);
        change_pos(test->name, "frame_changed", container.hwnd, 0, 0, frame);
        observe(test->name, "after_frame", top.hwnd, &container);
    }
    pump_case(test->name, &worker);
    observe(test->name, "after_pump", top.hwnd, &container);

cleanup:
    if (worker.thread)
    {
        InterlockedExchange(&worker.command, stop_worker);
        record_call(test->name, "SetEvent(stop)", SetEvent(worker.request));
        wait_without_paint(test->name, "worker exit", worker.thread);
        CloseHandle(worker.thread);
    }
    if (worker.done) CloseHandle(worker.done);
    if (worker.request) CloseHandle(worker.request);
    if (worker.ready) CloseHandle(worker.ready);
    if (top.hwnd) record_call(test->name, "DestroyWindow(top)", DestroyWindow(top.hwnd));
    DeleteCriticalSection(&ribbon.lock);
    DeleteCriticalSection(&container.lock);
    DeleteCriticalSection(&middle.lock);
    DeleteCriticalSection(&top.lock);
}

int main(void)
{
    static const struct scenario cases[] = {
        {"G_full_same", width, TRUE, FALSE, FALSE, FALSE, FALSE, 0},
        {"G_half_same", width / 2, TRUE, FALSE, FALSE, FALSE, FALSE, 0},
        {"G_none_same", 0, TRUE, FALSE, FALSE, FALSE, FALSE, 0},
        {"G_full_unpainted_same", width, FALSE, FALSE, FALSE, FALSE, FALSE, 0},
        {"G_half_unpainted_same", width / 2, FALSE, FALSE, FALSE, FALSE, FALSE, 0},
        {"G_none_unpainted_same", 0, FALSE, FALSE, FALSE, FALSE, FALSE, 0},
        {"G_full_other", width, TRUE, FALSE, FALSE, FALSE, TRUE, 0},
        {"G_half_other", width / 2, TRUE, FALSE, FALSE, FALSE, TRUE, 0},
        {"G_none_other", 0, TRUE, FALSE, FALSE, FALSE, TRUE, 0},
        {"G_full_unpainted_other", width, FALSE, FALSE, FALSE, FALSE, TRUE, 0},
        {"G_half_unpainted_other", width / 2, FALSE, FALSE, FALSE, FALSE, TRUE, 0},
        {"G_none_unpainted_other", 0, FALSE, FALSE, FALSE, FALSE, TRUE, 0},
        {"A", -1, FALSE, FALSE, FALSE, FALSE, FALSE, 0},
        {"B", -1, FALSE, TRUE, FALSE, FALSE, FALSE, 0},
        {"C", -1, TRUE, FALSE, TRUE, FALSE, FALSE, 0},
        {"D", -1, TRUE, FALSE, TRUE, TRUE, FALSE, 0},
        {"E_direct", -1, TRUE, FALSE, TRUE, FALSE, TRUE, 0},
        {"E_parent", -1, TRUE, FALSE, TRUE, TRUE, TRUE, 0},
        {"F_noredraw", -1, FALSE, FALSE, FALSE, FALSE, FALSE, SWP_NOREDRAW},
        {"F_nocopybits", -1, FALSE, FALSE, FALSE, FALSE, FALSE, SWP_NOCOPYBITS},
    };
    WNDCLASSA klass = {0};
    BOOL aware = SetProcessDPIAware();
    HDC screen = GetDC(NULL);
    unsigned i;

    printf("measure=winum-stale-bits pointer_bits=%u dpi_aware_call=%d dpi=%d,%d\n",
           (unsigned)(8 * sizeof(void *)), aware, screen ? GetDeviceCaps(screen, LOGPIXELSX) : 0,
           screen ? GetDeviceCaps(screen, LOGPIXELSY) : 0);
    if (screen) ReleaseDC(NULL, screen);
    klass.lpfnWndProc = pane_proc;
    klass.hInstance = GetModuleHandleA(NULL);
    klass.lpszClassName = class_name;
    if (!record_call("setup", "RegisterClass", !!RegisterClassA(&klass))) return 0;
    for (i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) run_case(&cases[i]);
    printf("measure_complete cases=%u\n", i);
    return 0;
}
