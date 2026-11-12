/*
 * What does OleCreateFromFile/OleCreateFromFileEx do with a file whose
 * class has no usable OLE embedding server?
 *
 * Inventor's Import CAD (Reference Model) embeds its translation report,
 * an .htm file, with
 *   OleCreateFromFileEx(CLSID_NULL, "<temp>\\name.htm", IID_IUnknown, 0,
 *                       OLERENDER_NONE, 0, NULL, NULL, NULL, NULL,
 *                       client_site, storage, &obj)
 * Wine fails that call, Windows does not.  This program makes the same call
 * for an .htm, a .txt and a file with an unregistered extension and prints
 * what comes back: the HRESULT, the class written into the storage, the
 * object's user class and type name, and the storage's elements.  It also
 * prints which persistence interfaces MSHTML's HTML document object has and
 * what its IPersistFile::Load returns, to tell whether Windows embeds the
 * file through MSHTML or wraps it in a package.
 *
 * Nothing here comes from Autodesk; the call parameters are those seen in a
 * WINEDEBUG=+ole trace of Inventor.
 *
 * Build: x86_64-w64-mingw32-gcc -O2 -o createfromfile.exe createfromfile.c -lole32 -luuid
 */
#define COBJMACROS
#include <windows.h>
#include <ole2.h>
#include <stdio.h>

static BOOL dump;

static void print_guid(const char *what, const GUID *g)
{
    printf("  %s {%08lX-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}\n", what, g->Data1, g->Data2, g->Data3,
           g->Data4[0], g->Data4[1], g->Data4[2], g->Data4[3], g->Data4[4], g->Data4[5], g->Data4[6], g->Data4[7]);
}

/* element names may start with \1, \2, \3 or \5; print those as hex */
static void print_name(const WCHAR *name)
{
    for (; *name; name++)
    {
        if (*name < 0x20 || *name > 0x7e) printf("\\x%02x", *name);
        else putchar(*name);
    }
}

/* hex dump of a stream; the temp path in it is replaced by the reader, not here */
static void dump_stream(IStorage *stg, const WCHAR *name)
{
    IStream *stm;
    BYTE buf[16];
    ULONG got, off = 0, i;

    if (FAILED(IStorage_OpenStream(stg, name, NULL, STGM_READ | STGM_SHARE_EXCLUSIVE, 0, &stm))) return;
    while (IStream_Read(stm, buf, sizeof(buf), &got) == S_OK && got)
    {
        printf("      %04lx ", off);
        for (i = 0; i < sizeof(buf); i++)
            if (i < got) printf(" %02x", buf[i]); else printf("   ");
        printf("  ");
        for (i = 0; i < got; i++) putchar(buf[i] >= 0x20 && buf[i] < 0x7f ? buf[i] : '.');
        printf("\n");
        off += got;
        if (got < sizeof(buf)) break;
    }
    IStream_Release(stm);
}

static void print_storage(IStorage *stg, int depth)
{
    IEnumSTATSTG *e;
    STATSTG st;

    if (FAILED(IStorage_EnumElements(stg, 0, NULL, 0, &e))) return;
    while (IEnumSTATSTG_Next(e, 1, &st, NULL) == S_OK)
    {
        printf("  %*s%s ", depth * 2, "", st.type == STGTY_STORAGE ? "storage" : "stream ");
        print_name(st.pwcsName);
        if (st.type == STGTY_STREAM) printf(" (%lu bytes)", (ULONG)st.cbSize.QuadPart);
        printf("\n");
        if (st.type == STGTY_STREAM && dump) dump_stream(stg, st.pwcsName);
        CoTaskMemFree(st.pwcsName);
    }
    IEnumSTATSTG_Release(e);
}

/* A client site that does nothing, like a container that only stores the object. */
static HRESULT WINAPI site_QueryInterface(IOleClientSite *iface, REFIID riid, void **obj)
{
    if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IOleClientSite))
    {
        *obj = iface;
        return S_OK;
    }
    *obj = NULL;
    return E_NOINTERFACE;
}
static ULONG WINAPI site_AddRef(IOleClientSite *iface) { return 2; }
static ULONG WINAPI site_Release(IOleClientSite *iface) { return 1; }
static HRESULT WINAPI site_SaveObject(IOleClientSite *iface) { return S_OK; }
static HRESULT WINAPI site_GetMoniker(IOleClientSite *iface, DWORD assign, DWORD which, IMoniker **mk)
{
    *mk = NULL;
    return E_NOTIMPL;
}
static HRESULT WINAPI site_GetContainer(IOleClientSite *iface, IOleContainer **c)
{
    *c = NULL;
    return E_NOINTERFACE;
}
static HRESULT WINAPI site_ShowObject(IOleClientSite *iface) { return S_OK; }
static HRESULT WINAPI site_OnShowWindow(IOleClientSite *iface, BOOL show) { return S_OK; }
static HRESULT WINAPI site_RequestNewObjectLayout(IOleClientSite *iface) { return E_NOTIMPL; }

static IOleClientSiteVtbl site_vtbl =
{
    site_QueryInterface, site_AddRef, site_Release, site_SaveObject, site_GetMoniker,
    site_GetContainer, site_ShowObject, site_OnShowWindow, site_RequestNewObjectLayout,
};
static IOleClientSite site = { &site_vtbl };

static void write_file(const WCHAR *path, const char *data)
{
    HANDLE h = CreateFileW(path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
    DWORD written;

    WriteFile(h, data, strlen(data), &written, NULL);
    CloseHandle(h);
}

static const char *has(IUnknown *unk, REFIID iid)
{
    IUnknown *tmp;

    if (FAILED(IUnknown_QueryInterface(unk, iid, (void **)&tmp))) return "no";
    IUnknown_Release(tmp);
    return "yes";
}

static void create_from_file(const WCHAR *path, IOleClientSite *client_site)
{
    IStorage *stg;
    IUnknown *obj = NULL;
    IOleObject *ole;
    IPersistStorage *ps;
    IStorage *copy;
    CLSID clsid;
    WCHAR *type;
    HRESULT hr;

    printf("\nOleCreateFromFileEx %ls, %s client site\n", path, client_site ? "with" : "no");
    hr = StgCreateDocfile(NULL, STGM_CREATE | STGM_READWRITE | STGM_SHARE_EXCLUSIVE | STGM_DELETEONRELEASE, 0, &stg);
    if (FAILED(hr))
    {
        printf("  StgCreateDocfile 0x%08lx\n", hr);
        return;
    }

    hr = OleCreateFromFileEx(&CLSID_NULL, path, &IID_IUnknown, 0, OLERENDER_NONE, 0, NULL, NULL, NULL, NULL,
                             client_site, stg, (void **)&obj);
    printf("  hr 0x%08lx, object %s\n", hr, obj ? "returned" : "NULL");

    if (SUCCEEDED(ReadClassStg(stg, &clsid))) print_guid("class in storage", &clsid);
    if (obj && SUCCEEDED(IUnknown_QueryInterface(obj, &IID_IOleObject, (void **)&ole)))
    {
        if (SUCCEEDED(IOleObject_GetUserClassID(ole, &clsid))) print_guid("user class", &clsid);
        if (SUCCEEDED(IOleObject_GetUserType(ole, USERCLASSTYPE_FULL, &type)))
        {
            printf("  user type \"%ls\"\n", type);
            CoTaskMemFree(type);
        }
        IOleObject_Release(ole);
    }
    if (obj)
        printf("  IOleObject %s, IPersistStorage %s, IDataObject %s, IViewObject %s\n",
               has(obj, &IID_IOleObject), has(obj, &IID_IPersistStorage), has(obj, &IID_IDataObject),
               has(obj, &IID_IViewObject));
    printf("  storage elements:\n");
    print_storage(stg, 1);

    /* what a container does when it saves its document */
    if (obj && SUCCEEDED(IUnknown_QueryInterface(obj, &IID_IPersistStorage, (void **)&ps)))
    {
        hr = IPersistStorage_GetClassID(ps, &clsid);
        printf("  IPersistStorage::GetClassID hr 0x%08lx\n", hr);
        if (SUCCEEDED(hr)) print_guid("class", &clsid);
        hr = IPersistStorage_IsDirty(ps);
        printf("  IPersistStorage::IsDirty hr 0x%08lx\n", hr);
        hr = StgCreateDocfile(NULL, STGM_CREATE | STGM_READWRITE | STGM_SHARE_EXCLUSIVE | STGM_DELETEONRELEASE, 0, &copy);
        if (SUCCEEDED(hr))
        {
            hr = OleSave(ps, copy, FALSE);
            printf("  OleSave to a new storage hr 0x%08lx\n", hr);
            hr = IPersistStorage_SaveCompleted(ps, NULL);
            printf("  SaveCompleted(NULL) hr 0x%08lx\n", hr);
            print_storage(copy, 2);
            IStorage_Release(copy);
        }
        IPersistStorage_Release(ps);
    }

    if (obj) IUnknown_Release(obj);
    IStorage_Release(stg);
}

static void print_key(const WCHAR *key)
{
    HKEY hkey;
    LONG ret = RegOpenKeyExW(HKEY_CLASSES_ROOT, key, 0, KEY_READ, &hkey);

    printf("  HKCR\\%ls %s\n", key, ret ? "missing" : "exists");
    if (!ret) RegCloseKey(hkey);
}

static void probe_mshtml(const WCHAR *path)
{
    /* {25336920-03F9-11CF-8FD0-00AA00686F13}: CLSID_HTMLDocument, the class of .htm (htmlfile) */
    static const CLSID clsid_htmldoc = { 0x25336920, 0x03f9, 0x11cf, { 0x8f, 0xd0, 0x00, 0xaa, 0x00, 0x68, 0x6f, 0x13 } };
    IUnknown *doc;
    IPersistFile *pf;
    CLSID clsid;
    HRESULT hr;

    printf("\nGetClassFile and MSHTML's HTML document\n");
    hr = GetClassFile(path, &clsid);
    printf("  GetClassFile hr 0x%08lx\n", hr);
    if (SUCCEEDED(hr)) print_guid("class of .htm", &clsid);

    hr = CoCreateInstance(&clsid_htmldoc, NULL, CLSCTX_INPROC_SERVER, &IID_IUnknown, (void **)&doc);
    printf("  CoCreateInstance(HTMLDocument) hr 0x%08lx\n", hr);
    if (FAILED(hr)) return;
    printf("  IOleObject %s, IPersistFile %s, IPersistStorage %s, IDataObject %s, IPersistMoniker %s\n",
           has(doc, &IID_IOleObject), has(doc, &IID_IPersistFile), has(doc, &IID_IPersistStorage),
           has(doc, &IID_IDataObject), has(doc, &IID_IPersistMoniker));
    if (SUCCEEDED(IUnknown_QueryInterface(doc, &IID_IPersistFile, (void **)&pf)))
    {
        hr = IPersistFile_Load(pf, path, STGM_READ);
        printf("  IPersistFile::Load hr 0x%08lx\n", hr);
        if (SUCCEEDED(hr)) printf("  IDataObject after Load %s\n", has(doc, &IID_IDataObject));
        IPersistFile_Release(pf);
    }
    IUnknown_Release(doc);
}

int main(void)
{
    WCHAR dir[MAX_PATH], htm[MAX_PATH], txt[MAX_PATH], unk[MAX_PATH];

    OleInitialize(NULL);

    GetTempPathW(MAX_PATH, dir);
    swprintf(htm, MAX_PATH, L"%lscreatefromfile-test.htm", dir);
    swprintf(txt, MAX_PATH, L"%lscreatefromfile-test.txt", dir);
    swprintf(unk, MAX_PATH, L"%lscreatefromfile-test.qqzz", dir);
    write_file(htm, "<html><body><p>translation report</p></body></html>\r\n");
    write_file(txt, "plain text\r\n");
    write_file(unk, "unknown type\r\n");

    probe_mshtml(htm);
    printf("\nOLE embedding servers are marked Insertable:\n");
    print_key(L"htmlfile\\Insertable");
    print_key(L"CLSID\\{25336920-03F9-11CF-8FD0-00AA00686F13}\\Insertable");
    print_key(L"CLSID\\{0003000C-0000-0000-C000-000000000046}\\Insertable");
    print_key(L"Package\\protocol\\StdFileEditing\\server");

    dump = TRUE;
    create_from_file(htm, &site);   /* what Inventor does */
    dump = FALSE;
    create_from_file(htm, NULL);
    create_from_file(txt, &site);
    dump = TRUE;
    create_from_file(unk, &site);

    DeleteFileW(htm);
    DeleteFileW(txt);
    DeleteFileW(unk);
    OleUninitialize();
    printf("\ndone\n");
    return 0;
}
