/* Original, AI-assisted (Codex), LGPL-2.1-or-later. */
#define _WIN32_WINNT 0x0600
#define COBJMACROS

#include <windows.h>
#include <shlobj.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

#define PROBE_MESSAGE (WM_APP + 1)
#define OBSERVE_MS 3000

struct observation
{
    LONG refs;
    DWORD ui_thread;
    HANDLE first_next, stop_probe;
    ULONG items, delay_ms;
    BOOL agile;
    LONG phase, active;
    LONG next_calls, next_ui, next_worker, during_input, before_input;
    LONG returned, eof, resets, clones, largest_request;
    LONG probe_responses, probe_during_next, probe_timeouts;
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
    HWND window;
    struct observation *obs;
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
        free(obs);
    }
}

static struct source *source_create(struct observation *obs, ULONG position);

static HRESULT WINAPI source_query_interface(IEnumString *iface, REFIID iid, void **out)
{
    struct source *source = CONTAINING_RECORD(iface, struct source, entry);

    *out = NULL;
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
    WCHAR text[32];

    if (!out || (!fetched && requested != 1)) return E_POINTER;
    if (fetched) *fetched = 0;
    InterlockedIncrement(&obs->next_calls);
    InterlockedIncrement(GetCurrentThreadId() == obs->ui_thread ? &obs->next_ui : &obs->next_worker);
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

        swprintf(text, sizeof(text) / sizeof(*text), L"acad-sample-%04lu.dwg",
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
    if (phase != 0) SetEvent(obs->first_next);
    if (obs->delay_ms && count) Sleep(obs->delay_ms * count);
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

static LRESULT CALLBACK window_proc(HWND window, UINT message, WPARAM wparam, LPARAM lparam)
{
    if (message == PROBE_MESSAGE)
    {
        struct observation *obs = (struct observation *)GetWindowLongPtrW(window, GWLP_USERDATA);
        return obs && read_count(&obs->active) ? 2 : 1;
    }
    return DefWindowProcW(window, message, wparam, lparam);
}

static DWORD WINAPI probe_thread(void *arg)
{
    struct probe *probe = arg;
    struct observation *obs = probe->obs;
    HANDLE events[] = { obs->stop_probe, obs->first_next };

    if (WaitForMultipleObjects(2, events, FALSE, INFINITE) != WAIT_OBJECT_0 + 1) return 0;
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

static void observe_messages(void)
{
    ULONGLONG end = GetTickCount64() + OBSERVE_MS;
    MSG message;

    while (GetTickCount64() < end)
    {
        while (PeekMessageW(&message, NULL, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&message);
            DispatchMessageW(&message);
        }
        MsgWaitForMultipleObjects(0, NULL, FALSE, 25, QS_ALLINPUT);
    }
}

static int run_case(const char *name, ULONG items, DWORD delay, BOOL agile)
{
    HINSTANCE instance = GetModuleHandleW(NULL);
    struct observation *obs = calloc(1, sizeof(*obs));
    IAutoComplete2 *autocomplete = NULL;
    struct source *source = NULL;
    HWND window = NULL, edit;
    HANDLE thread = NULL;
    struct probe probe;
    WCHAR text[64] = {0};
    ULONGLONG char_ms;
    HRESULT hr = E_FAIL;
    int status = 1;

    printf("case=%s items=%lu delay_per_item_ms=%lu marshal=%s\n", name,
           (unsigned long)items, (unsigned long)delay, agile ? "agile" : "standard");
    if (!obs) return status;
    obs->refs = 1;
    obs->ui_thread = GetCurrentThreadId();
    obs->items = items;
    obs->delay_ms = delay;
    obs->agile = agile;
    obs->first_next = CreateEventW(NULL, TRUE, FALSE, NULL);
    obs->stop_probe = CreateEventW(NULL, TRUE, FALSE, NULL);
    if (!obs->first_next || !obs->stop_probe) goto done;

    window = CreateWindowExW(0, L"WinumAutocompleteMeasure", L"Autocomplete measurement",
                             WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 450, 180,
                             NULL, NULL, instance, NULL);
    if (!window) goto done;
    SetWindowLongPtrW(window, GWLP_USERDATA, (LONG_PTR)obs);
    edit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP,
                           20, 20, 340, 28, window, NULL, instance, NULL);
    if (!edit) goto done;
    ShowWindow(window, SW_SHOW);
    SetForegroundWindow(window);
    SetActiveWindow(window);
    SetFocus(edit);

    source = source_create(obs, 0);
    if (!source) goto done;
    hr = CoCreateInstance(&CLSID_AutoComplete, NULL, CLSCTX_INPROC_SERVER,
                          &IID_IAutoComplete2, (void **)&autocomplete);
    if (FAILED(hr)) goto done;
    hr = IAutoComplete2_SetOptions(autocomplete, ACO_AUTOSUGGEST | ACO_AUTOAPPEND);
    if (FAILED(hr)) goto done;
    hr = IAutoComplete2_Init(autocomplete, edit, (IUnknown *)&source->entry, NULL, NULL);
    if (FAILED(hr)) goto done;

    printf("setup=ok edit_focus=%d\n", GetFocus() == edit);
    probe.window = window;
    probe.obs = obs;
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
    GetWindowTextW(edit, text, sizeof(text) / sizeof(*text));

    printf("input_dispatch_ms=%llu observed_after_ms=%d edit_chars=%d active_next=%ld\n",
           (unsigned long long)char_ms, OBSERVE_MS, lstrlenW(text), read_count(&obs->active));
    printf("next_calls=%ld pre_input=%ld in_input=%ld on_ui=%ld off_ui=%ld returned=%ld "
           "s_false=%ld resets=%ld clones=%ld largest_request=%ld\n",
           read_count(&obs->next_calls), read_count(&obs->before_input), read_count(&obs->during_input),
           read_count(&obs->next_ui), read_count(&obs->next_worker), read_count(&obs->returned),
           read_count(&obs->eof), read_count(&obs->resets), read_count(&obs->clones),
           read_count(&obs->largest_request));
    printf("ui_probe_replies=%ld replies_during_next=%ld ui_probe_timeouts=%ld\n",
           read_count(&obs->probe_responses), read_count(&obs->probe_during_next),
           read_count(&obs->probe_timeouts));
    status = read_count(&obs->next_calls) == read_count(&obs->before_input);
    if (status) puts("no_next_after_input");

done:
    if (status) printf("case_incomplete hr=%08lx\n", (unsigned long)hr);
    if (thread) CloseHandle(thread);
    if (window) DestroyWindow(window);
    if (autocomplete) IAutoComplete2_Release(autocomplete);
    if (source) IEnumString_Release(&source->entry);
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
    status |= run_case("empty", 0, 0, FALSE);
    status |= run_case("fast-long", 1024, 0, FALSE);
    status |= run_case("slow-standard", 128, 12, FALSE);
    status |= run_case("slow-agile", 128, 12, TRUE);

    CoUninitialize();
    SetEvent(done);
    WaitForSingleObject(thread, INFINITE);
    CloseHandle(thread);
    CloseHandle(done);
    return status;
}
