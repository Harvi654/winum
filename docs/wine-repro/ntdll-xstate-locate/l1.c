/*
 * L1 check for ntdll-locate-xstate-feature-enabled, standalone.
 *
 * The patch's test sits in Wine's ntdll exception tests, and that whole test
 * unit restarts Windows 10 VMs (seen on two hypervisors, 2026-11-08), so its
 * one added check is repeated here on its own: RtlLocateExtendedFeature for
 * a feature that is not enabled must return NULL without reading the XSAVE
 * area (the CONTEXT_EX points it at an invalid offset).
 *
 * Build: x86_64-w64-mingw32-gcc / i686-w64-mingw32-gcc -O2 -Wall -o l1.exe l1.c
 * Prints text only; exit code 0 = all checks passed.
 * Original, AI-assisted (Claude), LGPL-2.1-or-later.
 */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

#define MY_XSTATE_AVX   2
#define MY_XSTATE_CET_U 11
#define MY_CONTEXT_XSTATE_FLAG 0x40

typedef struct { LONG Offset; DWORD Length; } chunk_t;
typedef struct { chunk_t All, Legacy, XState; } my_context_ex;

static ULONG64 (WINAPI *pRtlGetEnabledExtendedFeatures)(ULONG64);
static DWORD (WINAPI *pRtlGetExtendedContextLength)(ULONG, ULONG *);
static DWORD (WINAPI *pRtlInitializeExtendedContext)(void *, ULONG, my_context_ex **);
static void * (WINAPI *pRtlLocateExtendedFeature)(my_context_ex *, ULONG, ULONG *);

static int failures, checks;

static void check(int cond, const char *what)
{
    checks++;
    if (!cond) failures++;
    printf("%s %s\n", cond ? "ok" : "FAIL", what);
}

int main(void)
{
    HMODULE ntdll = GetModuleHandleA("ntdll.dll");
#ifdef _WIN64
    ULONG flags = 0x00100000 | MY_CONTEXT_XSTATE_FLAG | 0x1f; /* CONTEXT_AMD64 | XSTATE | ALL */
#else
    ULONG flags = 0x00010000 | MY_CONTEXT_XSTATE_FLAG | 0x3f; /* CONTEXT_i386 | XSTATE | ALL */
#endif
    ULONG length = 0;
    my_context_ex *context_ex, bogus_ex;
    ULONG64 enabled;
    void *buffer, *p;

    pRtlGetEnabledExtendedFeatures = (void *)GetProcAddress(ntdll, "RtlGetEnabledExtendedFeatures");
    pRtlGetExtendedContextLength = (void *)GetProcAddress(ntdll, "RtlGetExtendedContextLength");
    pRtlInitializeExtendedContext = (void *)GetProcAddress(ntdll, "RtlInitializeExtendedContext");
    pRtlLocateExtendedFeature = (void *)GetProcAddress(ntdll, "RtlLocateExtendedFeature");
    printf("arch=%d\n", (int)(sizeof(void *) * 8));
    if (!pRtlGetEnabledExtendedFeatures || !pRtlGetExtendedContextLength ||
        !pRtlInitializeExtendedContext || !pRtlLocateExtendedFeature)
    {
        puts("SKIP xstate functions not exported");
        return 0;
    }
    enabled = pRtlGetEnabledExtendedFeatures(~(ULONG64)0);
    printf("enabled_features=0x%llx\n", (unsigned long long)enabled);
    if (!(enabled & (1ull << MY_XSTATE_AVX)))
    {
        puts("SKIP AVX not enabled (the Wine test also needs it)");
        return 0;
    }
    if (pRtlGetEnabledExtendedFeatures(1ull << MY_XSTATE_CET_U))
    {
        puts("SKIP CET_U is enabled on this machine (the check needs a disabled feature)");
        return 0;
    }
    if (pRtlGetExtendedContextLength(flags, &length))
    {
        puts("SKIP RtlGetExtendedContextLength failed");
        return 0;
    }
    buffer = calloc(1, length + 64);
    if (pRtlInitializeExtendedContext(buffer, flags, &context_ex))
    {
        puts("SKIP RtlInitializeExtendedContext failed");
        return 0;
    }
    printf("context_length=%lu xstate_offset=%ld xstate_length=%lu\n", length,
           context_ex->XState.Offset, context_ex->XState.Length);

    p = pRtlLocateExtendedFeature(context_ex, MY_XSTATE_AVX, NULL);
    check(p != NULL, "AVX located in a valid context");

    bogus_ex = *context_ex;
    bogus_ex.XState.Offset = 0x40000000;
    p = pRtlLocateExtendedFeature(&bogus_ex, MY_XSTATE_CET_U, NULL);
    check(p == NULL, "disabled CET_U gives NULL without reading the XSAVE area");

    printf("SUMMARY checks=%d failures=%d\n", checks, failures);
    return failures ? 1 : 0;
}
