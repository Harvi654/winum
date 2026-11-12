/*
 * Lists the storages and streams of a compound file (e.g. an Inventor .iam)
 * with their sizes and classes, to compare files saved on Windows and under
 * Wine.  Only names and sizes are printed, no contents.
 *
 * Usage: stglist.exe file.iam
 * Build: x86_64-w64-mingw32-gcc -O2 -o stglist.exe stglist.c -lole32 -luuid
 */
#define COBJMACROS
#include <windows.h>
#include <ole2.h>
#include <stdio.h>

static void print_name(const WCHAR *name)
{
    for (; *name; name++)
    {
        if (*name < 0x20 || *name > 0x7e) printf("\\x%02x", *name);
        else putchar(*name);
    }
}

static void list(IStorage *stg, int depth)
{
    IEnumSTATSTG *e;
    STATSTG st;
    IStorage *sub;

    if (FAILED(IStorage_EnumElements(stg, 0, NULL, 0, &e))) return;
    while (IEnumSTATSTG_Next(e, 1, &st, NULL) == S_OK)
    {
        printf("%*s%s ", depth * 2, "", st.type == STGTY_STORAGE ? "D" : "S");
        print_name(st.pwcsName);
        if (st.type == STGTY_STREAM) printf("  %lu", (ULONG)st.cbSize.QuadPart);
        else if (!IsEqualCLSID(&st.clsid, &CLSID_NULL))
            printf("  {%08lX-%04X-%04X-...}", st.clsid.Data1, st.clsid.Data2, st.clsid.Data3);
        printf("\n");
        if (st.type == STGTY_STORAGE &&
            SUCCEEDED(IStorage_OpenStorage(stg, st.pwcsName, NULL, STGM_READ | STGM_SHARE_EXCLUSIVE, NULL, 0, &sub)))
        {
            list(sub, depth + 1);
            IStorage_Release(sub);
        }
        CoTaskMemFree(st.pwcsName);
    }
    IEnumSTATSTG_Release(e);
}

int wmain(int argc, WCHAR **argv)
{
    IStorage *stg;
    HRESULT hr;

    if (argc < 2)
    {
        printf("usage: stglist file\n");
        return 1;
    }
    CoInitialize(NULL);
    hr = StgOpenStorage(argv[1], NULL, STGM_READ | STGM_SHARE_DENY_WRITE, NULL, 0, &stg);
    if (FAILED(hr))
    {
        printf("StgOpenStorage 0x%08lx\n", hr);
        return 1;
    }
    list(stg, 0);
    IStorage_Release(stg);
    CoUninitialize();
    return 0;
}
