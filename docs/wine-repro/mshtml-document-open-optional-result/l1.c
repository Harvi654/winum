/* Original, AI-assisted (Codex), LGPL-2.1-or-later.
 * Standalone native harness for the candidate's exact Wine test function. */
#define _WIN32_WINNT 0x0601
#define COBJMACROS
#include <windows.h>
#include <ole2.h>
#include <initguid.h>
#include <mshtml.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
static unsigned int checks, failures;
static void html_check(int condition, unsigned int line, const char *format, ...)
{
    va_list args;
    checks++;
    if(condition) return;
    failures++;
    printf("FAIL line=%u ", line);
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
}
#define ok(condition, ...) html_check(!!(condition), __LINE__, __VA_ARGS__)
#include "test-body.h"
static DWORD WINAPI watchdog_thread(void *event)
{
    if(WaitForSingleObject(event, 30000) == WAIT_OBJECT_0) return 0;
    TerminateProcess(GetCurrentProcess(), 2);
    ExitProcess(2);
}
int main(void)
{
    HANDLE cancel, watchdog;
    HRESULT hr;
    setvbuf(stdout, NULL, _IONBF, 0);
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
    printf("L1 topic=mshtml_open_optional_result bits=%u\n", (unsigned int)(8 * sizeof(void *)));
    cancel = CreateEventW(NULL, TRUE, FALSE, NULL);
    if(!cancel) return 1;
    watchdog = CreateThread(NULL, 0, watchdog_thread, cancel, 0, NULL);
    if(!watchdog) { CloseHandle(cancel); return 1; }
    hr = CoInitialize(NULL);
    ok(hr == S_OK, "CoInitialize returned %#lx.\n", hr);
    if(SUCCEEDED(hr)) {
        test_doc_open_optional_result();
        CoUninitialize();
    }
    if(!SetEvent(cancel)) { TerminateProcess(GetCurrentProcess(), 2); return 2; }
    if(WaitForSingleObject(watchdog, INFINITE) != WAIT_OBJECT_0) failures++;
    if(!CloseHandle(watchdog)) failures++;
    if(!CloseHandle(cancel)) failures++;
    printf("SUMMARY checks=%u failures=%u\n", checks, failures);
    return failures ? 1 : 0;
}
