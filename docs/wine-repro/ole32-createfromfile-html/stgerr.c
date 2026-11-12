/*
 * Which HRESULT do StgCreateDocfile / StgOpenStorage return when the file
 * cannot be created or opened?
 *
 * Inventor 2027 saves a Reference Model part ("local document") by first
 * calling StgCreateDocfile with a pseudo path of the form
 *   **<assembly path>*LocalDocs*<storage name>*<part>.ipt
 * (seen in a WINEDEBUG=+storage trace). That cannot be a file name. Wine
 * returns E_FAIL for every CreateFile error except ERROR_FILE_EXISTS, and
 * Inventor then leaves LocalDocs empty. This prints what Windows returns for
 * the same kind of names and for ordinary error cases.
 *
 * Build: x86_64-w64-mingw32-gcc -O2 -municode -o stgerr.exe stgerr.c -lole32 -luuid
 */
#define COBJMACROS
#include <windows.h>
#include <ole2.h>
#include <stdio.h>

static void create(const WCHAR *what, const WCHAR *name, DWORD mode)
{
    IStorage *stg = NULL;
    HRESULT hr;

    SetLastError(0xdeadbeef);
    hr = StgCreateDocfile(name, mode, 0, &stg);
    printf("StgCreateDocfile %-28ls mode %#06lx: hr 0x%08lx, GetLastError %lu, stg %s\n",
           what, mode, hr, GetLastError(), stg ? "set" : "NULL");
    if (stg) IStorage_Release(stg);
}

static void open(const WCHAR *what, const WCHAR *name, DWORD mode)
{
    IStorage *stg = NULL;
    HRESULT hr;

    SetLastError(0xdeadbeef);
    hr = StgOpenStorage(name, NULL, mode, NULL, 0, &stg);
    printf("StgOpenStorage   %-28ls mode %#06lx: hr 0x%08lx, GetLastError %lu, stg %s\n",
           what, mode, hr, GetLastError(), stg ? "set" : "NULL");
    if (stg) IStorage_Release(stg);
}

/* After the pseudo path fails, Inventor opens the assembly, opens the
 * "LocalDocs" sub-storage with STGM_SIMPLE | STGM_SHARE_EXCLUSIVE |
 * STGM_READWRITE (0x8000012), then opens or creates the part's storage inside
 * it with the same flags (create: 0x8001012). Wine refuses any OpenStorage in
 * a sub-storage opened with STGM_SIMPLE (STG_E_INVALIDFUNCTION). */
static void simple_substorage(const WCHAR *path)
{
    static const DWORD simple = STGM_SIMPLE | STGM_SHARE_EXCLUSIVE | STGM_READWRITE;
    IStorage *root, *docs = NULL, *part = NULL, *tmp;
    HRESULT hr;

    hr = StgCreateDocfile(path, STGM_CREATE | STGM_SHARE_EXCLUSIVE | STGM_READWRITE, 0, &root);
    printf("simple: StgCreateDocfile hr 0x%08lx\n", hr);
    if (FAILED(hr)) return;
    hr = IStorage_CreateStorage(root, L"LocalDocs", STGM_CREATE | STGM_SHARE_EXCLUSIVE | STGM_READWRITE, 0, 0, &tmp);
    printf("simple: CreateStorage(LocalDocs) hr 0x%08lx\n", hr);
    if (SUCCEEDED(hr))
    {
        if (SUCCEEDED(IStorage_CreateStorage(tmp, L"existing", STGM_CREATE | STGM_SHARE_EXCLUSIVE | STGM_READWRITE, 0, 0, &part)))
            IStorage_Release(part);
        IStorage_Release(tmp);
    }

    hr = IStorage_OpenStorage(root, L"LocalDocs", NULL, simple, NULL, 0, &docs);
    printf("simple: OpenStorage(LocalDocs, 0x8000012) hr 0x%08lx\n", hr);
    if (SUCCEEDED(hr))
    {
        part = NULL;
        hr = IStorage_OpenStorage(docs, L"missing", NULL, simple, NULL, 0, &part);
        printf("simple:   OpenStorage(missing, 0x8000012) hr 0x%08lx\n", hr);
        if (part) IStorage_Release(part);
        part = NULL;
        hr = IStorage_OpenStorage(docs, L"existing", NULL, simple, NULL, 0, &part);
        printf("simple:   OpenStorage(existing, 0x8000012) hr 0x%08lx\n", hr);
        if (part) IStorage_Release(part);
        part = NULL;
        hr = IStorage_CreateStorage(docs, L"created", simple | STGM_CREATE, 0, 0, &part);
        printf("simple:   CreateStorage(created, 0x8001012) hr 0x%08lx\n", hr);
        if (part)
        {
            IStream *stm = NULL;
            hr = IStorage_CreateStream(part, L"Data", STGM_CREATE | STGM_SHARE_EXCLUSIVE | STGM_READWRITE, 0, 0, &stm);
            printf("simple:     CreateStream in it hr 0x%08lx\n", hr);
            if (stm) IStream_Release(stm);
            hr = IStorage_Commit(part, 0);
            printf("simple:     Commit hr 0x%08lx\n", hr);
            IStorage_Release(part);
        }
        hr = IStorage_CreateStream(docs, L"stream", simple | STGM_CREATE, 0, 0, (IStream **)&tmp);
        printf("simple:   CreateStream(stream, 0x8001012) hr 0x%08lx\n", hr);
        if (SUCCEEDED(hr)) IStream_Release((IStream *)tmp);
        IStorage_Release(docs);
    }
    IStorage_Release(root);
    DeleteFileW(path);
}

int wmain(void)
{
    static const DWORD rw = STGM_CREATE | STGM_SHARE_EXCLUSIVE | STGM_READWRITE; /* 0x1012, what Inventor passes */
    WCHAR tmp[MAX_PATH], pseudo[MAX_PATH], nodir[MAX_PATH], star[MAX_PATH], bar[MAX_PATH],
          exist[MAX_PATH], pseudo_existing[MAX_PATH];
    IStorage *stg;

    CoInitialize(NULL);
    GetTempPathW(MAX_PATH, tmp);
    swprintf(pseudo, MAX_PATH, L"**%lsstgerr-missing.iam*LocalDocs*xAaaa*part.ipt", tmp);
    swprintf(exist, MAX_PATH, L"%lsstgerr-exists.iam", tmp);
    swprintf(pseudo_existing, MAX_PATH, L"**%ls*LocalDocs*xAaaa*part.ipt", exist);
    swprintf(nodir, MAX_PATH, L"%lsstgerr-no-such-dir\\a.stg", tmp);
    swprintf(star, MAX_PATH, L"%lsstgerr*a.stg", tmp);
    swprintf(bar, MAX_PATH, L"%lsstgerr|a.stg", tmp);

    /* an existing compound file, like the assembly being saved */
    if (SUCCEEDED(StgCreateDocfile(exist, rw, 0, &stg))) IStorage_Release(stg);

    create(L"pseudo path, file missing", pseudo, rw);
    create(L"pseudo path, file exists", pseudo_existing, rw);
    create(L"missing directory", nodir, rw);
    create(L"name with *", star, rw);
    create(L"name with |", bar, rw);
    create(L"existing, FAILIFTHERE", exist, STGM_FAILIFTHERE | STGM_SHARE_EXCLUSIVE | STGM_READWRITE);
    open(L"pseudo path, file missing", pseudo, STGM_SHARE_EXCLUSIVE | STGM_READWRITE);
    open(L"pseudo path, file exists", pseudo_existing, STGM_SHARE_EXCLUSIVE | STGM_READWRITE);
    open(L"pseudo path, read", pseudo_existing, STGM_SHARE_DENY_WRITE | STGM_READ);
    open(L"missing file", nodir, STGM_SHARE_EXCLUSIVE | STGM_READWRITE);
    open(L"name with *", star, STGM_SHARE_EXCLUSIVE | STGM_READWRITE);

    DeleteFileW(exist);
    swprintf(exist, MAX_PATH, L"%lsstgerr-simple.iam", tmp);
    simple_substorage(exist);
    CoUninitialize();
    printf("done\n");
    return 0;
}
