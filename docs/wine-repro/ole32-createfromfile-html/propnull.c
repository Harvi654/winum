/*
 * What does IPropertyStorage store for a VT_LPWSTR / VT_LPSTR property whose
 * string pointer is NULL?
 *
 * When Inventor 2027 saves a Reference Model part into its assembly it writes
 * such a property. Wine's PropertyStorage_WritePropertyToStream calls
 * lstrlenW/lstrlenA on the NULL pointer and crashes (the save is aborted);
 * Windows saves. This writes NULL strings into an ANSI and a Unicode property
 * set, commits, reads them back and dumps the property set streams.
 *
 * Build: x86_64-w64-mingw32-gcc -O2 -municode -o propnull.exe propnull.c -lole32 -luuid
 */
#define COBJMACROS
#include <windows.h>
#include <ole2.h>
#include <stdio.h>

/* 2026-11-12: the first version named the default set "ANSI" and flag 4
 * (PROPSETFLAG_UNBUFFERED, refused for a docfile) "Unicode"; the default
 * set is Unicode, an ANSI set needs PROPSETFLAG_ANSI */
static const FMTID fmtid_default = { 0x9b3d0a01, 0x1111, 0x4c4c, { 0x90, 0x01, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66 } };
static const FMTID fmtid_ansi = { 0x9b3d0a02, 0x1111, 0x4c4c, { 0x90, 0x01, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66 } };

static void dump_stream(IStorage *stg, const WCHAR *name)
{
    IStream *stm;
    BYTE buf[16];
    ULONG got, off = 0, i;

    if (FAILED(IStorage_OpenStream(stg, name, NULL, STGM_READ | STGM_SHARE_EXCLUSIVE, 0, &stm)))
    {
        printf("    (cannot open stream)\n");
        return;
    }
    while (IStream_Read(stm, buf, sizeof(buf), &got) == S_OK && got)
    {
        printf("    %04lx ", off);
        for (i = 0; i < got; i++) printf(" %02x", buf[i]);
        printf("\n");
        off += got;
        if (got < sizeof(buf)) break;
    }
    IStream_Release(stm);
}

static void test(IPropertySetStorage *pss, const FMTID *fmtid, DWORD flags, const char *what)
{
    PROPSPEC spec[3];
    PROPVARIANT var[3], out[3];
    IPropertyStorage *ps;
    HRESULT hr;
    int i;

    printf("%s property set\n", what);
    hr = IPropertySetStorage_Create(pss, fmtid, NULL, flags, STGM_CREATE | STGM_SHARE_EXCLUSIVE | STGM_READWRITE, &ps);
    printf("  Create hr 0x%08lx\n", hr);
    if (FAILED(hr)) return;

    for (i = 0; i < 3; i++)
    {
        spec[i].ulKind = PRSPEC_PROPID;
        spec[i].propid = 2 + i;
        PropVariantInit(&var[i]);
    }
    var[0].vt = VT_LPWSTR; var[0].pwszVal = NULL;
    var[1].vt = VT_LPSTR;  var[1].pszVal = NULL;
    var[2].vt = VT_I4;     var[2].lVal = 42; /* a normal property after them */

    hr = IPropertyStorage_WriteMultiple(ps, 3, spec, var, 2);
    printf("  WriteMultiple(NULL LPWSTR, NULL LPSTR, I4) hr 0x%08lx\n", hr);
    hr = IPropertyStorage_Commit(ps, STGC_DEFAULT);
    printf("  Commit hr 0x%08lx\n", hr);

    memset(out, 0, sizeof(out));
    hr = IPropertyStorage_ReadMultiple(ps, 3, spec, out);
    printf("  ReadMultiple hr 0x%08lx\n", hr);
    for (i = 0; i < 3; i++)
    {
        printf("    propid %lu: vt %u", spec[i].propid, out[i].vt);
        if (out[i].vt == VT_LPWSTR) printf(", %s", out[i].pwszVal ? "string" : "NULL");
        if (out[i].vt == VT_LPWSTR && out[i].pwszVal) printf(" \"%ls\" (len %u)", out[i].pwszVal, lstrlenW(out[i].pwszVal));
        if (out[i].vt == VT_LPSTR) printf(", %s", out[i].pszVal ? "string" : "NULL");
        if (out[i].vt == VT_LPSTR && out[i].pszVal) printf(" \"%s\" (len %u)", out[i].pszVal, lstrlenA(out[i].pszVal));
        if (out[i].vt == VT_I4) printf(", %ld", out[i].lVal);
        printf("\n");
    }
    FreePropVariantArray(3, out);
    IPropertyStorage_Release(ps);
}

int wmain(void)
{
    WCHAR path[MAX_PATH], name[32];
    IPropertySetStorage *pss;
    IStorage *stg;
    HRESULT hr;

    CoInitialize(NULL);
    GetTempPathW(MAX_PATH, path);
    lstrcatW(path, L"propnull.stg");
    hr = StgCreateDocfile(path, STGM_CREATE | STGM_SHARE_EXCLUSIVE | STGM_READWRITE, 0, &stg);
    printf("StgCreateDocfile hr 0x%08lx\n", hr);
    if (FAILED(hr)) return 1;
    hr = IStorage_QueryInterface(stg, &IID_IPropertySetStorage, (void **)&pss);
    printf("QueryInterface(IPropertySetStorage) hr 0x%08lx\n", hr);
    if (SUCCEEDED(hr))
    {
        test(pss, &fmtid_default, PROPSETFLAG_DEFAULT, "Unicode (default)");
        test(pss, &fmtid_ansi, PROPSETFLAG_ANSI, "ANSI");
        IPropertySetStorage_Release(pss);
    }

    hr = FmtIdToPropStgName(&fmtid_default, name);
    if (SUCCEEDED(hr)) { printf("Unicode (default) stream:\n"); dump_stream(stg, name); }
    hr = FmtIdToPropStgName(&fmtid_ansi, name);
    if (SUCCEEDED(hr)) { printf("ANSI stream:\n"); dump_stream(stg, name); }

    IStorage_Release(stg);
    DeleteFileW(path);
    CoUninitialize();
    printf("done\n");
    return 0;
}
