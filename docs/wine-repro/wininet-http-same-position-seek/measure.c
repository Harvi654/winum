/* Original, AI-assisted (Codex), LGPL-2.1-or-later.
 * Own loopback HTTP bodies: no seek, same-position seek and no-cache control. */
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include "../batch3-measure/measure-support.h"
#include <wininet.h>
#define BODY_SIZE 16384
struct server { SOCKET listener; HANDLE stop; LONG requests; };
static BYTE body[BODY_SIZE];
static unsigned int completed;
static BOOL send_all(SOCKET socket,const void *data,int length)
{
    const char *p=data;
    while(length) {
        int n=send(socket,p,length,0);
        if(n<=0) return FALSE;
        p+=n;length-=n;
    }
    return TRUE;
}
static void respond(SOCKET socket,const char *request)
{
    char header[256];
    int n;
    if(!strncmp(request,"GET /chunk/",11)) {
        static const char h[]="HTTP/1.1 200 OK\r\nContent-Type: application/octet-stream\r\nTransfer-Encoding: chunked\r\nConnection: close\r\n\r\n";
        unsigned int offset;
        if(!send_all(socket,h,sizeof(h)-1)) return;
        for(offset=0;offset<BODY_SIZE;offset+=4096)
            if(!send_all(socket,"1000\r\n",6)||!send_all(socket,body+offset,4096)||!send_all(socket,"\r\n",2)) return;
        send_all(socket,"0\r\n\r\n",5);
    } else if(!strncmp(request,"GET /close/",11)) {
        static const char h[]="HTTP/1.0 200 OK\r\nContent-Type: application/octet-stream\r\nConnection: close\r\n\r\n";
        if(send_all(socket,h,sizeof(h)-1)) send_all(socket,body,BODY_SIZE);
    } else {
        n=snprintf(header,sizeof(header),"HTTP/1.1 200 OK\r\nContent-Type: application/octet-stream\r\nContent-Length: %u\r\nConnection: close\r\n\r\n",BODY_SIZE);
        if(n>0&&send_all(socket,header,n)) send_all(socket,body,BODY_SIZE);
    }
}
static DWORD WINAPI serve(void *parameter)
{
    struct server *server=parameter;
    while(WaitForSingleObject(server->stop,0)!=WAIT_OBJECT_0) {
        fd_set set;
        struct timeval timeout={0,100000};
        SOCKET client;
        char request[8192];
        int length=0,selected;
        DWORD socket_timeout=2000;
        FD_ZERO(&set);FD_SET(server->listener,&set);
        selected=select(0,&set,NULL,NULL,&timeout);
        if(selected<=0) continue;
        client=accept(server->listener,NULL,NULL);
        if(client==INVALID_SOCKET) continue;
        setsockopt(client,SOL_SOCKET,SO_RCVTIMEO,(const char*)&socket_timeout,sizeof(socket_timeout));
        setsockopt(client,SOL_SOCKET,SO_SNDTIMEO,(const char*)&socket_timeout,sizeof(socket_timeout));
        while(length<(int)sizeof(request)-1) {
            int n=recv(client,request+length,sizeof(request)-1-length,0);
            if(n<=0) break;
            length+=n;request[length]=0;
            if(strstr(request,"\r\n\r\n")) {
                InterlockedIncrement(&server->requests);
                respond(client,request);
                break;
            }
        }
        closesocket(client);
    }
    return 0;
}
static void run_case(struct server *server,unsigned short port,const char *framing,unsigned int mode)
{
    char url[128];
    HINTERNET session=NULL,request=NULL;
    BYTE data[997];
    DWORD count=0,offset=0,calls=0,position=INVALID_SET_FILE_POINTER,error=0,read_error=0,timeout=4000;
    LONG before=InterlockedCompareExchange(&server->requests,0,0);
    BOOL matches=TRUE,read_ok=TRUE,eof=FALSE;
    DWORD flags=INTERNET_FLAG_RELOAD|INTERNET_FLAG_NO_COOKIES|INTERNET_FLAG_NO_AUTH|INTERNET_FLAG_NO_UI
                | (mode==3?INTERNET_FLAG_NO_CACHE_WRITE:0);
    snprintf(url,sizeof(url),"http://127.0.0.1:%u/%s/%u",port,framing,mode);
    printf("CASE framing=%s mode=%u flags=%08lx\n",framing,mode,flags);
    session=InternetOpenA("Winum native fixture",INTERNET_OPEN_TYPE_DIRECT,NULL,NULL,0);
    if(!session) { b3_error("InternetOpen",GetLastError()); goto done; }
    InternetSetOptionW(session,INTERNET_OPTION_CONNECT_TIMEOUT,&timeout,sizeof(timeout));
    InternetSetOptionW(session,INTERNET_OPTION_RECEIVE_TIMEOUT,&timeout,sizeof(timeout));
    InternetSetOptionW(session,INTERNET_OPTION_SEND_TIMEOUT,&timeout,sizeof(timeout));
    request=InternetOpenUrlA(session,url,NULL,0,flags,0);
    if(!request) { b3_error("OpenOwnLoopbackUrl",GetLastError()); goto done; }
    while(offset<17) {
        if(!InternetReadFile(request,data,17-offset,&count)||!count) { b3_error("ReadOwnPrefix",GetLastError()); goto done; }
        if(memcmp(data,body+offset,count)) matches=FALSE;
        offset+=count;
    }
    printf("PREFIX bytes=%lu matches=%u\n",offset,matches);
    if(!matches) { b3_error("OwnPrefixControl",ERROR_INVALID_DATA); goto done; }
    if(mode) {
        SetLastError(0xdeadbeef);
        position=InternetSetFilePointer(request,mode==2?17:0,NULL,mode==2?FILE_BEGIN:FILE_CURRENT,0);
        error=GetLastError();
        printf("SEEK position=%lu error=%08lx\n",position,error);
    }
    while(calls++<40) {
        if(!InternetReadFile(request,data,sizeof(data),&count)) { read_ok=FALSE; read_error=GetLastError(); break; }
        if(!count) { eof=TRUE; break; }
        if(offset+count>BODY_SIZE || memcmp(data,body+offset,count)) matches=FALSE;
        offset+=count;
    }
    printf("BODY read_ok=%u bytes=%lu matches=%u eof=%u error=%08lx\n",read_ok,offset,matches,eof,read_error);
    if(!mode && (!read_ok||!eof||offset!=BODY_SIZE||!matches)) b3_error("NoSeekBodyControl",ERROR_INVALID_DATA);
    printf("CASE_END framing=%s mode=%u\n",framing,mode);
    completed++;
done:
    if(request && !InternetCloseHandle(request)) b3_error("CloseRequest",GetLastError());
    if(session && !InternetCloseHandle(session)) b3_error("CloseSession",GetLastError());
    DeleteUrlCacheEntryA(url); /* Only this process's unique loopback URL. */
    printf("REQUESTS framing=%s mode=%u count=%ld\n",framing,mode,InterlockedCompareExchange(&server->requests,0,0)-before);
}
int main(void)
{
    static const char *frames[]={"length","chunk","close"};
    WSADATA data;
    struct sockaddr_in address={0};
    struct server server={INVALID_SOCKET,NULL,0};
    HANDLE thread=NULL;
    int size=sizeof(address),result;
    unsigned int i,j;
    if(!b3_start("wininet_same_position_seek",60000)) return 1;
    result=WSAStartup(MAKEWORD(2,2),&data);
    if(result) { b3_error("WSAStartup",result); return b3_finish(); }
    for(i=0;i<BODY_SIZE;i++) body[i]=(BYTE)((i*13+(i>>8)*7+5)&255);
    server.listener=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
    if(server.listener==INVALID_SOCKET) { b3_error("CreateListener",WSAGetLastError()); goto done; }
    address.sin_family=AF_INET;address.sin_addr.s_addr=htonl(INADDR_LOOPBACK);
    if(bind(server.listener,(const struct sockaddr*)&address,sizeof(address))||listen(server.listener,8)
       ||getsockname(server.listener,(struct sockaddr*)&address,&size)) { b3_error("BindOwnListener",WSAGetLastError()); goto done; }
    server.stop=CreateEventW(NULL,TRUE,FALSE,NULL);
    if(!server.stop) { b3_error("CreateServerStop",GetLastError()); goto done; }
    thread=CreateThread(NULL,0,serve,&server,0,NULL);
    if(!thread) { b3_error("CreateServerThread",GetLastError()); goto done; }
    for(i=0;i<3;i++) for(j=0;j<4;j++) run_case(&server,ntohs(address.sin_port),frames[i],j);
    printf("CASES completed=%u server_requests=%ld\n",completed,server.requests);
done:
    if(thread) {
        if(!SetEvent(server.stop)||WaitForSingleObject(thread,6000)!=WAIT_OBJECT_0) { TerminateProcess(GetCurrentProcess(),2); ExitProcess(2); }
        if(!CloseHandle(thread)) b3_error("CloseServerThread",GetLastError());
    }
    if(server.stop && !CloseHandle(server.stop)) b3_error("CloseServerStop",GetLastError());
    if(server.listener!=INVALID_SOCKET) closesocket(server.listener);
    WSACleanup();
    return b3_finish();
}
