/* Original, AI-assisted (Codex), LGPL-2.1-or-later.
 * Standalone native build of the unchanged candidate assertion body. */
#define _WIN32_WINNT 0x0601
#include <windows.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
static LONG checks, failures;
static void native_skip(const char *format, ...)
{
    va_list args;
    printf("SKIP ");
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
}
#define skip(...) native_skip(__VA_ARGS__)
#define trace(...) printf(__VA_ARGS__)
static void native_check(int value, unsigned int line, const char *format, ...)
{
    va_list args;
    InterlockedIncrement(&checks);
    if (value) return;
    InterlockedIncrement(&failures);
    printf("FAIL line=%u ", line);
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
}
#define ok(value, ...) native_check(!!(value), __LINE__, __VA_ARGS__)
/* Native expectations are ordinary assertions, including Wine-only todos. */
#define todo_wine_if(condition) for (int native_once = ((void)(condition), 1); native_once; native_once = 0)
#define winetest_push_context(...) ((void)0)
#define winetest_pop_context() ((void)0)
#include "test-body.h"
static DWORD WINAPI watchdog(void *done)
{
    if (WaitForSingleObject(done, 360000) == WAIT_OBJECT_0) return 0;
    puts("INCOMPLETE watchdog_timeout=1");
    TerminateProcess(GetCurrentProcess(), 2);
    return 2;
}
int main(void)
{
    HANDLE done, watcher;
    setvbuf(stdout, NULL, _IONBF, 0);
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
    printf("L1 topic=input_attachment_components bits=%u\n", (unsigned int)(8 * sizeof(void *)));
    done = CreateEventW(NULL, TRUE, FALSE, NULL);
    if (!done) return 2;
    watcher = CreateThread(NULL, 0, watchdog, done, 0, NULL);
    if (!watcher) { CloseHandle(done); return 2; }
    test_input_components();
    if (!SetEvent(done)) { TerminateProcess(GetCurrentProcess(), 2); return 2; }
    if (WaitForSingleObject(watcher, 3000) != WAIT_OBJECT_0) ++failures;
    CloseHandle(watcher);
    CloseHandle(done);
    printf("SUMMARY checks=%ld failures=%ld complete_scenarios=%u skipped_scenarios=%u\n", checks, failures, completed_scenarios, skipped_scenarios);
    printf("SCOPE component_snapshots=%u absolute_snapshots=%u absolute_state=%d positive_checks=%u\n",
           component_snapshots, absolute_snapshots, absolute_supported, component_positive_checks);
    if (skipped_scenarios || absolute_supported != 1 || component_snapshots != 592 ||
        absolute_snapshots != 592) return 2;
    return failures || completed_scenarios != 44 ? 1 : 0;
}
