/* Original, AI-assisted (Codex), LGPL-2.1-or-later.
 * Standard shell view hosted over three own files, no Explorer automation. */
#define COBJMACROS
#define CONST_VTABLE
#include "../batch3-measure/measure-support.h"
#include <ole2.h>
#include <shlobj.h>
#include <shobjidl.h>
struct host { IShellBrowser iface; LONG refs; HWND window; IShellView *view; };
static ULONG WINAPI host_add(IShellBrowser *iface) { return InterlockedIncrement(&((struct host*)iface)->refs); }
static ULONG WINAPI host_release(IShellBrowser *iface) { return InterlockedDecrement(&((struct host*)iface)->refs); }
static HRESULT WINAPI host_query(IShellBrowser *iface,REFIID iid,void **out)
{
    *out=NULL;
    if(!IsEqualIID(iid,&IID_IUnknown)&&!IsEqualIID(iid,&IID_IOleWindow)&&!IsEqualIID(iid,&IID_IShellBrowser)) return E_NOINTERFACE;
    *out=iface;host_add(iface);return S_OK;
}
static HRESULT WINAPI host_window(IShellBrowser *iface,HWND *out) { *out=((struct host*)iface)->window;return S_OK; }
static HRESULT WINAPI host_help(IShellBrowser *iface,BOOL enter) { (void)iface;(void)enter;return E_NOTIMPL; }
static HRESULT WINAPI host_insert(IShellBrowser *iface,HMENU menu,LPOLEMENUGROUPWIDTHS widths)
{ (void)iface;(void)menu;if(widths)memset(widths,0,sizeof(*widths));return S_OK; }
static HRESULT WINAPI host_menu(IShellBrowser *iface,HMENU menu,HOLEMENU reserved,HWND active)
{ (void)iface;(void)menu;(void)reserved;(void)active;return S_OK; }
static HRESULT WINAPI host_remove(IShellBrowser *iface,HMENU menu) { (void)iface;(void)menu;return S_OK; }
static HRESULT WINAPI host_status(IShellBrowser *iface,LPCOLESTR text) { (void)iface;(void)text;return S_OK; }
static HRESULT WINAPI host_modeless(IShellBrowser *iface,BOOL enable) { (void)iface;(void)enable;return S_OK; }
static HRESULT WINAPI host_accel(IShellBrowser *iface,MSG *message,WORD id)
{ (void)iface;(void)message;(void)id;return S_FALSE; }
static HRESULT WINAPI host_browse(IShellBrowser *iface,PCUIDLIST_RELATIVE pidl,UINT flags)
{ (void)iface;(void)pidl;(void)flags;return E_NOTIMPL; }
static HRESULT WINAPI host_stream(IShellBrowser *iface,DWORD mode,IStream **stream)
{ (void)iface;(void)mode;*stream=NULL;return E_NOTIMPL; }
static HRESULT WINAPI host_control(IShellBrowser *iface,UINT id,HWND *out)
{ (void)iface;(void)id;*out=NULL;return E_NOTIMPL; }
static HRESULT WINAPI host_message(IShellBrowser *iface,UINT id,UINT message,WPARAM wp,LPARAM lp,LRESULT *out)
{ (void)iface;(void)id;(void)message;(void)wp;(void)lp;if(out)*out=0;return E_NOTIMPL; }
static HRESULT WINAPI host_active(IShellBrowser *iface,IShellView **out)
{ *out=((struct host*)iface)->view;if(!*out)return E_FAIL;IShellView_AddRef(*out);return S_OK; }
static HRESULT WINAPI host_activate(IShellBrowser *iface,IShellView *view) { (void)iface;(void)view;return S_OK; }
static HRESULT WINAPI host_toolbar(IShellBrowser *iface,LPTBBUTTONSB buttons,UINT count,UINT flags)
{ (void)iface;(void)buttons;(void)count;(void)flags;return E_NOTIMPL; }
static const IShellBrowserVtbl host_vtable={
    host_query,host_add,host_release,host_window,host_help,host_insert,host_menu,host_remove,
    host_status,host_modeless,host_accel,host_browse,host_stream,host_control,host_message,
    host_active,host_activate,host_toolbar
};
static const WCHAR *names[]={L"aa.fixture",L"bb.fixture",L"cc.fixture"};
static void pump(DWORD duration)
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
static void array_result(HRESULT hr,IShellItemArray *array,const char *operation,const WCHAR *folder_leaf)
{
    DWORD count=0,i;
    HRESULT count_hr=array?IShellItemArray_GetCount(array,&count):E_UNEXPECTED;
    printf("ARRAY operation=%s hr=%08lx present=%u count_hr=%08lx count=%lu\n",operation,(DWORD)hr,array!=NULL,(DWORD)count_hr,count);
    if(array && SUCCEEDED(count_hr)) for(i=0;i<count && i<8;i++) {
        IShellItem *item=NULL;
        WCHAR *name=NULL;
        HRESULT item_hr=IShellItemArray_GetItemAt(array,i,&item),name_hr=E_UNEXPECTED;
        if(item) name_hr=IShellItem_GetDisplayName(item,SIGDN_PARENTRELATIVEPARSING,&name);
        printf("ARRAY_ITEM operation=%s index=%lu hr=%08lx name_hr=%08lx own_a=%u own_b=%u own_c=%u own_folder=%u\n",
               operation,i,(DWORD)item_hr,(DWORD)name_hr,name&&!lstrcmpiW(name,names[0]),name&&!lstrcmpiW(name,names[1]),
               name&&!lstrcmpiW(name,names[2]),name&&!lstrcmpiW(name,folder_leaf));
        CoTaskMemFree(name);
        if(item) IShellItem_Release(item);
    }
    if(array) IShellItemArray_Release(array);
}
static void observe(IFolderView2 *view,ITEMIDLIST **items,const char *stage,const WCHAR *folder_leaf)
{
    static const UINT flags[]={SVGIO_ALLVIEW,SVGIO_SELECTION,SVGIO_SELECTION|SVGIO_FLAG_VIEWORDER};
    unsigned int i;
    HRESULT hr;
    printf("STATE name=%s\n",stage);
    for(i=0;i<sizeof(flags)/sizeof(flags[0]);i++) {
        IShellItemArray *array=NULL;
        int count=-99;
        hr=IFolderView2_ItemCount(view,flags[i],&count);
        printf("COUNT flags=%08x hr=%08lx value=%d\n",flags[i],(DWORD)hr,count);
        hr=IFolderView2_Items(view,flags[i],&IID_IShellItemArray,(void**)&array);
        printf("ITEMS flags=%08x\n",flags[i]);
        array_result(hr,array,"Items",folder_leaf);
    }
    for(i=0;i<2;i++) {
        IShellItemArray *array=NULL;
        hr=IFolderView2_GetSelection(view,i,&array);
        printf("GET_SELECTION none_implies_folder=%u\n",i);
        array_result(hr,array,"GetSelection",folder_leaf);
    }
    for(i=0;i<4;i++) {
        int index=-99;
        hr=IFolderView2_GetSelectedItem(view,(int)i-1,&index);
        printf("SELECTED start=%d hr=%08lx index=%d\n",(int)i-1,(DWORD)hr,index);
    }
    for(i=0;i<3;i++) {
        DWORD state=0xdeadbeef;
        hr=IFolderView2_GetSelectionState(view,items[i],&state);
        printf("SELECTION_STATE own_item=%u hr=%08lx flags=%08lx\n",i,(DWORD)hr,state);
    }
    printf("STATE_END name=%s\n",stage);
}
int main(void)
{
    struct host host={{&host_vtable},1,NULL,NULL};
    IShellFolder *desktop=NULL,*folder=NULL;
    IFolderView2 *view=NULL;
    ITEMIDLIST *absolute=NULL,*items[3]={NULL,NULL,NULL};
    WCHAR directory[MAX_PATH],file[MAX_PATH];
    SFV_CREATE create={0};
    FOLDERSETTINGS settings={FVM_DETAILS,FWF_NOCLIENTEDGE};
    RECT rect={0,0,500,320};
    HWND view_window=NULL;
    BOOL own_directory=FALSE;
    BOOL activated=FALSE;
    HRESULT ready_hr=E_UNEXPECTED;
    int count=0,indices[3]={-1,-1,-1};
    unsigned int i,j;
    DWORD start;
    if(!b3_start("shell32_folderview_selection",45000)) return 1;
    if(!b3_hr(OleInitialize(NULL),"OleInitialize")) return b3_finish();
    if(!b3_tempdir(directory)) goto done;
    own_directory=TRUE;
    for(i=0;i<3;i++) if(!b3_path(file,directory,names[i])||!b3_write_file(file,"own",3)) goto done;
    host.window=CreateWindowExW(0,L"static",L"Winum Folder View Measure",WS_OVERLAPPEDWINDOW|WS_VISIBLE,
                                80,80,520,360,NULL,NULL,GetModuleHandleW(NULL),NULL);
    if(!host.window) { b3_error("CreateOwnHostWindow",GetLastError()); goto done; }
    if(!b3_hr(SHGetDesktopFolder(&desktop),"SHGetDesktopFolder")) goto done;
    if(!b3_hr(IShellFolder_ParseDisplayName(desktop,host.window,NULL,directory,NULL,&absolute,NULL),"ParseOwnDirectory")) goto done;
    if(!b3_hr(IShellFolder_BindToObject(desktop,absolute,NULL,&IID_IShellFolder,(void**)&folder),"BindOwnDirectory")) goto done;
    for(i=0;i<3;i++) {
        WCHAR name[32];
        lstrcpyW(name,names[i]);
        if(!b3_hr(IShellFolder_ParseDisplayName(folder,host.window,NULL,name,NULL,&items[i],NULL),"ParseOwnItem")) goto done;
    }
    create.cbSize=sizeof(create);create.pshf=folder;
    if(!b3_hr(SHCreateShellFolderView(&create,&host.view),"SHCreateShellFolderView")) goto done;
    if(!b3_hr(IShellView_CreateViewWindow(host.view,NULL,&settings,&host.iface,&rect,&view_window),"CreateOwnViewWindow")) goto done;
    if(!b3_hr(IShellView_QueryInterface(host.view,&IID_IFolderView2,(void**)&view),"QueryFolderView2")) goto done;
    if(!b3_hr(IShellView_UIActivate(host.view,SVUIA_ACTIVATE_NOFOCUS),"ActivateOwnView")) goto done;
    activated=TRUE;
    start=GetTickCount();
    do {
        ready_hr=IFolderView2_ItemCount(view,SVGIO_ALLVIEW,&count);
        if(SUCCEEDED(ready_hr)&&count==3) break;
        pump(50);
    }while(GetTickCount()-start<10000);
    printf("READY hr=%08lx items=%d\n",(DWORD)ready_hr,count);
    if(FAILED(ready_hr)||count!=3) { b3_error("OwnViewEnumeration",ERROR_INVALID_DATA); goto done; }
    for(i=0;i<3;i++) {
        ITEMIDLIST *item=NULL;
        if(!b3_hr(IFolderView2_Item(view,i,&item),"GetOwnViewItem")) goto done;
        for(j=0;j<3;j++) if(IShellFolder_CompareIDs(folder,SHCIDS_CANONICALONLY,item,items[j])==S_OK) indices[j]=i;
        CoTaskMemFree(item);
    }
    printf("INDEX own_a=%d own_b=%d own_c=%d\n",indices[0],indices[1],indices[2]);
    if(indices[0]<0||indices[1]<0||indices[2]<0) { b3_error("OwnItemIdentity",ERROR_INVALID_DATA); goto done; }
    for(i=0;i<3;i++) if(!b3_hr(IFolderView2_SelectItem(view,indices[i],SVSI_DESELECT),"ClearOwnSelection")) goto done;
    pump(50);
    observe(view,items,"empty",wcsrchr(directory,L'\\')+1);
    if(!b3_hr(IFolderView2_SelectItem(view,indices[1],SVSI_SELECT|SVSI_DESELECTOTHERS|SVSI_FOCUSED),"SelectOwnB")) goto done;
    pump(50);observe(view,items,"one",wcsrchr(directory,L'\\')+1);
    if(!b3_hr(IFolderView2_SelectItem(view,indices[2],SVSI_SELECT),"SelectOwnC")) goto done;
    pump(50);observe(view,items,"two",wcsrchr(directory,L'\\')+1);
    for(i=0;i<3;i++) if(!b3_hr(IFolderView2_SelectItem(view,indices[i],SVSI_DESELECT),"ClearOwnSelection")) goto done;
    pump(50);observe(view,items,"cleared",wcsrchr(directory,L'\\')+1);
    printf("CASES completed=4\n");
done:
    if(view) IFolderView2_Release(view);
    if(activated) b3_hr(IShellView_UIActivate(host.view,SVUIA_DEACTIVATE),"DeactivateOwnView");
    if(view_window) b3_hr(IShellView_DestroyViewWindow(host.view),"DestroyOwnViewWindow");
    if(host.view) { IShellView_Release(host.view);host.view=NULL; }
    if(folder) IShellFolder_Release(folder);
    if(desktop) IShellFolder_Release(desktop);
    CoTaskMemFree(absolute);
    for(i=0;i<3;i++) CoTaskMemFree(items[i]);
    if(host.window && !DestroyWindow(host.window)) b3_error("DestroyOwnHostWindow",GetLastError());
    OleUninitialize();
    if(own_directory) {
        for(i=0;i<3;i++) if(b3_path(file,directory,names[i])) b3_delete_file(file);
        b3_remove_dir(directory);
    }
    printf("CLEANUP host_refs=%ld\n",host.refs);
    return b3_finish();
}
