/* Original, AI-assisted (Codex), LGPL-2.1-or-later.
 * Native assertion harness for the exact candidate Wine test body. */
#define _WIN32_WINNT 0x0601
#define COBJMACROS
#define CONST_VTABLE
#include <windows.h>
#include <objbase.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
static LONG checks, failures;
static void stub_check(int condition, unsigned int line, const char *format, ...)
{
    va_list args;
    InterlockedIncrement(&checks);
    if (condition) return;
    InterlockedIncrement(&failures);
    printf("FAIL line=%u ", line);
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
}
#define ok(condition, ...) stub_check(!!(condition), __LINE__, __VA_ARGS__)
#define trace(...) do { if (0) printf(__VA_ARGS__); } while (0)
#include "test-body.h"
int main(void)
{
    setvbuf(stdout, NULL, _IONBF, 0);
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
    printf("L1 topic=com_stub_actctx bits=%u\n", (unsigned int)(8 * sizeof(void *)));
    test_stub_activation_context();
    printf("SUMMARY checks=%ld failures=%ld\n", checks, failures);
    return failures ? 1 : 0;
}
