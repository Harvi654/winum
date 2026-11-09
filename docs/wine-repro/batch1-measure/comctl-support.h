/* Original, AI-assisted (Claude), LGPL-2.1-or-later.
 * Batch 1 MEASURE helpers: load comctl32 version 5 or 6 (version 6 through
 * an activation context from an own temporary manifest), and run one case
 * in a child process of the same program so that a crash on Windows is
 * recorded as an exit code instead of ending the measurement. */
#ifndef WINUM_BATCH1_COMCTL_SUPPORT_H
#define WINUM_BATCH1_COMCTL_SUPPORT_H
#include "../batch3-measure/measure-support.h"
#include <commctrl.h>
#include <shlwapi.h>

typedef BOOL (WINAPI *cc_init_fn)(const INITCOMMONCONTROLSEX *);
typedef HRESULT (CALLBACK *cc_dllgetversion_fn)(DLLVERSIONINFO *);

struct cc
{
    int version;
    HMODULE module;
    HANDLE actctx;
    ULONG_PTR cookie;
    WCHAR dir[MAX_PATH], manifest_path[MAX_PATH];
};

static const char cc6_manifest[] =
    "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n"
    "<assembly xmlns=\"urn:schemas-microsoft-com:asm.v1\" manifestVersion=\"1.0\">\n"
    "<assemblyIdentity type=\"win32\" name=\"Winum.Measure.Comctl6\" version=\"1.0.0.0\" processorArchitecture=\"*\"/>\n"
    "<dependency><dependentAssembly><assemblyIdentity type=\"win32\" name=\"Microsoft.Windows.Common-Controls\""
    " version=\"6.0.0.0\" processorArchitecture=\"*\" publicKeyToken=\"6595b64144ccf1df\" language=\"*\"/>"
    "</dependentAssembly></dependency>\n</assembly>\n";

/* Version 6 stays activated until cc_unload. */
static inline BOOL cc_load(struct cc *cc, int version)
{
    INITCOMMONCONTROLSEX icc;
    cc_dllgetversion_fn getversion;
    DLLVERSIONINFO info;
    cc_init_fn init;
    ACTCTXW actctx;

    memset(cc, 0, sizeof(*cc));
    cc->version = version;
    cc->actctx = INVALID_HANDLE_VALUE;
    if (version == 6)
    {
        if (!b3_tempdir(cc->dir)) return FALSE;
        if (!b3_path(cc->manifest_path, cc->dir, L"cc6.manifest") ||
            !b3_write_file(cc->manifest_path, cc6_manifest, sizeof(cc6_manifest) - 1))
        {
            b3_remove_dir(cc->dir);
            cc->dir[0] = 0;
            return FALSE;
        }
        memset(&actctx, 0, sizeof(actctx));
        actctx.cbSize = sizeof(actctx);
        actctx.lpSource = cc->manifest_path;
        cc->actctx = CreateActCtxW(&actctx);
        if (cc->actctx == INVALID_HANDLE_VALUE) { b3_error("CreateActCtxW", GetLastError()); return FALSE; }
        if (!ActivateActCtx(cc->actctx, &cc->cookie))
        {
            b3_error("ActivateActCtx", GetLastError());
            cc->cookie = 0;
            return FALSE;
        }
    }
    cc->module = LoadLibraryW(L"comctl32.dll");
    if (!cc->module) { b3_error("LoadLibrary_comctl32", GetLastError()); return FALSE; }
    init = (cc_init_fn)(void *)GetProcAddress(cc->module, "InitCommonControlsEx");
    getversion = (cc_dllgetversion_fn)(void *)GetProcAddress(cc->module, "DllGetVersion");
    if (!init) { b3_error("GetProcAddress_InitCommonControlsEx", GetLastError()); return FALSE; }
    icc.dwSize = sizeof(icc);
    icc.dwICC = ICC_WIN95_CLASSES | (version == 6 ? ICC_STANDARD_CLASSES : 0);
    if (!init(&icc)) b3_error("InitCommonControlsEx", GetLastError());
    memset(&info, 0, sizeof(info));
    info.cbSize = sizeof(info);
    if (getversion && SUCCEEDED(getversion(&info)))
        printf("COMCTL32 requested=%d dll=%lu.%lu\n", version, info.dwMajorVersion, info.dwMinorVersion);
    else
        printf("COMCTL32 requested=%d dll=unknown\n", version);
    return TRUE;
}

static inline void cc_unload(struct cc *cc)
{
    if (cc->cookie && !DeactivateActCtx(0, cc->cookie)) b3_error("DeactivateActCtx", GetLastError());
    if (cc->actctx != INVALID_HANDLE_VALUE) ReleaseActCtx(cc->actctx);
    if (cc->manifest_path[0]) b3_delete_file(cc->manifest_path);
    if (cc->dir[0]) b3_remove_dir(cc->dir);
    cc->cookie = 0;
    cc->actctx = INVALID_HANDLE_VALUE;
    cc->manifest_path[0] = cc->dir[0] = 0;
}

static inline FARPROC cc_proc(const struct cc *cc, const char *name)
{
    FARPROC proc = GetProcAddress(cc->module, name);
    if (!proc) b3_error(name, GetLastError());
    return proc;
}

/* Child side: an exception that reaches the top is printed and becomes the
 * exit code. */
static LONG WINAPI child_unhandled(EXCEPTION_POINTERS *ptrs)
{
    printf("EXCEPTION code=%08lx\n", ptrs->ExceptionRecord->ExceptionCode);
    fflush(stdout);
    TerminateProcess(GetCurrentProcess(), ptrs->ExceptionRecord->ExceptionCode);
    return EXCEPTION_EXECUTE_HANDLER;
}

static inline void child_start(void)
{
    setvbuf(stdout, NULL, _IONBF, 0);
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
    SetUnhandledExceptionFilter(child_unhandled);
}

/* Parent side: runs "<this program> <arguments>" with the same stdout and
 * reports its exit code; a child is ended after timeout milliseconds. */
static inline DWORD run_child(const char *label, const WCHAR *arguments, DWORD timeout)
{
    WCHAR self[MAX_PATH], command[2 * MAX_PATH];
    PROCESS_INFORMATION pi;
    STARTUPINFOW si;
    DWORD code = 0xffffffff;
    int n;

    if (!GetModuleFileNameW(NULL, self, MAX_PATH)) { b3_error("GetModuleFileNameW", GetLastError()); return code; }
    n = swprintf(command, ARRAYSIZE(command), L"\"%ls\" %ls", self, arguments);
    if (n < 0 || n >= (int)ARRAYSIZE(command)) { b3_error("ChildCommandLength", ERROR_BUFFER_OVERFLOW); return code; }
    memset(&si, 0, sizeof(si));
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    si.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    si.hStdError = GetStdHandle(STD_ERROR_HANDLE);
    fflush(stdout);
    printf("CHILD_BEGIN %s\n", label);
    if (!CreateProcessW(NULL, command, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi))
    {
        b3_error("CreateProcessW", GetLastError());
        return code;
    }
    if (WaitForSingleObject(pi.hProcess, timeout) != WAIT_OBJECT_0)
    {
        printf("CHILD_TIMEOUT %s\n", label);
        TerminateProcess(pi.hProcess, 3);
        WaitForSingleObject(pi.hProcess, INFINITE);
    }
    if (!GetExitCodeProcess(pi.hProcess, &code)) b3_error("GetExitCodeProcess", GetLastError());
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    printf("CHILD_END %s exit=%08lx\n", label, code);
    return code;
}
#endif
