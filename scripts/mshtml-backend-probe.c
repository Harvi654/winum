/* Original, AI-assisted (Codex), LGPL-2.1-or-later.
 * L2 prerequisite, not a Windows conformance test: load only our own HTML
 * through builtin mshtml and require its title and a complete document.
 * The caller bounds the whole process, including COM/Gecko cleanup. */
#define COBJMACROS
#define _WIN32_WINNT 0x0601
#include <windows.h>
#include <ole2.h>
#include <initguid.h>
#include <mshtml.h>
#include <stdio.h>
#include <string.h>

int main(void)
{
    static const char html[] = "<html><head><title>Winum L2 Gecko</title></head><body>fixture</body></html>";
    IHTMLDocument2 *document = NULL;
    IPersistStreamInit *persist = NULL;
    IStream *stream = NULL;
    IHTMLWindow2 *window = NULL;
    HMODULE module;
    HRESULT (WINAPI *register_server)(void);
    BSTR state = NULL, title = NULL;
    LARGE_INTEGER beginning;
    ULONGLONG deadline;
    ULONG written = 0;
    HRESULT hr;
    const char *stage = "CoInitialize";
    BOOL initialized = FALSE;
    int result = 1;

    setvbuf(stdout, NULL, _IONBF, 0);
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
    hr = CoInitialize(NULL);
    if (FAILED(hr)) goto done;
    initialized = TRUE;
    /* wineboot ran with mshtml disabled. Register the builtin within this
     * test process, in the disposable prefix and this architecture's view;
     * do not enable mshtml in another process or set a registry override. */
    stage = "load-builtin";
    module = LoadLibraryW(L"mshtml.dll");
    if (!module) { hr = HRESULT_FROM_WIN32(GetLastError()); goto done; }
    stage = "register";
    register_server = (void *)GetProcAddress(module, "DllRegisterServer");
    if (!register_server) { hr = E_NOINTERFACE; goto done; }
    hr = register_server();
    if (FAILED(hr)) goto done;
    /* Keep the module loaded until process exit, as the mshtml test units
     * do; Gecko is not designed to be repeatedly unloaded. */
    stage = "create";
    hr = CoCreateInstance(&CLSID_HTMLDocument, NULL, CLSCTX_INPROC_SERVER,
                          &IID_IHTMLDocument2, (void **)&document);
    if (FAILED(hr) || !document) goto done;
    stage = "persist";
    hr = IHTMLDocument2_QueryInterface(document, &IID_IPersistStreamInit, (void **)&persist);
    if (FAILED(hr) || !persist) goto done;
    stage = "stream";
    hr = CreateStreamOnHGlobal(NULL, TRUE, &stream);
    if (FAILED(hr)) goto done;
    hr = IStream_Write(stream, html, sizeof(html) - 1, &written);
    if (FAILED(hr) || written != sizeof(html) - 1) goto done;
    beginning.QuadPart = 0;
    hr = IStream_Seek(stream, beginning, STREAM_SEEK_SET, NULL);
    if (FAILED(hr)) goto done;
    stage = "load";
    hr = IPersistStreamInit_Load(persist, stream);
    if (FAILED(hr)) goto done;
    stage = "ready";
    deadline = GetTickCount64() + 5000;
    for (;;)
    {
        MSG message;
        hr = IHTMLDocument2_get_readyState(document, &state);
        if (FAILED(hr)) goto done;
        if (state && !lstrcmpiW(state, L"complete")) break;
        SysFreeString(state);
        state = NULL;
        if (GetTickCount64() >= deadline) goto done;
        while (GetTickCount64() < deadline && PeekMessageW(&message, NULL, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&message);
            DispatchMessageW(&message);
        }
        Sleep(10);
    }
    stage = "title";
    hr = IHTMLDocument2_get_title(document, &title);
    if (FAILED(hr) || !title || lstrcmpW(title, L"Winum L2 Gecko")) goto done;
    stage = "window";
    hr = IHTMLDocument2_get_parentWindow(document, &window);
    if (FAILED(hr) || !window) goto done;
    result = 0;
done:
    SysFreeString(state);
    SysFreeString(title);
    if (window) IHTMLWindow2_Release(window);
    if (stream) IStream_Release(stream);
    if (persist) IPersistStreamInit_Release(persist);
    if (document) IHTMLDocument2_Release(document);
    if (initialized) CoUninitialize();
    printf("MSHTML_BACKEND usable=%d bits=%u stage=%s hr=%#lx\n",
           !result, (unsigned int)(8 * sizeof(void *)), stage, hr);
    return result;
}
