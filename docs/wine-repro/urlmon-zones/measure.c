/*
 * MEASURE: what Windows returns from IInternetSecurityManager::MapUrlToZone
 * for res: URLs with and without a module, and for URLs whose security URL
 * cannot be computed (the case where urlmon copies the URL itself).
 * Prints one line per URL; no personal data is printed.
 * Build: x86_64-w64-mingw32-gcc -Wall -Werror measure.c -o measure.exe -lurlmon -lole32
 */
#define COBJMACROS
#include <stdio.h>
#include <windows.h>
#include <urlmon.h>

static const WCHAR *urls[] = {
    L"res:///btn.xmx/icon.png",
    L"res:///%prodname%btn.xmx/RIBBON/icon.png",
    L"res://shell32.dll/#2/#130",
    L"res://notexisting.dll/x.png",
    L"res:",
    L"res:x",
    L"unknownscheme:abc",
    L"unknownscheme://host/path",
    L"x",
    L"about:blank",
    L"file:///C:/Windows/notepad.exe",
    L"http://localhost/",
    L"http://www.example.com/",
    L"",
};

int main(void)
{
    IInternetSecurityManager *mgr;
    HRESULT hr;
    unsigned i;

    CoInitialize(NULL);
    hr = CoInternetCreateSecurityManager(NULL, &mgr, 0);
    printf("create hr=0x%08lx\n", (unsigned long)hr);
    if (FAILED(hr)) return 1;

    for (i = 0; i < sizeof(urls) / sizeof(urls[0]); i++)
    {
        DWORD zone = 0xdeadbeef;
        WCHAR *sec = NULL;
        HRESULT shr;

        hr = IInternetSecurityManager_MapUrlToZone(mgr, urls[i], &zone, 0);
        shr = CoInternetGetSecurityUrl(urls[i], &sec, PSU_SECURITY_URL_ONLY, 0);
        printf("url=%ls map hr=0x%08lx zone=%ld secur hr=0x%08lx secur=%ls\n",
               urls[i], (unsigned long)hr, (long)zone, (unsigned long)shr,
               sec ? sec : L"(null)");
        CoTaskMemFree(sec);
    }

    IInternetSecurityManager_Release(mgr);
    CoUninitialize();
    return 0;
}
