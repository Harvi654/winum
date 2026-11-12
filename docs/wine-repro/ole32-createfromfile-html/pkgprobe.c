/*
 * MEASURE: the OLE Package object that OleCreateFromFile creates for a file
 * without an embedding server - the points left open by the review of the
 * inventor-reference-model candidate (v2, 2026-11-12):
 *
 *   A  what a saved Package storage holds: OleSave and IPersistStorage::Save
 *      into a fresh storage (\1CompObj or not)
 *   B  IViewObject2::GetExtent / IOleObject::GetExtent / IViewObject::Draw
 *      per aspect and lindex
 *   C  IViewObject::SetAdvise with and without ADVF_PRIMEFIRST: is
 *      OnViewChange sent at once?
 *   D  IOleObject::EnumAdvise before and after Advise
 *   E  OleCreateFromFile failures: missing file, a directory, a file another
 *      handle has opened without sharing, an empty file, a too long path
 *   F  OLERENDER_FORMAT on a Package: result and the storage's elements
 *   G  which registered classes are embedded through their class and which
 *      as a Package: an extension whose ProgID/CLSID is or is not marked
 *      Insertable (registered under HKCU, removed afterwards; no server)
 *   H  loading a Package whose file name does not fit into the temporary
 *      directory (TMP set to a long directory for this process)
 *   I  where the extent comes from: the Package's own CF_METAFILEPICT and
 *      OleMetafilePictFromIconAndLabel for the file's icon and name, for a
 *      short and a long name
 *   J  the label's size in the icon title font, to relate it to the extent
 *   K  loading with a temporary directory near the limit, the name fitting
 *      exactly or not, with and without a name collision (" (2)")
 *   L  Draw on a DC with a scaled, offset mapping: where the pixels land
 *   M  the label's size in other fonts, to find the one behind the extent
 *   N  the extent for several icon title font sizes (changed for this
 *      session only, restored at the end; only with the argument "n")
 *   O  a Package without the UTF-16 names (ANSI only) near the temporary
 *      directory limit (only with the argument "o")
 *
 * Only public API, nothing from Autodesk or Microsoft internals.  Prints
 * text; files go to a fresh directory under %TEMP%, removed at the end.
 *
 * Build: x86_64-w64-mingw32-gcc -O2 -municode -o pkgprobe.exe pkgprobe.c -lole32 -luuid -lgdi32 -lshell32
 */
#define COBJMACROS
#include <windows.h>
#include <ole2.h>
#include <shellapi.h>
#include <stdio.h>

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

static WCHAR dir[MAX_PATH];

static void print_name(const WCHAR *name)
{
    for (; *name; name++)
        if (*name < 0x20) printf("\\%u", *name); else printf("%lc", *name);
}

static void list_storage(IStorage *stg)
{
    IEnumSTATSTG *enm;
    STATSTG st;
    CLSID clsid;
    CLIPFORMAT cf;
    WCHAR *type = NULL;
    HRESULT hr;

    if (FAILED(IStorage_EnumElements(stg, 0, NULL, 0, &enm))) { printf("    (no enum)\n"); return; }
    while (IEnumSTATSTG_Next(enm, 1, &st, NULL) == S_OK)
    {
        printf("    %s ", st.type == STGTY_STORAGE ? "storage" : "stream ");
        print_name(st.pwcsName);
        printf(" %lu\n", st.cbSize.u.LowPart);
        CoTaskMemFree(st.pwcsName);
    }
    IEnumSTATSTG_Release(enm);
    hr = ReadClassStg(stg, &clsid);
    printf("    ReadClassStg %#lx {%08lX-...}\n", hr, clsid.Data1);
    hr = ReadFmtUserTypeStg(stg, &cf, &type);
    printf("    ReadFmtUserTypeStg %#lx cf %u type ", hr, cf);
    if (type) print_name(type);
    printf("\n");
    CoTaskMemFree(type);
}

static IStorage *new_storage(void)
{
    ILockBytes *lb;
    IStorage *stg = NULL;
    CreateILockBytesOnHGlobal(NULL, TRUE, &lb);
    StgCreateDocfileOnILockBytes(lb, STGM_CREATE | STGM_READWRITE | STGM_SHARE_EXCLUSIVE, 0, &stg);
    ILockBytes_Release(lb);
    return stg;
}

static void make_file(const WCHAR *name, const char *text, WCHAR *path)
{
    HANDLE f;
    DWORD w;
    swprintf(path, MAX_PATH, L"%ls\\%ls", dir, name);
    f = CreateFileW(path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
    if (text) WriteFile(f, text, strlen(text), &w, NULL);
    CloseHandle(f);
}

/* ---- advise sink counting notifications ---- */
static LONG view_changes, data_changes, closes, saves;
static HRESULT WINAPI sink_QI(IAdviseSink *s, REFIID riid, void **obj)
{
    if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IAdviseSink)) { *obj = s; return S_OK; }
    *obj = NULL; return E_NOINTERFACE;
}
static ULONG WINAPI sink_AddRef(IAdviseSink *s) { return 2; }
static ULONG WINAPI sink_Release(IAdviseSink *s) { return 1; }
static void WINAPI sink_OnDataChange(IAdviseSink *s, FORMATETC *f, STGMEDIUM *m) { data_changes++; }
static void WINAPI sink_OnViewChange(IAdviseSink *s, DWORD aspect, LONG index) { view_changes++; }
static void WINAPI sink_OnRename(IAdviseSink *s, IMoniker *m) {}
static void WINAPI sink_OnSave(IAdviseSink *s) { saves++; }
static void WINAPI sink_OnClose(IAdviseSink *s) { closes++; }
static IAdviseSinkVtbl sink_vtbl = { sink_QI, sink_AddRef, sink_Release, sink_OnDataChange,
    sink_OnViewChange, sink_OnRename, sink_OnSave, sink_OnClose };
static IAdviseSink sink = { &sink_vtbl };

/* ---- minimal client site ---- */
static HRESULT WINAPI site_QI(IOleClientSite *s, REFIID riid, void **obj)
{
    if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IOleClientSite)) { *obj = s; return S_OK; }
    *obj = NULL; return E_NOINTERFACE;
}
static ULONG WINAPI site_AddRef(IOleClientSite *s) { return 2; }
static ULONG WINAPI site_Release(IOleClientSite *s) { return 1; }
static HRESULT WINAPI site_SaveObject(IOleClientSite *s) { return S_OK; }
static HRESULT WINAPI site_GetMoniker(IOleClientSite *s, DWORD a, DWORD w, IMoniker **m) { *m = NULL; return E_NOTIMPL; }
static HRESULT WINAPI site_GetContainer(IOleClientSite *s, IOleContainer **c) { *c = NULL; return E_NOINTERFACE; }
static HRESULT WINAPI site_ShowObject(IOleClientSite *s) { return S_OK; }
static HRESULT WINAPI site_OnShowWindow(IOleClientSite *s, BOOL f) { return S_OK; }
static HRESULT WINAPI site_RequestNewObjectLayout(IOleClientSite *s) { return E_NOTIMPL; }
static IOleClientSiteVtbl site_vtbl = { site_QI, site_AddRef, site_Release, site_SaveObject, site_GetMoniker,
    site_GetContainer, site_ShowObject, site_OnShowWindow, site_RequestNewObjectLayout };
static IOleClientSite site = { &site_vtbl };

static HRESULT create(const WCHAR *path, IStorage *stg, IUnknown **obj)
{
    return OleCreateFromFile(&CLSID_NULL, path, &IID_IUnknown, OLERENDER_NONE, NULL, &site, stg, (void **)obj);
}

static const DWORD aspects[] = { DVASPECT_CONTENT, DVASPECT_THUMBNAIL, DVASPECT_ICON, DVASPECT_DOCPRINT };

static void part_a(const WCHAR *txt)
{
    IStorage *stg = new_storage(), *fresh;
    IPersistStorage *ps;
    IUnknown *obj;
    HRESULT hr;

    printf("A  saving a Package\n");
    hr = create(txt, stg, &obj);
    printf("  OleCreateFromFile %#lx\n", hr);
    if (FAILED(hr)) return;
    printf("  storage after create:\n"); list_storage(stg);
    IUnknown_QueryInterface(obj, &IID_IPersistStorage, (void **)&ps);
    fresh = new_storage();
    hr = OleSave(ps, fresh, FALSE);
    printf("  OleSave(fresh) %#lx\n", hr); list_storage(fresh);
    IPersistStorage_SaveCompleted(ps, NULL);
    IStorage_Release(fresh);
    fresh = new_storage();
    hr = IPersistStorage_Save(ps, fresh, FALSE);
    printf("  IPersistStorage::Save(fresh) %#lx\n", hr); list_storage(fresh);
    IPersistStorage_SaveCompleted(ps, NULL);
    IStorage_Release(fresh);
    hr = IPersistStorage_IsDirty(ps);
    printf("  IsDirty %#lx\n", hr);
    IPersistStorage_Release(ps);
    IUnknown_Release(obj);
    IStorage_Release(stg);
}

static void part_bcd(const WCHAR *txt)
{
    IStorage *stg = new_storage();
    IViewObject2 *vo;
    IOleObject *oo;
    IEnumSTATDATA *en;
    IUnknown *obj;
    HDC dc;
    HBITMAP bmp, old;
    RECTL rc = { 0, 0, 100, 100 };
    DWORD conn, aspect, advf;
    IAdviseSink *got;
    SIZEL sz;
    LONG idx;
    HRESULT hr;
    int i;

    printf("B  extents and drawing\n");
    if (FAILED(hr = create(txt, stg, &obj))) { printf("  create %#lx\n", hr); return; }
    IUnknown_QueryInterface(obj, &IID_IOleObject, (void **)&oo);
    hr = IUnknown_QueryInterface(obj, &IID_IViewObject2, (void **)&vo);
    printf("  QI IViewObject2 %#lx\n", hr);
    dc = CreateCompatibleDC(NULL);
    bmp = CreateCompatibleBitmap(dc, 100, 100);
    old = SelectObject(dc, bmp);
    printf("  screen dpi %d x %d\n", GetDeviceCaps(dc, LOGPIXELSX), GetDeviceCaps(dc, LOGPIXELSY));
    for (i = 0; i < ARRAY_SIZE(aspects); i++)
        for (idx = -1; idx <= 1; idx++)
        {
            sz.cx = sz.cy = 0xdead;
            hr = vo ? IViewObject2_GetExtent(vo, aspects[i], idx, NULL, &sz) : E_NOINTERFACE;
            printf("  aspect %lu lindex %ld: View GetExtent %#lx %ldx%ld", aspects[i], idx, hr, sz.cx, sz.cy);
            sz.cx = sz.cy = 0xdead;
            hr = IOleObject_GetExtent(oo, aspects[i], &sz);
            printf(", Ole GetExtent %#lx %ldx%ld", hr, sz.cx, sz.cy);
            hr = vo ? IViewObject2_Draw(vo, aspects[i], idx, NULL, NULL, NULL, dc, &rc, NULL, NULL, 0) : E_NOINTERFACE;
            printf(", Draw %#lx\n", hr);
        }
    SelectObject(dc, old);
    DeleteObject(bmp);
    DeleteDC(dc);

    printf("C  view advise\n");
    if (vo)
    {
        view_changes = 0;
        hr = IViewObject2_SetAdvise(vo, DVASPECT_CONTENT, 0, &sink);
        printf("  SetAdvise(CONTENT, 0) %#lx, OnViewChange %ld\n", hr, view_changes);
        view_changes = 0;
        hr = IViewObject2_SetAdvise(vo, DVASPECT_CONTENT, ADVF_PRIMEFIRST, &sink);
        printf("  SetAdvise(CONTENT, PRIMEFIRST) %#lx, OnViewChange %ld\n", hr, view_changes);
        view_changes = 0;
        hr = IViewObject2_SetAdvise(vo, DVASPECT_ICON, ADVF_PRIMEFIRST | ADVF_ONLYONCE, &sink);
        printf("  SetAdvise(ICON, PRIMEFIRST|ONLYONCE) %#lx, OnViewChange %ld\n", hr, view_changes);
        got = NULL; aspect = advf = 0xdead;
        hr = IViewObject2_GetAdvise(vo, &aspect, &advf, &got);
        printf("  GetAdvise %#lx aspect %lu advf %#lx sink %s\n", hr, aspect, advf, got == &sink ? "ours" : got ? "other" : "NULL");
        hr = IViewObject2_SetAdvise(vo, DVASPECT_CONTENT, 0, NULL);
        printf("  SetAdvise(NULL) %#lx\n", hr);
    }

    printf("D  object advise\n");
    en = (void *)0xdeadbeef;
    hr = IOleObject_EnumAdvise(oo, &en);
    printf("  EnumAdvise before Advise %#lx enum %s\n", hr, en == (void *)0xdeadbeef ? "untouched" : en ? "set" : "NULL");
    if (SUCCEEDED(hr) && en && en != (void *)0xdeadbeef) IEnumSTATDATA_Release(en);
    view_changes = data_changes = closes = saves = 0;
    hr = IOleObject_Advise(oo, &sink, &conn);
    printf("  Advise %#lx conn %lu, notifications view %ld data %ld\n", hr, conn, view_changes, data_changes);
    en = NULL;
    hr = IOleObject_EnumAdvise(oo, &en);
    printf("  EnumAdvise after Advise %#lx enum %s\n", hr, en ? "set" : "NULL");
    if (en) IEnumSTATDATA_Release(en);
    hr = IOleObject_Close(oo, OLECLOSE_NOSAVE);
    printf("  Close(NOSAVE) %#lx, OnClose %ld OnSave %ld\n", hr, closes, saves);
    hr = IOleObject_Unadvise(oo, conn);
    printf("  Unadvise %#lx\n", hr);

    if (vo) IViewObject2_Release(vo);
    IOleObject_Release(oo);
    IUnknown_Release(obj);
    IStorage_Release(stg);
}

static void part_e(void)
{
    WCHAR path[MAX_PATH * 2], longname[300];
    IStorage *stg;
    IUnknown *obj;
    HANDLE locked;
    HRESULT hr;
    int i;

    printf("E  failures\n");
    swprintf(path, MAX_PATH, L"%ls\\missing.txt", dir);
    stg = new_storage(); obj = NULL;
    hr = create(path, stg, &obj);
    printf("  missing file %#lx obj %s\n", hr, obj ? "set" : "NULL"); list_storage(stg);
    if (obj) IUnknown_Release(obj);
    IStorage_Release(stg);

    swprintf(path, MAX_PATH, L"%ls\\subdir", dir);
    CreateDirectoryW(path, NULL);
    stg = new_storage(); obj = NULL;
    hr = create(path, stg, &obj);
    printf("  directory %#lx\n", hr);
    if (obj) IUnknown_Release(obj);
    IStorage_Release(stg);

    make_file(L"locked.txt", "locked\r\n", path);
    locked = CreateFileW(path, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
    stg = new_storage(); obj = NULL;
    hr = create(path, stg, &obj);
    printf("  opened without sharing %#lx\n", hr);
    if (obj) IUnknown_Release(obj);
    IStorage_Release(stg);
    CloseHandle(locked);

    make_file(L"empty.txt", NULL, path);
    stg = new_storage(); obj = NULL;
    hr = create(path, stg, &obj);
    printf("  empty file %#lx\n", hr); list_storage(stg);
    if (obj) IUnknown_Release(obj);
    IStorage_Release(stg);

    for (i = 0; i < 270; i++) longname[i] = 'a' + i % 26;
    longname[270] = 0;
    swprintf(path, ARRAY_SIZE(path), L"%ls\\%ls.txt", dir, longname);
    stg = new_storage(); obj = NULL;
    hr = create(path, stg, &obj);
    printf("  path of %u characters (file does not exist) %#lx\n", (unsigned)wcslen(path), hr);
    if (obj) IUnknown_Release(obj);
    IStorage_Release(stg);
}

static void part_f(const WCHAR *txt)
{
    static const CLIPFORMAT cfs[] = { CF_METAFILEPICT, CF_ENHMETAFILE, CF_TEXT };
    IStorage *stg;
    IUnknown *obj;
    IDataObject *data;
    FORMATETC fmt;
    STGMEDIUM med;
    HRESULT hr;
    int i;

    printf("F  OLERENDER_FORMAT\n");
    for (i = 0; i < ARRAY_SIZE(cfs); i++)
    {
        fmt.cfFormat = cfs[i];
        fmt.ptd = NULL;
        fmt.dwAspect = DVASPECT_CONTENT;
        fmt.lindex = -1;
        fmt.tymed = cfs[i] == CF_TEXT ? TYMED_HGLOBAL : cfs[i] == CF_METAFILEPICT ? TYMED_MFPICT : TYMED_ENHMF;
        stg = new_storage(); obj = NULL;
        hr = OleCreateFromFile(&CLSID_NULL, txt, &IID_IUnknown, OLERENDER_FORMAT, &fmt, &site, stg, (void **)&obj);
        printf("  cf %u: OleCreateFromFile %#lx\n", cfs[i], hr);
        if (obj)
        {
            if (SUCCEEDED(IUnknown_QueryInterface(obj, &IID_IDataObject, (void **)&data)))
            {
                memset(&med, 0, sizeof(med));
                hr = IDataObject_GetData(data, &fmt, &med);
                printf("    GetData %#lx tymed %lu\n", hr, med.tymed);
                if (SUCCEEDED(hr)) ReleaseStgMedium(&med);
                IDataObject_Release(data);
            }
            list_storage(stg);
            IUnknown_Release(obj);
        }
        IStorage_Release(stg);
    }
}

#define PROBE_CLSID L"{6D1C1E66-0B35-4E4C-9C61-2F1A4B8E7A01}"
static void reg_set(const WCHAR *key, const WCHAR *value)
{
    HKEY hk;
    WCHAR full[256];
    swprintf(full, ARRAY_SIZE(full), L"Software\\Classes\\%ls", key);
    RegCreateKeyExW(HKEY_CURRENT_USER, full, 0, NULL, 0, KEY_WRITE, NULL, &hk, NULL);
    if (value) RegSetValueExW(hk, NULL, 0, REG_SZ, (const BYTE *)value, (wcslen(value) + 1) * sizeof(WCHAR));
    RegCloseKey(hk);
}
static void reg_clear(void)
{
    RegDeleteTreeW(HKEY_CURRENT_USER, L"Software\\Classes\\.wtsprobe");
    RegDeleteTreeW(HKEY_CURRENT_USER, L"Software\\Classes\\WineTest.PkgProbe");
    RegDeleteTreeW(HKEY_CURRENT_USER, L"Software\\Classes\\CLSID\\" PROBE_CLSID);
}

static void part_g(void)
{
    static const struct { const char *what; BOOL progid_insertable, clsid_insertable, clsid; } cases[] = {
        { "ProgID only, not Insertable", FALSE, FALSE, FALSE },
        { "ProgID with CLSID, neither Insertable", FALSE, FALSE, TRUE },
        { "ProgID Insertable, CLSID without server", TRUE, FALSE, TRUE },
        { "CLSID Insertable, without server", FALSE, TRUE, TRUE },
    };
    WCHAR path[MAX_PATH];
    IStorage *stg;
    IUnknown *obj;
    CLSID clsid;
    HRESULT hr;
    int i;

    printf("G  registered classes (HKCU, no server)\n");
    make_file(L"probe.wtsprobe", "probe\r\n", path);
    for (i = 0; i < ARRAY_SIZE(cases); i++)
    {
        reg_clear();
        reg_set(L".wtsprobe", L"WineTest.PkgProbe");
        reg_set(L"WineTest.PkgProbe", L"Package probe");
        if (cases[i].clsid)
        {
            reg_set(L"WineTest.PkgProbe\\CLSID", PROBE_CLSID);
            reg_set(L"CLSID\\" PROBE_CLSID, L"Package probe");
            reg_set(L"CLSID\\" PROBE_CLSID L"\\ProgID", L"WineTest.PkgProbe");
        }
        if (cases[i].progid_insertable) reg_set(L"WineTest.PkgProbe\\Insertable", NULL);
        if (cases[i].clsid_insertable) reg_set(L"CLSID\\" PROBE_CLSID L"\\Insertable", NULL);
        stg = new_storage(); obj = NULL;
        hr = create(path, stg, &obj);
        memset(&clsid, 0, sizeof(clsid));
        ReadClassStg(stg, &clsid);
        printf("  %s: %#lx, storage class {%08lX-...}\n", cases[i].what, hr, clsid.Data1);
        if (obj) IUnknown_Release(obj);
        IStorage_Release(stg);
    }
    reg_clear();
}

static void part_h(void)
{
    WCHAR path[MAX_PATH], name[200], deep[MAX_PATH], old_tmp[MAX_PATH], old_temp[MAX_PATH];
    IStorage *stg;
    IUnknown *obj, *loaded;
    HRESULT hr;
    int i, n;

    printf("H  loading with a long temporary directory\n");
    for (i = 0; i < 150; i++) name[i] = 'n';
    wcscpy(name + 150, L".txt");
    make_file(name, "long name\r\n", path);
    stg = new_storage(); obj = NULL;
    hr = create(path, stg, &obj);
    printf("  create (name of %u) %#lx\n", (unsigned)wcslen(name), hr);
    if (obj) IUnknown_Release(obj);
    if (FAILED(hr)) { IStorage_Release(stg); return; }

    GetEnvironmentVariableW(L"TMP", old_tmp, MAX_PATH);
    GetEnvironmentVariableW(L"TEMP", old_temp, MAX_PATH);
    wcscpy(deep, dir);
    for (n = 0; wcslen(deep) < 150; n++)
    {
        wcscat(deep, L"\\dddddddddddddddddddd");
        CreateDirectoryW(deep, NULL);
    }
    SetEnvironmentVariableW(L"TMP", deep);
    SetEnvironmentVariableW(L"TEMP", deep);
    loaded = NULL;
    hr = OleLoad(stg, &IID_IUnknown, &site, (void **)&loaded);
    printf("  OleLoad with TMP of %u characters (needs %u) %#lx obj %s\n", (unsigned)wcslen(deep),
           (unsigned)(wcslen(deep) + 1 + wcslen(name)), hr, loaded ? "set" : "NULL");
    if (loaded) IUnknown_Release(loaded);
    SetEnvironmentVariableW(L"TMP", old_tmp);
    SetEnvironmentVariableW(L"TEMP", old_temp);
    IStorage_Release(stg);
    for (; n > 0; n--)
    {
        RemoveDirectoryW(deep);
        *wcsrchr(deep, '\\') = 0;
    }
}

static void print_mfpict(const char *what, HGLOBAL h)
{
    METAFILEPICT *mfp;
    if (!h || !(mfp = GlobalLock(h))) { printf("    %s: none\n", what); return; }
    printf("    %s: mm %ld ext %ldx%ld\n", what, mfp->mm, mfp->xExt, mfp->yExt);
    GlobalUnlock(h);
}

static void part_i(void)
{
    static const WCHAR *names[] = { L"report.txt", L"a much longer file name for the package label.txt" };
    WCHAR path[MAX_PATH];
    SHFILEINFOW info;
    ICONINFO ii;
    BITMAP bm;
    IStorage *stg;
    IUnknown *obj;
    IDataObject *data;
    IViewObject2 *vo;
    FORMATETC fmt = { CF_METAFILEPICT, NULL, DVASPECT_CONTENT, -1, TYMED_MFPICT };
    STGMEDIUM med;
    SIZEL sz;
    HGLOBAL h;
    HRESULT hr;
    int i;

    printf("I  extent sources\n");
    printf("  SM_CXICON %d SM_CYICON %d\n", GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON));
    for (i = 0; i < ARRAY_SIZE(names); i++)
    {
        make_file(names[i], "label probe\r\n", path);
        printf("  %ls\n", names[i]);
        stg = new_storage(); obj = NULL;
        hr = create(path, stg, &obj);
        if (FAILED(hr)) { printf("    create %#lx\n", hr); IStorage_Release(stg); continue; }
        if (SUCCEEDED(IUnknown_QueryInterface(obj, &IID_IViewObject2, (void **)&vo)))
        {
            hr = IViewObject2_GetExtent(vo, DVASPECT_CONTENT, -1, NULL, &sz);
            printf("    View GetExtent %#lx %ldx%ld\n", hr, sz.cx, sz.cy);
            IViewObject2_Release(vo);
        }
        if (SUCCEEDED(IUnknown_QueryInterface(obj, &IID_IDataObject, (void **)&data)))
        {
            memset(&med, 0, sizeof(med));
            hr = IDataObject_GetData(data, &fmt, &med);
            printf("    GetData(CF_METAFILEPICT) %#lx\n", hr);
            if (SUCCEEDED(hr)) { print_mfpict("package picture", med.hMetaFilePict); ReleaseStgMedium(&med); }
            IDataObject_Release(data);
        }
        else printf("    no IDataObject\n");
        IUnknown_Release(obj);
        IStorage_Release(stg);

        memset(&info, 0, sizeof(info));
        if (SHGetFileInfoW(path, 0, &info, sizeof(info), SHGFI_ICON | SHGFI_LARGEICON) && info.hIcon)
        {
            if (GetIconInfo(info.hIcon, &ii))
            {
                GetObjectW(ii.hbmColor ? ii.hbmColor : ii.hbmMask, sizeof(bm), &bm);
                printf("    file icon %ldx%ld\n", bm.bmWidth, bm.bmHeight);
                if (ii.hbmColor) DeleteObject(ii.hbmColor);
                DeleteObject(ii.hbmMask);
            }
            h = OleMetafilePictFromIconAndLabel(info.hIcon, (WCHAR *)names[i], NULL, 0);
            print_mfpict("OleMetafilePictFromIconAndLabel(icon, name)", h);
            if (h) { METAFILEPICT *m = GlobalLock(h); DeleteMetaFile(m->hMF); GlobalUnlock(h); GlobalFree(h); }
            h = OleMetafilePictFromIconAndLabel(info.hIcon, NULL, NULL, 0);
            print_mfpict("OleMetafilePictFromIconAndLabel(icon, NULL)", h);
            if (h) { METAFILEPICT *m = GlobalLock(h); DeleteMetaFile(m->hMF); GlobalUnlock(h); GlobalFree(h); }
            DestroyIcon(info.hIcon);
        }
        else printf("    SHGetFileInfo gave no icon\n");
        DeleteFileW(path);
    }
}

static void part_j(void)
{
    static const WCHAR *names[] = { L"report.txt", L"r.txt", L"a much longer file name for the package label.txt" };
    WCHAR path[MAX_PATH];
    IViewObject2 *vo;
    IStorage *stg;
    IUnknown *obj;
    LOGFONTW lf;
    HFONT font, old;
    TEXTMETRICW tm;
    SIZE text;
    SIZEL sz;
    HDC dc;
    int i;

    printf("J  label size\n");
    if (!SystemParametersInfoW(SPI_GETICONTITLELOGFONT, sizeof(lf), &lf, 0)) { printf("  no icon title font\n"); return; }
    printf("  icon title font %ls height %ld; SM_CXICONSPACING %d SM_CYICONSPACING %d\n", lf.lfFaceName, lf.lfHeight,
           GetSystemMetrics(SM_CXICONSPACING), GetSystemMetrics(SM_CYICONSPACING));
    dc = GetDC(NULL);
    font = CreateFontIndirectW(&lf);
    old = SelectObject(dc, font);
    GetTextMetricsW(dc, &tm);
    printf("  tmHeight %ld tmExternalLeading %ld\n", tm.tmHeight, tm.tmExternalLeading);
    for (i = 0; i < ARRAY_SIZE(names); i++)
    {
        GetTextExtentPoint32W(dc, names[i], lstrlenW(names[i]), &text);
        make_file(names[i], "label probe\r\n", path);
        stg = new_storage(); obj = NULL;
        sz.cx = sz.cy = 0;
        if (SUCCEEDED(create(path, stg, &obj)))
        {
            if (SUCCEEDED(IUnknown_QueryInterface(obj, &IID_IViewObject2, (void **)&vo)))
            {
                IViewObject2_GetExtent(vo, DVASPECT_CONTENT, -1, NULL, &sz);
                IViewObject2_Release(vo);
            }
            IUnknown_Release(obj);
        }
        IStorage_Release(stg);
        DeleteFileW(path);
        printf("  %ls: text %ldx%ld px, extent %ldx%ld HIMETRIC = %.2fx%.2f px at 96 dpi\n", names[i], text.cx, text.cy,
               sz.cx, sz.cy, sz.cx * 96.0 / 2540, sz.cy * 96.0 / 2540);
    }
    SelectObject(dc, old);
    DeleteObject(font);
    ReleaseDC(NULL, dc);
}

static void remove_dir(const WCHAR *d)
{
    WIN32_FIND_DATAW fd;
    WCHAR pat[MAX_PATH], p[MAX_PATH];
    HANDLE h;
    swprintf(pat, MAX_PATH, L"%ls\\*", d);
    if ((h = FindFirstFileW(pat, &fd)) == INVALID_HANDLE_VALUE) return;
    do
    {
        if (!wcscmp(fd.cFileName, L".") || !wcscmp(fd.cFileName, L"..")) continue;
        swprintf(p, MAX_PATH, L"%ls\\%ls", d, fd.cFileName);
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) remove_dir(p); else DeleteFileW(p);
    } while (FindNextFileW(h, &fd));
    FindClose(h);
    RemoveDirectoryW(d);
}


static void set_tmp(const WCHAR *d)
{
    SetEnvironmentVariableW(L"TMP", d);
    SetEnvironmentVariableW(L"TEMP", d);
}

/* a directory whose GetTempPath length (with the trailing backslash) is len */
static BOOL make_dir_of_len(WCHAR *out, int len)
{
    int i, n;
    wcscpy(out, dir);
    n = len - 1 - (int)wcslen(out);
    if (n < 2) return FALSE;
    wcscat(out, L"\\");
    n--;
    i = wcslen(out);
    while (n-- > 0) out[i++] = 'k';
    out[i] = 0;
    return CreateDirectoryW(out, NULL) || GetLastError() == ERROR_ALREADY_EXISTS;
}

static void part_k(void)
{
    static const int lens[] = { 100, 104, 105, 106, 108, 109, 110 };
    WCHAR name[200], path[MAX_PATH], tdir[MAX_PATH], got[MAX_PATH + 2], old_tmp[MAX_PATH], old_temp[MAX_PATH];
    IStorage *stg;
    IUnknown *obj, *a, *b;
    HRESULT hr, hr2;
    int i;

    printf("K  temporary directory near the limit (file name of 150)\n");
    for (i = 0; i < 146; i++) name[i] = 'm';
    wcscpy(name + 146, L".txt");
    make_file(name, "near the limit\r\n", path);
    stg = new_storage(); obj = NULL;
    hr = create(path, stg, &obj);
    if (obj) IUnknown_Release(obj);
    if (FAILED(hr)) { printf("  create %#lx\n", hr); IStorage_Release(stg); return; }
    GetEnvironmentVariableW(L"TMP", old_tmp, MAX_PATH);
    GetEnvironmentVariableW(L"TEMP", old_temp, MAX_PATH);
    for (i = 0; i < ARRAY_SIZE(lens); i++)
    {
        if (!make_dir_of_len(tdir, lens[i])) { printf("  len %d: no dir\n", lens[i]); continue; }
        set_tmp(tdir);
        GetTempPathW(ARRAY_SIZE(got), got);
        a = b = NULL;
        hr = OleLoad(stg, &IID_IUnknown, &site, (void **)&a);
        hr2 = OleLoad(stg, &IID_IUnknown, &site, (void **)&b);
        printf("  GetTempPath %u + name %u + 1 = %u: first load %#lx, second (collides) %#lx\n",
               (unsigned)wcslen(got), (unsigned)wcslen(name), (unsigned)(wcslen(got) + wcslen(name) + 1), hr, hr2);
        if (b) IUnknown_Release(b);
        if (a) IUnknown_Release(a);
        set_tmp(old_tmp[0] ? old_tmp : old_temp);
        SetEnvironmentVariableW(L"TEMP", old_temp);
        remove_dir(tdir);
    }
    IStorage_Release(stg);
    DeleteFileW(path);
}

static void bbox(HDC dc, int w, int h, RECT *r)
{
    int x, y;
    SetRect(r, w, h, -1, -1);
    for (y = 0; y < h; y++)
        for (x = 0; x < w; x++)
            if (GetPixel(dc, x, y) != RGB(255, 255, 255))
            {
                if (x < r->left) r->left = x;
                if (y < r->top) r->top = y;
                if (x > r->right) r->right = x;
                if (y > r->bottom) r->bottom = y;
            }
}

static void part_l(void)
{
    WCHAR txt[MAX_PATH];
    IStorage *stg = new_storage();
    IViewObject2 *vo;
    IUnknown *obj;
    HDC screen, dc;
    HBITMAP bmp, old;
    RECTL bounds;
    RECT box;
    SIZE ext;
    POINT org;
    HRESULT hr;
    int pass;

    printf("L  Draw on mapped DCs (bitmap 400x400, white)\n");
    make_file(L"draw.txt", "draw\r\n", txt);
    if (FAILED(hr = create(txt, stg, &obj))) { printf("  create %#lx\n", hr); IStorage_Release(stg); return; }
    if (FAILED(IUnknown_QueryInterface(obj, &IID_IViewObject2, (void **)&vo))) { printf("  no view\n"); return; }
    screen = GetDC(NULL);
    dc = CreateCompatibleDC(screen);
    bmp = CreateCompatibleBitmap(screen, 400, 400);
    ReleaseDC(NULL, screen);
    old = SelectObject(dc, bmp);
    for (pass = 0; pass < 3; pass++)
    {
        PatBlt(dc, 0, 0, 400, 400, WHITENESS);
        SetMapMode(dc, MM_TEXT);
        SetWindowOrgEx(dc, 0, 0, NULL);
        SetViewportOrgEx(dc, 0, 0, NULL);
        if (pass == 0) SetRect((RECT *)&bounds, 10, 10, 110, 110);
        else
        {
            SetMapMode(dc, MM_ANISOTROPIC);
            SetWindowExtEx(dc, 100, 100, NULL);
            SetViewportExtEx(dc, 200, 200, NULL);
            SetViewportOrgEx(dc, 50, 50, NULL);
            if (pass == 2) SetWindowOrgEx(dc, -20, -20, NULL);
            SetRect((RECT *)&bounds, 10, 10, 60, 60);
        }
        hr = IViewObject2_Draw(vo, DVASPECT_CONTENT, -1, NULL, NULL, NULL, dc, &bounds, NULL, NULL, 0);
        GetWindowExtEx(dc, &ext);
        GetViewportOrgEx(dc, &org);
        SetMapMode(dc, MM_TEXT);
        SetWindowOrgEx(dc, 0, 0, NULL);
        SetViewportOrgEx(dc, 0, 0, NULL);
        bbox(dc, 400, 400, &box);
        printf("  pass %d (%s): Draw %#lx, drawn pixels x %ld..%ld y %ld..%ld; after Draw window ext %ldx%ld viewport org %ld,%ld\n",
               pass, pass == 0 ? "MM_TEXT, bounds 10,10-110,110" :
               pass == 1 ? "x2 scale, viewport org 50,50, bounds 10,10-60,60 = device 70,70-170,170" :
                           "as 1 plus window org -20,-20 = device 110,110-210,210",
               hr, box.left, box.right, box.top, box.bottom, ext.cx, ext.cy, org.x, org.y);
    }
    SelectObject(dc, old);
    DeleteObject(bmp);
    DeleteDC(dc);
    IViewObject2_Release(vo);
    IUnknown_Release(obj);
    IStorage_Release(stg);
}

static void part_m(void)
{
    static const WCHAR *names[] = { L"r.txt", L"report.txt", L"a much longer file name for the package label.txt" };
    static const struct { const char *what; int stock; } stocks[] = {
        { "SYSTEM_FONT", SYSTEM_FONT }, { "DEFAULT_GUI_FONT", DEFAULT_GUI_FONT },
        { "ANSI_VAR_FONT", ANSI_VAR_FONT }, { "SYSTEM_FIXED_FONT", SYSTEM_FIXED_FONT },
    };
    NONCLIENTMETRICSW ncm = { sizeof(ncm) };
    LOGFONTW lfs[4];
    const char *lfnames[] = { "icon title", "message", "caption", "status" };
    HFONT font, old;
    TEXTMETRICW tm;
    SIZE text;
    HDC dc;
    int i, j;

    printf("M  label width per font (screen DC)\n");
    SystemParametersInfoW(SPI_GETICONTITLELOGFONT, sizeof(lfs[0]), &lfs[0], 0);
    SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0);
    lfs[1] = ncm.lfMessageFont; lfs[2] = ncm.lfCaptionFont; lfs[3] = ncm.lfStatusFont;
    dc = GetDC(NULL);
    for (j = 0; j < ARRAY_SIZE(stocks) + ARRAY_SIZE(lfs); j++)
    {
        if (j < ARRAY_SIZE(stocks)) { font = GetStockObject(stocks[j].stock); old = SelectObject(dc, font); }
        else { font = CreateFontIndirectW(&lfs[j - ARRAY_SIZE(stocks)]); old = SelectObject(dc, font); }
        GetTextMetricsW(dc, &tm);
        printf("  %-18s h %2ld:", j < ARRAY_SIZE(stocks) ? stocks[j].what : lfnames[j - ARRAY_SIZE(stocks)], tm.tmHeight);
        for (i = 0; i < ARRAY_SIZE(names); i++)
        {
            GetTextExtentPoint32W(dc, names[i], lstrlenW(names[i]), &text);
            printf(" %ld", text.cx);
        }
        printf("\n");
        SelectObject(dc, old);
        if (j >= ARRAY_SIZE(stocks)) DeleteObject(font);
    }
    ReleaseDC(NULL, dc);
    printf("  (measured extents 40, 62, 325 x 54 px)\n");
}


static void part_n(void)
{
    static const int heights[] = { -9, -11, -12, -13, -15, -16, -18, -20, -24, -30 };
    static const WCHAR *names[] = { L"r.txt", L"report.txt", L"a much longer file name for the package label.txt" };
    LOGFONTW orig, lf;
    WCHAR path[MAX_PATH];
    IViewObject2 *vo;
    IStorage *stg;
    IUnknown *obj;
    TEXTMETRICW tm;
    HFONT font, old;
    SIZE text;
    SIZEL sz;
    HDC dc;
    int h, i;

    printf("N  extent per icon title font size (session only)\n");
    if (!SystemParametersInfoW(SPI_GETICONTITLELOGFONT, sizeof(orig), &orig, 0)) return;
    for (h = 0; h < ARRAY_SIZE(heights); h++)
    {
        lf = orig;
        lf.lfHeight = heights[h];
        if (!SystemParametersInfoW(SPI_SETICONTITLELOGFONT, sizeof(lf), &lf, 0)) { printf("  set failed %lu\n", GetLastError()); break; }
        dc = GetDC(NULL);
        font = CreateFontIndirectW(&lf);
        old = SelectObject(dc, font);
        GetTextMetricsW(dc, &tm);
        printf("  lfHeight %d tmHeight %ld ascent %ld descent %ld:", heights[h], tm.tmHeight, tm.tmAscent, tm.tmDescent);
        for (i = 0; i < ARRAY_SIZE(names); i++)
        {
            GetTextExtentPoint32W(dc, names[i], lstrlenW(names[i]), &text);
            make_file(names[i], "n\r\n", path);
            stg = new_storage(); obj = NULL; sz.cx = sz.cy = 0;
            if (SUCCEEDED(create(path, stg, &obj)))
            {
                if (SUCCEEDED(IUnknown_QueryInterface(obj, &IID_IViewObject2, (void **)&vo)))
                {
                    IViewObject2_GetExtent(vo, DVASPECT_CONTENT, -1, NULL, &sz);
                    IViewObject2_Release(vo);
                }
                IUnknown_Release(obj);
            }
            IStorage_Release(stg);
            DeleteFileW(path);
            printf(" text %ldx%ld -> %.2fx%.2f px;", text.cx, text.cy, sz.cx * 96.0 / 2540, sz.cy * 96.0 / 2540);
        }
        printf("\n");
        SelectObject(dc, old);
        DeleteObject(font);
        ReleaseDC(NULL, dc);
    }
    SystemParametersInfoW(SPI_SETICONTITLELOGFONT, sizeof(orig), &orig, 0);
}


static HRESULT write_ansi_package(IStorage *stg, const char *name)
{
    static const CLSID clsid_package = {0x0003000c, 0, 0, {0xc0, 0, 0, 0, 0, 0, 0, 0x46}};
    static const WORD type = 2, unknown[2] = { 0, 3 };
    static const char data[] = "ansi only\r\n";
    char path[MAX_PATH];
    DWORD total, n, size = sizeof(data) - 1;
    IStream *stm;
    HRESULT hr;

    sprintf(path, "C:\\%s", name);
    total = sizeof(type) + strlen(name) + 1 + strlen(path) + 1 + sizeof(unknown) + 4 + strlen(path) + 1 + 4 + size;
    hr = IStorage_CreateStream(stg, L"\1Ole10Native", STGM_CREATE | STGM_WRITE | STGM_SHARE_EXCLUSIVE, 0, 0, &stm);
    if (FAILED(hr)) return hr;
    IStream_Write(stm, &total, 4, NULL);
    IStream_Write(stm, &type, 2, NULL);
    IStream_Write(stm, name, strlen(name) + 1, NULL);
    IStream_Write(stm, path, strlen(path) + 1, NULL);
    IStream_Write(stm, unknown, 4, NULL);
    n = strlen(path) + 1;
    IStream_Write(stm, &n, 4, NULL);
    IStream_Write(stm, path, n, NULL);
    IStream_Write(stm, &size, 4, NULL);
    IStream_Write(stm, data, size, NULL);
    IStream_Release(stm);
    return WriteClassStg(stg, &clsid_package);
}

static void part_o(void)
{
    static const int lens[] = { 100, 105, 106, 108, 109, 110 };
    WCHAR tdir[MAX_PATH], got[MAX_PATH + 2], old_tmp[MAX_PATH], old_temp[MAX_PATH];
    char name[200];
    IStorage *stg;
    IUnknown *a, *b;
    HRESULT hr, hr2;
    int i;

    printf("O  ANSI-only Package (no UTF-16 names), file name of 150, near the limit\n");
    memset(name, 'q', 146);
    strcpy(name + 146, ".txt");
    stg = new_storage();
    hr = write_ansi_package(stg, name);
    if (FAILED(hr)) { printf("  write %#lx\n", hr); return; }
    a = NULL;
    hr = OleLoad(stg, &IID_IUnknown, &site, (void **)&a);
    printf("  load with the normal TMP: %#lx\n", hr);
    if (a) IUnknown_Release(a);
    GetEnvironmentVariableW(L"TMP", old_tmp, MAX_PATH);
    GetEnvironmentVariableW(L"TEMP", old_temp, MAX_PATH);
    for (i = 0; i < ARRAY_SIZE(lens); i++)
    {
        if (!make_dir_of_len(tdir, lens[i])) { printf("  len %d: no dir\n", lens[i]); continue; }
        set_tmp(tdir);
        GetTempPathW(ARRAY_SIZE(got), got);
        a = b = NULL;
        hr = OleLoad(stg, &IID_IUnknown, &site, (void **)&a);
        hr2 = OleLoad(stg, &IID_IUnknown, &site, (void **)&b);
        printf("  GetTempPath %u + name 150 + 1 = %u: first load %#lx, second (collides) %#lx\n",
               (unsigned)wcslen(got), (unsigned)(wcslen(got) + 151), hr, hr2);
        if (b) IUnknown_Release(b);
        if (a) IUnknown_Release(a);
        SetEnvironmentVariableW(L"TMP", old_tmp);
        SetEnvironmentVariableW(L"TEMP", old_temp);
        remove_dir(tdir);
    }
    IStorage_Release(stg);
}

int wmain(int argc, WCHAR **argv)
{
    WCHAR tmp[MAX_PATH], txt[MAX_PATH];
    OSVERSIONINFOW v = { sizeof(v) };

    GetVersionExW(&v);
    printf("pkgprobe: Windows %lu.%lu.%lu, ACP %u, %u-bit\n", v.dwMajorVersion, v.dwMinorVersion,
           v.dwBuildNumber, GetACP(), (unsigned)sizeof(void *) * 8);
    OleInitialize(NULL);
    GetTempPathW(MAX_PATH, tmp);
    swprintf(dir, MAX_PATH, L"%lspkgprobe-%lu", tmp, GetCurrentProcessId());
    CreateDirectoryW(dir, NULL);
    make_file(L"report.txt", "report text\r\n", txt);

    if (argc > 1 && !wcscmp(argv[1], L"o"))
    {
        part_o();
        OleUninitialize();
        remove_dir(dir);
        return 0;
    }
    if (argc > 1 && !wcscmp(argv[1], L"n"))
    {
        part_n();
        OleUninitialize();
        remove_dir(dir);
        return 0;
    }
    part_a(txt);
    part_bcd(txt);
    part_e();
    part_f(txt);
    part_g();
    part_h();
    part_i();
    part_j();
    part_k();
    part_l();
    part_m();

    OleUninitialize();
    remove_dir(dir);
    printf("done\n");
    return 0;
}
