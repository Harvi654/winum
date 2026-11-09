/* Original, AI-assisted (Codex), LGPL-2.1-or-later.
 * Own Windows 10 MEASURE, x86_64/i686, 2026-11-09; valid functional inputs.
 * No NULL-output/invalid-PIDL or unspecified array-order assertions. */
struct selection_host { IShellBrowser iface; LONG refs; HWND window; IShellView *view; };
static ULONG WINAPI selection_host_add(IShellBrowser *iface) { return InterlockedIncrement(&((struct selection_host*)iface)->refs); }
static ULONG WINAPI selection_host_release(IShellBrowser *iface) { return InterlockedDecrement(&((struct selection_host*)iface)->refs); }
static HRESULT WINAPI selection_host_query(IShellBrowser *iface,REFIID iid,void **out)
{
    *out=NULL;
    if(!IsEqualIID(iid,&IID_IUnknown)&&!IsEqualIID(iid,&IID_IOleWindow)&&!IsEqualIID(iid,&IID_IShellBrowser)) return E_NOINTERFACE;
    *out=iface;selection_host_add(iface);return S_OK;
}
static HRESULT WINAPI selection_host_window(IShellBrowser *iface,HWND *out) { *out=((struct selection_host*)iface)->window;return S_OK; }
static HRESULT WINAPI selection_host_help(IShellBrowser *iface,BOOL enter) { (void)iface;(void)enter;return E_NOTIMPL; }
static HRESULT WINAPI selection_host_insert(IShellBrowser *iface,HMENU menu,LPOLEMENUGROUPWIDTHS widths)
{ (void)iface;(void)menu;if(widths)memset(widths,0,sizeof(*widths));return S_OK; }
static HRESULT WINAPI selection_host_menu(IShellBrowser *iface,HMENU menu,HOLEMENU reserved,HWND active)
{ (void)iface;(void)menu;(void)reserved;(void)active;return S_OK; }
static HRESULT WINAPI selection_host_remove(IShellBrowser *iface,HMENU menu) { (void)iface;(void)menu;return S_OK; }
static HRESULT WINAPI selection_host_status(IShellBrowser *iface,LPCOLESTR text) { (void)iface;(void)text;return S_OK; }
static HRESULT WINAPI selection_host_modeless(IShellBrowser *iface,BOOL enable) { (void)iface;(void)enable;return S_OK; }
static HRESULT WINAPI selection_host_accel(IShellBrowser *iface,MSG *message,WORD id)
{ (void)iface;(void)message;(void)id;return S_FALSE; }
static HRESULT WINAPI selection_host_browse(IShellBrowser *iface,PCUIDLIST_RELATIVE pidl,UINT flags)
{ (void)iface;(void)pidl;(void)flags;return E_NOTIMPL; }
static HRESULT WINAPI selection_host_stream(IShellBrowser *iface,DWORD mode,IStream **stream)
{ (void)iface;(void)mode;*stream=NULL;return E_NOTIMPL; }
static HRESULT WINAPI selection_host_control(IShellBrowser *iface,UINT id,HWND *out)
{ (void)iface;(void)id;*out=NULL;return E_NOTIMPL; }
static HRESULT WINAPI selection_host_message(IShellBrowser *iface,UINT id,UINT message,WPARAM wp,LPARAM lp,LRESULT *out)
{ (void)iface;(void)id;(void)message;(void)wp;(void)lp;if(out)*out=0;return E_NOTIMPL; }
static HRESULT WINAPI selection_host_active(IShellBrowser *iface,IShellView **out)
{ *out=((struct selection_host*)iface)->view;if(!*out)return E_FAIL;IShellView_AddRef(*out);return S_OK; }
static HRESULT WINAPI selection_host_activate(IShellBrowser *iface,IShellView *view) { (void)iface;(void)view;return S_OK; }
static HRESULT WINAPI selection_host_toolbar(IShellBrowser *iface,LPTBBUTTONSB buttons,UINT count,UINT flags)
{ (void)iface;(void)buttons;(void)count;(void)flags;return E_NOTIMPL; }
static const IShellBrowserVtbl selection_host_vtable={
    selection_host_query,selection_host_add,selection_host_release,selection_host_window,selection_host_help,selection_host_insert,selection_host_menu,selection_host_remove,
    selection_host_status,selection_host_modeless,selection_host_accel,selection_host_browse,selection_host_stream,selection_host_control,selection_host_message,
    selection_host_active,selection_host_activate,selection_host_toolbar
};
static const WCHAR *selection_names[]={L"aa.fixture",L"bb.fixture",L"cc.fixture"};
static void selection_pump(DWORD duration)
{
    DWORD start=GetTickCount();
    do {
        MSG message;
        while(GetTickCount()-start<duration && PeekMessageW(&message,NULL,0,0,PM_REMOVE)) {
            TranslateMessage(&message);DispatchMessageW(&message);
        }
        Sleep(10);
    }while(GetTickCount()-start<duration);
}

static unsigned int selection_completed;

static BOOL selection_hr(HRESULT hr, const char *operation)
{
    ok(hr == S_OK, "%s returned %#lx\n", operation, hr);
    return hr == S_OK;
}

static BOOL selection_path(WCHAR *path, const WCHAR *directory, const WCHAR *leaf)
{
    int length = swprintf(path, MAX_PATH, L"%ls\\%ls", directory, leaf);
    ok(length >= 0 && length < MAX_PATH, "Own file path does not fit\n");
    return length >= 0 && length < MAX_PATH;
}

static BOOL selection_directory(WCHAR *directory)
{
    WCHAR parent[MAX_PATH];
    DWORD length = GetTempPathW(MAX_PATH, parent);
    BOOL success;
    ok(length && length < MAX_PATH, "Temporary directory query failed\n");
    if(!length || length >= MAX_PATH) return FALSE;
    success = GetTempFileNameW(parent, L"w3m", 0, directory) != 0;
    ok(success, "Own temporary path allocation failed\n");
    if(!success) return FALSE;
    success = DeleteFileW(directory);
    ok(success, "Own temporary placeholder removal failed\n");
    if(!success) return FALSE;
    success = CreateDirectoryW(directory, NULL);
    ok(success, "Own directory creation failed\n");
    return success;
}

static BOOL selection_file(const WCHAR *path)
{
    HANDLE file = CreateFileW(path, GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
    DWORD written = 0;
    BOOL success;
    ok(file != INVALID_HANDLE_VALUE, "Own fixture file creation failed\n");
    if(file == INVALID_HANDLE_VALUE) return FALSE;
    success = WriteFile(file, "own", 3, &written, NULL);
    ok(success && written == 3, "Own fixture file write failed\n");
    ok(CloseHandle(file), "Own fixture file close failed\n");
    return success && written == 3;
}

static unsigned int selection_popcount(unsigned int mask)
{
    unsigned int count = 0;
    while(mask) { count += mask & 1; mask >>= 1; }
    return count;
}

static void selection_array(HRESULT hr, IShellItemArray *array, const char *operation,
                             unsigned int expected, const WCHAR *folder_leaf)
{
    HRESULT count_hr;
    DWORD count = 0, i;
    unsigned int seen = 0, identity, j;
    ok(hr == (expected ? S_OK : HRESULT_FROM_WIN32(ERROR_NOT_FOUND)),
       "%s: array result %#lx\n", operation, hr);
    ok((array != NULL) == (expected != 0), "%s: unexpected array presence\n", operation);
    if(!array) return;
    count_hr = IShellItemArray_GetCount(array, &count);
    ok(count_hr == S_OK, "%s: GetCount returned %#lx\n", operation, count_hr);
    ok(count == selection_popcount(expected), "%s: array count %lu, expected %u\n",
       operation, count, selection_popcount(expected));
    if(count_hr != S_OK || count > 4) goto done;
    for(i = 0; i < count; ++i)
    {
        IShellItem *item = NULL;
        WCHAR *name = NULL;
        HRESULT item_hr = IShellItemArray_GetItemAt(array, i, &item), name_hr;
        ok(item_hr == S_OK, "%s: GetItemAt returned %#lx\n", operation, item_hr);
        ok(item != NULL, "%s: missing array item\n", operation);
        if(!item) continue;
        name_hr = IShellItem_GetDisplayName(item, SIGDN_PARENTRELATIVEPARSING, &name);
        ok(name_hr == S_OK, "%s: GetDisplayName returned %#lx\n", operation, name_hr);
        ok(name != NULL, "%s: missing own-item name\n", operation);
        identity = 0;
        if(name)
        {
            for(j = 0; j < 3; ++j) if(!lstrcmpiW(name, selection_names[j])) identity = 1u << j;
            if(!lstrcmpiW(name, folder_leaf)) identity = 8;
        }
        ok(identity && !(seen & identity), "%s: unknown or duplicate own item\n", operation);
        seen |= identity;
        CoTaskMemFree(name);
        IShellItem_Release(item);
    }
    ok(seen == expected, "%s: array identities %#x, expected %#x\n", operation, seen, expected);
done:
    IShellItemArray_Release(array);
}

static void selection_state(IFolderView2 *view, ITEMIDLIST **items, const int *indices,
                             unsigned int expected, int focus, const WCHAR *folder_leaf)
{
    static const UINT modes[] = {SVGIO_ALLVIEW, SVGIO_SELECTION, SVGIO_SELECTION | SVGIO_FLAG_VIEWORDER};
    unsigned int i, j;
    HRESULT hr;
    for(i = 0; i < ARRAY_SIZE(modes); ++i)
    {
        IShellItemArray *array = NULL;
        unsigned int mask = i ? expected : 7;
        int count = -99;
        hr = IFolderView2_ItemCount(view, modes[i], &count);
        ok(hr == S_OK, "ItemCount returned %#lx\n", hr);
        ok(count == (int)selection_popcount(mask), "ItemCount returned %d, expected %u\n", count, selection_popcount(mask));
        hr = IFolderView2_Items(view, modes[i], &IID_IShellItemArray, (void **)&array);
        selection_array(hr, array, "Items", mask, folder_leaf);
    }
    for(i = 0; i < 2; ++i)
    {
        IShellItemArray *array = NULL;
        hr = IFolderView2_GetSelection(view, i, &array);
        selection_array(hr, array, "GetSelection", expected ? expected : i ? 8 : 0, folder_leaf);
    }
    for(i = 0; i < 4; ++i)
    {
        int index = -99, start = (int)i - 1, next = -1;
        for(j = 0; j < 3; ++j)
            if((expected & (1u << j)) && indices[j] > start && (next == -1 || indices[j] < next)) next = indices[j];
        hr = IFolderView2_GetSelectedItem(view, start, &index);
        ok(hr == (next == -1 ? S_FALSE : S_OK), "GetSelectedItem start %d returned %#lx\n", start, hr);
        ok(index == next, "GetSelectedItem start %d returned %d, expected %d\n", start, index, next);
    }
    for(i = 0; i < 3; ++i)
    {
        DWORD state = 0xdeadbeef, flags = (expected & (1u << i) ? SVSI_SELECT : 0) | ((int)i == focus ? SVSI_FOCUSED : 0);
        hr = IFolderView2_GetSelectionState(view, items[i], &state);
        ok(hr == S_OK, "GetSelectionState returned %#lx\n", hr);
        ok(state == flags, "Own item %u state %#lx, expected %#lx\n", i, state, flags);
    }
    ++selection_completed;
}

static void test_folderview_selection(void)
{
    struct selection_host host = {{&selection_host_vtable}, 1, NULL, NULL};
    IShellFolder *desktop = NULL, *folder = NULL;
    IFolderView2 *view = NULL;
    ITEMIDLIST *absolute = NULL, *items[3] = {NULL, NULL, NULL};
    WCHAR directory[MAX_PATH], file[MAX_PATH];
    SFV_CREATE create = {0};
    FOLDERSETTINGS settings = {FVM_DETAILS, FWF_NOCLIENTEDGE};
    RECT rect = {0, 0, 500, 320};
    HWND view_window = NULL;
    BOOL own_directory = FALSE, activated = FALSE;
    HRESULT hr = OleInitialize(NULL), ready_hr = E_UNEXPECTED;
    int count = 0, indices[3] = {-1, -1, -1};
    unsigned int i, j;
    DWORD start;

    selection_completed = 0;
    if(!selection_hr(hr, "OleInitialize")) { if(SUCCEEDED(hr)) OleUninitialize(); return; }
    if(!selection_directory(directory)) goto done;
    own_directory = TRUE;
    for(i = 0; i < 3; ++i) if(!selection_path(file, directory, selection_names[i]) || !selection_file(file)) goto done;
    host.window = CreateWindowExW(0, L"static", L"Winum Folder View Measure", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                                  80, 80, 520, 360, NULL, NULL, GetModuleHandleW(NULL), NULL);
    ok(host.window != NULL, "Own host window creation failed\n");
    if(!host.window) goto done;
    if(!selection_hr(SHGetDesktopFolder(&desktop), "SHGetDesktopFolder")) goto done;
    ok(desktop != NULL, "Missing desktop interface\n");
    if(!desktop) goto done;
    if(!selection_hr(IShellFolder_ParseDisplayName(desktop, host.window, NULL, directory, NULL, &absolute, NULL), "ParseOwnDirectory")) goto done;
    ok(absolute != NULL, "Missing own directory PIDL\n");
    if(!absolute) goto done;
    if(!selection_hr(IShellFolder_BindToObject(desktop, absolute, NULL, &IID_IShellFolder, (void **)&folder), "BindOwnDirectory")) goto done;
    ok(folder != NULL, "Missing own folder interface\n");
    if(!folder) goto done;
    for(i = 0; i < 3; ++i)
    {
        WCHAR name[32];
        lstrcpyW(name, selection_names[i]);
        if(!selection_hr(IShellFolder_ParseDisplayName(folder, host.window, NULL, name, NULL, &items[i], NULL), "ParseOwnItem")) goto done;
        ok(items[i] != NULL, "Missing own item PIDL\n");
        if(!items[i]) goto done;
    }
    create.cbSize = sizeof(create);
    create.pshf = folder;
    if(!selection_hr(SHCreateShellFolderView(&create, &host.view), "SHCreateShellFolderView")) goto done;
    ok(host.view != NULL, "Missing own shell view\n");
    if(!host.view) goto done;
    if(!selection_hr(IShellView_CreateViewWindow(host.view, NULL, &settings, &host.iface, &rect, &view_window), "CreateOwnViewWindow")) goto done;
    ok(view_window != NULL, "Missing view window\n");
    if(!view_window) goto done;
    if(!selection_hr(IShellView_QueryInterface(host.view, &IID_IFolderView2, (void **)&view), "QueryFolderView2")) goto done;
    ok(view != NULL, "Missing IFolderView2\n");
    if(!view) goto done;
    if(!selection_hr(IShellView_UIActivate(host.view, SVUIA_ACTIVATE_NOFOCUS), "ActivateOwnView")) goto done;
    activated = TRUE;
    start = GetTickCount();
    do
    {
        ready_hr = IFolderView2_ItemCount(view, SVGIO_ALLVIEW, &count);
        if(ready_hr == S_OK && count == 3) break;
        selection_pump(50);
    }while(GetTickCount() - start < 10000);
    ok(ready_hr == S_OK && count == 3, "Own view is not ready with three items\n");
    if(ready_hr != S_OK || count != 3) goto done;
    for(i = 0; i < 3; ++i)
    {
        ITEMIDLIST *item = NULL;
        hr = IFolderView2_Item(view, i, &item);
        if(!selection_hr(hr, "GetOwnViewItem")) { CoTaskMemFree(item); goto done; }
        ok(item != NULL, "Missing enumerated own item\n");
        if(!item) goto done;
        for(j = 0; j < 3; ++j) if(IShellFolder_CompareIDs(folder, SHCIDS_CANONICALONLY, item, items[j]) == S_OK) indices[j] = i;
        CoTaskMemFree(item);
    }
    ok(indices[0] >= 0 && indices[1] >= 0 && indices[2] >= 0
       && indices[0] != indices[1] && indices[0] != indices[2] && indices[1] != indices[2], "Own item identities are not distinct\n");
    if(indices[0] < 0 || indices[1] < 0 || indices[2] < 0
       || indices[0] == indices[1] || indices[0] == indices[2] || indices[1] == indices[2]) goto done;
    for(i = 0; i < 3; ++i) if(!selection_hr(IFolderView2_SelectItem(view, indices[i], SVSI_DESELECT), "ClearOwnSelection")) goto done;
    selection_pump(50);
    selection_state(view, items, indices, 0, -1, wcsrchr(directory, L'\\') + 1);
    if(!selection_hr(IFolderView2_SelectItem(view, indices[1], SVSI_SELECT | SVSI_DESELECTOTHERS | SVSI_FOCUSED), "SelectOwnB")) goto done;
    selection_pump(50);
    selection_state(view, items, indices, 2, 1, wcsrchr(directory, L'\\') + 1);
    if(!selection_hr(IFolderView2_SelectItem(view, indices[2], SVSI_SELECT), "SelectOwnC")) goto done;
    selection_pump(50);
    selection_state(view, items, indices, 6, 1, wcsrchr(directory, L'\\') + 1);
    for(i = 0; i < 3; ++i) if(!selection_hr(IFolderView2_SelectItem(view, indices[i], SVSI_DESELECT), "ClearOwnSelection")) goto done;
    selection_pump(50);
    selection_state(view, items, indices, 0, 1, wcsrchr(directory, L'\\') + 1);
    ok(selection_completed == 4, "Only %u selection states completed\n", selection_completed);
done:
    if(view) IFolderView2_Release(view);
    if(activated) selection_hr(IShellView_UIActivate(host.view, SVUIA_DEACTIVATE), "DeactivateOwnView");
    if(view_window) selection_hr(IShellView_DestroyViewWindow(host.view), "DestroyOwnViewWindow");
    if(host.view) { IShellView_Release(host.view); host.view = NULL; }
    if(folder) IShellFolder_Release(folder);
    if(desktop) IShellFolder_Release(desktop);
    CoTaskMemFree(absolute);
    for(i = 0; i < 3; ++i) CoTaskMemFree(items[i]);
    if(host.window) ok(DestroyWindow(host.window), "Own host window destruction failed\n");
    OleUninitialize();
    if(own_directory)
    {
        for(i = 0; i < 3; ++i) if(selection_path(file, directory, selection_names[i]))
        {
            BOOL removed = DeleteFileW(file);
            ok(removed || GetLastError() == ERROR_FILE_NOT_FOUND, "Own fixture file removal failed\n");
        }
        ok(RemoveDirectoryW(directory), "Own directory removal failed\n");
    }
    ok(host.refs == 1, "Own host refs after cleanup %ld, expected one\n", host.refs);
}
