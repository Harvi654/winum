/* Original, AI-assisted (Codex), LGPL-2.1-or-later.
 * Observe native NDR32 dispatch using an SDK proxy/stub and our own IDL. */
#define _WIN32_WINNT 0x0601
#define COBJMACROS
#define CONST_VTABLE
#define USE_STUBLESS_PROXY
#include <windows.h>
#include <objbase.h>
#include <rpcproxy.h>
#include <stdio.h>
#include <stdlib.h>
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
static unsigned int api_errors, server_calls, stub_calls;
static LONG stub_result;
static DWORD stub_phase;
typedef LONG (WINAPI *stub_call_fn)(IRpcStubBuffer *, IRpcChannelBuffer *, PRPC_MESSAGE, DWORD *);
static stub_call_fn dispatch;
static const CLSID proxy_clsid = {0x88a5ac71,0x8ba1,0x48fb,{0x9f,0x3b,0xd2,0x38,0x5e,0xcf,0x12,0xa7}};

static BOOL api(HRESULT hr, const char *name)
{
    printf("API name=%s hr=%08lx\n", name, (DWORD)hr);
    if (FAILED(hr)) api_errors++;
    return SUCCEEDED(hr);
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
    if (!IsEqualIID(iid, &IID_IUnknown) && !IsEqualIID(iid, &IID_IWinumNdrProbe)) return E_NOINTERFACE;
    *result = iface; object_addref(iface); return S_OK;
}
static HRESULT WINAPI calculate(IWinumNdrProbe *iface, LONG input, LONG *value)
{
    (void)iface;
    server_calls++;
    printf("SERVER input=%ld output_present=%u\n", input, value != NULL);
    if (!value) return E_POINTER;
    *value = input + 7;
    return S_OK;
}
static const IWinumNdrProbeVtbl object_vtable = {object_query,object_addref,object_release,calculate};
struct channel { IRpcChannelBuffer iface; LONG refs; IRpcStubBuffer *stub; void *buffers[8]; };
static void free_owned(struct channel *channel, void *buffer)
{
    unsigned int i;
    for (i = 0; i < 8; i++) if (buffer && channel->buffers[i] == buffer)
    { CoTaskMemFree(buffer); channel->buffers[i] = NULL; return; }
}
static ULONG WINAPI channel_addref(IRpcChannelBuffer *iface)
{ return InterlockedIncrement(&((struct channel *)iface)->refs); }
static ULONG WINAPI channel_release(IRpcChannelBuffer *iface)
{ return InterlockedDecrement(&((struct channel *)iface)->refs); }
static HRESULT WINAPI channel_query(IRpcChannelBuffer *iface, REFIID iid, void **result)
{
    *result = NULL;
    if (!IsEqualIID(iid,&IID_IUnknown) && !IsEqualIID(iid,&IID_IRpcChannelBuffer)) return E_NOINTERFACE;
    *result = iface; channel_addref(iface); return S_OK;
}
static HRESULT WINAPI channel_buffer(IRpcChannelBuffer *iface, RPCOLEMESSAGE *msg, REFIID iid)
{
    struct channel *channel = (struct channel *)iface;
    unsigned int i;
    (void)iid;
    for (i = 0; i < 8; i++) if (!channel->buffers[i])
    {
        void *buffer = CoTaskMemAlloc(msg->cbBuffer ? msg->cbBuffer : 1);
        if (!buffer) return E_OUTOFMEMORY;
        channel->buffers[i] = msg->Buffer = buffer;
        return S_OK;
    }
    return E_OUTOFMEMORY;
}
static HRESULT WINAPI channel_send(IRpcChannelBuffer *iface, RPCOLEMESSAGE *msg, ULONG *status)
{
    struct channel *channel = (struct channel *)iface;
    void *input = msg->Buffer;
    HRESULT hr;
    printf("WIRE method=%lu bytes=%lu representation=%08lx\n", msg->iMethod,msg->cbBuffer,msg->dataRepresentation);
    hr = IRpcStubBuffer_Invoke(channel->stub,msg,iface);
    if (msg->Buffer != input) free_owned(channel,input);
    *status = 0;
    printf("INVOKE hr=%08lx reply_bytes=%lu\n", (DWORD)hr,msg->cbBuffer);
    return hr;
}
static HRESULT WINAPI channel_free(IRpcChannelBuffer *iface, RPCOLEMESSAGE *msg)
{
    free_owned((struct channel *)iface,msg->Buffer);msg->Buffer=NULL;return S_OK;
}
static HRESULT WINAPI channel_dest(IRpcChannelBuffer *iface, DWORD *context, void **data)
{ (void)iface;*context=MSHCTX_INPROC;*data=NULL;return S_OK; }
static HRESULT WINAPI channel_connected(IRpcChannelBuffer *iface)
{ (void)iface;return S_OK; }
static const IRpcChannelBufferVtbl channel_vtable = {
    channel_query,channel_addref,channel_release,channel_buffer,channel_send,channel_free,channel_dest,channel_connected
};
static void __RPC_STUB invoke_dispatch(IRpcStubBuffer *stub,IRpcChannelBuffer *channel,PRPC_MESSAGE msg,DWORD *phase)
{
    stub_calls++;
    printf("DISPATCH mode=%u before_phase=%lu\n",STUB_CALL_VERSION,*phase);
    stub_result=dispatch(stub,channel,msg,phase);
    stub_phase=*phase;
    printf("DISPATCH_DONE result=%08lx after_phase=%lu\n",(DWORD)stub_result,stub_phase);
}
static DWORD WINAPI watchdog_thread(void *event)
{
    if (WaitForSingleObject(event,30000)==WAIT_OBJECT_0) return 0;
    TerminateProcess(GetCurrentProcess(),2);
    ExitProcess(2);
}
int main(void)
{
    struct object object={{&object_vtable},1};
    struct channel channel={{&channel_vtable},1,NULL,{0}};
    CInterfaceStubVtbl stub_vtable=_IWinumNdrProbeStubVtbl;
    const CInterfaceStubVtbl *stub_list[]={&stub_vtable,NULL};
    ProxyFileInfo proxy_info=probe_ProxyFileInfo;
    const ProxyFileInfo *files[]={&proxy_info,NULL};
    MIDL_SERVER_INFO server_info=IWinumNdrProbe_ServerInfo;
    MIDL_SYNTAX_INFO syntax={0};
    PRPC_STUB_FUNCTION methods[4]={NULL,NULL,NULL,invoke_dispatch};
    IPSFactoryBuffer *ps=NULL;
    IRpcProxyBuffer *proxy=NULL;
    IWinumNdrProbe *client=NULL;
    LONG value=0x12345678;
    HRESULT hr;
    unsigned int i,remaining=0;
    HMODULE module;
    HANDLE cancel,watchdog;
    union { FARPROC raw;stub_call_fn call; } proc;
    setvbuf(stdout,NULL,_IONBF,0);
    SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX);
    printf("MEASURE topic=ndrstubcall3 schema=1 bits=%u mode=%u\n",(unsigned)(8*sizeof(void*)),STUB_CALL_VERSION);
    cancel=CreateEventW(NULL,TRUE,FALSE,NULL);
    if(!cancel) {printf("ERROR operation=CreateEventW error=%lu\n",GetLastError());return 1;}
    watchdog=CreateThread(NULL,0,watchdog_thread,cancel,0,NULL);
    if(!watchdog) {printf("ERROR operation=CreateThread error=%lu\n",GetLastError());CloseHandle(cancel);return 1;}
    module=LoadLibraryW(L"rpcrt4.dll");
    if (!module) {printf("ERROR operation=LoadLibraryW error=%lu\n",GetLastError());api_errors++;goto done_module;}
    proc.raw=GetProcAddress(module,STUB_CALL_VERSION==2?"NdrStubCall2":"NdrStubCall3");dispatch=proc.call;
    printf("EXPORT mode=%u available=%u\n",STUB_CALL_VERSION,dispatch!=NULL);
    if (!dispatch) goto done_module;
    if (!api(CoInitializeEx(NULL,COINIT_APARTMENTTHREADED),"CoInitializeEx")) goto done_module;
    if (STUB_CALL_VERSION==3)
    {
        static const RPC_SYNTAX_IDENTIFIER ndr32={{0x8a885d04,0x1ceb,0x11c9,{0x9f,0xe8,0x08,0x00,0x2b,0x10,0x48,0x60}},{2,0}};
        syntax.TransferSyntax=ndr32;
        syntax.ProcString=server_info.ProcString;
        syntax.FmtStringOffset=server_info.FmtStringOffset;
        syntax.TypeString=server_info.pStubDesc->pFormatTypes;
        server_info.pTransferSyntax=&syntax.TransferSyntax;
        server_info.nCount=1;server_info.pSyntaxInfo=&syntax;
        server_info.ProcString=NULL;server_info.FmtStringOffset=NULL;
    }
    stub_vtable.header.pServerInfo=&server_info;
    stub_vtable.header.pDispatchTable=methods;
    proxy_info.pStubVtblList=(const PCInterfaceStubVtblList *)stub_list;
    if (!api(NdrDllGetClassObject(&proxy_clsid,&IID_IPSFactoryBuffer,(void**)&ps,files,&proxy_clsid,&factory),"NdrDllGetClassObject")) goto done;
    if (!api(IPSFactoryBuffer_CreateStub(ps,&IID_IWinumNdrProbe,(IUnknown*)&object.iface,&channel.stub),"CreateStub")) goto done;
    if (!api(IPSFactoryBuffer_CreateProxy(ps,NULL,&IID_IWinumNdrProbe,&proxy,(void**)&client),"CreateProxy")) goto done;
    if (!api(IRpcProxyBuffer_Connect(proxy,&channel.iface),"Connect")) goto done;
    hr=IWinumNdrProbe_Calculate(client,35,&value);
    printf("CALL hr=%08lx value=%ld server_calls=%u stub_calls=%u stub_result=%08lx phase=%lu\n",
           (DWORD)hr,value,server_calls,stub_calls,(DWORD)stub_result,stub_phase);
    if (STUB_CALL_VERSION==2 && (hr!=S_OK||server_calls!=1||stub_calls!=1||value!=42)) api_errors++;
done:
    if (proxy) IRpcProxyBuffer_Disconnect(proxy);
    if (client) IWinumNdrProbe_Release(client);
    if (proxy) IRpcProxyBuffer_Release(proxy);
    if (channel.stub) {IRpcStubBuffer_Disconnect(channel.stub);IRpcStubBuffer_Release(channel.stub);}
    if (ps) IPSFactoryBuffer_Release(ps);
    for(i=0;i<8;i++)if(channel.buffers[i]){remaining++;CoTaskMemFree(channel.buffers[i]);}
    printf("CLEANUP retained_buffers=%u object_refs=%ld channel_refs=%ld\n",remaining,object.refs,channel.refs);
    if(remaining||object.refs!=1||channel.refs!=1) api_errors++;
    CoUninitialize();
done_module:
    if(module) FreeLibrary(module);
    if(!SetEvent(cancel)) {TerminateProcess(GetCurrentProcess(),2);return 2;}
    if(WaitForSingleObject(watchdog,INFINITE)!=WAIT_OBJECT_0) api_errors++;
    if(!CloseHandle(watchdog)) api_errors++;
    if(!CloseHandle(cancel)) api_errors++;
    printf("SUMMARY available=%u server_calls=%u stub_calls=%u api_errors=%u complete=1 expectations=none\n",
           dispatch!=NULL,server_calls,stub_calls,api_errors);
    return api_errors?1:0;
}
