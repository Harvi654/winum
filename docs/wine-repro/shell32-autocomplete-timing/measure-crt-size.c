/* Original, AI-assisted (Codex), LGPL-2.1-or-later. */
#include <windows.h>
#include <stdio.h>

static void measure(const WCHAR *dll, const char *name)
{
    unsigned int (__cdecl *query)(void);
    HMODULE module = LoadLibraryExW(dll, NULL, LOAD_LIBRARY_SEARCH_SYSTEM32);

    if (!module)
    {
        printf("crt_query_size module=%s load_error=%lu\n", name, (unsigned long)GetLastError());
        return;
    }

    query = (void *)GetProcAddress(module, "__CxxQueryExceptionSize");
    if (query)
        printf("crt_query_size module=%s bytes=%u\n", name, query());
    else
        printf("crt_query_size module=%s export=missing\n", name);
    FreeLibrary(module);
}

int main(void)
{
    printf("probe=crt-exception-size pointer_bytes=%u\n", (unsigned int)sizeof(void *));
    measure(L"ucrtbase.dll", "ucrtbase.dll");
    measure(L"vcruntime140.dll", "vcruntime140.dll");
    measure(L"msvcrt.dll", "msvcrt.dll");
    return 0;
}
