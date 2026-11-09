/* Original, AI-assisted (Codex), LGPL-2.1-or-later.
 * Standard COM calls into our own class factory. Observe activation markers
 * through public APIs; no guessed native context-selection expectations. */
#define _WIN32_WINNT 0x0601
#define COBJMACROS
#define CONST_VTABLE
#include <windows.h>
#include <objbase.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define COUNT(a) (sizeof(a) / sizeof((a)[0]))
struct context_file { HANDLE context; WCHAR path[MAX_PATH]; BOOL owned; };
struct factory {
    IClassFactory iface;
    LONG refs, calls, client_tid;
    DWORD creator_tid;
    unsigned int scenario;
};
struct client_args {
    IStream *streams[2];
    HANDLE contexts[2];
    struct factory *factory;
    DWORD apartment;
    unsigned int scenario;
};
static LONG setup_errors, total_calls;

static void error_api(const char *operation, DWORD error)
{
    InterlockedIncrement(&setup_errors);
    printf("ERROR operation=%s code=%08lx\n", operation, error);
}
static BOOL api_hr(HRESULT hr, const char *operation)
{
    printf("API operation=%s hr=%08lx\n", operation, (DWORD)hr);
    if (FAILED(hr)) { error_api(operation, (DWORD)hr); return FALSE; }
    return TRUE;
}
static unsigned int query_markers(DWORD *errors)
{
    static const WCHAR *names[] = {L"Winum.Stub.Measure.A", L"Winum.Stub.Measure.B"};
    unsigned int mask = 0, i;
    for (i = 0; i < COUNT(names); i++)
    {
        ACTCTX_SECTION_KEYED_DATA data = {0};
        BOOL ret;
        data.cbSize = sizeof(data);
        SetLastError(0);
        ret = FindActCtxSectionStringW(0, NULL, ACTIVATION_CONTEXT_SECTION_WINDOW_CLASS_REDIRECTION, names[i], &data);
        errors[i] = ret ? 0 : GetLastError();
        if (ret) mask |= 1u << i;
    }
    return mask;
}
static unsigned int observe(unsigned int scenario, const char *stage)
{
    DWORD errors[2];
    unsigned int mask = query_markers(errors);
    HANDLE current = NULL;
    BOOL ret;
    APTTYPE apartment;
    APTTYPEQUALIFIER qualifier;
    HRESULT apartment_hr;
    ret = GetCurrentActCtx(&current);
    if (!ret) error_api("GetCurrentActCtx", GetLastError());
    apartment_hr = CoGetApartmentType(&apartment, &qualifier);
    if (FAILED(apartment_hr)) error_api("CoGetApartmentType", (DWORD)apartment_hr);
    printf("OBS scenario=%u stage=%s mask=%u error_a=%lu error_b=%lu current_query=%u current_nonnull=%u apartment_hr=%08lx apartment=%d qualifier=%d\n",
           scenario, stage, mask, errors[0], errors[1], ret, current != NULL, (DWORD)apartment_hr,
           SUCCEEDED(apartment_hr) ? (int)apartment : -1, SUCCEEDED(apartment_hr) ? (int)qualifier : -1);
    if (ret && current) ReleaseActCtx(current);
    return mask;
}
static ULONG WINAPI factory_addref(IClassFactory *iface)
{
    struct factory *object = (struct factory *)iface;
    return InterlockedIncrement(&object->refs);
}
static ULONG WINAPI factory_release(IClassFactory *iface)
{
    struct factory *object = (struct factory *)iface;
    ULONG refs = InterlockedDecrement(&object->refs);
    if (!refs) free(object);
    return refs;
}
static HRESULT WINAPI factory_query(IClassFactory *iface, REFIID iid, void **result)
{
    *result = NULL;
    if (!IsEqualIID(iid, &IID_IUnknown) && !IsEqualIID(iid, &IID_IClassFactory)) return E_NOINTERFACE;
    *result = iface;
    factory_addref(iface);
    return S_OK;
}
static HRESULT WINAPI factory_create(IClassFactory *iface, IUnknown *outer, REFIID iid, void **result)
{
    struct factory *object = (struct factory *)iface;
    LONG call = InterlockedIncrement(&object->calls);
    unsigned int mask;
    (void)outer;
    (void)iid;
    *result = NULL;
    mask = observe(object->scenario, "server_call");
    InterlockedIncrement(&total_calls);
    printf("SERVER scenario=%u call=%ld mask=%u thread_is_creator=%u thread_is_client=%u\n",
           object->scenario, call, mask, GetCurrentThreadId() == object->creator_tid,
           GetCurrentThreadId() == (DWORD)InterlockedCompareExchange(&object->client_tid, 0, 0));
    /* This deliberately unimplemented factory method is the probe itself.
     * A failed HRESULT requires no returned object to be marshaled. */
    if (GetCurrentThreadId() == (DWORD)InterlockedCompareExchange(&object->client_tid, 0, 0))
        error_api("cross_apartment_dispatch", ERROR_INVALID_DATA);
    return E_NOTIMPL;
}
static HRESULT WINAPI factory_lock(IClassFactory *iface, BOOL lock)
{
    (void)iface;
    (void)lock;
    return S_OK;
}
static const IClassFactoryVtbl factory_vtable = {
    factory_query, factory_addref, factory_release, factory_create, factory_lock
};

static BOOL push_context(HANDLE context, ULONG_PTR *cookie)
{
    if (!context) return TRUE;
    if (ActivateActCtx(context, cookie)) return TRUE;
    error_api("ActivateActCtx", GetLastError());
    return FALSE;
}
static void pop_context(HANDLE context, ULONG_PTR cookie)
{
    if (context && !DeactivateActCtx(0, cookie))
    {
        error_api("DeactivateActCtx", GetLastError());
        /* Continuing would contaminate subsequent observations. */
        fflush(stdout);
        ExitProcess(2);
    }
}
static BOOL make_context(unsigned int index, struct context_file *out)
{
    WCHAR directory[MAX_PATH];
    char xml[512];
    ACTCTXW request = {0};
    HANDLE file;
    DWORD length, written, error;
    ULONG_PTR cookie = 0;
    int count;
    BOOL ret, closed;
    out->context = INVALID_HANDLE_VALUE;
    out->path[0] = 0;
    out->owned = FALSE;
    length = GetTempPathW(COUNT(directory), directory);
    if (!length || length >= COUNT(directory)) { error_api("GetTempPathW", length ? ERROR_INSUFFICIENT_BUFFER : GetLastError()); return FALSE; }
    if (!GetTempFileNameW(directory, L"wsc", 0, out->path)) { error_api("GetTempFileNameW", GetLastError()); return FALSE; }
    out->owned = TRUE;
    count = snprintf(xml, sizeof(xml), "<assembly xmlns=\"urn:schemas-microsoft-com:asm.v1\" manifestVersion=\"1.0\">"
                     "<assemblyIdentity name=\"Winum.Stub.Marker.%u\" version=\"1.0.0.0\" type=\"win32\"/>"
                     "<file name=\"winum-stub-marker.dll\"><windowClass versioned=\"no\">"
                     "Winum.Stub.Measure.%c</windowClass></file></assembly>", index, 'A' + (int)index);
    if (count < 0 || (unsigned int)count >= sizeof(xml)) { error_api("format_manifest", ERROR_INSUFFICIENT_BUFFER); return FALSE; }
    file = CreateFileW(out->path, GENERIC_WRITE, 0, NULL, TRUNCATE_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE) { error_api("CreateFileW", GetLastError()); return FALSE; }
    ret = WriteFile(file, xml, count, &written, NULL);
    if (!ret || written != (DWORD)count) error_api("WriteFile", ret ? ERROR_WRITE_FAULT : GetLastError());
    closed = CloseHandle(file);
    if (!closed) error_api("CloseHandle", GetLastError());
    if (!ret || written != (DWORD)count || !closed) return FALSE;
    request.cbSize = sizeof(request);
    request.lpSource = out->path;
    out->context = CreateActCtxW(&request);
    error = out->context == INVALID_HANDLE_VALUE ? GetLastError() : 0;
    printf("CONTEXT marker=%c created=%u error=%lu\n", 'A' + (int)index, out->context != INVALID_HANDLE_VALUE, error);
    if (out->context == INVALID_HANDLE_VALUE) { error_api("CreateActCtxW", error); return FALSE; }
    if (!push_context(out->context, &cookie)) return FALSE;
    {
        DWORD errors[2];
        unsigned int active_mask = query_markers(errors), baseline;
        pop_context(out->context, cookie);
        baseline = query_markers(errors);
        printf("MARKER marker=%c active_mask=%u baseline_after=%u\n", 'A' + (int)index, active_mask, baseline);
        if (active_mask != (1u << index) || baseline)
        {
            error_api("marker_selfcheck", ERROR_INVALID_DATA);
            return FALSE;
        }
    }
    return TRUE;
}

static DWORD WINAPI client_thread(void *parameter)
{
    struct client_args *args = parameter;
    IClassFactory *proxies[2] = {NULL, NULL};
    HRESULT hr;
    unsigned int phase, stream;
    InterlockedExchange(&args->factory->client_tid, GetCurrentThreadId());
    if (!api_hr(CoInitializeEx(NULL, args->apartment), "client_CoInitializeEx")) return 1;
    observe(args->scenario, "client_before_unmarshal");
    for (stream = 0; stream < 2; stream++)
    {
        hr = CoGetInterfaceAndReleaseStream(args->streams[stream], &IID_IClassFactory, (void **)&proxies[stream]);
        args->streams[stream] = NULL; /* consumed even when unmarshaling fails */
        if (!api_hr(hr, "CoGetInterfaceAndReleaseStream")) goto done;
    }
    for (phase = 0; phase < 3; phase++)
    {
        HANDLE context = phase ? args->contexts[phase - 1] : NULL;
        ULONG_PTR cookie = 0;
        if (!push_context(context, &cookie)) break;
        for (stream = 0; stream < 2; stream++)
        {
            void *result = NULL;
            unsigned int before = observe(args->scenario, "client_before_call");
            LONG calls_before = InterlockedCompareExchange(&args->factory->calls, 0, 0);
            unsigned int after;
            hr = IClassFactory_CreateInstance(proxies[stream], NULL, &IID_IUnknown, &result);
            after = observe(args->scenario, "client_after_call");
            printf("CALL scenario=%u phase=%u stream=%u hr=%08lx returned_object=%u before=%u after=%u callbacks=%ld\n",
                   args->scenario, phase, stream, (DWORD)hr, result != NULL, before, after,
                   InterlockedCompareExchange(&args->factory->calls, 0, 0) - calls_before);
            if (result) IUnknown_Release((IUnknown *)result);
            if (hr != E_NOTIMPL) error_api("probe_return", (DWORD)hr);
            if (InterlockedCompareExchange(&args->factory->calls, 0, 0) - calls_before != 1)
                error_api("probe_callback_count", ERROR_INVALID_DATA);
        }
        pop_context(context, cookie);
    }
done:
    for (stream = 0; stream < 2; stream++) if (proxies[stream]) IClassFactory_Release(proxies[stream]);
    observe(args->scenario, "client_before_uninitialize");
    CoUninitialize();
    return 0;
}
static DWORD WINAPI watchdog_thread(void *parameter)
{
    DWORD wait = WaitForSingleObject((HANDLE)parameter, 30000);
    if (wait == WAIT_OBJECT_0) return 0;
    /* No stdio or DLL shutdown: either can wait for a lock on a stuck thread.
     * Terminate only this dedicated measurement process, never a worker
     * thread while leaving its apartment/object state alive. */
    TerminateProcess(GetCurrentProcess(), 2);
    ExitProcess(2); /* fallback if process termination unexpectedly fails */
}
static void wait_client(HANDLE thread)
{
    for (;;)
    {
        DWORD wait = MsgWaitForMultipleObjects(1, &thread, FALSE, INFINITE, QS_ALLINPUT);
        MSG message;
        if (wait == WAIT_OBJECT_0) return;
        if (wait == WAIT_FAILED) { error_api("MsgWaitForMultipleObjects", GetLastError()); ExitProcess(2); }
        while (PeekMessageW(&message, NULL, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&message);
            DispatchMessageW(&message);
        }
    }
}
static BOOL run_scenario(unsigned int id, DWORD server_apartment, unsigned int variant, HANDLE *contexts)
{
    struct client_args args = {0};
    struct factory *object = NULL;
    HANDLE thread = NULL, construction = variant ? contexts[0] : NULL;
    ULONG_PTR cookie = 0;
    HANDLE watchdog, cancel;
    unsigned int stream;
    LONG errors_before = setup_errors;
    LARGE_INTEGER zero = {{0}};
    HRESULT hr;
    printf("SCENARIO id=%u server=%s variant=%u construction=%s first=%s second=%s\n", id,
           server_apartment == COINIT_APARTMENTTHREADED ? "STA" : "MTA", variant,
           construction ? "A" : "none", variant == 1 ? "A" : variant == 2 ? "B" : "none", variant ? "A" : "none");
    cancel = CreateEventW(NULL, TRUE, FALSE, NULL);
    if (!cancel) { error_api("CreateEventW", GetLastError()); return FALSE; }
    watchdog = CreateThread(NULL, 0, watchdog_thread, cancel, 0, NULL);
    if (!watchdog) { error_api("watchdog_CreateThread", GetLastError()); CloseHandle(cancel); return FALSE; }
    if (!api_hr(CoInitializeEx(NULL, server_apartment), "server_CoInitializeEx")) goto finish;
    args.scenario = id;
    args.apartment = server_apartment == COINIT_APARTMENTTHREADED ? COINIT_MULTITHREADED : COINIT_APARTMENTTHREADED;
    args.contexts[0] = contexts[0]; args.contexts[1] = contexts[1];
    if (!push_context(construction, &cookie)) goto done;
    object = calloc(1, sizeof(*object));
    if (object)
    {
        object->iface.lpVtbl = &factory_vtable;
        object->refs = 1; object->creator_tid = GetCurrentThreadId(); object->scenario = id;
        observe(id, "object_construction");
    }
    pop_context(construction, cookie);
    if (!object) { error_api("calloc", ERROR_NOT_ENOUGH_MEMORY); goto done; }
    args.factory = object;
    for (stream = 0; stream < 2; stream++)
    {
        HANDLE context = variant ? contexts[variant == 2 && !stream ? 1 : 0] : NULL;
        if (!push_context(context, &cookie)) goto done;
        observe(id, stream ? "marshal_second" : "marshal_first");
        hr = CoMarshalInterThreadInterfaceInStream(&IID_IClassFactory, (IUnknown *)&object->iface, &args.streams[stream]);
        pop_context(context, cookie);
        if (!api_hr(hr, "CoMarshalInterThreadInterfaceInStream")) goto done;
    }
    observe(id, "server_before_client");
    thread = CreateThread(NULL, 0, client_thread, &args, 0, NULL);
    if (!thread) { error_api("CreateThread", GetLastError()); goto done; }
    wait_client(thread);
    if (!CloseHandle(thread)) error_api("client_CloseHandle", GetLastError());
    observe(id, "server_after_client");
done:
    for (stream = 0; stream < 2; stream++) if (args.streams[stream])
    {
        hr = IStream_Seek(args.streams[stream], zero, STREAM_SEEK_SET, NULL);
        if (api_hr(hr, "cleanup_stream_seek")) api_hr(CoReleaseMarshalData(args.streams[stream]), "CoReleaseMarshalData");
        IStream_Release(args.streams[stream]);
    }
    if (object)
    {
        api_hr(CoDisconnectObject((IUnknown *)&object->iface, 0), "CoDisconnectObject");
        printf("SCENARIO_CALLS id=%u calls=%ld\n", id, object->calls);
        factory_release(&object->iface);
    }
    CoUninitialize();
finish:
    if (!SetEvent(cancel)) { error_api("watchdog_SetEvent", GetLastError()); ExitProcess(2); }
    if (WaitForSingleObject(watchdog, INFINITE) != WAIT_OBJECT_0)
    { error_api("watchdog_join", GetLastError()); ExitProcess(2); }
    if (!CloseHandle(watchdog)) error_api("watchdog_CloseHandle", GetLastError());
    if (!CloseHandle(cancel)) error_api("cancel_CloseHandle", GetLastError());
    printf("SCENARIO_END id=%u setup_errors=%ld\n", id, setup_errors - errors_before);
    return setup_errors == errors_before;
}
int main(void)
{
    struct context_file files[2] = {{0}};
    HANDLE contexts[2];
    unsigned int apartment, variant, completed = 0;
    BOOL complete = TRUE;
    setvbuf(stdout, NULL, _IONBF, 0);
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
    printf("MEASURE topic=com_stub_actctx schema=1 bits=%u\n", (unsigned int)(8 * sizeof(void *)));
    files[0].context = files[1].context = INVALID_HANDLE_VALUE;
    if (!make_context(0, &files[0]) || !make_context(1, &files[1])) { complete = FALSE; goto done; }
    contexts[0] = files[0].context; contexts[1] = files[1].context;
    for (apartment = 0; apartment < 2 && complete; apartment++)
        for (variant = 0; variant < 4 && complete; variant++)
        {
            /* Repeat the no-context control after context-bearing calls. */
            complete = run_scenario(apartment * 4 + variant,
                                   apartment ? COINIT_MULTITHREADED : COINIT_APARTMENTTHREADED,
                                   variant == 3 ? 0 : variant, contexts);
            if (complete) completed++;
        }
done:
    for (variant = 0; variant < 2; variant++)
    {
        if (files[variant].context != INVALID_HANDLE_VALUE) ReleaseActCtx(files[variant].context);
        if (files[variant].owned && !DeleteFileW(files[variant].path)) error_api("DeleteFileW", GetLastError());
    }
    printf("SUMMARY scenarios=%u calls=%ld setup_errors=%ld complete=%u expectations=none\n",
           completed, total_calls, setup_errors, complete);
    return setup_errors || !complete ? 1 : 0;
}
