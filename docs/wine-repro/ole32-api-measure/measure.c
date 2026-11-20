/* Original, AI-assisted (Codex), LGPL-2.1-or-later. */
#define _WIN32_WINNT 0x0a00
#define COBJMACROS
#include <windows.h>
#include <objbase.h>
#include <stdio.h>
#include <string.h>

typedef HRESULT (WINAPI *initialize_fn)(LPVOID, DWORD);
typedef void (WINAPI *uninitialize_fn)(void);
typedef HRESULT (WINAPI *apartment_fn)(APTTYPE *, APTTYPEQUALIFIER *);
typedef HRESULT (WINAPI *create_stream_fn)(HGLOBAL, BOOL, IStream **);
typedef LONG (WINAPI *version_fn)(OSVERSIONINFOW *);

static const char *const exports[] = {
    "CoCreateInstance", "CoGetApartmentType", "CoGetClassObject", "CoInitializeEx",
    "CoMarshalInterface", "CoTreatAsClass", "CoUninitialize", "CoUnmarshalInterface",
    "CreateStreamOnHGlobal", "GetHGlobalFromStream", "StgCreateDocfile", "StgOpenStorage",
};

static void print_hr(const char *scenario, const char *step, HRESULT hr)
{
    printf("case=%s step=%s hr=0x%08lx\n", scenario, step, (unsigned long)hr);
}

static void print_apartment(apartment_fn get_apartment, const char *step)
{
    APTTYPE type = (APTTYPE)-42;
    APTTYPEQUALIFIER qualifier = (APTTYPEQUALIFIER)-42;
    HRESULT hr = get_apartment(&type, &qualifier);

    printf("case=apartment step=%s hr=0x%08lx", step, (unsigned long)hr);
    if (SUCCEEDED(hr)) printf(" type=%d qualifier=%d", type, qualifier);
    putchar('\n');
}

static void probe_com(HMODULE ole32)
{
    initialize_fn initialize = (initialize_fn)(void *)GetProcAddress(ole32, "CoInitializeEx");
    uninitialize_fn uninitialize = (uninitialize_fn)(void *)GetProcAddress(ole32, "CoUninitialize");
    apartment_fn get_apartment = (apartment_fn)(void *)GetProcAddress(ole32, "CoGetApartmentType");
    HRESULT sta, repeated, changed, mta;

    if (!initialize || !uninitialize)
    {
        puts("case=init_sequence step=setup skipped=missing_export");
        puts("case=apartment step=setup skipped=missing_export");
        return;
    }
    if (get_apartment) print_apartment(get_apartment, "fresh");
    else puts("case=apartment step=setup skipped=missing_export");

    sta = initialize(NULL, COINIT_APARTMENTTHREADED);
    print_hr("init_sequence", "first_sta", sta);
    if (SUCCEEDED(sta) && get_apartment) print_apartment(get_apartment, "sta");

    repeated = initialize(NULL, COINIT_APARTMENTTHREADED);
    print_hr("init_sequence", "second_sta", repeated);
    changed = initialize(NULL, COINIT_MULTITHREADED);
    print_hr("init_sequence", "mta_while_sta", changed);

    if (SUCCEEDED(changed)) uninitialize();
    if (SUCCEEDED(repeated)) uninitialize();
    if (SUCCEEDED(sta)) uninitialize();

    mta = initialize(NULL, COINIT_MULTITHREADED);
    print_hr("init_sequence", "mta_after_cleanup", mta);
    if (SUCCEEDED(mta) && get_apartment) print_apartment(get_apartment, "mta");
    if (SUCCEEDED(mta)) uninitialize();
    if (get_apartment) print_apartment(get_apartment, "after_cleanup");
}

static void read_payload(IStream *stream, const char *scenario, const char *step,
                         ULONG requested, const char *expected, ULONG size)
{
    char buffer[8] = {0};
    ULONG read = 0;
    HRESULT hr = IStream_Read(stream, buffer, requested, &read);

    printf("case=%s step=%s hr=0x%08lx", scenario, step, (unsigned long)hr);
    if (SUCCEEDED(hr))
        printf(" bytes=%lu data_matches=%d", read, read == size && !memcmp(buffer, expected, size));
    putchar('\n');
}

static void probe_stream(HMODULE ole32)
{
    create_stream_fn create = (create_stream_fn)(void *)GetProcAddress(ole32, "CreateStreamOnHGlobal");
    LARGE_INTEGER zero = {0};
    IStream *stream = NULL, *clone = NULL;
    STATSTG stat = {0};
    ULONG written = 0;
    HRESULT hr;

    if (!create)
    {
        puts("case=stream_io step=setup skipped=missing_export");
        puts("case=stream_clone step=setup skipped=missing_export");
        return;
    }

    hr = create(NULL, TRUE, &stream);
    printf("case=stream_io step=create hr=0x%08lx object=%d\n", (unsigned long)hr, stream != NULL);
    if (FAILED(hr) || !stream) return;
    hr = IStream_Write(stream, "WIN!", 4, &written);
    printf("case=stream_io step=write hr=0x%08lx", (unsigned long)hr);
    if (SUCCEEDED(hr)) printf(" bytes=%lu", written);
    putchar('\n');

    hr = IStream_Stat(stream, &stat, STATFLAG_NONAME);
    printf("case=stream_io step=stat hr=0x%08lx", (unsigned long)hr);
    if (SUCCEEDED(hr)) printf(" type=%lu size=%llu", stat.type, (unsigned long long)stat.cbSize.QuadPart);
    putchar('\n');

    hr = IStream_Seek(stream, zero, STREAM_SEEK_SET, NULL);
    print_hr("stream_io", "seek_start", hr);
    if (SUCCEEDED(hr))
    {
        read_payload(stream, "stream_io", "short_read", 8, "WIN!", 4);
        read_payload(stream, "stream_io", "read_eof", 1, "", 0);
    }

    hr = IStream_Seek(stream, zero, STREAM_SEEK_SET, NULL);
    print_hr("stream_clone", "seek_start", hr);
    if (SUCCEEDED(hr))
    {
        hr = IStream_Clone(stream, &clone);
        printf("case=stream_clone step=clone hr=0x%08lx object=%d\n", (unsigned long)hr, clone != NULL);
        if (SUCCEEDED(hr) && clone)
        {
            read_payload(clone, "stream_clone", "read_clone", 2, "WI", 2);
            read_payload(stream, "stream_clone", "read_original", 4, "WIN!", 4);
            IStream_Release(clone);
        }
    }
    IStream_Release(stream);
}

int main(void)
{
    OSVERSIONINFOW version = {0};
    HMODULE ntdll = GetModuleHandleW(L"ntdll.dll");
    version_fn get_version = (version_fn)(void *)GetProcAddress(ntdll, "RtlGetVersion");
    HMODULE ole32;
    unsigned int i;

    setvbuf(stdout, NULL, _IONBF, 0);
    version.dwOSVersionInfoSize = sizeof(version);
    if (!get_version || get_version(&version)) return 2;
    printf("probe=ole32-v1 arch=%s runtime=%s reported_os=%lu.%lu.%lu\n",
           sizeof(void *) == 8 ? "x86_64" : "i686",
           GetProcAddress(ntdll, "wine_get_version") ? "wine" : "windows",
           version.dwMajorVersion, version.dwMinorVersion, version.dwBuildNumber);
    ole32 = LoadLibraryExW(L"ole32.dll", NULL, LOAD_LIBRARY_SEARCH_SYSTEM32);
    if (!ole32)
    {
        printf("setup=load_library_error win32=%lu\n", GetLastError());
        return 2;
    }
    for (i = 0; i < sizeof(exports) / sizeof(exports[0]); ++i)
        printf("export=%s present=%d\n", exports[i], GetProcAddress(ole32, exports[i]) != NULL);

    probe_com(ole32);
    probe_stream(ole32);
    FreeLibrary(ole32);
    puts("complete=1");
    return 0;
}
