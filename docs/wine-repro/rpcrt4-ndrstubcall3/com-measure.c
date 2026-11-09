/* Original, AI-assisted (Codex), LGPL-2.1-or-later.
 * Own NDR method over standard cross-apartment COM marshaling. */
#define _WIN32_WINNT 0x0601
#define COBJMACROS
#define CONST_VTABLE
#define USE_STUBLESS_PROXY
#include <windows.h>
#include <objbase.h>
#include <rpcproxy.h>
#include <stdio.h>
#include <string.h>
#ifndef STUB_CALL_VERSION
#define STUB_CALL_VERSION 3
#endif
#ifdef _WIN64
#include "generated/x86_64/probe_p.c"
#include "generated/x86_64/probe_i.c"
#else
#include "generated/i686/probe_p.c"
#include "generated/i686/probe_i.c"
#endif

static CStdPSFactoryBuffer factory;
static LONG api_errors, server_calls, stub_calls;
static LONG dispatch_result;
static DWORD dispatch_phase;
static BOOL server_on_creator, server_on_client;
static DWORD creator_thread, client_thread_id;
typedef LONG (WINAPI *stub_call_fn)(IRpcStubBuffer *, IRpcChannelBuffer *, PRPC_MESSAGE, DWORD *);
static stub_call_fn dispatch;
static const CLSID proxy_clsid = {0x88a5ac71,0x8ba1,0x48fb,{0x9f,0x3b,0xd2,0x38,0x5e,0xcf,0x12,0xa7}};
static CInterfaceStubVtbl stub_vtable;
static const CInterfaceStubVtbl *stub_list[] = {&stub_vtable, NULL};
static const ProxyFileInfo *files[] = {NULL, NULL};
static MIDL_SERVER_INFO server_info;
static MIDL_SYNTAX_INFO syntax;

static BOOL api(HRESULT hr, const char *role, const char *name)
{
    printf("API role=%s name=%s hr=%08lx\n", role, name, (DWORD)hr);
    if(FAILED(hr)) InterlockedIncrement(&api_errors);
    return SUCCEEDED(hr);
}
static void system_error(const char *name)
{
    printf("ERROR name=%s code=%lu\n", name, GetLastError());
    InterlockedIncrement(&api_errors);
}
ULONG WINAPI CStdStubBuffer_Release(IRpcStubBuffer *stub)
{
    return NdrCStdStubBuffer_Release(stub, (IPSFactoryBuffer *)&factory);
}
struct object { IWinumNdrProbe iface; LONG refs; };
static ULONG WINAPI object_addref(IWinumNdrProbe *iface)
{
    return InterlockedIncrement(&((struct object *)iface)->refs);
}
static ULONG WINAPI object_release(IWinumNdrProbe *iface)
{
    return InterlockedDecrement(&((struct object *)iface)->refs);
}
static HRESULT WINAPI object_query(IWinumNdrProbe *iface, REFIID iid, void **result)
{
    *result = NULL;
    if(!IsEqualIID(iid, &IID_IUnknown) && !IsEqualIID(iid, &IID_IWinumNdrProbe)) return E_NOINTERFACE;
    *result = iface;
    object_addref(iface);
    return S_OK;
}
static HRESULT WINAPI calculate(IWinumNdrProbe *iface, LONG input, LONG *value)
{
    (void)iface;
    InterlockedIncrement(&server_calls);
    server_on_creator = GetCurrentThreadId() == creator_thread;
    server_on_client = GetCurrentThreadId() == client_thread_id;
    printf("SERVER input=%ld output_present=%u on_creator=%u on_client=%u\n",
           input, value != NULL, server_on_creator, server_on_client);
    if(!value) return E_POINTER;
    *value = input + 7;
    return S_OK;
}
static const IWinumNdrProbeVtbl object_vtable = {object_query, object_addref, object_release, calculate};
static void __RPC_STUB invoke_dispatch(IRpcStubBuffer *stub, IRpcChannelBuffer *channel,
                                      PRPC_MESSAGE msg, DWORD *phase)
{
    InterlockedIncrement(&stub_calls);
    printf("DISPATCH mode=%u method=%u bytes=%u before_phase=%lu\n",
           STUB_CALL_VERSION, msg->ProcNum, msg->BufferLength, *phase);
    dispatch_result = dispatch(stub, channel, msg, phase);
    dispatch_phase = *phase;
    printf("DISPATCH_DONE result=%08lx after_phase=%lu\n", (DWORD)dispatch_result, dispatch_phase);
}
static const PRPC_STUB_FUNCTION methods[] = {NULL, NULL, NULL, invoke_dispatch};
struct registration { IPSFactoryBuffer *ps; DWORD cookie; BOOL registered; };
static BOOL register_proxy(struct registration *registration, const char *role)
{
    HRESULT hr;
    hr = NdrDllGetClassObject(&proxy_clsid, &IID_IPSFactoryBuffer, (void **)&registration->ps,
                             files, &proxy_clsid, &factory);
    if(!api(hr, role, "NdrDllGetClassObject")) return FALSE;
    hr = CoRegisterClassObject(&proxy_clsid, (IUnknown *)registration->ps,
                               CLSCTX_INPROC_SERVER, REGCLS_MULTIPLEUSE, &registration->cookie);
    if(!api(hr, role, "CoRegisterClassObject")) return FALSE;
    registration->registered = TRUE;
    return api(CoRegisterPSClsid(&IID_IWinumNdrProbe, &proxy_clsid), role, "CoRegisterPSClsid");
}
static void unregister_proxy(struct registration *registration, const char *role)
{
    if(registration->registered)
        api(CoRevokeClassObject(registration->cookie), role, "CoRevokeClassObject");
    if(registration->ps) IPSFactoryBuffer_Release(registration->ps);
}
struct client_args { IStream *stream; HRESULT result; LONG value; BOOL completed; };
static DWORD WINAPI call_thread(void *parameter)
{
    struct client_args *args = parameter;
    struct registration registration = {0};
    IWinumNdrProbe *proxy = NULL;
    IStream *stream;
    HRESULT hr;
    client_thread_id = GetCurrentThreadId();
    if(!api(CoInitializeEx(NULL, COINIT_MULTITHREADED), "client", "CoInitializeEx")) return 1;
    if(!register_proxy(&registration, "client")) goto done;
    stream = args->stream;
    args->stream = NULL; /* CoGetInterfaceAndReleaseStream consumes it on failure too. */
    hr = CoGetInterfaceAndReleaseStream(stream, &IID_IWinumNdrProbe, (void **)&proxy);
    if(!api(hr, "client", "CoGetInterfaceAndReleaseStream")) goto done;
    printf("CALL_BEGIN mode=%u generated_stub=%u transport=com server=STA client=MTA\n",
           STUB_CALL_VERSION, STUB_CALL_VERSION == 0);
    args->result = IWinumNdrProbe_Calculate(proxy, 35, &args->value);
    args->completed = TRUE;
    printf("CALL hr=%08lx value=%ld server_calls=%ld stub_calls=%ld\n",
           (DWORD)args->result, args->value, server_calls, stub_calls);
done:
    if(proxy) IWinumNdrProbe_Release(proxy);
    unregister_proxy(&registration, "client");
    CoUninitialize();
    return 0;
}
static DWORD WINAPI watchdog_thread(void *event)
{
    if(WaitForSingleObject(event, 30000) == WAIT_OBJECT_0) return 0;
    TerminateProcess(GetCurrentProcess(), 2);
    ExitProcess(2);
}
static void wait_client(HANDLE thread)
{
    for(;;) {
        DWORD wait = MsgWaitForMultipleObjects(1, &thread, FALSE, INFINITE, QS_ALLINPUT);
        MSG message;
        if(wait == WAIT_OBJECT_0) return;
        if(wait == WAIT_FAILED) {
            system_error("MsgWaitForMultipleObjects");
            TerminateProcess(GetCurrentProcess(), 2);
            ExitProcess(2);
        }
        while(PeekMessageW(&message, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&message);
            DispatchMessageW(&message);
        }
    }
}
int main(void)
{
    static const RPC_SYNTAX_IDENTIFIER ndr32 = {
        {0x8a885d04,0x1ceb,0x11c9,{0x9f,0xe8,0x08,0x00,0x2b,0x10,0x48,0x60}},{2,0}};
    struct registration registration = {0};
    struct object object = {{&object_vtable}, 1};
    struct client_args args = {NULL, E_UNEXPECTED, 0x12345678, FALSE};
    ProxyFileInfo proxy_info = probe_ProxyFileInfo;
    HANDLE cancel, watchdog, thread = NULL;
    HMODULE module = NULL;
    BOOL marshaled = FALSE, functional = FALSE;
    union { FARPROC raw; stub_call_fn call; } proc;
    setvbuf(stdout, NULL, _IONBF, 0);
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
    printf("MEASURE topic=ndrstubcall3 schema=3 transport=com bits=%u mode=%u\n",
           (unsigned int)(8 * sizeof(void *)), STUB_CALL_VERSION);
    cancel = CreateEventW(NULL, TRUE, FALSE, NULL);
    if(!cancel) { system_error("CreateEventW"); return 1; }
    watchdog = CreateThread(NULL, 0, watchdog_thread, cancel, 0, NULL);
    if(!watchdog) { system_error("watchdog_CreateThread"); CloseHandle(cancel); return 1; }
    module = LoadLibraryW(L"rpcrt4.dll");
    if(!module) { system_error("LoadLibraryW"); goto finish; }
    proc.raw = GetProcAddress(module, STUB_CALL_VERSION == 3 ? "NdrStubCall3" : "NdrStubCall2");
    dispatch = proc.call;
    printf("EXPORT mode=%u available=%u\n", STUB_CALL_VERSION, dispatch != NULL);
    if(!dispatch) goto finish;
    files[0] = &proxy_info;
    if(STUB_CALL_VERSION) {
        stub_vtable = _IWinumNdrProbeStubVtbl;
        server_info = IWinumNdrProbe_ServerInfo;
        if(STUB_CALL_VERSION == 3) {
            syntax.TransferSyntax = ndr32;
            syntax.ProcString = server_info.ProcString;
            syntax.FmtStringOffset = server_info.FmtStringOffset;
            syntax.TypeString = server_info.pStubDesc->pFormatTypes;
            server_info.pTransferSyntax = &syntax.TransferSyntax;
            server_info.nCount = 1;
            server_info.pSyntaxInfo = &syntax;
            server_info.ProcString = NULL;
            server_info.FmtStringOffset = NULL;
        }
        stub_vtable.header.pServerInfo = &server_info;
        stub_vtable.header.pDispatchTable = methods;
        proxy_info.pStubVtblList = (const PCInterfaceStubVtblList *)stub_list;
    }
    if(!api(CoInitializeEx(NULL, COINIT_APARTMENTTHREADED), "server", "CoInitializeEx")) goto finish;
    creator_thread = GetCurrentThreadId();
    if(!register_proxy(&registration, "server")) goto uninit;
    if(!api(CoMarshalInterThreadInterfaceInStream(&IID_IWinumNdrProbe, (IUnknown *)&object.iface,
                                                  &args.stream), "server", "CoMarshalInterThreadInterfaceInStream")) goto uninit;
    marshaled = TRUE;
    thread = CreateThread(NULL, 0, call_thread, &args, 0, NULL);
    if(!thread) { system_error("client_CreateThread"); goto uninit; }
    wait_client(thread);
    if(!CloseHandle(thread)) system_error("client_CloseHandle");
    functional = args.completed && args.result == S_OK && args.value == 42 && server_calls == 1
                 && server_on_creator && !server_on_client
                 && stub_calls == (STUB_CALL_VERSION != 0);
    if(STUB_CALL_VERSION != 3 && !functional) {
        printf("ERROR name=control_dispatch functional=0\n");
        InterlockedIncrement(&api_errors);
    }
uninit:
    if(args.stream) {
        LARGE_INTEGER zero = {{0}};
        if(marshaled && api(IStream_Seek(args.stream, zero, STREAM_SEEK_SET, NULL), "server", "SeekMarshalStream"))
            api(CoReleaseMarshalData(args.stream), "server", "CoReleaseMarshalData");
        IStream_Release(args.stream);
    }
    if(marshaled) api(CoDisconnectObject((IUnknown *)&object.iface, 0), "server", "CoDisconnectObject");
    unregister_proxy(&registration, "server");
    CoUninitialize();
    printf("CLEANUP object_refs=%ld\n", object.refs);
    object_release(&object.iface);
finish:
    if(module) FreeLibrary(module);
    if(!SetEvent(cancel)) { TerminateProcess(GetCurrentProcess(), 2); return 2; }
    if(WaitForSingleObject(watchdog, INFINITE) != WAIT_OBJECT_0) InterlockedIncrement(&api_errors);
    if(!CloseHandle(watchdog)) system_error("watchdog_CloseHandle");
    if(!CloseHandle(cancel)) system_error("cancel_CloseHandle");
    printf("SUMMARY available=%u completed=%u functional=%u server_calls=%ld stub_calls=%ld api_errors=%ld complete=1 expectations=none\n",
           dispatch != NULL, args.completed, functional, server_calls, stub_calls, api_errors);
    return api_errors ? 1 : 0;
}
