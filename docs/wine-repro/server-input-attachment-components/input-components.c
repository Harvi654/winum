/* Original, AI-assisted (Codex), LGPL-2.1-or-later.
 * Public-API MEASURE of input attachment graphs and implicit child links.
 * No expected Windows values, real input, vendor windows or registry writes.
 */
#include "../batch3-measure/measure-support.h"

#define THREADS 4
#define COMMAND (WM_APP + 31)
enum operation { FOCUS, ACTIVATE, CAPTURE, RELEASE, CHILD_CREATE, CHILD_FOCUS,
                 CHILD_UNPARENT, CHILD_DESTROY, STOP };
struct worker
{
    HANDLE thread, ready;
    DWORD tid;
    HWND window, children[2];
    unsigned int index;
};
struct edge { unsigned int from, to; };
struct graph
{
    const char *name;
    unsigned int count, cuts;
    struct edge edges[5], remove[3];
    int exit_worker;
    unsigned int other_anchor;
};
static struct worker workers[THREADS];
static const char *scenario;

static const struct graph graphs[] =
{
    { "isolated-nonedge", 0, 1, {{0,0}}, {{0,1}}, -1, 1 },
    { "pair", 1, 1, {{0,1}}, {{0,1}}, -1, 1 },
    { "chain", 2, 1, {{0,1},{1,2}}, {{1,2}}, -1, 2 },
    { "bridge", 3, 1, {{0,1},{1,2},{2,3}}, {{1,2}}, -1, 3 },
    { "cycle", 3, 2, {{0,1},{1,2},{2,0}}, {{0,1},{1,2}}, -1, 2 },
    { "duplicate", 2, 3, {{0,1},{0,1}}, {{0,1},{0,1},{0,1}}, -1, 1 },
    { "opposite-edges", 2, 2, {{0,1},{1,0}}, {{0,1},{1,0}}, -1, 1 },
    { "connected-nonedge", 2, 1, {{0,1},{1,2}}, {{0,2}}, -1, 2 },
    { "self-detach", 1, 1, {{0,1}}, {{0,0}}, -1, 1 },
    { "exit-neighbor", 2, 0, {{0,1},{1,2}}, {{0,0}}, 2, 2 },
};

static const char *role(HWND window)
{
    static const char *names[] = { "A", "B", "C", "D" };
    unsigned int i;
    if (!window) return "none";
    for (i = 0; i < THREADS; ++i)
        if (window == workers[i].window) return names[i];
    if (window == workers[2].children[0]) return "child-A";
    if (window == workers[2].children[1]) return "child-B";
    return "other";
}

static void snapshot(const char *stage)
{
    unsigned int i;
    for (i = 0; i < THREADS; ++i)
    {
        GUITHREADINFO info;
        DWORD error;
        BOOL ret;
        if (!workers[i].thread) continue;
        memset(&info, 0, sizeof(info));
        info.cbSize = sizeof(info);
        SetLastError(0xdeadbeef);
        ret = GetGUIThreadInfo(workers[i].tid, &info);
        error = GetLastError();
        printf("STATE case=%s stage=%s observer=%c ok=%d error=%08lx "
               "active=%s focus=%s capture=%s foreground=%s flags=%08lx\n",
               scenario, stage, 'A' + i, ret, error, role(info.hwndActive),
               role(info.hwndFocus), role(info.hwndCapture),
               role(GetForegroundWindow()), info.flags);
        if (!ret) b3_error("GetGUIThreadInfo_live_worker", error);
    }
}

static LRESULT CALLBACK window_proc(HWND hwnd, UINT message, WPARAM wp, LPARAM lp)
{
    struct worker *w = (struct worker *)GetWindowLongPtrW(hwnd, GWLP_USERDATA);
    HWND previous = NULL;
    DWORD error;
    BOOL ret = TRUE;
    int foreground_ret = -1;
    if (message == WM_NCCREATE)
    {
        const CREATESTRUCTW *cs = (const CREATESTRUCTW *)lp;
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)cs->lpCreateParams);
    }
    if (message != COMMAND) return DefWindowProcW(hwnd, message, wp, lp);
    SetLastError(0xdeadbeef);
    switch (wp)
    {
    case FOCUS:
        previous = SetFocus(hwnd);
        error = GetLastError();
        ret = GetFocus() == hwnd;
        break;
    case ACTIVATE:
        foreground_ret = SetForegroundWindow(hwnd);
        error = GetLastError();
        printf("FOREGROUND case=%s worker=%c ret=%d error=%08lx\n",
               scenario, 'A' + w->index, foreground_ret, error);
        SetLastError(0xdeadbeef);
        previous = SetActiveWindow(hwnd);
        error = GetLastError();
        printf("ACTIVE case=%s worker=%c previous=%s error=%08lx current=%s\n",
               scenario, 'A' + w->index, role(previous), error, role(GetActiveWindow()));
        SetLastError(0xdeadbeef);
        previous = SetFocus(hwnd);
        error = GetLastError();
        ret = GetActiveWindow() == hwnd && GetFocus() == hwnd;
        break;
    case CAPTURE:
        previous = SetCapture(hwnd);
        error = GetLastError();
        ret = GetCapture() == hwnd;
        break;
    case RELEASE:
        ret = ReleaseCapture();
        error = GetLastError();
        break;
    case CHILD_CREATE:
        if (lp < 0 || lp > 1 || w->index != 2) return FALSE;
        w->children[lp] = CreateWindowExW(0, L"WinumAttachmentMeasure", L"child",
                WS_CHILD | WS_VISIBLE, 5, 5, 50, 30, workers[lp].window,
                NULL, GetModuleHandleW(NULL), NULL);
        error = GetLastError();
        ret = w->children[lp] != NULL;
        break;
    case CHILD_FOCUS:
        previous = SetFocus(w->children[0]);
        error = GetLastError();
        ret = GetFocus() == w->children[0];
        break;
    case CHILD_UNPARENT:
        previous = SetParent(w->children[1], NULL);
        error = GetLastError();
        ret = previous != NULL;
        break;
    case CHILD_DESTROY:
        ret = DestroyWindow(w->children[1]);
        error = GetLastError();
        w->children[1] = NULL;
        break;
    case STOP: PostQuitMessage(0); return TRUE;
    default: return FALSE;
    }
    printf("API case=%s worker=%c operation=%u arg=%ld operation_ok=%d previous=%s "
           "error=%08lx local_active=%s local_focus=%s local_capture=%s\n",
           scenario, 'A' + w->index, (unsigned int)wp, (long)lp, ret,
           role(previous), error, role(GetActiveWindow()), role(GetFocus()),
           role(GetCapture()));
    if (!ret && (wp == CHILD_CREATE || wp == CHILD_UNPARENT || wp == CHILD_DESTROY))
        b3_error("OwnChildOperation", error);
    return ret;
}

static DWORD WINAPI worker_main(void *arg)
{
    struct worker *w = arg;
    MSG msg;
    int ret;
    w->window = CreateWindowExW(0, L"WinumAttachmentMeasure", L"Input attachment MEASURE",
            WS_OVERLAPPEDWINDOW | WS_VISIBLE, 20 + 130 * w->index, 30,
            120, 140, NULL, NULL, GetModuleHandleW(NULL), w);
    if (!w->window) b3_error("CreateOwnWindow", GetLastError());
    if (!SetEvent(w->ready)) b3_error("SignalReady", GetLastError());
    if (!w->window) return 1;
    while ((ret = GetMessageW(&msg, NULL, 0, 0)) > 0)
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    if (ret < 0) b3_error("GetMessageW", GetLastError());
    if (w->children[1]) DestroyWindow(w->children[1]);
    if (w->children[0]) DestroyWindow(w->children[0]);
    DestroyWindow(w->window);
    return 0;
}

static BOOL command(unsigned int worker, enum operation op, LPARAM arg)
{
    DWORD_PTR result;
    if (!SendMessageTimeoutW(workers[worker].window, COMMAND, op, arg,
                             SMTO_ABORTIFHUNG, 5000, &result))
    {
        b3_error("SendOwnCommand", GetLastError());
        return FALSE;
    }
    /* A NULL previous focus/capture is not a failed operation. Only the
     * fixture's own create/unparent/destroy results are prerequisites;
     * focus/activation/capture outcomes themselves are measurements. */
    if ((op == CHILD_CREATE || op == CHILD_UNPARENT || op == CHILD_DESTROY) && !result)
        return FALSE;
    return TRUE;
}

static void stop_worker(unsigned int i)
{
    if (!workers[i].thread) return;
    if (WaitForSingleObject(workers[i].thread, 0) == WAIT_TIMEOUT)
        command(i, STOP, 0);
    if (WaitForSingleObject(workers[i].thread, 7000) != WAIT_OBJECT_0)
    {
        /* Do not terminate an individual thread holding USER locks. */
        b3_error("JoinOwnWorker", ERROR_TIMEOUT);
        TerminateProcess(GetCurrentProcess(), 2);
    }
    CloseHandle(workers[i].thread);
    CloseHandle(workers[i].ready);
    workers[i].thread = workers[i].ready = NULL;
    workers[i].window = NULL;
}

static BOOL start_workers(void)
{
    unsigned int i;
    memset(workers, 0, sizeof(workers));
    for (i = 0; i < THREADS; ++i)
    {
        workers[i].index = i;
        workers[i].ready = CreateEventW(NULL, TRUE, FALSE, NULL);
        if (!workers[i].ready) { b3_error("CreateReady", GetLastError()); return FALSE; }
        workers[i].thread = CreateThread(NULL, 0, worker_main, &workers[i], 0, &workers[i].tid);
        if (!workers[i].thread)
        {
            b3_error("CreateOwnWorker", GetLastError());
            CloseHandle(workers[i].ready);
            workers[i].ready = NULL;
            return FALSE;
        }
        if (WaitForSingleObject(workers[i].ready, 7000) != WAIT_OBJECT_0 || !workers[i].window)
        { b3_error("WaitOwnWorker", ERROR_TIMEOUT); return FALSE; }
    }
    return TRUE;
}

static BOOL edge_call(unsigned int from, unsigned int to, BOOL attach)
{
    DWORD error;
    BOOL ret;
    SetLastError(0xdeadbeef);
    ret = AttachThreadInput(workers[from].tid, workers[to].tid, attach);
    error = GetLastError();
    printf("EDGE case=%s from=%c to=%c attach=%d ret=%d error=%08lx\n",
           scenario, 'A' + from, 'A' + to, attach, ret, error);
    return ret;
}

static BOOL probe_focus(const char *stage)
{
    unsigned int i;
    char label[64];
    for (i = 0; i < THREADS; ++i)
    {
        if (!workers[i].thread) continue;
        if (!command(i, FOCUS, 0)) return FALSE;
        snprintf(label, sizeof(label), "%s-probe-%c", stage, 'A' + i);
        snapshot(label);
    }
    return TRUE;
}

static BOOL graph_case(const struct graph *g, BOOL reverse, unsigned int anchor)
{
    char name[96], stage[40];
    unsigned int i;
    BOOL reached_end = FALSE;
    LONG errors_before = b3_errors;
    snprintf(name, sizeof(name), "%s-reverse%d-anchor%c", g->name, reverse, 'A' + anchor);
    scenario = name;
    printf("CASE name=%s kind=explicit\n", scenario);
    if (!start_workers()) goto cleanup;
    if (!command(anchor, ACTIVATE, 0)) goto cleanup;
    snapshot("isolated");
    for (i = 0; i < g->count; ++i)
    {
        if (!edge_call(g->edges[i].from, g->edges[i].to, TRUE))
        { b3_error("EstablishOwnGraph", ERROR_INVALID_DATA); goto cleanup; }
    }
    snapshot("joined");
    if (!probe_focus("joined")) goto cleanup;
    for (i = 0; i < g->cuts; ++i)
    {
        if (!command(anchor, ACTIVATE, 0)) goto cleanup;
        if (!command(anchor, CAPTURE, 0)) goto cleanup;
        snprintf(stage, sizeof(stage), "cut%u-before", i);
        snapshot(stage);
        edge_call(reverse ? g->remove[i].to : g->remove[i].from,
                  reverse ? g->remove[i].from : g->remove[i].to, FALSE);
        snprintf(stage, sizeof(stage), "cut%u-passive", i);
        snapshot(stage);
        if (!command(anchor, RELEASE, 0)) goto cleanup;
        snprintf(stage, sizeof(stage), "cut%u", i);
        if (!probe_focus(stage)) goto cleanup;
    }
    if (g->exit_worker >= 0)
    {
        if (!command(anchor, ACTIVATE, 0)) goto cleanup;
        snapshot("exit-before");
        stop_worker(g->exit_worker);
        snapshot("exit-passive");
        if (!probe_focus("exit")) goto cleanup;
    }
    reached_end = TRUE;
cleanup:
    /* Stopping all own threads also removes their explicit attachments. */
    for (i = THREADS; i-- > 0;) stop_worker(i);
    reached_end = reached_end && b3_errors == errors_before;
    printf("END case=%s complete=%d\n", scenario, reached_end);
    return reached_end;
}

static BOOL child_case(BOOL late, BOOL unparent)
{
    char name[64];
    unsigned int i;
    BOOL reached_end = FALSE;
    LONG errors_before = b3_errors;
    snprintf(name, sizeof(name), "children-late%d-unparent%d", late, unparent);
    scenario = name;
    printf("CASE name=%s kind=implicit-same-process\n", scenario);
    if (!start_workers()) goto cleanup;
    if (!command(0, ACTIVATE, 0)) goto cleanup;
    if (!command(2, CHILD_CREATE, 0)) goto cleanup;
    if (!late && !command(2, CHILD_FOCUS, 0)) goto cleanup;
    snapshot("one-child");
    if (!command(2, CHILD_CREATE, 1)) goto cleanup;
    if (!command(2, CHILD_FOCUS, 0)) goto cleanup;
    snapshot("two-children");
    if (unparent)
    {
        if (!command(2, CHILD_UNPARENT, 0)) goto cleanup;
        snapshot("unparent-passive");
        if (!command(2, CHILD_FOCUS, 0)) goto cleanup;
        snapshot("unparent-refocus");
    }
    if (!command(2, CHILD_DESTROY, 0)) goto cleanup;
    snapshot("destroy-passive");
    if (!command(2, CHILD_FOCUS, 0)) goto cleanup;
    snapshot("destroy-refocus");
    reached_end = TRUE;
cleanup:
    for (i = THREADS; i-- > 0;) stop_worker(i);
    reached_end = reached_end && b3_errors == errors_before;
    printf("END case=%s complete=%d\n", scenario, reached_end);
    return reached_end;
}

int main(void)
{
    WNDCLASSW wc;
    unsigned int i, reverse, late, unparent, completed = 0, returned = 0;
    if (!b3_start("server-input-attachment-components", 240000)) return 2;
    printf("LEGEND A,B,C,D=own-GUI-threads child-A,child-B=children-owned-by-C "
           "other=not-an-own-window operations=0:focus,1:activate,2:capture,3:release,"
           "4:create-child,5:focus-child-A,6:unparent-child-B,7:destroy-child-B\n");
    memset(&wc, 0, sizeof(wc));
    wc.lpfnWndProc = window_proc;
    wc.hInstance = GetModuleHandleW(NULL);
    wc.lpszClassName = L"WinumAttachmentMeasure";
    if (!RegisterClassW(&wc)) { b3_error("RegisterOwnClass", GetLastError()); return b3_finish(); }
    for (i = 0; i < ARRAYSIZE(graphs); ++i)
        for (reverse = 0; reverse < 2; ++reverse)
        {
            completed += graph_case(&graphs[i], reverse, 0);
            completed += graph_case(&graphs[i], reverse, graphs[i].other_anchor);
            returned += 2;
        }
    for (late = 0; late < 2; ++late)
        for (unparent = 0; unparent < 2; ++unparent)
        { completed += child_case(late, unparent); ++returned; }
    if (!UnregisterClassW(wc.lpszClassName, wc.hInstance)) b3_error("UnregisterOwnClass", GetLastError());
    printf("MATRIX scenarios_returned=%u complete_scenarios=%u planned_scenarios=44 assertions=none\n",
           returned, completed);
    return b3_finish();
}
