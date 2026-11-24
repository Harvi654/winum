/* Original registry MEASURE program, AI-assisted (Codex), LGPL-2.1-or-later. */
#define _WIN32_WINNT 0x0a00
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <objbase.h>
#include <shellapi.h>
#include <stdio.h>
#include <string.h>
#include <wchar.h>

#define COUNT(a) (sizeof(a) / sizeof((a)[0]))
#define GUID_LEN 39
#define FIXTURES 12
#define PROBES (FIXTURES + 1)
#define WAIT_MS 120000

static const WCHAR *const roots[] = {L"CLSID", L"Wow6432Node\\CLSID"};
static const WCHAR *const value_names[] = {NULL, L"Shared", L"UserOnly", L"MachineOnly"};
static const char *const value_labels[] = {"default", "shared", "user_only", "machine_only"};
static const char *const view_labels[] = {"default", "32", "64", "literal_wow"};
static const char *const hive_labels[] = {"user", "machine", "both"};
static const REGSAM views[] = {0, KEY_WOW64_32KEY, KEY_WOW64_64KEY, KEY_WOW64_64KEY};
static const DWORD get_views[] = {0, RRF_SUBKEY_WOW6432KEY, RRF_SUBKEY_WOW6464KEY, RRF_SUBKEY_WOW6464KEY};
static WCHAR ids[PROBES][GUID_LEN];
static BOOL wow;

static BOOL has_wow(void)
{
    SYSTEM_INFO info;
    GetNativeSystemInfo(&info);
    return info.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64 ||
           info.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_ARM64;
}

static BOOL enabled(unsigned int i)
{
    return i == FIXTURES || wow || !(i % COUNT(views));
}

static void class_path(WCHAR *path, size_t count, unsigned int i, unsigned int view, BOOL physical, BOOL leaf)
{
    swprintf(path, count, L"%ls%ls\\%ls%ls", physical ? L"Software\\Classes\\" : L"",
             roots[view == 3], ids[i], leaf ? L"\\InprocServer32" : L"");
}

static LSTATUS put_value(HKEY key, const WCHAR *name, const WCHAR *value)
{
    DWORD size = (lstrlenW(value) + 1) * sizeof(WCHAR);
    return RegSetValueExW(key, name, 0, REG_SZ, (const BYTE *)value, size);
}

static LSTATUS register_class(HKEY hive, unsigned int i)
{
    WCHAR path[160];
    const WCHAR *mark = hive == HKEY_CURRENT_USER ? L"USER" : L"MACHINE";
    const WCHAR *extra = hive == HKEY_CURRENT_USER ? L"UserOnly" : L"MachineOnly";
    unsigned int view = i % COUNT(views);
    DWORD disposition;
    HKEY key;
    LSTATUS status;

    class_path(path, COUNT(path), i, view, TRUE, TRUE);
    status = RegCreateKeyExW(hive, path, 0, NULL, REG_OPTION_VOLATILE,
                             KEY_SET_VALUE | views[view], NULL, &key, &disposition);
    if (status) return status;
    if (disposition != REG_CREATED_NEW_KEY) status = ERROR_ALREADY_EXISTS;
    if (!status) status = put_value(key, NULL, mark);
    if (!status) status = put_value(key, L"Shared", mark);
    if (!status) status = put_value(key, extra, mark);
    RegCloseKey(key);
    return status;
}

static LSTATUS check_classes(HKEY hive, BOOL remove)
{
    unsigned int i, view;
    LSTATUS result = ERROR_SUCCESS;

    /* Open each exact random GUID with the corresponding view before deleting. */
    for (i = 0; i < PROBES; ++i)
    {
        if (!enabled(i)) continue;
        for (view = 0; view < (wow ? COUNT(views) : 1); ++view)
        {
            WCHAR path[160];
            HKEY key;
            LSTATUS status;

            class_path(path, COUNT(path), i, view, TRUE, FALSE);
            status = RegOpenKeyExW(hive, path, 0, (remove ? KEY_READ | KEY_WRITE | DELETE : KEY_READ)
                                   | views[view], &key);
            if (!status)
            {
                if (!remove)
                {
                    RegCloseKey(key);
                    return ERROR_ALREADY_EXISTS;
                }
                status = RegDeleteTreeW(key, NULL);
                RegCloseKey(key);
                if (!status) status = RegDeleteKeyExW(hive, path, views[view], 0);
            }
            if (status && status != ERROR_FILE_NOT_FOUND && status != ERROR_PATH_NOT_FOUND) result = status;
        }
    }
    return result;
}

static const char *marker(LSTATUS status, const WCHAR *data, DWORD bytes)
{
    if (status) return "-";
    if (bytes == sizeof(L"USER") && !memcmp(data, L"USER", bytes)) return "USER";
    if (bytes == sizeof(L"MACHINE") && !memcmp(data, L"MACHINE", bytes)) return "MACHINE";
    if (bytes == sizeof(L"WRITE") && !memcmp(data, L"WRITE", bytes)) return "WRITE";
    return "other";
}

static void query(HKEY key, const WCHAR *name, const char *label)
{
    WCHAR data[32];
    DWORD type = 0, size = sizeof(data);
    LSTATUS status = RegQueryValueExW(key, name, NULL, &type, (BYTE *)data, &size);
    printf(" query api=RegQueryValueExW value=%s status=%ld type=%lu data=%s\n",
           label, status, type, marker(status, data, size));
}

static void query_key(HKEY key)
{
    unsigned int i;
    for (i = 0; i < COUNT(value_names); ++i) query(key, value_names[i], value_labels[i]);
}

static void enum_class(const WCHAR *id)
{
    HKEY parent;
    DWORD index;
    BOOL seen = FALSE;
    LSTATUS status = RegOpenKeyExW(HKEY_CLASSES_ROOT, L"CLSID", 0, KEY_ENUMERATE_SUB_KEYS, &parent);
    if (!status)
    {
        for (index = 0;; ++index)
        {
            WCHAR name[256];
            DWORD size = COUNT(name);
            status = RegEnumKeyExW(parent, index, name, &size, NULL, NULL, NULL, NULL);
            if (status == ERROR_MORE_DATA) continue;
            if (status) break;
            if (!lstrcmpiW(name, id))
            {
                seen = TRUE;
                break;
            }
        }
        RegCloseKey(parent);
    }
    printf(" enum api=RegEnumKeyExW status=%ld own_class_seen=%d\n", status, seen);
}

static void read_class(unsigned int i)
{
    unsigned int view, value;
    WCHAR path[160], child[80], data[32];

    swprintf(child, COUNT(child), L"%ls\\InprocServer32", ids[i]);
    for (view = 0; view < (wow ? COUNT(views) : 1); ++view)
    {
        HKEY key, parent;
        char ansi_path[160];
        LSTATUS status;

        class_path(path, COUNT(path), i, view, FALSE, TRUE);
        status = RegOpenKeyExW(HKEY_CLASSES_ROOT, path, 0, KEY_QUERY_VALUE | views[view], &key);
        printf(" open api=RegOpenKeyExW from=HKCR view=%s status=%ld\n", view_labels[view], status);
        if (!status)
        {
            query_key(key);
            RegCloseKey(key);
        }

        /* All names generated by the probe are ASCII. */
        if (WideCharToMultiByte(CP_ACP, 0, path, -1, ansi_path, sizeof(ansi_path), NULL, NULL))
        {
            status = RegOpenKeyExA(HKEY_CLASSES_ROOT, ansi_path, 0, KEY_QUERY_VALUE | views[view], &key);
            printf(" open api=RegOpenKeyExA from=HKCR view=%s status=%ld\n", view_labels[view], status);
            if (!status)
            {
                query_key(key);
                RegCloseKey(key);
            }
        }

        status = RegOpenKeyExW(HKEY_CLASSES_ROOT, roots[view == 3], 0, KEY_READ | views[view], &parent);
        printf(" open api=RegOpenKeyExW from=HKCR_parent view=%s status=%ld\n", view_labels[view], status);
        if (!status)
        {
            status = RegOpenKeyExW(parent, child, 0, KEY_QUERY_VALUE | views[view], &key);
            printf(" open api=RegOpenKeyExW from=parent_handle view=%s status=%ld\n",
                   view_labels[view], status);
            if (!status)
            {
                query_key(key);
                RegCloseKey(key);
            }
            RegCloseKey(parent);
        }

        for (value = 0; value < COUNT(value_names); ++value)
        {
            DWORD type = 0, size = sizeof(data);
            status = RegGetValueW(HKEY_CLASSES_ROOT, path, value_names[value],
                                  RRF_RT_REG_SZ | get_views[view], &type, data, &size);
            printf(" query api=RegGetValueW view=%s value=%s status=%ld type=%lu data=%s\n",
                   view_labels[view], value_labels[value], status, type, marker(status, data, size));
        }
        if (!views[view])
        {
            LONG size = sizeof(data);
            status = RegQueryValueW(HKEY_CLASSES_ROOT, path, data, &size);
            printf(" query api=RegQueryValueW view=%s status=%ld data=%s\n",
                   view_labels[view], status, marker(status, data, size));
        }
    }
    enum_class(ids[i]);
}

static void write_class(unsigned int i, unsigned int view)
{
    WCHAR path[160], data[32];
    HKEY key;
    DWORD disposition = 0, size;
    LSTATUS status;
    unsigned int n;
    const HKEY hives[] = {HKEY_CURRENT_USER, HKEY_LOCAL_MACHINE};
    const char *const labels[] = {"HKCU", "HKLM"};

    class_path(path, COUNT(path), i, view, FALSE, TRUE);
    status = RegCreateKeyExW(HKEY_CLASSES_ROOT, path, 0, NULL, REG_OPTION_VOLATILE,
                             KEY_SET_VALUE | views[view], NULL, &key, &disposition);
    printf(" write api=RegCreateKeyExW view=%s status=%ld disposition=%lu\n",
           view_labels[view], status, disposition);
    if (!status)
    {
        status = put_value(key, L"WriteProbe", L"WRITE");
        printf(" write api=RegSetValueExW status=%ld\n", status);
        RegCloseKey(key);
    }
    class_path(path, COUNT(path), i, view, TRUE, TRUE);
    for (n = 0; n < COUNT(hives); ++n)
    {
        size = sizeof(data);
        status = RegGetValueW(hives[n], path, L"WriteProbe", RRF_RT_REG_SZ | get_views[view],
                              NULL, data, &size);
        printf(" write_destination hive=%s view=%s status=%ld data=%s\n",
               labels[n], view_labels[view], status, marker(status, data, size));
    }
}

static LSTATUS setup_machine(void)
{
    unsigned int i;
    for (i = 0; i < FIXTURES; ++i)
    {
        LSTATUS status;
        if (!enabled(i) || i / COUNT(views) == 0) continue;
        status = register_class(HKEY_LOCAL_MACHINE, i);
        if (status) return status;
    }
    return ERROR_SUCCESS;
}

static void event_name(WCHAR *name, const WCHAR *kind)
{
    swprintf(name, 100, L"Local\\winum-hkcr-%ls-%ls", ids[FIXTURES], kind);
}

static int machine_helper(void)
{
    WCHAR name[100];
    HANDLE ready, done;
    LSTATUS status = ERROR_INVALID_HANDLE, cleanup;

    event_name(name, L"ready");
    ready = OpenEventW(EVENT_MODIFY_STATE, FALSE, name);
    event_name(name, L"done");
    done = OpenEventW(SYNCHRONIZE, FALSE, name);
    if (!ready || !done)
    {
        if (ready) CloseHandle(ready);
        if (done) CloseHandle(done);
        return ERROR_INVALID_HANDLE;
    }
    status = check_classes(HKEY_LOCAL_MACHINE, FALSE);
    if (status)
    {
        CloseHandle(ready);
        CloseHandle(done);
        return status;
    }
    status = setup_machine();
    if (!status)
        status = SetEvent(ready) ? WaitForSingleObject(done, WAIT_MS) : GetLastError();
    cleanup = check_classes(HKEY_LOCAL_MACHINE, TRUE);
    CloseHandle(ready);
    CloseHandle(done);
    return status ? status : cleanup;
}

static HANDLE launch_helper(HANDLE *ready, HANDLE *done)
{
    WCHAR name[100], module[MAX_PATH], args[700] = L"--machine-helper";
    SHELLEXECUTEINFOW launch = {0};
    unsigned int i;
    DWORD len;

    event_name(name, L"ready");
    *ready = CreateEventW(NULL, TRUE, FALSE, name);
    event_name(name, L"done");
    *done = CreateEventW(NULL, TRUE, FALSE, name);
    len = GetModuleFileNameW(NULL, module, COUNT(module));
    if (!*ready || !*done || !len || len >= COUNT(module)) return NULL;
    for (i = 0; i < PROBES; ++i)
    {
        wcscat(args, L" ");
        wcscat(args, ids[i]);
    }
    launch.cbSize = sizeof(launch);
    launch.fMask = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_NOASYNC;
    launch.lpVerb = L"runas";
    launch.lpFile = module;
    launch.lpParameters = args;
    launch.nShow = SW_HIDE;
    if (!ShellExecuteExW(&launch))
    {
        printf("helper_launch_error=%lu\n", GetLastError());
        return NULL;
    }
    return launch.hProcess;
}

static DWORD run_unit(const WCHAR *exe)
{
    STARTUPINFOW startup = {0};
    PROCESS_INFORMATION process;
    WCHAR command[MAX_PATH + 40];
    DWORD status;

    if (wcslen(exe) >= MAX_PATH || wcschr(exe, '"')) return ERROR_INVALID_PARAMETER;
    if (!SetEnvironmentVariableW(L"WINUM_HKCR_COLLISION", ids[2 * COUNT(views)]))
        return GetLastError();
    swprintf(command, COUNT(command), L"\"%ls\" hkcr_class_reads", exe);
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESTDHANDLES;
    startup.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    startup.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    startup.hStdError = GetStdHandle(STD_ERROR_HANDLE);
    if (!CreateProcessW(exe, command, NULL, NULL, TRUE, 0, NULL, NULL, &startup, &process))
        return GetLastError();

    status = WaitForSingleObject(process.hProcess, 30000);
    if (status == WAIT_TIMEOUT)
    {
        TerminateProcess(process.hProcess, WAIT_TIMEOUT);
        WaitForSingleObject(process.hProcess, 5000);
    }
    else if (status == WAIT_OBJECT_0 && !GetExitCodeProcess(process.hProcess, &status))
        status = GetLastError();
    CloseHandle(process.hThread);
    CloseHandle(process.hProcess);
    return status;
}

int wmain(int argc, WCHAR **argv)
{
    BOOL high = argc > 1 && !wcscmp(argv[1], L"--elevated");
    BOOL direct = high || (argc > 1 && !wcscmp(argv[1], L"--direct-machine"));
    int arg = direct ? 2 : 1;
    const WCHAR *unit = NULL;
    HANDLE user_token, helper = NULL, ready = NULL, done = NULL, handles[2];
    TOKEN_ELEVATION token;
    DWORD returned, wait_result, helper_exit = 0;
    unsigned int i, hive, view;
    LSTATUS status = ERROR_SUCCESS, cleanup_user, cleanup_machine = ERROR_SUCCESS;

    wow = has_wow();
    if (argc == PROBES + 2 && !wcscmp(argv[1], L"--machine-helper"))
    {
        GUID parsed;
        for (i = 0; i < PROBES; ++i)
        {
            if (wcslen(argv[i + 2]) != GUID_LEN - 1 || FAILED(CLSIDFromString(argv[i + 2], &parsed)))
                return ERROR_INVALID_PARAMETER;
            wcscpy(ids[i], argv[i + 2]);
        }
        return machine_helper();
    }
    if (argc == arg + 2 && !wcscmp(argv[arg], L"--run-test")) unit = argv[arg + 1];
    else if (argc != arg) return ERROR_INVALID_PARAMETER;

    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &user_token)) return GetLastError();
    if (!GetTokenInformation(user_token, TokenElevation, &token, sizeof(token), &returned))
    {
        CloseHandle(user_token);
        return 1;
    }
    if (high)
    {
        union
        {
            TOKEN_MANDATORY_LABEL label;
            BYTE buffer[sizeof(TOKEN_MANDATORY_LABEL) + SECURITY_MAX_SID_SIZE];
        } integrity;
        DWORD rid = 0;
        PSID sid;

        if (GetTokenInformation(user_token, TokenIntegrityLevel, &integrity, sizeof(integrity), &returned))
        {
            sid = integrity.label.Label.Sid;
            rid = *GetSidSubAuthority(sid, *GetSidSubAuthorityCount(sid) - 1);
        }
        printf("token_integrity=0x%04lx elevated=%lu\n", rid, token.TokenIsElevated);
        if (!token.TokenIsElevated || rid != SECURITY_MANDATORY_HIGH_RID)
        {
            puts("setup_error=run_from_high_integrity_administrator_shell");
            CloseHandle(user_token);
            return 1;
        }
    }
    CloseHandle(user_token);
    printf("measure=hkcr-read bits=%u wow=%d elevated=%lu machine_setup=%s\n",
           (unsigned int)(sizeof(void *) * 8), wow, token.TokenIsElevated, direct ? "direct" : "runas_helper");
    if (!direct && token.TokenIsElevated)
    {
        puts("setup_error=run_from_non_elevated_shell");
        return 1;
    }
    for (i = 0; i < PROBES; ++i)
    {
        GUID generated;
        if (FAILED(CoCreateGuid(&generated)) || !StringFromGUID2(&generated, ids[i], GUID_LEN)) return 1;
    }

    status = check_classes(HKEY_CURRENT_USER, FALSE);
    if (!status && direct) status = check_classes(HKEY_LOCAL_MACHINE, FALSE);
    if (status)
    {
        printf("setup_error=registry_preflight status=%ld\n", status);
        return 1;
    }
    if (direct) status = setup_machine();
    else
    {
        helper = launch_helper(&ready, &done);
        if (!helper) status = ERROR_CANCELLED;
        else
        {
            handles[0] = ready;
            handles[1] = helper;
            wait_result = WaitForMultipleObjects(2, handles, FALSE, WAIT_MS);
            if (wait_result != WAIT_OBJECT_0)
                status = wait_result == WAIT_OBJECT_0 + 1 ? ERROR_GEN_FAILURE : wait_result;
        }
    }
    printf("machine_setup_status=%ld\n", status);
    if (status) goto cleanup;

    if (unit)
    {
        status = register_class(HKEY_CURRENT_USER, 2 * COUNT(views));
        if (!status)
        {
            fflush(stdout);
            status = run_unit(unit);
        }
        printf("l1_unit_status=%ld\n", status);
        goto cleanup;
    }

    for (i = 0; i < FIXTURES; ++i)
    {
        if (!enabled(i)) continue;
        hive = i / COUNT(views);
        view = i % COUNT(views);
        printf("case=%s registration_view=%s\n", hive_labels[hive], view_labels[view]);
        if (hive != 1)
        {
            status = register_class(HKEY_CURRENT_USER, i);
            printf("user_setup_status=%ld\n", status);
            if (status) goto cleanup;
        }
        read_class(i);
        write_class(i, view);
    }
    puts("case=absent registration_view=none");
    read_class(FIXTURES);
    write_class(FIXTURES, 0);

cleanup:
    cleanup_user = check_classes(HKEY_CURRENT_USER, TRUE);
    if (direct) cleanup_machine = check_classes(HKEY_LOCAL_MACHINE, TRUE);
    if (helper)
    {
        if (done) SetEvent(done);
        wait_result = WaitForSingleObject(helper, WAIT_MS);
        if (wait_result != WAIT_OBJECT_0) cleanup_machine = wait_result;
        else if (GetExitCodeProcess(helper, &helper_exit)) cleanup_machine = helper_exit;
        else cleanup_machine = GetLastError();
        CloseHandle(helper);
    }
    if (ready) CloseHandle(ready);
    if (done) CloseHandle(done);
    printf("measure_complete=%d user_cleanup=%ld machine_cleanup=%ld\n",
           !status && !cleanup_user && !cleanup_machine, cleanup_user, cleanup_machine);
    return !!(status || cleanup_user || cleanup_machine);
}
