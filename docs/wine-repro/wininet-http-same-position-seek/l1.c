/* Original, AI-assisted (Codex), LGPL-2.1-or-later.
 * Native harness for the unchanged candidate test block. */
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <windows.h>
#include <wininet.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#ifndef ARRAY_SIZE
#define ARRAY_SIZE(a) (sizeof(a)/sizeof((a)[0]))
#endif
static unsigned int checks, failures;
static void samepos_check(int condition, unsigned int line, const char *format, ...)
{
    va_list args;
    checks++;
    if(condition) return;
    failures++;
    printf("FAIL line=%u ",line);
    va_start(args,format);vprintf(format,args);va_end(args);
}
#define ok(condition, ...) samepos_check(!!(condition),__LINE__,__VA_ARGS__)
/* Native Windows: Wine-only todos are ordinary assertions. */
#define todo_wine_if(condition) if ((void)(condition), 1)
#include "test-body.h"
static DWORD WINAPI watchdog(void *event)
{
    if(WaitForSingleObject(event,60000)==WAIT_OBJECT_0) return 0;
    TerminateProcess(GetCurrentProcess(),2);ExitProcess(2);
}
int main(void)
{
    HANDLE cancel,thread;
    setvbuf(stdout,NULL,_IONBF,0);
    SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX);
    printf("L1 topic=wininet_same_position_seek bits=%u\n",(unsigned int)(8*sizeof(void*)));
    cancel=CreateEventW(NULL,TRUE,FALSE,NULL);
    if(!cancel) return 1;
    thread=CreateThread(NULL,0,watchdog,cancel,0,NULL);
    if(!thread) {CloseHandle(cancel);return 1;}
    test_same_position_seek();
    if(!SetEvent(cancel)) {TerminateProcess(GetCurrentProcess(),2);return 2;}
    if(WaitForSingleObject(thread,INFINITE)!=WAIT_OBJECT_0) failures++;
    if(!CloseHandle(thread)) failures++;
    if(!CloseHandle(cancel)) failures++;
    printf("SUMMARY checks=%u failures=%u completed_cases=%u requests=%ld\n",checks,failures,samepos_completed,samepos_total_requests);
    return failures?1:0;
}
