/* Original MEASURE program, AI-assisted (Codex), LGPL-2.1-or-later. */

#define COBJMACROS
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <ole2.h>
#include <stdio.h>
#include <wchar.h>

#define TIMEOUT_MS 15000
#define LAST_ERROR_SEED 0xdeadbeef

struct target
{
    IDropTarget iface;
    LONG refs;
    LONG releases;
};

struct windows
{
    HWND registered;
    HWND unregistered;
    BOOL ready;
};

struct owner
{
    struct windows *windows;
    HANDLE ready;
    HANDLE proceed;
    const char *scenario;
};

static HRESULT WINAPI query_interface(IDropTarget *iface, REFIID iid, void **out)
{
    *out = NULL;
    if (!IsEqualIID(iid, &IID_IUnknown) && !IsEqualIID(iid, &IID_IDropTarget)) return E_NOINTERFACE;
    *out = iface;
    IDropTarget_AddRef(iface);
    return S_OK;
}

static ULONG WINAPI add_ref(IDropTarget *iface)
{
    struct target *target = CONTAINING_RECORD(iface, struct target, iface);
    return InterlockedIncrement(&target->refs);
}

static ULONG WINAPI release(IDropTarget *iface)
{
    struct target *target = CONTAINING_RECORD(iface, struct target, iface);
    InterlockedIncrement(&target->releases);
    return InterlockedDecrement(&target->refs);
}

static HRESULT WINAPI drag_over(IDropTarget *iface, DWORD keys, POINTL point, DWORD *effect)
{
    *effect = DROPEFFECT_NONE;
    return S_OK;
}

static HRESULT WINAPI drag_enter(IDropTarget *iface, IDataObject *data, DWORD keys, POINTL point, DWORD *effect)
{
    return drag_over(iface, keys, point, effect);
}

static HRESULT WINAPI drag_leave(IDropTarget *iface)
{
    return S_OK;
}

static IDropTargetVtbl target_vtbl =
{
    query_interface, add_ref, release, drag_enter, drag_over, drag_leave, drag_enter
};

static HRESULT measure_call(const char *scenario, const char *operation, HWND hwnd, IDropTarget *target)
{
    HRESULT hr;
    DWORD last_error;

    SetLastError(LAST_ERROR_SEED);
    hr = target ? RegisterDragDrop(hwnd, target) : RevokeDragDrop(hwnd);
    last_error = GetLastError();
    printf("case=%s operation=%s hr=0x%08lx last_error=0x%08lx\n",
           scenario, operation, (unsigned long)hr, (unsigned long)last_error);
    return hr;
}

static void snapshot(const char *scenario, const char *step, struct target *target)
{
    LONG refs = InterlockedCompareExchange(&target->refs, 0, 0);
    LONG releases = InterlockedCompareExchange(&target->releases, 0, 0);

    printf("case=%s step=%s refs=%ld releases=%ld\n", scenario, step, refs, releases);
}

static DWORD wait_pumping(DWORD count, const HANDLE *handles)
{
    ULONGLONG end = GetTickCount64() + TIMEOUT_MS;
    DWORD result;
    MSG msg;

    for (;;)
    {
        ULONGLONG now = GetTickCount64();
        if (now >= end) return WAIT_TIMEOUT;
        result = MsgWaitForMultipleObjects(count, handles, FALSE, (DWORD)(end - now), QS_ALLINPUT);
        if (result != WAIT_OBJECT_0 + count) return result;
        while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT) return WAIT_FAILED;
            DispatchMessageW(&msg);
        }
    }
}

static DWORD WINAPI owner_main(void *arg)
{
    struct owner *owner = arg;
    struct windows *windows = owner->windows;
    struct target target = {{&target_vtbl}, 1, 0};
    DWORD result, code = 1;
    HRESULT hr = OleInitialize(NULL);

    if (FAILED(hr))
    {
        printf("case=%s setup=owner_ole_initialize hr=0x%08lx\n", owner->scenario, (unsigned long)hr);
        return 1;
    }

    windows->registered = CreateWindowExW(0, L"STATIC", L"", WS_POPUP, 0, 0, 1, 1,
                                          NULL, NULL, GetModuleHandleW(NULL), NULL);
    windows->unregistered = CreateWindowExW(0, L"STATIC", L"", WS_POPUP, 0, 0, 1, 1,
                                            NULL, NULL, GetModuleHandleW(NULL), NULL);
    if (!windows->registered || !windows->unregistered)
    {
        printf("case=%s setup=create_windows_failed error=%lu\n", owner->scenario, GetLastError());
        goto done;
    }
    hr = measure_call(owner->scenario, "owner_register", windows->registered, &target.iface);
    if (hr != S_OK) goto done;
    snapshot(owner->scenario, "before_caller", &target);

    windows->ready = TRUE;
    if (!SetEvent(owner->ready)) goto done;
    result = wait_pumping(1, &owner->proceed);
    if (result != WAIT_OBJECT_0)
    {
        printf("case=%s setup=owner_wait_failed result=0x%08lx\n", owner->scenario, result);
        RevokeDragDrop(windows->registered);
        goto done;
    }
    snapshot(owner->scenario, "after_caller", &target);
    measure_call(owner->scenario, "owner_register_again", windows->registered, &target.iface);
    measure_call(owner->scenario, "owner_revoke", windows->registered, NULL);
    snapshot(owner->scenario, "after_owner_cleanup", &target);
    code = 0;

done:
    if (windows->unregistered) DestroyWindow(windows->unregistered);
    if (windows->registered) DestroyWindow(windows->registered);
    OleUninitialize();
    return code;
}

static struct windows *map_windows(HANDLE mapping)
{
    return MapViewOfFile(mapping, FILE_MAP_WRITE, 0, 0, sizeof(struct windows));
}

static DWORD child_main(HANDLE mapping, HANDLE ready, HANDLE proceed)
{
    struct owner owner = {map_windows(mapping), ready, proceed, "cross_process"};
    DWORD code;

    if (!owner.windows)
    {
        printf("case=cross_process setup=child_map_failed error=%lu\n", GetLastError());
        return 1;
    }
    code = owner_main(&owner);
    UnmapViewOfFile(owner.windows);
    return code;
}

static HANDLE start_child(HANDLE mapping, HANDLE ready, HANDLE proceed)
{
    WCHAR exe[MAX_PATH], command[MAX_PATH + 96];
    STARTUPINFOW startup = {sizeof(startup)};
    PROCESS_INFORMATION process;
    DWORD len = GetModuleFileNameW(NULL, exe, ARRAYSIZE(exe));

    if (!len || len >= ARRAYSIZE(exe)) return NULL;
    if (swprintf(command, ARRAYSIZE(command), L"\"%ls\" --owner %llu %llu %llu", exe,
                 (unsigned long long)(ULONG_PTR)mapping, (unsigned long long)(ULONG_PTR)ready,
                 (unsigned long long)(ULONG_PTR)proceed) < 0)
        return NULL;
    startup.dwFlags = STARTF_USESTDHANDLES;
    startup.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    startup.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    startup.hStdError = GetStdHandle(STD_ERROR_HANDLE);
    /* Put the owner's checkpoints in the same report as the caller's. */
    if (!CreateProcessW(exe, command, NULL, NULL, TRUE, 0, NULL, NULL, &startup, &process)) return NULL;
    CloseHandle(process.hThread);
    return process.hProcess;
}

static BOOL run_scenario(BOOL cross_process)
{
    SECURITY_ATTRIBUTES inherit = {sizeof(inherit), NULL, cross_process};
    struct windows local = {0};
    struct owner owner = {&local, NULL, NULL, cross_process ? "cross_process" : "same_process_other_thread"};
    struct target caller = {{&target_vtbl}, 1, 0};
    HANDLE mapping = NULL, worker = NULL, handles[2];
    DWORD pid, tid, result, worker_code = 1;
    BOOL passed = FALSE;
    HRESULT hr;

    if (cross_process)
    {
        mapping = CreateFileMappingW(INVALID_HANDLE_VALUE, &inherit, PAGE_READWRITE, 0, sizeof(local), NULL);
        if (!mapping || !(owner.windows = map_windows(mapping))) goto setup_failed;
    }
    owner.ready = CreateEventW(&inherit, TRUE, FALSE, NULL);
    owner.proceed = CreateEventW(&inherit, TRUE, FALSE, NULL);
    if (!owner.ready || !owner.proceed) goto setup_failed;
    worker = cross_process ? start_child(mapping, owner.ready, owner.proceed)
                           : CreateThread(NULL, 0, owner_main, &owner, 0, NULL);
    if (!worker) goto setup_failed;

    handles[0] = owner.ready;
    handles[1] = worker;
    result = wait_pumping(ARRAYSIZE(handles), handles);
    if (result != WAIT_OBJECT_0 || !owner.windows->ready)
    {
        printf("case=%s setup=caller_wait_failed result=0x%08lx\n", owner.scenario, result);
        goto join;
    }
    tid = GetWindowThreadProcessId(owner.windows->registered, &pid);
    if (!tid || tid == GetCurrentThreadId() || (pid != GetCurrentProcessId()) != cross_process)
    {
        printf("case=%s setup=unexpected_window_owner\n", owner.scenario);
        goto join;
    }
    printf("case=%s owner=verified\n", owner.scenario);
    measure_call(owner.scenario, "caller_revoke", owner.windows->registered, NULL);
    if (cross_process)
    {
        hr = measure_call(owner.scenario, "caller_register_unregistered",
                          owner.windows->unregistered, &caller.iface);
        if (SUCCEEDED(hr))
            measure_call(owner.scenario, "caller_revoke_own_registration",
                         owner.windows->unregistered, NULL);
    }
    passed = TRUE;

join:
    SetEvent(owner.proceed);
    result = wait_pumping(1, &worker);
    if (result != WAIT_OBJECT_0)
    {
        printf("case=%s setup=worker_join_failed result=0x%08lx\n", owner.scenario, result);
        ExitProcess(1);
    }
    if (cross_process) GetExitCodeProcess(worker, &worker_code);
    else GetExitCodeThread(worker, &worker_code);
    if (worker_code)
    {
        printf("case=%s setup=worker_exit code=0x%08lx\n", owner.scenario, worker_code);
        passed = FALSE;
    }
    goto done;

setup_failed:
    printf("case=%s setup=caller_create_failed error=%lu\n", owner.scenario, GetLastError());
done:
    if (worker) CloseHandle(worker);
    if (owner.proceed) CloseHandle(owner.proceed);
    if (owner.ready) CloseHandle(owner.ready);
    if (mapping)
    {
        if (owner.windows && owner.windows != &local) UnmapViewOfFile(owner.windows);
        CloseHandle(mapping);
    }
    return passed;
}

int wmain(int argc, WCHAR **argv)
{
    BOOL cross_process, same_process;
    HRESULT hr;

    setvbuf(stdout, NULL, _IONBF, 0);
    if (GetProcAddress(GetModuleHandleW(L"ntdll.dll"), "wine_get_version"))
    {
        puts("setup=native_windows_required");
        return 1;
    }
    if (argc == 5 && !wcscmp(argv[1], L"--owner"))
        return child_main((HANDLE)(ULONG_PTR)wcstoull(argv[2], NULL, 10),
                          (HANDLE)(ULONG_PTR)wcstoull(argv[3], NULL, 10),
                          (HANDLE)(ULONG_PTR)wcstoull(argv[4], NULL, 10));
    if (argc != 1) return 1;

    printf("measure=ole32-revokedragdrop-cross-process bits=%u last_error_seed=0x%08x\n",
           (unsigned int)(sizeof(void *) * 8), LAST_ERROR_SEED);
    hr = OleInitialize(NULL);
    if (FAILED(hr))
    {
        printf("setup=caller_ole_initialize hr=0x%08lx\n", (unsigned long)hr);
        return 1;
    }
    cross_process = run_scenario(TRUE);
    same_process = run_scenario(FALSE);
    OleUninitialize();
    printf("measure_complete=%d\n", cross_process && same_process);
    return cross_process && same_process ? 0 : 1;
}
