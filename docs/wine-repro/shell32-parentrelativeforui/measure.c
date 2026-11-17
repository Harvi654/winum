/* Original AI-assisted (Codex), LGPL-2.1-or-later. */
#define COBJMACROS
#define _WIN32_WINNT 0x0601
#include <windows.h>
#include <shlobj.h>
#include <stdio.h>

static const struct
{
    const char *name;
    SIGDN value;
} formats[] =
{
    {"NORMALDISPLAY", SIGDN_NORMALDISPLAY},
    {"PARENTRELATIVE", SIGDN_PARENTRELATIVE},
    {"PARENTRELATIVEFORADDRESSBAR", SIGDN_PARENTRELATIVEFORADDRESSBAR},
    {"PARENTRELATIVEFORUI", (SIGDN)0x80094001},
};

static void print_result(const char *api, const char *format, HRESULT hr,
                         const WCHAR *name)
{
    printf("  %s %s hr=0x%08lx name=", api, format, (unsigned long)hr);
    if (name)
    {
        putchar('"');
        for (; *name; ++name)
        {
            if (*name >= 0x20 && *name <= 0x7e && *name != '\\' && *name != '"')
                putchar(*name);
            else
                printf("\\u%04x", (unsigned int)*name);
        }
        putchar('"');
    }
    else
        printf("NULL");
    putchar('\n');
}

static void measure_item(const char *label, PCIDLIST_ABSOLUTE pidl)
{
    IShellItem *item = NULL;
    HRESULT create_hr, hr;
    WCHAR *name;
    unsigned int i;

    printf("item=%s\n", label);
    create_hr = SHCreateItemFromIDList(pidl, &IID_IShellItem, (void **)&item);
    printf("  SHCreateItemFromIDList hr=0x%08lx\n", (unsigned long)create_hr);

    for (i = 0; i < ARRAYSIZE(formats); ++i)
    {
        name = NULL;
        hr = SHGetNameFromIDList(pidl, formats[i].value, &name);
        print_result("SHGetNameFromIDList", formats[i].name, hr, name);
        CoTaskMemFree(name);

        if (FAILED(create_hr))
            continue;
        name = NULL;
        hr = IShellItem_GetDisplayName(item, formats[i].value, &name);
        print_result("IShellItem::GetDisplayName", formats[i].name, hr, name);
        CoTaskMemFree(name);
    }

    if (item)
        IShellItem_Release(item);
}

static void measure_path(const char *label, const WCHAR *path)
{
    PIDLIST_ABSOLUTE pidl = NULL;
    HRESULT hr = SHParseDisplayName(path, NULL, &pidl, 0, NULL);

    if (SUCCEEDED(hr))
        measure_item(label, pidl);
    else
        printf("item=%s SHParseDisplayName hr=0x%08lx\n", label, (unsigned long)hr);
    CoTaskMemFree(pidl);
}

static void measure_known_folder(const char *label, REFKNOWNFOLDERID id)
{
    PIDLIST_ABSOLUTE pidl = NULL;
    HRESULT hr = SHGetKnownFolderIDList(id, 0, NULL, &pidl);

    if (SUCCEEDED(hr))
        measure_item(label, pidl);
    else
        printf("item=%s SHGetKnownFolderIDList hr=0x%08lx\n", label, (unsigned long)hr);
    CoTaskMemFree(pidl);
}

int wmain(int argc, WCHAR **argv)
{
    HRESULT hr;

    if (argc != 3)
    {
        fprintf(stderr, "Usage: measure.exe <folder> <drive-root>\n");
        return 2;
    }

    hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    if (FAILED(hr))
    {
        fprintf(stderr, "CoInitializeEx hr=0x%08lx\n", (unsigned long)hr);
        return 1;
    }

    printf("arch=%u\n", (unsigned int)(sizeof(void *) * 8));
    measure_path("folder", argv[1]);
    measure_path("drive-root", argv[2]);
    measure_known_folder("Documents", &FOLDERID_Documents);
    measure_known_folder("This PC", &FOLDERID_ComputerFolder);

    CoUninitialize();
    return 0;
}
