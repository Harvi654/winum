/* Original, AI-assisted (Claude), LGPL-2.1-or-later.
 * Native harness for the unchanged candidate test block (test-body.h),
 * for L1 on Windows 10 and a look under Wine. ok/skip follow Wine's
 * test.h; the block has no todo_wine. */
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <windows.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#ifndef ARRAY_SIZE
#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
#endif
static unsigned int checks, failures, skipped;
static void l1_check(int condition, unsigned int line, const char *format, ...)
{
    va_list args;
    checks++;
    if (condition) return;
    failures++;
    printf("FAIL line=%u ", line);
    va_start(args, format); vprintf(format, args); va_end(args);
}
#define ok(condition, ...) l1_check(!!(condition), __LINE__, __VA_ARGS__)
static void __attribute__((unused)) l1_skip(const char *format, ...)
{
    va_list args;
    skipped++;
    printf("SKIP ");
    va_start(args, format); vprintf(format, args); va_end(args);
}
#define skip l1_skip
#include "test-body.h"
int main(void)
{
    setvbuf(stdout, NULL, _IONBF, 0);
    printf("L1 topic=iphlpapi_adapters_dns_family bits=%u\n", (unsigned int)(8 * sizeof(void *)));
    test_GetAdaptersAddresses_dns_family();
    printf("SUMMARY checks=%u failures=%u skipped=%u\n", checks, failures, skipped);
    return failures ? 1 : 0;
}
