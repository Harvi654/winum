/* Original AI-assisted (Codex), LGPL-2.1-or-later. */
#define COBJMACROS
#define _WIN32_WINNT 0x0601
#include <windows.h>
#include <shlobj.h>
#include <stdio.h>

static void print_caption(const WCHAR *caption)
{
    putchar('"');
    for (; *caption; ++caption)
    {
        if (*caption >= 0x20 && *caption <= 0x7e && *caption != '"' && *caption != '\\')
            putchar(*caption);
        else
            printf("\\u%04x", (unsigned int)*caption);
    }
    putchar('"');
}

static void print_menu(HMENU menu, unsigned int depth)
{
    int i, count = GetMenuItemCount(menu);

    for (i = 0; i < count; ++i)
    {
        WCHAR caption[256] = {0};
        MENUITEMINFOW item = {0};

        item.cbSize = sizeof(item);
        item.fMask = MIIM_FTYPE | MIIM_ID | MIIM_SUBMENU | MIIM_STRING | MIIM_STATE;
        item.dwTypeData = caption;
        item.cch = ARRAYSIZE(caption);
        if (!GetMenuItemInfoW(menu, i, TRUE, &item))
        {
            printf("menu_item_error depth=%u index=%d error=%lu\n", depth, i, GetLastError());
            continue;
        }
        if (item.fType & MFT_SEPARATOR)
            continue;

        printf("item depth=%u index=%d id=0x%x submenu=%u enabled=%u caption=",
               depth, i, item.wID, !!item.hSubMenu, !(item.fState & MFS_DISABLED));
        print_caption(caption);
        putchar('\n');
        if (item.hSubMenu)
            print_menu(item.hSubMenu, depth + 1);
    }
}

static void measure_menu(IShellFolder *folder, HWND owner, UINT first, UINT last)
{
    IContextMenu *context = NULL;
    HMENU menu;
    HRESULT hr;

    printf("range first=0x%x last=0x%x flags=CMF_NORMAL\n", first, last);
    hr = IShellFolder_CreateViewObject(folder, owner, &IID_IContextMenu, (void **)&context);
    printf("CreateViewObject hr=0x%08lx\n", (unsigned long)hr);
    if (FAILED(hr))
        return;

    menu = CreatePopupMenu();
    if (menu)
    {
        hr = IContextMenu_QueryContextMenu(context, menu, 0, first, last, CMF_NORMAL);
        printf("QueryContextMenu hr=0x%08lx top_level=%d\n", (unsigned long)hr,
               GetMenuItemCount(menu));
        if (SUCCEEDED(hr))
            print_menu(menu, 0);
        DestroyMenu(menu);
    }
    else
        printf("CreatePopupMenu error=%lu\n", GetLastError());
    IContextMenu_Release(context);
}

int wmain(int argc, WCHAR **argv)
{
    PIDLIST_ABSOLUTE pidl = NULL;
    IShellFolder *desktop = NULL, *folder = NULL;
    HRESULT hr;

    if (argc != 2)
    {
        fprintf(stderr, "Usage: measure.exe <existing empty folder>\n");
        return 2;
    }

    printf("arch=%u\n", (unsigned int)(8 * sizeof(void *)));
    hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    if (FAILED(hr))
    {
        printf("CoInitializeEx hr=0x%08lx\n", (unsigned long)hr);
        return 1;
    }

    hr = SHParseDisplayName(argv[1], NULL, &pidl, 0, NULL);
    printf("SHParseDisplayName hr=0x%08lx\n", (unsigned long)hr);
    if (SUCCEEDED(hr))
    {
        hr = SHGetDesktopFolder(&desktop);
        printf("SHGetDesktopFolder hr=0x%08lx\n", (unsigned long)hr);
    }
    if (SUCCEEDED(hr))
    {
        hr = IShellFolder_BindToObject(desktop, pidl, NULL, &IID_IShellFolder,
                                     (void **)&folder);
        printf("BindToObject hr=0x%08lx\n", (unsigned long)hr);
    }
    if (SUCCEEDED(hr))
    {
        HWND owner = GetConsoleWindow();

        measure_menu(folder, owner, 0, 0x7fff);
        measure_menu(folder, owner, 1, 0x7fff);
        measure_menu(folder, owner, 0x7000, 0x7fff);
        IShellFolder_Release(folder);
    }
    if (desktop)
        IShellFolder_Release(desktop);
    CoTaskMemFree(pidl);
    CoUninitialize();
    return FAILED(hr);
}
