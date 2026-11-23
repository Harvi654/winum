/* Original COM measurement, AI-assisted (Codex), LGPL-2.1-or-later. */
#define COBJMACROS
#include <windows.h>
#include <ole2.h>
#include <objidl.h>
#include <shellapi.h>
#include <stdio.h>
#include <wchar.h>

#define COUNT(a) (sizeof(a) / sizeof(*(a)))
#define CLASSES L"Software\\Classes\\"
#define CLSID_ROOT L"CLSID"
#define WOW_ROOT L"Wow6432Node\\CLSID"

static WCHAR module[MAX_PATH], directory[MAX_PATH];
static unsigned int cleanup_errors;

static void report_cleanup(const char *kind, LONG status)
{
    printf("cleanup kind=%s status=%ld\n", kind, status);
    if (status != ERROR_SUCCESS && status != ERROR_FILE_NOT_FOUND) ++cleanup_errors;
}

static LONG remove_key(HKEY hive, const WCHAR *path, REGSAM view)
{
    HKEY key;
    LONG ret = RegOpenKeyExW(hive, path, 0, KEY_READ | KEY_WRITE | DELETE | view, &key);

    if (ret) return ret;
    ret = RegDeleteTreeW(key, NULL);
    RegCloseKey(key);
    return ret ? ret : RegDeleteKeyExW(hive, path, view, 0);
}

static LONG new_key(HKEY hive, const WCHAR *path, REGSAM view, HKEY *out)
{
    DWORD disposition;
    LONG ret;

    ret = RegCreateKeyExW(hive, path, 0, NULL, REG_OPTION_VOLATILE, KEY_WRITE | view,
                         NULL, out, &disposition);
    if (!ret && disposition != REG_CREATED_NEW_KEY)
    {
        RegCloseKey(*out);
        *out = NULL;
        return ERROR_ALREADY_EXISTS;
    }
    return ret;
}

static LONG set_string(HKEY key, const WCHAR *name, const WCHAR *value)
{
    return RegSetValueExW(key, name, 0, REG_SZ, (const BYTE *)value,
                          (DWORD)((wcslen(value) + 1) * sizeof(WCHAR)));
}

static LONG child_value(HKEY parent, const WCHAR *name, const WCHAR *value, REGSAM view)
{
    HKEY child;
    LONG ret = new_key(parent, name, view, &child);
    if (ret) return ret;
    ret = set_string(child, NULL, value);
    RegCloseKey(child);
    return ret;
}

static void clsid_path(WCHAR *path, const WCHAR *root, const WCHAR *id)
{
    swprintf(path, 160, L"%ls%ls\\%ls", CLASSES, root, id);
}

static LONG register_class(HKEY hive, const WCHAR *path, REGSAM view, const WCHAR *server,
                           const WCHAR *progid, const WCHAR *treat_as, BOOL *owned)
{
    HKEY key, child;
    LONG ret = new_key(hive, path, view, &key);

    *owned = ret == ERROR_SUCCESS;
    if (ret) return ret;
    if (server)
    {
        ret = new_key(key, L"InprocServer32", view, &child);
        if (!ret)
        {
            ret = set_string(child, NULL, server);
            if (!ret) ret = set_string(child, L"ThreadingModel", L"Both");
            RegCloseKey(child);
        }
    }
    if (!ret && progid) ret = child_value(key, L"ProgID", progid, view);
    if (!ret && treat_as) ret = child_value(key, L"TreatAs", treat_as, view);
    RegCloseKey(key);
    return ret;
}

static BOOL new_guid(CLSID *clsid, WCHAR *id)
{
    return SUCCEEDED(CoCreateGuid(clsid)) && StringFromGUID2(clsid, id, 40);
}

static BOOL dll_path(WCHAR *path, BOOL machine, unsigned int bits)
{
    DWORD attrs;
    if (swprintf(path, MAX_PATH, L"%ls\\%ls-%u.dll", directory,
                 machine ? L"machine" : L"user", bits) < 0)
        return FALSE;
    attrs = GetFileAttributesW(path);
    return attrs != INVALID_FILE_ATTRIBUTES && !(attrs & FILE_ATTRIBUTE_DIRECTORY);
}

static void report_object(const char *api, HRESULT hr, IPersist *obj)
{
    CLSID id;
    HRESULT marker_hr = E_UNEXPECTED;
    DWORD marker = 0;

    if (obj)
    {
        marker_hr = IPersist_GetClassID(obj, &id);
        if (SUCCEEDED(marker_hr)) marker = id.Data1;
        IPersist_Release(obj);
    }
    printf("com api=%s hr=0x%08lx marker_hr=0x%08lx marker=%lu\n",
           api, (unsigned long)hr, (unsigned long)marker_hr, marker);
}

static void activate(REFCLSID clsid)
{
    IClassFactory *f = NULL;
    IPersist *obj = NULL;
    HRESULT hr;

    hr = CoGetClassObject(clsid, CLSCTX_INPROC_SERVER, NULL, &IID_IClassFactory, (void **)&f);
    printf("com api=CoGetClassObject hr=0x%08lx\n", (unsigned long)hr);
    if (f)
    {
        hr = IClassFactory_CreateInstance(f, NULL, &IID_IPersist, (void **)&obj);
        report_object("IClassFactory.CreateInstance", hr, obj);
        IClassFactory_Release(f);
    }
    obj = NULL;
    hr = CoCreateInstance(clsid, NULL, CLSCTX_INPROC_SERVER, &IID_IPersist, (void **)&obj);
    report_object("CoCreateInstance", hr, obj);
}

static void observe_hkcr(const WCHAR *id)
{
    static const REGSAM views[] = {0, KEY_WOW64_32KEY, KEY_WOW64_64KEY};
    static const char *const names[] = {"default", "32", "64"};
    WCHAR path[100], server[MAX_PATH], expected[MAX_PATH];
    HKEY key;
    LONG ret, query;
    DWORD size, type, selection;
    unsigned int i;

    swprintf(path, COUNT(path), L"CLSID\\%ls", id);
    for (i = 0; i < COUNT(views); ++i)
    {
        selection = 0;
        query = ERROR_FILE_NOT_FOUND;
        ret = RegOpenKeyExW(HKEY_CLASSES_ROOT, path, 0, KEY_READ | views[i], &key);
        if (!ret)
        {
            size = sizeof(server);
            query = RegGetValueW(key, L"InprocServer32", NULL, RRF_RT_REG_SZ, &type, server, &size);
            if (!query)
            {
                if (dll_path(expected, FALSE, 32) && !wcsicmp(server, expected)) selection = 132;
                if (dll_path(expected, FALSE, 64) && !wcsicmp(server, expected)) selection = 164;
                if (dll_path(expected, TRUE, 32) && !wcsicmp(server, expected)) selection = 232;
                if (dll_path(expected, TRUE, 64) && !wcsicmp(server, expected)) selection = 264;
            }
            RegCloseKey(key);
        }
        printf("hkcr view=%s open=%ld inproc=%ld marker=%lu\n", names[i], ret, query, selection);
    }
}

static void user_case(const char *label, const WCHAR *root, REGSAM view, unsigned int bits)
{
    WCHAR id[40], alias_id[40], path[160], progid[80], prog_path[160], alias_path[160], dll[MAX_PATH];
    CLSID clsid, alias, found;
    BOOL owned = FALSE, prog_owned = FALSE, alias_owned = FALSE;
    LONG ret;
    HRESULT hr;
    HKEY key;
    LPOLESTR found_name = NULL;

    printf("case=%s\n", label);
    if (!new_guid(&clsid, id) || !dll_path(dll, FALSE, bits))
    {
        printf("setup_error=1\n");
        ++cleanup_errors;
        return;
    }
    clsid_path(path, root, id);
    swprintf(progid, COUNT(progid), L"WinumMeasure.P%08lx%04x%04x%02x%02x",
             clsid.Data1, clsid.Data2, clsid.Data3, clsid.Data4[0], clsid.Data4[1]);
    swprintf(prog_path, COUNT(prog_path), L"%ls%ls", CLASSES, progid);
    ret = register_class(HKEY_CURRENT_USER, path, view, dll, view || wcscmp(root, CLSID_ROOT) ? NULL : progid,
                         NULL, &owned);
    printf("register scope=hkcu status=%ld\n", ret);
    if (ret) goto done;
    observe_hkcr(id);
    activate(&clsid);
    if (view || wcscmp(root, CLSID_ROOT)) goto done;

    ret = new_key(HKEY_CURRENT_USER, prog_path, 0, &key);
    prog_owned = ret == ERROR_SUCCESS;
    if (!ret)
    {
        ret = child_value(key, L"CLSID", id, 0);
        RegCloseKey(key);
    }
    printf("register scope=progid status=%ld\n", ret);
    if (!ret)
    {
        LONG machine_before = RegOpenKeyExW(HKEY_LOCAL_MACHINE, prog_path, 0, KEY_READ, &key);

        if (!machine_before) RegCloseKey(key);
        hr = CLSIDFromProgID(progid, &found);
        printf("com api=CLSIDFromProgID hr=0x%08lx match=%d\n", (unsigned long)hr,
               SUCCEEDED(hr) && IsEqualCLSID(&found, &clsid));
        hr = ProgIDFromCLSID(&clsid, &found_name);
        printf("com api=ProgIDFromCLSID hr=0x%08lx match=%d\n", (unsigned long)hr,
               SUCCEEDED(hr) && found_name && !wcscmp(found_name, progid));
        CoTaskMemFree(found_name);
        /* Some implementations try OLE 1 registration when lookup fails. */
        if (machine_before == ERROR_FILE_NOT_FOUND)
            report_cleanup("synthetic_machine_progid", remove_key(HKEY_LOCAL_MACHINE, prog_path, 0));
    }

    if (!new_guid(&alias, alias_id))
    {
        ++cleanup_errors;
        goto done;
    }
    clsid_path(alias_path, CLSID_ROOT, alias_id);
    ret = register_class(HKEY_CURRENT_USER, alias_path, 0, NULL, NULL, id, &alias_owned);
    printf("register scope=treatas status=%ld\n", ret);
    if (!ret)
    {
        hr = CoGetTreatAsClass(&alias, &found);
        printf("com api=CoGetTreatAsClass hr=0x%08lx match=%d\n", (unsigned long)hr,
               SUCCEEDED(hr) && IsEqualCLSID(&found, &clsid));
    }

done:
    if (alias_owned) report_cleanup("user_treatas", remove_key(HKEY_CURRENT_USER, alias_path, 0));
    if (prog_owned) report_cleanup("user_progid", remove_key(HKEY_CURRENT_USER, prog_path, 0));
    if (owned) report_cleanup("user_class", remove_key(HKEY_CURRENT_USER, path, view));
}

static void event_name(WCHAR *name, const WCHAR *id, const WCHAR *role)
{
    swprintf(name, 100, L"Local\\WinumComMeasure-%ls-%ls", id, role);
}

static int machine_helper(const WCHAR *id)
{
    WCHAR ready_name[100], done_name[100], path[160], dll[MAX_PATH];
    CLSID clsid;
    HANDLE ready, done;
    LONG ret = ERROR_INVALID_PARAMETER;
    BOOL owned = FALSE;

    if (FAILED(CLSIDFromString(id, &clsid)) || !dll_path(dll, TRUE, 8 * sizeof(void *)))
        return ret;
    event_name(ready_name, id, L"ready");
    event_name(done_name, id, L"done");
    ready = OpenEventW(EVENT_MODIFY_STATE, FALSE, ready_name);
    done = OpenEventW(SYNCHRONIZE, FALSE, done_name);
    if (ready && done)
    {
        clsid_path(path, CLSID_ROOT, id);
        ret = register_class(HKEY_LOCAL_MACHINE, path, 0, dll, NULL, NULL, &owned);
        if (!ret)
        {
            if (SetEvent(ready)) ret = WaitForSingleObject(done, 90000);
            else ret = GetLastError();
        }
        if (owned)
        {
            LONG removed = remove_key(HKEY_LOCAL_MACHINE, path, 0);
            if (!ret) ret = removed;
        }
    }
    if (ready) CloseHandle(ready);
    if (done) CloseHandle(done);
    return ret;
}

static void precedence(BOOL direct)
{
    WCHAR id[40], path[160], dll[MAX_PATH], params[96], name[100];
    CLSID clsid;
    HANDLE ready = NULL, done = NULL, handles[2];
    SHELLEXECUTEINFOW launch = {0};
    BOOL user_owned = FALSE, machine_owned = FALSE, measured = FALSE;
    LONG ret;
    DWORD status;

    printf("case=user_machine_conflict machine_setup=%s\n", direct ? "direct" : "runas_helper");
    if (!new_guid(&clsid, id) || !dll_path(dll, FALSE, 8 * sizeof(void *)))
    {
        ++cleanup_errors;
        return;
    }
    clsid_path(path, CLSID_ROOT, id);
    ret = register_class(HKEY_CURRENT_USER, path, 0, dll, NULL, NULL, &user_owned);
    printf("register scope=hkcu status=%ld\n", ret);
    if (ret) goto done;
    if (direct)
    {
        if (!dll_path(dll, TRUE, 8 * sizeof(void *))) goto done;
        ret = register_class(HKEY_LOCAL_MACHINE, path, 0, dll, NULL, NULL, &machine_owned);
        printf("register scope=hklm status=%ld\n", ret);
        if (ret) goto done;
    }
    else
    {
        event_name(name, id, L"ready");
        ready = CreateEventW(NULL, TRUE, FALSE, name);
        event_name(name, id, L"done");
        done = CreateEventW(NULL, TRUE, FALSE, name);
        if (!ready || !done)
        {
            printf("machine event_error=%lu\n", GetLastError());
            goto done;
        }
        swprintf(params, COUNT(params), L"--machine-helper \"%ls\"", id);
        launch.cbSize = sizeof(launch);
        launch.fMask = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_NOASYNC;
        launch.lpVerb = L"runas";
        launch.lpFile = module;
        launch.lpParameters = params;
        launch.nShow = SW_HIDE;
        if (!ShellExecuteExW(&launch))
        {
            printf("machine launch_error=%lu\n", GetLastError());
            goto done;
        }
        handles[0] = ready;
        handles[1] = launch.hProcess;
        status = WaitForMultipleObjects(2, handles, FALSE, 30000);
        printf("machine ready_wait=%lu\n", status);
        if (status != WAIT_OBJECT_0) goto done;
        printf("register scope=hklm status=0\n");
    }
    observe_hkcr(id);
    activate(&clsid);
    measured = TRUE;

done:
    printf("machine conflict_measured=%d\n", measured);
    if (launch.hProcess)
    {
        if (done) SetEvent(done);
        status = WaitForSingleObject(launch.hProcess, 35000);
        if (status == WAIT_OBJECT_0 && GetExitCodeProcess(launch.hProcess, &status))
            report_cleanup("machine_helper", (LONG)status);
        else report_cleanup("machine_helper_wait", (LONG)status);
        CloseHandle(launch.hProcess);
    }
    if (ready) CloseHandle(ready);
    if (done) CloseHandle(done);
    if (machine_owned) report_cleanup("machine_class", remove_key(HKEY_LOCAL_MACHINE, path, 0));
    if (user_owned) report_cleanup("conflict_user", remove_key(HKEY_CURRENT_USER, path, 0));
}

int wmain(int argc, WCHAR **argv)
{
    WCHAR *slash;
    DWORD len, returned;
    BOOL wow = FALSE;
    TOKEN_ELEVATION elevation = {0};
    HANDLE token;
    HRESULT hr;

    len = GetModuleFileNameW(NULL, module, COUNT(module));
    if (!len || len >= COUNT(module)) return 2;
    wcscpy(directory, module);
    if (!(slash = wcsrchr(directory, '\\'))) return 2;
    *slash = 0;
    if (argc == 3 && !wcscmp(argv[1], L"--machine-helper")) return machine_helper(argv[2]);
    if (argc != 1 && (argc != 2 || wcscmp(argv[1], L"--direct-machine"))) return 2;
    setvbuf(stdout, NULL, _IONBF, 0);
    IsWow64Process(GetCurrentProcess(), &wow);
    if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token))
    {
        GetTokenInformation(token, TokenElevation, &elevation, sizeof(elevation), &returned);
        CloseHandle(token);
    }
    printf("measure=v1 bits=%u wow64=%d elevated=%lu\n", (unsigned int)(8 * sizeof(void *)), wow,
           elevation.TokenIsElevated);
    hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    printf("ole_init=0x%08lx\n", (unsigned long)hr);
    if (FAILED(hr)) return 2;

    user_case("user_only", CLSID_ROOT, 0, 8 * sizeof(void *));
    user_case("user_explicit_32", CLSID_ROOT, KEY_WOW64_32KEY, 32);
    user_case("user_explicit_64", CLSID_ROOT, KEY_WOW64_64KEY, 64);
    user_case("user_literal_wow6432node", WOW_ROOT, 0, 32);
    precedence(argc == 2);
    CoUninitialize();
    printf("measure_complete=1 cleanup_errors=%u\n", cleanup_errors);
    return cleanup_errors ? 2 : 0;
}
