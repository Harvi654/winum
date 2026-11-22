/* Original AI-assisted (Codex), LGPL-2.1-or-later. */
#define COBJMACROS
#define _WIN32_WINNT 0x0601
#include <windows.h>
#include <shlobj.h>
#include <stdio.h>
#include <wchar.h>

static void pump_messages(DWORD duration)
{
    DWORD start = GetTickCount();
    MSG msg;

    do
    {
        while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
        if (duration)
            Sleep(10);
    } while (GetTickCount() - start < duration);
}

static BOOL folder_exists(const WCHAR *path)
{
    DWORD attrs = GetFileAttributesW(path);

    return attrs != INVALID_FILE_ATTRIBUTES && (attrs & FILE_ATTRIBUTE_DIRECTORY);
}

static void print_state(const char *phase, HWND edit, const WCHAR *original, const WCHAR *renamed)
{
    printf("%s original=%u renamed=%u editor_alive=%u editor_focused=%u\n",
           phase, (unsigned int)folder_exists(original), (unsigned int)folder_exists(renamed),
           (unsigned int)IsWindow(edit), GetFocus() == edit);
}

static BOOL measure_enter(IExplorerBrowser *browser, PCIDLIST_ABSOLUTE parent,
                          const WCHAR *original, const WCHAR *renamed)
{
    PIDLIST_ABSOLUTE item = NULL;
    IShellView *view = NULL;
    WCHAR class_name[64] = {0}, text[64] = {0};
    HWND view_window = NULL, edit;
    HRESULT hr;
    MSG msg = {0};
    BOOL measured = FALSE;

    hr = IExplorerBrowser_BrowseToIDList(browser, parent, SBSP_ABSOLUTE);
    printf("BrowseToIDList hr=0x%08lx\n", (unsigned long)hr);
    if (FAILED(hr))
        goto done;
    pump_messages(300);

    hr = IExplorerBrowser_GetCurrentView(browser, &IID_IShellView, (void **)&view);
    printf("GetCurrentView hr=0x%08lx\n", (unsigned long)hr);
    if (FAILED(hr))
        goto done;

    hr = IShellView_GetWindow(view, &view_window);
    printf("GetWindow hr=0x%08lx\n", (unsigned long)hr);
    if (FAILED(hr))
        goto done;

    hr = SHParseDisplayName(original, NULL, &item, 0, NULL);
    printf("Parse child hr=0x%08lx\n", (unsigned long)hr);
    if (FAILED(hr))
        goto done;

    hr = IShellView_UIActivate(view, SVUIA_ACTIVATE_FOCUS);
    printf("UIActivate hr=0x%08lx\n", (unsigned long)hr);
    hr = IShellView_SelectItem(view, ILFindLastID(item), SVSI_DESELECTOTHERS | SVSI_EDIT |
                             SVSI_ENSUREVISIBLE | SVSI_FOCUSED | SVSI_SELECT);
    printf("SelectItem SVSI_EDIT hr=0x%08lx\n", (unsigned long)hr);
    if (FAILED(hr))
        goto done;
    pump_messages(250);

    edit = GetFocus();
    GetClassNameW(edit, class_name, ARRAYSIZE(class_name));
    printf("Focus class=%ls child_of_view=%u\n", class_name, (unsigned int)IsChild(view_window, edit));
    if (!IsChild(view_window, edit) || lstrcmpiW(class_name, L"Edit"))
    {
        puts("No focused Edit control in the shell view.");
        goto done;
    }

    if (!SetWindowTextW(edit, L"RenamedProbe"))
    {
        printf("SetWindowText error=%lu\n", GetLastError());
        goto done;
    }
    GetWindowTextW(edit, text, ARRAYSIZE(text));
    printf("Editor text matches requested label=%u\n", !lstrcmpW(text, L"RenamedProbe"));
    if (lstrcmpW(text, L"RenamedProbe"))
        goto done;

    print_state("before", edit, original, renamed);
    msg.hwnd = edit;
    msg.message = WM_KEYDOWN;
    msg.wParam = VK_RETURN;
    msg.lParam = 0x001c0001;
    msg.time = GetTickCount();
    hr = IShellView_TranslateAccelerator(view, &msg);
    printf("TranslateAccelerator VK_RETURN hr=0x%08lx\n", (unsigned long)hr);
    pump_messages(250);
    print_state("after_translate", edit, original, renamed);

    /* Record the direct dispatch separately if translation left the editor active. */
    if (hr == S_FALSE && IsWindow(edit) && GetFocus() == edit &&
        folder_exists(original) && !folder_exists(renamed))
    {
        DispatchMessageW(&msg);
        pump_messages(250);
        print_state("after_dispatch", edit, original, renamed);
    }
    measured = TRUE;

done:
    if (view) IShellView_Release(view);
    CoTaskMemFree(item);
    return measured;
}

int wmain(int argc, WCHAR **argv)
{
    WCHAR original[MAX_PATH], renamed[MAX_PATH];
    PIDLIST_ABSOLUTE parent = NULL;
    IExplorerBrowser *browser = NULL;
    FOLDERSETTINGS settings = {FVM_LIST, FWF_NONE};
    HWND host = NULL;
    RECT rect;
    BOOL measured = FALSE, initialized = FALSE;
    HRESULT hr;

    printf("arch=%u\n", (unsigned int)(8 * sizeof(void *)));
    if (argc != 2 || wcslen(argv[1]) + ARRAYSIZE(L"\\OriginalProbe") > MAX_PATH)
    {
        fputs("Usage: measure.exe <existing empty sample folder>\n", stderr);
        return 2;
    }

    lstrcpyW(original, argv[1]);
    lstrcatW(original, L"\\OriginalProbe");
    lstrcpyW(renamed, argv[1]);
    lstrcatW(renamed, L"\\RenamedProbe");
    if (GetFileAttributesW(renamed) != INVALID_FILE_ATTRIBUTES || !CreateDirectoryW(original, NULL))
    {
        puts("Could not create a fresh sample folder.");
        return 2;
    }

    hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    printf("CoInitializeEx hr=0x%08lx\n", (unsigned long)hr);
    if (FAILED(hr))
        goto remove_sample;

    hr = SHParseDisplayName(argv[1], NULL, &parent, 0, NULL);
    printf("Parse parent hr=0x%08lx\n", (unsigned long)hr);
    if (FAILED(hr))
        goto cleanup;

    host = CreateWindowExW(0, L"Static", L"winum shell view MEASURE", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                           CW_USEDEFAULT, CW_USEDEFAULT, 480, 260, NULL, NULL, GetModuleHandleW(NULL), NULL);
    if (!host)
    {
        printf("CreateWindow error=%lu\n", GetLastError());
        goto cleanup;
    }

    hr = CoCreateInstance(&CLSID_ExplorerBrowser, NULL, CLSCTX_INPROC_SERVER,
                          &IID_IExplorerBrowser, (void **)&browser);
    printf("Create ExplorerBrowser hr=0x%08lx\n", (unsigned long)hr);
    if (FAILED(hr))
        goto cleanup;

    GetClientRect(host, &rect);
    hr = IExplorerBrowser_Initialize(browser, host, &rect, &settings);
    printf("Initialize hr=0x%08lx\n", (unsigned long)hr);
    if (SUCCEEDED(hr))
    {
        initialized = TRUE;
        SetActiveWindow(host);
        measured = measure_enter(browser, parent, original, renamed);
    }

cleanup:
    if (initialized) IExplorerBrowser_Destroy(browser);
    if (browser) IExplorerBrowser_Release(browser);
    if (host) DestroyWindow(host);
    CoTaskMemFree(parent);
    CoUninitialize();

remove_sample:
    RemoveDirectoryW(original);
    RemoveDirectoryW(renamed);
    return measured ? 0 : 1;
}
