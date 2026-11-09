/* Original, AI-assisted (Codex), LGPL-2.1-or-later.
 * Native Windows harness: todo_wine assertions are ordinary assertions here.
 * Wine's integrated test uses its own todo handling. */
#define _WIN32_WINNT 0x0601
#include <windows.h>
#include <stdio.h>
#include <stdarg.h>
#ifndef ARRAY_SIZE
#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
#endif
static unsigned int checks, failures;
static void model_check(int condition, unsigned int line, const char *format, ...)
{
    va_list args;
    checks++;
    if (condition) return;
    failures++;
    printf("FAIL line=%u ", line);
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
}
#define ok(condition, ...) model_check(!!(condition), __LINE__, __VA_ARGS__)
#define todo_wine
#define todo_wine_if(condition)
#include "test-body.h"
int main(void)
{
    setvbuf(stdout, NULL, _IONBF, 0);
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
    printf("L1 topic=actctx_threadingmodel_case bits=%u\n", (unsigned int)(8 * sizeof(void *)));
    test_com_threading_model_case();
    printf("SUMMARY checks=%u failures=%u\n", checks, failures);
    return failures ? 1 : 0;
}
