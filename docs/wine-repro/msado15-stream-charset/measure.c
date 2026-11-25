/* Original MEASURE program, AI-assisted (Codex), LGPL-2.1-or-later. */
#define COBJMACROS
#include <windows.h>
#include <ole2.h>
#include <oleauto.h>
#include <stdio.h>
#include <wchar.h>

#define COUNT(a) (sizeof(a) / sizeof((a)[0]))
#define CALL DISPATCH_METHOD
#define GET DISPATCH_PROPERTYGET
#define PUT DISPATCH_PROPERTYPUT

enum { AD_BINARY = 1, AD_TEXT = 2, AD_CRLF = -1, AD_LF = 10,
       AD_READ_ALL = -1, AD_READ_LINE = -2, AD_WRITE_CHAR = 0,
       AD_WRITE_LINE = 1, AD_OVERWRITE = 2 };

static const WCHAR *const charsets[] =
{
    L"utf-8", L"UTF-8", L"Unicode", L"us-ascii", L"windows-1252", L"iso-8859-1"
};
static const WCHAR sample[] = {L'A', 0x010d, 0x20ac, 0xd83d, 0xde03, 0};
static const BYTE fixture[] =
{
    0xef, 0xbb, 0xbf, 0x41, 0xc4, 0x8d, 0xe2, 0x82, 0xac, 0xf0, 0x9f, 0x98, 0x83,
    0x0d, 0x0a, 0x5a, 0x0a, 0x51
};

static VARIANT number(LONG n)
{
    VARIANT v = {0};
    V_VT(&v) = VT_I4;
    V_I4(&v) = n;
    return v;
}

static VARIANT string(const WCHAR *s)
{
    VARIANT v = {0};
    V_VT(&v) = VT_BSTR;
    V_BSTR(&v) = SysAllocString(s);
    return v;
}

static VARIANT object_arg(IDispatch *obj)
{
    VARIANT v = {0};
    V_VT(&v) = VT_DISPATCH;
    V_DISPATCH(&v) = obj;
    return v;
}

static HRESULT invoke(IDispatch *obj, const WCHAR *member, WORD flags,
                      UINT n, VARIANT first, VARIANT second, VARIANT *ret, HRESULT *detail)
{
    VARIANT args[] = {second, first};
    DISPID put = DISPID_PROPERTYPUT, id;
    DISPPARAMS params = {args + 2 - n, flags == PUT ? &put : NULL, n, flags == PUT};
    EXCEPINFO exception = {0};
    WCHAR *name = (WCHAR *)member;
    HRESULT hr;

    VariantInit(ret);
    *detail = S_OK;
    hr = IDispatch_GetIDsOfNames(obj, &IID_NULL, &name, 1, LOCALE_USER_DEFAULT, &id);
    if (SUCCEEDED(hr))
        hr = IDispatch_Invoke(obj, id, &IID_NULL, LOCALE_USER_DEFAULT, flags, &params,
                              ret, &exception, NULL);
    if (exception.pfnDeferredFillIn) exception.pfnDeferredFillIn(&exception);
    *detail = exception.scode;
    SysFreeString(exception.bstrSource);
    SysFreeString(exception.bstrDescription);
    SysFreeString(exception.bstrHelpFile);
    /* Dispatch arguments borrow the caller's reference. BSTRs belong to here. */
    if (V_VT(&first) == VT_BSTR) VariantClear(&first);
    if (V_VT(&second) == VT_BSTR) VariantClear(&second);
    return hr;
}

static void print_variant(const VARIANT *v)
{
    UINT i;

    printf("%u:", V_VT(v));
    switch (V_VT(v))
    {
    case VT_I2: printf("%d", V_I2(v)); break;
    case VT_I4: printf("%ld", V_I4(v)); break;
    case VT_UI4: printf("%lu", V_UI4(v)); break;
    case VT_I8: printf("%lld", (long long)V_I8(v)); break;
    case VT_BOOL: printf("%d", V_BOOL(v)); break;
    case VT_BSTR:
        putchar('[');
        for (i = 0; i < SysStringLen(V_BSTR(v)); ++i)
            printf("%s%04x", i ? "," : "", V_BSTR(v)[i]);
        putchar(']');
        break;
    default: break;
    }
}

static void state(IDispatch *obj)
{
    static const WCHAR *const names[] = {L"Size", L"Position", L"EOS"};
    VARIANT value;
    HRESULT hr, detail;
    UINT i;

    for (i = 0; i < COUNT(names); ++i)
    {
        hr = invoke(obj, names[i], GET, 0, number(0), number(0), &value, &detail);
        printf(" %ls=", names[i]);
        if (SUCCEEDED(hr)) print_variant(&value);
        else printf("error/0x%08lx/0x%08lx", (unsigned long)hr, (unsigned long)detail);
        VariantClear(&value);
    }
}

static HRESULT step(IDispatch *obj, const char *label, const WCHAR *name, WORD flags,
                    UINT n, VARIANT first, VARIANT second)
{
    VARIANT value;
    HRESULT hr, detail;

    hr = invoke(obj, name, flags, n, first, second, &value, &detail);
    printf("c step=%s hr=0x%08lx exception=0x%08lx result=", label,
           (unsigned long)hr, (unsigned long)detail);
    print_variant(&value);
    VariantClear(&value);
    state(obj);
    putchar('\n');
    return hr;
}

#define STEP(o,l,m)        step(o,l,L##m,CALL,0,number(0),number(0))
#define INTSTEP(o,l,m,f,n) step(o,l,L##m,f,1,number(n),number(0))
#define STRSTEP(o,l,m,f,s) step(o,l,L##m,f,1,string(s),number(0))

static IDispatch *create_stream(LONG type, const WCHAR *charset)
{
    IDispatch *obj = NULL;
    CLSID clsid;
    HRESULT hr = CLSIDFromProgID(L"ADODB.Stream", &clsid);

    if (SUCCEEDED(hr))
        hr = CoCreateInstance(&clsid, NULL, CLSCTX_INPROC_SERVER, &IID_IDispatch, (void **)&obj);
    printf("c create type=%ld hr=0x%08lx\n", type, (unsigned long)hr);
    if (!obj) return NULL;
    INTSTEP(obj, "type", "Type", PUT, type);
    if (charset)
    {
        STRSTEP(obj, "charset_set", "Charset", PUT, charset);
        step(obj, "charset_get", L"Charset", GET, 0, number(0), number(0));
    }
    if (FAILED(STEP(obj, "open", "Open")))
    {
        IDispatch_Release(obj);
        return NULL;
    }
    return obj;
}

static void close_stream(IDispatch *obj)
{
    STEP(obj, "close", "Close");
    IDispatch_Release(obj);
}

static void dump_file(const char *origin, const WCHAR *name, const WCHAR *path)
{
    BYTE bytes[512];
    DWORD count, i;
    HANDLE f = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);

    printf("file origin=%s label=%ls", origin, name);
    if (f == INVALID_HANDLE_VALUE)
        printf(" error=%lu", GetLastError());
    else
    {
        if (ReadFile(f, bytes, sizeof(bytes), &count, NULL))
        {
            printf(" bytes=%lu hex=", count);
            for (i = 0; i < count; ++i) printf("%02x", bytes[i]);
            if (count == sizeof(bytes)) printf(" limit_reached=1");
        }
        else printf(" error=%lu", GetLastError());
        CloseHandle(f);
    }
    putchar('\n');
}

static void save(IDispatch *obj, const char *label, const WCHAR *name, const WCHAR *path)
{
    step(obj, label, L"SaveToFile", CALL, 2, string(path), number(AD_OVERWRITE));
    dump_file("c", name, path);
}

static void measure_charset(const WCHAR *charset, const WCHAR *output, const WCHAR *copy)
{
    IDispatch *text, *binary;

    printf("c case=write charset=%ls\n", charset);
    DeleteFileW(output);
    DeleteFileW(copy);
    text = create_stream(AD_TEXT, charset);
    if (!text) return;
    step(text, "write_empty", L"WriteText", CALL, 2, string(L""), number(AD_WRITE_CHAR));
    step(text, "write_char", L"WriteText", CALL, 2, string(sample), number(AD_WRITE_CHAR));
    INTSTEP(text, "set_crlf", "LineSeparator", PUT, AD_CRLF);
    step(text, "write_line_crlf", L"WriteText", CALL, 2, string(L"B"), number(AD_WRITE_LINE));
    INTSTEP(text, "set_lf", "LineSeparator", PUT, AD_LF);
    step(text, "write_line_lf", L"WriteText", CALL, 2, string(L"C"), number(AD_WRITE_LINE));
    save(text, "save_initial", L"initial", output);

    INTSTEP(text, "rewind_read", "Position", PUT, 0);
    INTSTEP(text, "read_n1", "ReadText", CALL, 1);
    INTSTEP(text, "read_n3", "ReadText", CALL, 3);
    INTSTEP(text, "read_line_lf", "ReadText", CALL, AD_READ_LINE);
    INTSTEP(text, "read_remaining", "ReadText", CALL, AD_READ_ALL);

    INTSTEP(text, "rewind_copy", "Position", PUT, 0);
    binary = create_stream(AD_BINARY, NULL);
    if (binary)
    {
        step(text, "copy_text_to_binary", L"CopyTo", CALL, 2, object_arg(binary), number(-1));
        printf("c target=text_to_binary");
        state(binary);
        putchar('\n');
        save(binary, "save_binary_target", L"binary_target", copy);
        close_stream(binary);
    }
    INTSTEP(text, "rewind_eos", "Position", PUT, 0);
    STEP(text, "set_eos", "SetEOS");
    save(text, "save_truncated", L"truncated", output);
    close_stream(text);

    printf("c case=change_at_zero charset=%ls\n", charset);
    text = create_stream(AD_TEXT, L"Unicode");
    if (text)
    {
        STRSTEP(text, "charset_after_open", "Charset", PUT, charset);
        step(text, "charset_after_open_get", L"Charset", GET, 0, number(0), number(0));
        step(text, "write_before_change", L"WriteText", CALL, 2, string(sample), number(AD_WRITE_CHAR));
        STRSTEP(text, "charset_after_write", "Charset", PUT, charset[0] == L'U' ? L"utf-8" : L"Unicode");
        step(text, "charset_after_write_get", L"Charset", GET, 0, number(0), number(0));
        close_stream(text);
    }
}

static void measure_fixture(const WCHAR *charset, const char *kind, const WCHAR *path)
{
    IDispatch *text, *binary;

    printf("c case=load charset=%ls fixture=%s\n", charset, kind);
    text = create_stream(AD_TEXT, charset);
    if (!text) return;
    STRSTEP(text, "load_text", "LoadFromFile", CALL, path);
    INTSTEP(text, "read_n", "ReadText", CALL, 1);
    INTSTEP(text, "read_all_after_n", "ReadText", CALL, AD_READ_ALL);
    INTSTEP(text, "rewind_lines", "Position", PUT, 0);
    INTSTEP(text, "set_crlf", "LineSeparator", PUT, AD_CRLF);
    INTSTEP(text, "read_crlf", "ReadText", CALL, AD_READ_LINE);
    INTSTEP(text, "set_lf", "LineSeparator", PUT, AD_LF);
    INTSTEP(text, "read_lf", "ReadText", CALL, AD_READ_LINE);
    INTSTEP(text, "read_tail", "ReadText", CALL, AD_READ_ALL);
    INTSTEP(text, "rewind_eos", "Position", PUT, 0);
    STEP(text, "set_eos", "SetEOS");

    binary = create_stream(AD_BINARY, NULL);
    if (binary)
    {
        STRSTEP(binary, "load_binary", "LoadFromFile", CALL, path);
        step(binary, "copy_binary_to_text", L"CopyTo", CALL, 2, object_arg(text), number(-1));
        printf("c target=binary_to_text");
        state(text);
        putchar('\n');
        INTSTEP(text, "rewind_from_binary", "Position", PUT, 0);
        INTSTEP(text, "read_from_binary", "ReadText", CALL, AD_READ_ALL);
        close_stream(binary);
    }
    close_stream(text);
}

static BOOL write_fixture(const WCHAR *path, const BYTE *bytes, DWORD size)
{
    HANDLE f = CreateFileW(path, GENERIC_WRITE, 0, NULL, CREATE_NEW, 0, NULL);
    DWORD written = 0;
    BOOL ok;

    if (f == INVALID_HANDLE_VALUE) return FALSE;
    ok = WriteFile(f, bytes, size, &written, NULL);
    CloseHandle(f);
    return ok && written == size;
}

static BOOL run_vbs(const WCHAR *directory)
{
    WCHAR system[MAX_PATH], cmd[MAX_PATH * 2 + 80];
    STARTUPINFOW si = {0};
    PROCESS_INFORMATION pi;
    DWORD wait, code;
    UINT len = GetSystemDirectoryW(system, COUNT(system));
    BOOL ok = FALSE;

    if (!len || len >= COUNT(system) ||
        swprintf(cmd, COUNT(cmd), L"\"%ls\\cscript.exe\" //nologo \"stream.vbs\" \"%ls\"",
                 system, directory) < 0)
        return FALSE;

    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    si.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    si.hStdError = GetStdHandle(STD_ERROR_HANDLE);
    if (!CreateProcessW(NULL, cmd, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi))
    {
        printf("cscript create_error=%lu\n", GetLastError());
        return FALSE;
    }
    wait = WaitForSingleObject(pi.hProcess, 60000);
    if (wait != WAIT_OBJECT_0)
    {
        printf("cscript wait=0x%08lx\n", wait);
        if (TerminateProcess(pi.hProcess, 124)) WaitForSingleObject(pi.hProcess, 5000);
    }
    else if (GetExitCodeProcess(pi.hProcess, &code))
    {
        printf("cscript exit=%lu\n", code);
        ok = code == 0;
    }
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    return ok;
}

static void make_path(WCHAR *path, const WCHAR *directory, const WCHAR *file)
{
    swprintf(path, MAX_PATH * 2, L"%ls\\%ls", directory, file);
}

int main(void)
{
    WCHAR temp[MAX_PATH], directory[MAX_PATH * 2], guid_text[40];
    WCHAR output[MAX_PATH * 2], copy[MAX_PATH * 2], bom[MAX_PATH * 2], bare[MAX_PATH * 2];
    WCHAR file[MAX_PATH * 2], name[40];
    const WCHAR *const saves[] = {L"initial", L"binary", L"truncated"};
    HRESULT hr;
    GUID guid;
    DWORD len;
    UINT i, j;
    BOOL ok, cleaned;

    setvbuf(stdout, NULL, _IONBF, 0);
    printf("measure=v1 bits=%u\n", (unsigned int)(sizeof(void *) * 8));
    hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    printf("ole_init=0x%08lx\n", (unsigned long)hr);
    if (FAILED(hr)) return 2;
    len = GetTempPathW(COUNT(temp), temp);
    if (!len || len >= COUNT(temp) || FAILED(CoCreateGuid(&guid)) ||
        !StringFromGUID2(&guid, guid_text, COUNT(guid_text)) ||
        swprintf(directory, COUNT(directory), L"%lswinum-ado-%ls", temp, guid_text) < 0 ||
        !CreateDirectoryW(directory, NULL))
    {
        printf("scratch_create=0 error=%lu\n", GetLastError());
        CoUninitialize();
        return 2;
    }
    make_path(output, directory, L"c-output.bin");
    make_path(copy, directory, L"c-copy.bin");
    make_path(bom, directory, L"utf8-bom.bin");
    make_path(bare, directory, L"utf8-bare.bin");

    ok = write_fixture(bom, fixture, COUNT(fixture)) && write_fixture(bare, fixture + 3, COUNT(fixture) - 3);
    printf("fixtures_ready=%d\n", ok);
    if (ok)
    {
        for (i = 0; i < COUNT(charsets); ++i) measure_charset(charsets[i], output, copy);
        for (i = 0; i < 2; ++i)
        {
            measure_fixture(charsets[i], "bom", bom);
            measure_fixture(charsets[i], "bare", bare);
        }
        ok = run_vbs(directory);
    }

    for (i = 0; i < COUNT(charsets); ++i)
        for (j = 0; j < COUNT(saves); ++j)
        {
            swprintf(name, COUNT(name), L"vbs-%u-%ls.bin", i, saves[j]);
            make_path(file, directory, name);
            if (ok) dump_file("vbs", name, file);
            DeleteFileW(file);
        }
    DeleteFileW(output);
    DeleteFileW(copy);
    DeleteFileW(bom);
    DeleteFileW(bare);
    cleaned = RemoveDirectoryW(directory);
    printf("scratch_cleanup=%d\nmeasure_complete=%d\n", cleaned, ok);
    CoUninitialize();
    return ok && cleaned ? 0 : 2;
}
