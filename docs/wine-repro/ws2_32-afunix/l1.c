/* Original, AI-assisted (Claude), LGPL-2.1-or-later.
 * Native harness for the unchanged candidate test block (test-body.h),
 * for L1 on Windows 10 and a look under Wine. ok/skip/todo_wine follow
 * Wine's test.h: on Windows todo_wine changes nothing; under Wine a
 * failing check inside todo_wine counts as todo and a passing one as a
 * failure ("test succeeded inside todo block"). */
#include <winsock2.h>
#include <ws2tcpip.h>
#include <mswsock.h>
#include <afunix.h>
#include <windows.h>
#include <stddef.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#ifndef ARRAY_SIZE
#define ARRAY_SIZE(a) (sizeof(a)/sizeof((a)[0]))
#endif
#ifndef IO_REPARSE_TAG_AF_UNIX
#define IO_REPARSE_TAG_AF_UNIX 0x80000023
#endif
static unsigned int checks, failures, todos, skipped;
static int on_wine, in_todo;
static char dbg[4][512];
static int dbg_n;
static const char *debugstr_a(const char *s)
{
    char *b = dbg[dbg_n++ & 3];
    if (!s) return "(null)";
    snprintf(b, sizeof(dbg[0]), "\"%.500s\"", s);
    return b;
}
static const char *debugstr_w(const WCHAR *s)
{
    char *b = dbg[dbg_n++ & 3];
    if (!s) return "(null)";
    b[0] = '"';
    WideCharToMultiByte(CP_UTF8, 0, s, -1, b + 1, sizeof(dbg[0]) - 2, NULL, NULL);
    strcat(b, "\"");
    return b;
}
static void check(int condition, unsigned int line, const char *format, ...)
{
    va_list args;
    checks++;
    if (in_todo)
    {
        if (!condition) { todos++; return; }
        failures++;
        printf("FAIL line=%u test succeeded inside todo block: ", line);
    }
    else
    {
        if (condition) return;
        failures++;
        printf("FAIL line=%u ", line);
    }
    va_start(args, format); vprintf(format, args); va_end(args);
}
#define ok(condition, ...) check(!!(condition), __LINE__, __VA_ARGS__)
static void do_skip(const char *format, ...)
{
    va_list args;
    skipped++;
    printf("SKIP ");
    va_start(args, format); vprintf(format, args); va_end(args);
}
#define skip do_skip
static int todo_begin(void) { in_todo = on_wine; return 1; }
static int todo_end(void) { in_todo = 0; return 0; }
#define todo_wine for (int todo_once_ = todo_begin(); todo_once_; todo_once_ = todo_end())
#include "test-body.h"
static DWORD WINAPI watchdog(void *event)
{
    if (WaitForSingleObject(event, 60000) == WAIT_OBJECT_0) return 0;
    printf("TIMEOUT\n");
    TerminateProcess(GetCurrentProcess(), 2);
    ExitProcess(2);
}
int main(void)
{
    WSADATA wsa;
    HANDLE cancel, thread;
    setvbuf(stdout, NULL, _IONBF, 0);
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
    on_wine = GetProcAddress(GetModuleHandleA("ntdll.dll"), "wine_get_version") != NULL;
    printf("L1 topic=ws2_32_afunix bits=%u wine=%d\n", (unsigned int)(8 * sizeof(void *)), on_wine);
    if (WSAStartup(MAKEWORD(2, 2), &wsa)) return 2;
    cancel = CreateEventW(NULL, TRUE, FALSE, NULL);
    if (!cancel) return 2;
    thread = CreateThread(NULL, 0, watchdog, cancel, 0, NULL);
    if (!thread) return 2;
    test_afunix_winum();
    SetEvent(cancel);
    WaitForSingleObject(thread, INFINITE);
    WSACleanup();
    printf("SUMMARY checks=%u failures=%u todos=%u skipped=%u\n", checks, failures, todos, skipped);
    return failures ? 1 : 0;
}
