/* Case H models a frame painting its own band above a nested child. */
static void run_h(const char *name, BOOL all_children, BOOL pending, LONG ownership)
{
    const DWORD frame_style = WS_OVERLAPPEDWINDOW | WS_VISIBLE | WS_CLIPSIBLINGS;
    const DWORD container_style = WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;
    const UINT move = SWP_NOZORDER | SWP_NOACTIVATE;
    const UINT nccalc = move | SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE;
    UINT redraw = RDW_INVALIDATE | RDW_UPDATENOW | (all_children ? RDW_ALLCHILDREN : 0);
    struct pane top = {0}, container = {0}, nested = {0};
    struct worker worker = {0};
    struct geometry geometry = {1570, h_band, h_probes, ARRAYSIZE(h_probes)};
    RECT frame = {0, 0, 1920, 1040}, dirty = {250, 0, 350, 240};
    POINT origin = {0, 0};
    BOOL ok;
    DWORD error;
    int screen_width = GetSystemMetrics(SM_CXSCREEN), screen_height = GetSystemMetrics(SM_CYSCREEN);

    top.root = TRUE;
    top.red_band = h_band;
    top.color = RGB(64, 64, 64);
    container.color = RGB(0, 0, 255);
    nested.color = RGB(0, 128, 0);
    InitializeCriticalSection(&top.lock);
    InitializeCriticalSection(&container.lock);
    InitializeCriticalSection(&nested.lock);
    if (!record_call(name, "AdjustWindowRectEx", AdjustWindowRectEx(&frame, frame_style, FALSE, WS_EX_TOPMOST)))
        goto cleanup;
    top.hwnd = CreateWindowExA(WS_EX_TOPMOST, class_name, "Winum stale bits MEASURE H", frame_style,
                               0, 0, frame.right - frame.left, frame.bottom - frame.top,
                               NULL, NULL, GetModuleHandleA(NULL), &top);
    if (!record_call(name, "CreateWindowEx(frame H)", !!top.hwnd)) goto cleanup;
    if (!record_call(name, "ClientToScreen(frame H)", ClientToScreen(top.hwnd, &origin))) goto cleanup;
    if (geometry.x + h_width > screen_width - origin.x) geometry.x = screen_width - origin.x - h_width;
    if (geometry.x < h_width || origin.y + geometry.y + h_height > screen_height)
    {
        printf("case=%s error=desktop_too_small screen=%d,%d origin=%ld,%ld\n",
               name, screen_width, screen_height, origin.x, origin.y);
        goto cleanup;
    }
    printf("case=%s setup=H screen=%d,%d target=%d,%d size=%d,%d band=%d child_size=%d,%d\n",
           name, screen_width, screen_height, geometry.x, geometry.y, h_width, h_height, h_band,
           h_nested, h_nested);
    record_call(name, "UpdateWindow(frame H)", UpdateWindow(top.hwnd));
    drain(name);
    container.hwnd = CreateWindowExA(0, class_name, "", WS_CHILD | container_style, 0, 0, h_width,
                                     h_height, top.hwnd, NULL, GetModuleHandleA(NULL), &container);
    if (!record_call(name, "CreateWindowEx(container H)", !!container.hwnd)) goto cleanup;
    if (ownership)
    {
        if (!start_worker(&worker, name, &nested, container.hwnd, WS_VISIBLE, h_nested, h_nested))
            goto cleanup;
    }
    else
        nested.hwnd = CreateWindowExA(0, class_name, "", WS_CHILD | WS_VISIBLE, 0, 0, h_nested,
                                      h_nested, container.hwnd, NULL, GetModuleHandleA(NULL), &nested);
    if (!record_call(name, "CreateWindowEx(nested H)", !!nested.hwnd)) goto cleanup;
    pump_case(name, &worker);
    observe_layout(name, "initial", top.hwnd, &container, &geometry, &top, &nested);
    record_call(name, "ValidateRect(container H)", ValidateRect(container.hwnd, NULL));
    if (pending)
        record_call(name, "InvalidateRect(container H)", InvalidateRect(container.hwnd, &dirty, FALSE));
    if (ownership == pump_live) set_worker(name, &worker, pump_live);
    observe_layout(name, "before_redraw", top.hwnd, &container, &geometry, &top, &nested);

    ok = RedrawWindow(top.hwnd, NULL, NULL, redraw);
    error = ok ? 0 : GetLastError();
    printf("case=%s op=redraw_parent flags=%04x ok=%d last_error=%lu\n", name, redraw, ok, error);
    record_call(name, "RedrawWindow(frame H)", ok);
    observe_layout(name, "after_redraw", top.hwnd, &container, &geometry, &top, &nested);

    ok = SetWindowPos(container.hwnd, NULL, 0, 0, 0, 0, nccalc);
    error = ok ? 0 : GetLastError();
    printf("case=%s op=frame_changed flags=%04x ok=%d last_error=%lu\n", name, nccalc, ok, error);
    record_call(name, "SetWindowPos(frame_changed H)", ok);
    observe_layout(name, "after_frame", top.hwnd, &container, &geometry, &top, &nested);

    ok = SetWindowPos(container.hwnd, NULL, geometry.x, geometry.y, h_width, h_height, move);
    error = ok ? 0 : GetLastError();
    printf("case=%s op=move flags=%04x ok=%d last_error=%lu\n", name, move, ok, error);
    record_call(name, "SetWindowPos(move H)", ok);
    observe_layout(name, "after_move", top.hwnd, &container, &geometry, &top, &nested);

    if (ownership == pump_live) set_worker(name, &worker, hold_queue);
    pump_case(name, &worker);
    observe_layout(name, "after_pump", top.hwnd, &container, &geometry, &top, &nested);

cleanup:
    stop_and_close_worker(name, &worker);
    if (top.hwnd) record_call(name, "DestroyWindow(frame H)", DestroyWindow(top.hwnd));
    DeleteCriticalSection(&nested.lock);
    DeleteCriticalSection(&container.lock);
    DeleteCriticalSection(&top.lock);
}
