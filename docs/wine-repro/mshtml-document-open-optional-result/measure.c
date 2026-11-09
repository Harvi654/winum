/* Original, AI-assisted (Codex), LGPL-2.1-or-later.
 * Own empty HTML documents, public COM APIs, no expected native open result. */
#define _WIN32_WINNT 0x0601
#define COBJMACROS
#include <windows.h>
#include <ole2.h>
#include <initguid.h>
#include <mshtml.h>
#include <stdio.h>
#include <string.h>
#ifndef OMIT_RESULT
#define OMIT_RESULT 1
#endif
static unsigned int api_errors, cases, completed;
static BOOL setup(HRESULT hr, const char *operation)
{
    printf("API operation=%s hr=%08lx\n",operation,(DWORD)hr);
    if(FAILED(hr))api_errors++;
    return SUCCEEDED(hr);
}
static DWORD WINAPI watchdog_thread(void *event)
{
    if(WaitForSingleObject(event,30000)==WAIT_OBJECT_0)return 0;
    TerminateProcess(GetCurrentProcess(),2);
    ExitProcess(2);
}
static BOOL ready(IHTMLDocument2 *doc)
{
    ULONGLONG deadline=GetTickCount64()+5000;
    for(;;)
    {
        BSTR state=NULL;
        HRESULT hr=IHTMLDocument2_get_readyState(doc,&state);
        BOOL complete=SUCCEEDED(hr)&&state&&!lstrcmpiW(state,L"complete");
        MSG message;
        SysFreeString(state);
        if(complete){printf("READY complete=1 hr=%08lx\n",(DWORD)hr);return TRUE;}
        if(FAILED(hr)||GetTickCount64()>=deadline)
        {
            printf("READY complete=0 hr=%08lx\n",(DWORD)hr);
            api_errors++;return FALSE;
        }
        while(PeekMessageW(&message,NULL,0,0,PM_REMOVE))
        {TranslateMessage(&message);DispatchMessageW(&message);}
        Sleep(10);
    }
}
static void measure_case(unsigned int state)
{
    static const char html[]="<html><head><title>Winum fixture</title></head><body></body></html>";
    static const char *names[]={"created","initialized","loaded"};
    IHTMLDocument2 *doc=NULL;
    IUnknown *identity=NULL;
    IPersistStreamInit *persist=NULL;
    IHTMLWindow2 *window=NULL;
    IDispatch *result=NULL;
    IStream *stream=NULL;
    BSTR mime=NULL;
    VARIANT omitted;
    HRESULT hr;
    cases++;
    printf("CASE state=%s omit_result=%u\n",names[state],OMIT_RESULT);
    if(!setup(CoCreateInstance(&CLSID_HTMLDocument,NULL,CLSCTX_INPROC_SERVER|CLSCTX_INPROC_HANDLER,
                               &IID_IHTMLDocument2,(void**)&doc),"CoCreateInstance"))goto done;
    if(!setup(IHTMLDocument2_QueryInterface(doc,&IID_IUnknown,(void**)&identity),"QueryIdentity"))goto done;
    if(state)
    {
        if(!setup(IHTMLDocument2_QueryInterface(doc,&IID_IPersistStreamInit,(void**)&persist),"QueryPersistStreamInit"))goto done;
        if(state==1)
        {
            if(!setup(IPersistStreamInit_InitNew(persist),"InitNew"))goto done;
        }
        else
        {
            HGLOBAL memory=GlobalAlloc(GMEM_MOVEABLE,sizeof(html)-1);
            void *bytes;
            if(!memory){api_errors++;goto done;}
            bytes=GlobalLock(memory);
            if(!bytes){GlobalFree(memory);api_errors++;goto done;}
            memcpy(bytes,html,sizeof(html)-1);
            GlobalUnlock(memory);
            hr=CreateStreamOnHGlobal(memory,TRUE,&stream);
            if(!setup(hr,"CreateStreamOnHGlobal")){GlobalFree(memory);goto done;}
            if(!setup(IPersistStreamInit_Load(persist,stream),"LoadOwnHtml"))goto done;
            if(!ready(doc))goto done;
        }
    }
    mime=SysAllocString(L"text/html");
    if(!mime){api_errors++;goto done;}
    VariantInit(&omitted);
    V_VT(&omitted)=VT_ERROR;
    V_ERROR(&omitted)=DISP_E_PARAMNOTFOUND;
    hr=IHTMLDocument2_get_parentWindow(doc,&window);
    printf("WINDOW_BEFORE state=%s hr=%08lx present=%u\n",names[state],(DWORD)hr,window!=NULL);
    if(window){IHTMLWindow2_Release(window);window=NULL;}
    printf("OPEN_BEGIN state=%s omit_result=%u\n",names[state],OMIT_RESULT);
    hr=IHTMLDocument2_open(doc,mime,omitted,omitted,omitted,OMIT_RESULT?NULL:&result);
    printf("OPEN state=%s omit_result=%u hr=%08lx returned_dispatch=%u\n",names[state],OMIT_RESULT,(DWORD)hr,result!=NULL);
    if(result)
    {
        HRESULT query=IDispatch_QueryInterface(result,&IID_IHTMLWindow2,(void**)&window);
        printf("RESULT_WINDOW hr=%08lx present=%u\n",(DWORD)query,window!=NULL);
        if(window){IHTMLWindow2_Release(window);window=NULL;}
    }
    if(SUCCEEDED(hr))
    {
        HRESULT close=IHTMLDocument2_close(doc);
        printf("CLOSE hr=%08lx\n",(DWORD)close);
    }
    hr=IHTMLDocument2_get_parentWindow(doc,&window);
    printf("PARENT_WINDOW hr=%08lx present=%u\n",(DWORD)hr,window!=NULL);
    completed++;
done:
    if(window)IHTMLWindow2_Release(window);
    if(result)IDispatch_Release(result);
    SysFreeString(mime);
    if(stream)IStream_Release(stream);
    if(persist)IPersistStreamInit_Release(persist);
    if(doc)IHTMLDocument2_Release(doc);
    if(identity)IUnknown_Release(identity);
    printf("CASE_END state=%s\n",names[state]);
}
int main(void)
{
    HANDLE cancel,watchdog;
    HRESULT hr;
    unsigned int state;
    setvbuf(stdout,NULL,_IONBF,0);
    SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX);
    printf("MEASURE topic=mshtml_open_optional_result schema=1 bits=%u omit_result=%u\n",(unsigned)(8*sizeof(void*)),OMIT_RESULT);
    cancel=CreateEventW(NULL,TRUE,FALSE,NULL);
    if(!cancel)return 1;
    watchdog=CreateThread(NULL,0,watchdog_thread,cancel,0,NULL);
    if(!watchdog){CloseHandle(cancel);return 1;}
    hr=OleInitialize(NULL);
    if(setup(hr,"OleInitialize"))
    {
        for(state=0;state<3;state++)measure_case(state);
        OleUninitialize();
    }
    if(!SetEvent(cancel)){TerminateProcess(GetCurrentProcess(),2);return 2;}
    if(WaitForSingleObject(watchdog,INFINITE)!=WAIT_OBJECT_0)api_errors++;
    if(!CloseHandle(watchdog))api_errors++;
    if(!CloseHandle(cancel))api_errors++;
    printf("SUMMARY cases=%u completed=%u api_errors=%u expectations=none\n",cases,completed,api_errors);
    return api_errors||completed!=3?1:0;
}
