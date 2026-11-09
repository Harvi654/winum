/* Original, AI-assisted (Codex), LGPL-2.1-or-later.
 * Minimal standalone harness for the exact Wine test body in test-body.h. */
#define COBJMACROS
#include <windows.h>
#include <oleauto.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#ifndef ARRAY_SIZE
#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
#endif
static unsigned int checks, failures;
static void offsets_check(int condition, unsigned int line, const char *format, ...)
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
#define ok(condition, ...) offsets_check(!!(condition), __LINE__, __VA_ARGS__)
#include "test-body.h"
int main(void)
{
    setvbuf(stdout, NULL, _IONBF, 0);
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
    printf("L1 topic=typedesc_offsets bits=%u\n", (unsigned)(8 * sizeof(void *)));
    test_large_typedesc_offsets();
    printf("SUMMARY checks=%u failures=%u\n", checks, failures);
    return failures ? 1 : 0;
}
