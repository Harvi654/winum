/* Original, AI-assisted (Codex), LGPL-2.1-or-later. */
#define _WIN32_WINNT 0x0602
#define COBJMACROS

#include <windows.h>
#include <shlobj.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

#define PROBE_MESSAGE (WM_APP + 1)
#define ACTION_MESSAGE (WM_APP + 2)
#define OBSERVE_MS 3000
#define HOLD_MS 1500

enum edit_action { ACTION_NONE, ACTION_REPLACE, ACTION_DISABLE, ACTION_DESTROY };
enum provider_kind { PROVIDER_PLAIN, PROVIDER_FTM, PROVIDER_PROXY };

struct observation
{
    LONG refs;
    DWORD ui_thread, owner_thread;
    HANDLE first_next, stop_probe, release_next;
    ULONG items, delay_ms;
    BOOL agile;
    LONG phase, active;
    LONG next_calls, next_ui, next_worker, next_owner, during_input, before_input;
    LONG returned, eof, resets, clones, largest_request;
    LONG probe_responses, probe_during_next, probe_timeouts;
    LONG apartment_sample, marshal_queries, agile_queries;
    LONG held_next, hold_result;
    HRESULT apartment_hr;
    APTTYPE apartment;
    APTTYPEQUALIFIER apartment_qualifier;
};

struct source
{
    IEnumString entry;
    LONG refs;
    SRWLOCK lock;
    IUnknown *marshaler;
    struct observation *obs;
    ULONG position;
};

struct probe
{
    HWND window, edit;
    struct observation *obs;
    IAutoComplete2 *autocomplete;
    enum edit_action action;
    HRESULT action_hr;
    BOOL action_received;
    LONG started_during_hold, returned_during_hold, next_at_start, next_at_return;
    ULONGLONG action_ms;
};

struct marshal_control
{
    IStream *stream;
    HRESULT init_hr, unmarshal_hr, next_hr;
    ULONG fetched;
};

struct sta_provider
{
    struct observation *obs;
    HANDLE ready, stop;
    IStream *stream;
    HRESULT marshal_hr;
};

static LONG read_count(LONG *counter)
{
    return InterlockedCompareExchange(counter, 0, 0);
}

static void release_observation(struct observation *obs)
{
    if (!InterlockedDecrement(&obs->refs))
    {
        if (obs->first_next) CloseHandle(obs->first_next);
        if (obs->stop_probe) CloseHandle(obs->stop_probe);
        if (obs->release_next) CloseHandle(obs->release_next);
        free(obs);
    }
}

static struct source *source_create(struct observation *obs, ULONG position);

static HRESULT WINAPI source_query_interface(IEnumString *iface, REFIID iid, void **out)
{
    struct source *source = CONTAINING_RECORD(iface, struct source, entry);

    *out = NULL;
    if (IsEqualIID(iid, &IID_IMarshal)) InterlockedIncrement(&source->obs->marshal_queries);
    if (IsEqualIID(iid, &IID_IAgileObject)) InterlockedIncrement(&source->obs->agile_queries);
    if (IsEqualIID(iid, &IID_IUnknown) || IsEqualIID(iid, &IID_IEnumString))
    {
        *out = iface;
        IEnumString_AddRef(iface);
        return S_OK;
    }
    if (source->marshaler && IsEqualIID(iid, &IID_IMarshal))
        return IUnknown_QueryInterface(source->marshaler, iid, out);
    return E_NOINTERFACE;
}

static ULONG WINAPI source_add_ref(IEnumString *iface)
{
    struct source *source = CONTAINING_RECORD(iface, struct source, entry);

    return InterlockedIncrement(&source->refs);
}

static ULONG WINAPI source_release(IEnumString *iface)
{
    struct source *source = CONTAINING_RECORD(iface, struct source, entry);
    ULONG refs = InterlockedDecrement(&source->refs);

    if (!refs)
    {
        if (source->marshaler) IUnknown_Release(source->marshaler);
        release_observation(source->obs);
        free(source);
    }
    return refs;
}

static HRESULT WINAPI source_next(IEnumString *iface, ULONG requested, LPOLESTR *out, ULONG *fetched)
{
    struct source *source = CONTAINING_RECORD(iface, struct source, entry);
    struct observation *obs = source->obs;
    ULONG count, index;
    LONG previous, phase;
    BOOL hold;
    WCHAR text[32];

    if (!out || (!fetched && requested != 1)) return E_POINTER;
    if (fetched) *fetched = 0;
    if (!InterlockedCompareExchange(&obs->apartment_sample, 1, 0))
    {
        obs->apartment = APTTYPE_CURRENT;
        obs->apartment_qualifier = APTTYPEQUALIFIER_NONE;
        obs->apartment_hr = CoGetApartmentType(&obs->apartment, &obs->apartment_qualifier);
        InterlockedExchange(&obs->apartment_sample, 2);
    }
    InterlockedIncrement(&obs->next_calls);
    InterlockedIncrement(GetCurrentThreadId() == obs->ui_thread ? &obs->next_ui : &obs->next_worker);
    if (GetCurrentThreadId() == obs->owner_thread) InterlockedIncrement(&obs->next_owner);
    phase = read_count(&obs->phase);
    if (phase == 0) InterlockedIncrement(&obs->before_input);
    if (phase == 1) InterlockedIncrement(&obs->during_input);
    while ((previous = read_count(&obs->largest_request)) < (LONG)requested)
    {
        if (InterlockedCompareExchange(&obs->largest_request, requested, previous) == previous) break;
    }

    AcquireSRWLockExclusive(&source->lock);
    count = min(requested, obs->items - source->position);
    for (index = 0; index < count; ++index)
    {
        size_t bytes;

        swprintf(text, sizeof(text) / sizeof(*text), L"%ls-sample-%04lu.dwg",
                 obs->release_next && (source->position + index) % 2 ? L"beta" : L"acad",
                 (unsigned long)(source->position + index));
        bytes = (wcslen(text) + 1) * sizeof(*text);
        out[index] = CoTaskMemAlloc(bytes);
        if (!out[index])
        {
            while (index)
            {
                CoTaskMemFree(out[--index]);
                out[index] = NULL;
            }
            ReleaseSRWLockExclusive(&source->lock);
            return E_OUTOFMEMORY;
        }
        memcpy(out[index], text, bytes);
    }
    source->position += count;
    ReleaseSRWLockExclusive(&source->lock);

    InterlockedIncrement(&obs->active);
    hold = obs->release_next && phase != 0 && !InterlockedCompareExchange(&obs->held_next, 1, 0);
    if (phase != 0) SetEvent(obs->first_next);
    if (hold)
    {
        InterlockedExchange(&obs->hold_result, WaitForSingleObject(obs->release_next, HOLD_MS));
        InterlockedExchange(&obs->held_next, 2);
    }
    else if (obs->delay_ms && count) Sleep(obs->delay_ms * count);
    if (fetched) *fetched = count;
    InterlockedExchangeAdd(&obs->returned, count);
    if (count < requested) InterlockedIncrement(&obs->eof);
    InterlockedDecrement(&obs->active);
    return count == requested ? S_OK : S_FALSE;
}

static HRESULT WINAPI source_skip(IEnumString *iface, ULONG requested)
{
    struct source *source = CONTAINING_RECORD(iface, struct source, entry);
    ULONG count;

    AcquireSRWLockExclusive(&source->lock);
    count = min(requested, source->obs->items - source->position);
    source->position += count;
    ReleaseSRWLockExclusive(&source->lock);
    return count == requested ? S_OK : S_FALSE;
}

static HRESULT WINAPI source_reset(IEnumString *iface)
{
    struct source *source = CONTAINING_RECORD(iface, struct source, entry);

    AcquireSRWLockExclusive(&source->lock);
    source->position = 0;
    ReleaseSRWLockExclusive(&source->lock);
    InterlockedIncrement(&source->obs->resets);
    return S_OK;
}

static HRESULT WINAPI source_clone(IEnumString *iface, IEnumString **out)
{
    struct source *source = CONTAINING_RECORD(iface, struct source, entry), *clone;
    ULONG position;

    *out = NULL;
    AcquireSRWLockShared(&source->lock);
    position = source->position;
    ReleaseSRWLockShared(&source->lock);
    clone = source_create(source->obs, position);
    if (!clone) return E_OUTOFMEMORY;
    *out = &clone->entry;
    InterlockedIncrement(&source->obs->clones);
    return S_OK;
}

static IEnumStringVtbl source_vtable =
{
    source_query_interface,
    source_add_ref,
    source_release,
    source_next,
    source_skip,
    source_reset,
    source_clone
};

static struct source *source_create(struct observation *obs, ULONG position)
{
    struct source *source = calloc(1, sizeof(*source));

    if (!source) return NULL;
    source->entry.lpVtbl = &source_vtable;
    source->refs = 1;
    source->obs = obs;
    source->position = position;
    InitializeSRWLock(&source->lock);
    InterlockedIncrement(&obs->refs);
    if (obs->agile && FAILED(CoCreateFreeThreadedMarshaler((IUnknown *)&source->entry, &source->marshaler)))
    {
        IEnumString_Release(&source->entry);
        return NULL;
    }
    return source;
}

static void perform_edit_action(struct probe *probe)
{
    struct observation *obs = probe->obs;
    ULONGLONG start = GetTickCount64();

    probe->action_received = TRUE;
    probe->started_during_hold = read_count(&obs->held_next) == 1;
    probe->next_at_start = read_count(&obs->next_calls);
    switch (probe->action)
    {
    case ACTION_REPLACE:
        SendMessageW(probe->edit, EM_SETSEL, 0, -1);
        SendMessageW(probe->edit, WM_CHAR, L'b', 1);
        probe->action_hr = S_OK;
        break;
    case ACTION_DISABLE:
        probe->action_hr = IAutoComplete2_Enable(probe->autocomplete, FALSE);
        break;
    case ACTION_DESTROY:
        probe->action_hr = DestroyWindow(probe->edit) ? S_OK : E_FAIL;
        break;
    case ACTION_NONE:
        break;
    }
    probe->action_ms = GetTickCount64() - start;
    probe->next_at_return = read_count(&obs->next_calls);
    probe->returned_during_hold = read_count(&obs->held_next) == 1;
    SetEvent(obs->release_next);
}

static LRESULT CALLBACK window_proc(HWND window, UINT message, WPARAM wparam, LPARAM lparam)
{
    struct probe *probe = (struct probe *)GetWindowLongPtrW(window, GWLP_USERDATA);

    if (message == PROBE_MESSAGE)
        return probe && read_count(&probe->obs->active) ? 2 : 1;
    if (message == ACTION_MESSAGE)
    {
        if (probe && probe->edit && probe->action != ACTION_NONE && !probe->action_received)
            perform_edit_action(probe);
        return 0;
    }
    return DefWindowProcW(window, message, wparam, lparam);
}

static DWORD WINAPI probe_thread(void *arg)
{
    struct probe *probe = arg;
    struct observation *obs = probe->obs;
    HANDLE events[] = { obs->stop_probe, obs->first_next };

    if (WaitForMultipleObjects(2, events, FALSE, INFINITE) != WAIT_OBJECT_0 + 1) return 0;
    if (probe->action != ACTION_NONE) PostMessageW(probe->window, ACTION_MESSAGE, 0, 0);
    while (WaitForSingleObject(obs->stop_probe, 0) == WAIT_TIMEOUT)
    {
        DWORD_PTR reply = 0;

        if (SendMessageTimeoutW(probe->window, PROBE_MESSAGE, 0, 0,
                                SMTO_ABORTIFHUNG | SMTO_BLOCK, 50, &reply))
        {
            InterlockedIncrement(&obs->probe_responses);
            if (reply == 2) InterlockedIncrement(&obs->probe_during_next);
        }
        else
            InterlockedIncrement(&obs->probe_timeouts);

        if (WaitForSingleObject(obs->stop_probe, 25) != WAIT_TIMEOUT) break;
    }
    return 0;
}

static void pump_messages(void)
{
    MSG message;

    while (PeekMessageW(&message, NULL, 0, 0, PM_REMOVE))
    {
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }
}

static void pump_until_signaled(HANDLE event)
{
    DWORD result;

    while ((result = MsgWaitForMultipleObjects(1, &event, FALSE, INFINITE, QS_ALLINPUT)) == WAIT_OBJECT_0 + 1)
        pump_messages();
    if (result == WAIT_OBJECT_0) return;
    printf("message_wait_error=%lu\n", (unsigned long)GetLastError());
    ExitProcess(2);
}

static DWORD WINAPI provider_sta_thread(void *arg)
{
    struct sta_provider *provider = arg;
    struct source *source;
    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    provider->marshal_hr = hr;
    if (FAILED(hr))
    {
        SetEvent(provider->ready);
        return 0;
    }
    provider->obs->owner_thread = GetCurrentThreadId();
    source = source_create(provider->obs, 0);
    provider->marshal_hr = source ? CoMarshalInterThreadInterfaceInStream(&IID_IEnumString,
          (IUnknown *)&source->entry, &provider->stream) : E_OUTOFMEMORY;
    SetEvent(provider->ready);

    pump_until_signaled(provider->stop);
    if (provider->stream)
    {
        CoReleaseMarshalData(provider->stream);
        IStream_Release(provider->stream);
    }
    if (source) IEnumString_Release(&source->entry);
    CoUninitialize();
    return 0;
}

static void print_apartment(struct observation *obs)
{
    if (read_count(&obs->apartment_sample) != 2)
    {
        puts("first_next_apartment_sample=unavailable");
        return;
    }
    printf("first_next_apartment_hr=%08lx type=%d qualifier=%d\n",
           (unsigned long)obs->apartment_hr, obs->apartment, obs->apartment_qualifier);
}

static DWORD WINAPI marshal_worker(void *arg)
{
    struct marshal_control *control = arg;
    IEnumString *enumerator;
    IStream *stream;
    LPOLESTR text = NULL;

    control->init_hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    if (FAILED(control->init_hr)) return 0;

    stream = control->stream;
    control->stream = NULL;
    control->unmarshal_hr = CoGetInterfaceAndReleaseStream(stream, &IID_IEnumString, (void **)&enumerator);
    if (SUCCEEDED(control->unmarshal_hr))
    {
        control->next_hr = IEnumString_Next(enumerator, 1, &text, &control->fetched);
        CoTaskMemFree(text);
        IEnumString_Release(enumerator);
    }
    CoUninitialize();
    return 0;
}

static int run_marshal_control(BOOL agile)
{
    struct marshal_control control = {NULL, E_UNEXPECTED, E_UNEXPECTED, E_UNEXPECTED, 0};
    struct observation *obs = calloc(1, sizeof(*obs));
    struct source *source = NULL;
    HANDLE worker = NULL;
    HRESULT hr = E_OUTOFMEMORY;
    int status = 1;

    printf("marshal_control=%s\n", agile ? "free-threaded" : "plain-sta");
    if (!obs) return status;
    obs->refs = 1;
    obs->ui_thread = GetCurrentThreadId();
    obs->owner_thread = obs->ui_thread;
    obs->agile = agile;
    source = source_create(obs, 0);
    if (!source) goto done;
    hr = CoMarshalInterThreadInterfaceInStream(&IID_IEnumString, (IUnknown *)&source->entry, &control.stream);
    if (FAILED(hr)) goto done;

    worker = CreateThread(NULL, 0, marshal_worker, &control, 0, NULL);
    if (!worker)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto done;
    }
    pump_until_signaled(worker);
    printf("marshaled_worker_init=%08lx unmarshal=%08lx next=%08lx fetched=%lu on_ui=%ld off_ui=%ld\n",
           (unsigned long)control.init_hr, (unsigned long)control.unmarshal_hr, (unsigned long)control.next_hr,
           (unsigned long)control.fetched, read_count(&obs->next_ui), read_count(&obs->next_worker));
    print_apartment(obs);
    status = FAILED(control.init_hr) || FAILED(control.unmarshal_hr) || FAILED(control.next_hr)
        || !read_count(&obs->next_calls);

done:
    if (status) printf("marshal_control_incomplete hr=%08lx\n", (unsigned long)hr);
    if (worker) CloseHandle(worker);
    if (control.stream)
    {
        CoReleaseMarshalData(control.stream);
        IStream_Release(control.stream);
    }
    if (source) IEnumString_Release(&source->entry);
    release_observation(obs);
    return status;
}

static void observe_messages(void)
{
    ULONGLONG end = GetTickCount64() + OBSERVE_MS;

    while (GetTickCount64() < end)
    {
        pump_messages();
        MsgWaitForMultipleObjects(0, NULL, FALSE, 25, QS_ALLINPUT);
    }
}

static int run_case(const char *name, ULONG items, DWORD delay, enum provider_kind kind, enum edit_action action)
{
    HINSTANCE instance = GetModuleHandleW(NULL);
    struct observation *obs = calloc(1, sizeof(*obs));
    IAutoComplete2 *autocomplete = NULL;
    IEnumString *enumerator = NULL;
    HWND window = NULL, edit;
    HANDLE thread = NULL, owner = NULL;
    struct sta_provider provider = {0};
    struct probe probe = {0};
    WCHAR text[64] = {0};
    ULONGLONG char_ms;
    HRESULT hr = E_FAIL;
    int status = 1;

    printf("case=%s items=%lu delay_per_item_ms=%lu provider=%s\n", name,
           (unsigned long)items, (unsigned long)delay,
           kind == PROVIDER_PROXY ? "separate-sta-proxy" : kind == PROVIDER_FTM ? "free-threaded" : "plain-sta");
    if (!obs) return status;
    obs->refs = 1;
    obs->ui_thread = GetCurrentThreadId();
    obs->owner_thread = obs->ui_thread;
    obs->items = items;
    obs->delay_ms = delay;
    obs->agile = kind == PROVIDER_FTM;
    obs->first_next = CreateEventW(NULL, TRUE, FALSE, NULL);
    obs->stop_probe = CreateEventW(NULL, TRUE, FALSE, NULL);
    if (!obs->first_next || !obs->stop_probe) goto done;
    if (action != ACTION_NONE)
    {
        obs->release_next = CreateEventW(NULL, TRUE, FALSE, NULL);
        if (!obs->release_next) goto done;
        obs->hold_result = -1;
    }

    window = CreateWindowExW(0, L"WinumAutocompleteMeasure", L"Autocomplete measurement",
                             WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 450, 180,
                             NULL, NULL, instance, NULL);
    if (!window) goto done;
    probe.window = window;
    probe.obs = obs;
    probe.action = action;
    probe.action_hr = E_PENDING;
    SetWindowLongPtrW(window, GWLP_USERDATA, (LONG_PTR)&probe);
    edit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP,
                           20, 20, 340, 28, window, NULL, instance, NULL);
    if (!edit) goto done;
    ShowWindow(window, SW_SHOW);
    SetForegroundWindow(window);
    SetActiveWindow(window);
    SetFocus(edit);

    if (kind == PROVIDER_PROXY)
    {
        IStream *stream;

        provider.obs = obs;
        provider.ready = CreateEventW(NULL, TRUE, FALSE, NULL);
        provider.stop = CreateEventW(NULL, TRUE, FALSE, NULL);
        if (!provider.ready || !provider.stop) goto done;
        owner = CreateThread(NULL, 0, provider_sta_thread, &provider, 0, NULL);
        if (!owner) goto done;
        pump_until_signaled(provider.ready);
        hr = provider.marshal_hr;
        if (FAILED(hr)) goto done;
        stream = provider.stream;
        provider.stream = NULL;
        hr = CoGetInterfaceAndReleaseStream(stream, &IID_IEnumString, (void **)&enumerator);
        printf("provider_proxy_unmarshal=%08lx\n", (unsigned long)hr);
        if (FAILED(hr)) goto done;
    }
    else
    {
        struct source *source = source_create(obs, 0);

        if (!source) goto done;
        enumerator = &source->entry;
    }
    hr = CoCreateInstance(&CLSID_AutoComplete, NULL, CLSCTX_INPROC_SERVER,
                          &IID_IAutoComplete2, (void **)&autocomplete);
    if (FAILED(hr)) goto done;
    hr = IAutoComplete2_SetOptions(autocomplete, ACO_AUTOSUGGEST | ACO_AUTOAPPEND);
    if (FAILED(hr)) goto done;
    hr = IAutoComplete2_Init(autocomplete, edit, (IUnknown *)enumerator, NULL, NULL);
    if (FAILED(hr)) goto done;

    printf("setup=ok edit_focus=%d\n", GetFocus() == edit);
    probe.edit = edit;
    probe.autocomplete = autocomplete;
    thread = CreateThread(NULL, 0, probe_thread, &probe, 0, NULL);
    if (!thread) goto done;
    InterlockedExchange(&obs->phase, 1);
    char_ms = GetTickCount64();
    SendMessageW(edit, WM_CHAR, L'a', 1);
    InterlockedExchange(&obs->phase, 2);
    char_ms = GetTickCount64() - char_ms;
    observe_messages();
    SetEvent(obs->stop_probe);
    WaitForSingleObject(thread, INFINITE);
    if (IsWindow(edit)) GetWindowTextW(edit, text, sizeof(text) / sizeof(*text));

    printf("input_dispatch_ms=%llu observed_after_ms=%d edit_chars=%d active_next=%ld\n",
           (unsigned long long)char_ms, OBSERVE_MS, lstrlenW(text), read_count(&obs->active));
    printf("next_calls=%ld pre_input=%ld in_input=%ld on_ui=%ld off_ui=%ld returned=%ld "
           "s_false=%ld resets=%ld clones=%ld largest_request=%ld on_provider_owner=%ld\n",
           read_count(&obs->next_calls), read_count(&obs->before_input), read_count(&obs->during_input),
           read_count(&obs->next_ui), read_count(&obs->next_worker), read_count(&obs->returned),
           read_count(&obs->eof), read_count(&obs->resets), read_count(&obs->clones),
           read_count(&obs->largest_request), read_count(&obs->next_owner));
    printf("ui_probe_replies=%ld replies_during_next=%ld ui_probe_timeouts=%ld\n",
           read_count(&obs->probe_responses), read_count(&obs->probe_during_next),
           read_count(&obs->probe_timeouts));
    print_apartment(obs);
    printf("provider_qi_imarshal=%ld qi_iagileobject=%ld\n",
           read_count(&obs->marshal_queries), read_count(&obs->agile_queries));
    status = read_count(&obs->next_calls) == read_count(&obs->before_input);
    if (status) puts("no_next_after_input");
    if (action != ACTION_NONE)
    {
        printf("action_received=%d action_hr=%08lx action_ms=%llu started_during_hold=%ld "
               "returned_during_hold=%ld hold_wait=%ld next_at_start=%ld next_at_return=%ld "
               "edit_exists=%d final_edit=%s\n", probe.action_received, (unsigned long)probe.action_hr,
               (unsigned long long)probe.action_ms, probe.started_during_hold, probe.returned_during_hold,
               read_count(&obs->hold_result), probe.next_at_start, probe.next_at_return, IsWindow(edit),
               text[0] == L'a' ? "a-prefix" : text[0] == L'b' ? "b-prefix" : text[0] ? "other" : "empty");
        if (!probe.action_received || !read_count(&obs->held_next)) status = 1;
    }

done:
    if (status) printf("case_incomplete hr=%08lx\n", (unsigned long)hr);
    if (thread) CloseHandle(thread);
    if (window) DestroyWindow(window);
    if (autocomplete) IAutoComplete2_Release(autocomplete);
    if (enumerator) IEnumString_Release(enumerator);
    if (owner)
    {
        SetEvent(provider.stop);
        pump_until_signaled(owner);
        CloseHandle(owner);
    }
    if (provider.ready) CloseHandle(provider.ready);
    if (provider.stop) CloseHandle(provider.stop);
    release_observation(obs);
    return status;
}

static DWORD WINAPI watchdog(void *done)
{
    if (WaitForSingleObject(done, 45000) != WAIT_OBJECT_0)
    {
        puts("measurement_timeout");
        TerminateProcess(GetCurrentProcess(), 2);
    }
    return 0;
}

int main(void)
{
    WNDCLASSW klass = {0};
    HANDLE done, thread;
    HRESULT hr;
    int status = 0;

    setvbuf(stdout, NULL, _IONBF, 0);
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
    hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    if (FAILED(hr)) return 2;
    klass.hInstance = GetModuleHandleW(NULL);
    klass.lpfnWndProc = window_proc;
    klass.lpszClassName = L"WinumAutocompleteMeasure";
    klass.hCursor = LoadCursorW(NULL, MAKEINTRESOURCEW(32512));
    klass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    if (!RegisterClassW(&klass)) return 2;
    done = CreateEventW(NULL, TRUE, FALSE, NULL);
    thread = done ? CreateThread(NULL, 0, watchdog, done, 0, NULL) : NULL;
    if (!thread) return 2;

    printf("MEASURE shell32-autocomplete-timing bits=%u\n", (unsigned)(8 * sizeof(void *)));
    status |= run_marshal_control(FALSE);
    status |= run_marshal_control(TRUE);
    status |= run_case("empty", 0, 0, PROVIDER_PLAIN, ACTION_NONE);
    status |= run_case("fast-long", 1024, 0, PROVIDER_PLAIN, ACTION_NONE);
    status |= run_case("slow-standard", 128, 12, PROVIDER_PLAIN, ACTION_NONE);
    status |= run_case("slow-agile", 128, 12, PROVIDER_FTM, ACTION_NONE);
    status |= run_case("separate-sta", 8, 20, PROVIDER_PROXY, ACTION_NONE);
    status |= run_case("replace-during-next", 24, 12, PROVIDER_PLAIN, ACTION_REPLACE);
    status |= run_case("disable-during-next", 24, 12, PROVIDER_PLAIN, ACTION_DISABLE);
    status |= run_case("destroy-during-next", 24, 12, PROVIDER_PLAIN, ACTION_DESTROY);

    CoUninitialize();
    SetEvent(done);
    WaitForSingleObject(thread, INFINITE);
    CloseHandle(thread);
    CloseHandle(done);
    return status;
}
