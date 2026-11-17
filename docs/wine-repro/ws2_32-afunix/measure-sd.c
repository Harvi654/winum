/*
 * MEASURE: opening and securing an AF_UNIX socket file, as Windows 10 does.
 *
 * Original AI-assisted (Claude), LGPL-2.1-or-later. Public API observations
 * only. CodeMeter binds an AF_UNIX socket and then calls SetNamedSecurityInfoA
 * on its file; under Wine that open fails. This prints how Windows opens the
 * socket file for each access/flag combination, what SetNamedSecurityInfo and
 * GetNamedSecurityInfo return on it, and whether a client can still connect
 * afterwards. Files live in a new folder under %TEMP% and are removed.
 *
 * Exit code: 0 when every observation ran, 2 when a setup step failed.
 */

#include <winsock2.h>
#include <windows.h>
#include <winternl.h>
#include <aclapi.h>
#include <afunix.h>
#include <stddef.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#ifndef FILE_OPEN
#define FILE_OPEN 0x00000001
#endif
#ifndef FILE_OPEN_FOR_BACKUP_INTENT
#define FILE_OPEN_FOR_BACKUP_INTENT 0x00004000
#endif
#ifndef FILE_OPEN_REPARSE_POINT
#define FILE_OPEN_REPARSE_POINT 0x00200000
#endif

typedef NTSTATUS (WINAPI *ntcreatefile_t)( HANDLE *, ACCESS_MASK, OBJECT_ATTRIBUTES *, IO_STATUS_BLOCK *,
                                           LARGE_INTEGER *, ULONG, ULONG, ULONG, ULONG, void *, ULONG );
typedef BOOLEAN (WINAPI *dos_to_nt_t)( const WCHAR *, UNICODE_STRING *, WCHAR **, void * );
typedef void (WINAPI *free_us_t)( UNICODE_STRING * );

static int incomplete;
static char dir[MAX_PATH + 32];

static void out(const char *key, const char *fmt, ...)
{
    va_list ap;
    printf("%s ", key);
    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);
    printf("\n");
    fflush(stdout);
}

static void set_addr(SOCKADDR_UN *addr, const char *path)
{
    size_t len = strlen(path);

    memset(addr, 0, sizeof(*addr));
    addr->sun_family = AF_UNIX;
    if (len >= sizeof(addr->sun_path)) { incomplete = 1; len = sizeof(addr->sun_path) - 1; }
    memcpy(addr->sun_path, path, len);
}

static SOCKET bound_socket(const char *path)
{
    SOCKADDR_UN addr;
    SOCKET s = socket(AF_UNIX, SOCK_STREAM, 0);

    if (s == INVALID_SOCKET) return s;
    set_addr(&addr, path);
    if (bind(s, (struct sockaddr *)&addr, (int)(offsetof(SOCKADDR_UN, sun_path) + strlen(path) + 1)) ||
        listen(s, 5))
    {
        closesocket(s);
        return INVALID_SOCKET;
    }
    return s;
}

static void measure_createfile(const char *path)
{
    static const struct { const char *name; DWORD access; } accesses[] =
    {
        { "none", 0 },
        { "read_control", READ_CONTROL },
        { "write_dac", WRITE_DAC },
        { "read_control_write_dac", READ_CONTROL | WRITE_DAC },
        { "read_attributes", FILE_READ_ATTRIBUTES },
        { "write_attributes", FILE_WRITE_ATTRIBUTES },
        { "generic_read", GENERIC_READ },
        { "delete", DELETE },
    };
    static const struct { const char *name; DWORD flags; } flagsets[] =
    {
        { "0", 0 },
        { "backup", FILE_FLAG_BACKUP_SEMANTICS },
        { "reparse", FILE_FLAG_OPEN_REPARSE_POINT },
        { "backup_reparse", FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT },
    };
    unsigned int i, j;
    char key[96];

    for (i = 0; i < sizeof(accesses) / sizeof(accesses[0]); i++)
        for (j = 0; j < sizeof(flagsets) / sizeof(flagsets[0]); j++)
        {
            HANDLE h = CreateFileA(path, accesses[i].access, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                   NULL, OPEN_EXISTING, flagsets[j].flags, NULL);
            DWORD err = h == INVALID_HANDLE_VALUE ? GetLastError() : 0;
            snprintf(key, sizeof(key), "createfile_%s_%s", accesses[i].name, flagsets[j].name);
            out(key, "ok=%d err=%lu", h != INVALID_HANDLE_VALUE, err);
            if (h != INVALID_HANDLE_VALUE) CloseHandle(h);
        }
}

static void measure_ntcreatefile(const char *path)
{
    HMODULE ntdll = GetModuleHandleA("ntdll.dll");
    ntcreatefile_t pNtCreateFile = (ntcreatefile_t)(void (*)(void))GetProcAddress(ntdll, "NtCreateFile");
    dos_to_nt_t pDos = (dos_to_nt_t)(void (*)(void))GetProcAddress(ntdll, "RtlDosPathNameToNtPathName_U");
    free_us_t pFree = (free_us_t)(void (*)(void))GetProcAddress(ntdll, "RtlFreeUnicodeString");
    static const struct { const char *name; ULONG options; } options[] =
    {
        { "backup_intent", FILE_OPEN_FOR_BACKUP_INTENT },
        { "backup_intent_reparse", FILE_OPEN_FOR_BACKUP_INTENT | FILE_OPEN_REPARSE_POINT },
        { "none", 0 },
    };
    WCHAR pathW[MAX_PATH];
    UNICODE_STRING name;
    unsigned int i;
    char key[96];

    if (!pNtCreateFile || !pDos || !pFree) { incomplete = 1; return; }
    MultiByteToWideChar(CP_ACP, 0, path, -1, pathW, MAX_PATH);
    if (!pDos(pathW, &name, NULL, NULL)) { incomplete = 1; return; }

    /* the way an advapi32 SetNamedSecurityInfo could open it: WRITE_DAC | READ_CONTROL */
    for (i = 0; i < sizeof(options) / sizeof(options[0]); i++)
    {
        OBJECT_ATTRIBUTES attr;
        IO_STATUS_BLOCK io;
        HANDLE h = NULL;
        NTSTATUS status;

        InitializeObjectAttributes(&attr, &name, OBJ_CASE_INSENSITIVE, NULL, NULL);
        status = pNtCreateFile(&h, WRITE_DAC | READ_CONTROL | SYNCHRONIZE, &attr, &io, NULL,
                               FILE_FLAG_BACKUP_SEMANTICS, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                               FILE_OPEN, options[i].options, NULL, 0);
        snprintf(key, sizeof(key), "ntcreatefile_dac_%s", options[i].name);
        out(key, "status=%#lx", (unsigned long)status);
        if (!status) CloseHandle(h);
    }
    pFree(&name);
}

static void measure_security(const char *path)
{
    PSECURITY_DESCRIPTOR sd = NULL;
    PACL dacl = NULL, new_dacl = NULL;
    EXPLICIT_ACCESSA ea;
    DWORD ret;
    char sid_buf[SECURITY_MAX_SID_SIZE];
    DWORD sid_size = sizeof(sid_buf);

    ret = GetNamedSecurityInfoA(path, SE_FILE_OBJECT, DACL_SECURITY_INFORMATION, NULL, NULL, &dacl, NULL, &sd);
    out("getnamedsecurityinfo_dacl", "ret=%lu dacl=%d aces=%u", ret, dacl != NULL, dacl ? dacl->AceCount : 0);
    if (sd) LocalFree(sd);
    sd = NULL;

    if (!CreateWellKnownSid(WinWorldSid, NULL, sid_buf, &sid_size)) { incomplete = 1; return; }
    memset(&ea, 0, sizeof(ea));
    ea.grfAccessPermissions = GENERIC_ALL;
    ea.grfAccessMode = SET_ACCESS;
    ea.grfInheritance = NO_INHERITANCE;
    ea.Trustee.TrusteeForm = TRUSTEE_IS_SID;
    ea.Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
    ea.Trustee.ptstrName = (char *)sid_buf;
    ret = SetEntriesInAclA(1, &ea, NULL, &new_dacl);
    if (ret) { out("setentriesinacl", "ret=%lu", ret); incomplete = 1; return; }

    ret = SetNamedSecurityInfoA((char *)path, SE_FILE_OBJECT, DACL_SECURITY_INFORMATION, NULL, NULL, new_dacl, NULL);
    out("setnamedsecurityinfo_dacl", "ret=%lu", ret);
    ret = SetNamedSecurityInfoA((char *)path, SE_FILE_OBJECT,
                                DACL_SECURITY_INFORMATION | PROTECTED_DACL_SECURITY_INFORMATION,
                                NULL, NULL, new_dacl, NULL);
    out("setnamedsecurityinfo_protected_dacl", "ret=%lu", ret);
    LocalFree(new_dacl);

    dacl = NULL;
    ret = GetNamedSecurityInfoA(path, SE_FILE_OBJECT, DACL_SECURITY_INFORMATION, NULL, NULL, &dacl, NULL, &sd);
    out("getnamedsecurityinfo_after", "ret=%lu dacl=%d aces=%u", ret, dacl != NULL, dacl ? dacl->AceCount : 0);
    if (sd) LocalFree(sd);
}

int main(void)
{
    WSADATA wsa;
    char tmp[MAX_PATH], path[MAX_PATH];
    SOCKET listener, client;
    SOCKADDR_UN addr;
    int ret;

    out("measure", "ws2_32-afunix-sd format=1 arch=%s", sizeof(void *) == 8 ? "x86_64" : "i686");
    if (WSAStartup(MAKEWORD(2, 2), &wsa)) return 2;
    GetTempPathA(sizeof(tmp), tmp);
    ret = snprintf(dir, sizeof(dir), "%.200safunix-sd-%lu", tmp, GetCurrentProcessId());
    if (ret < 0 || ret >= MAX_PATH - 16 || !CreateDirectoryA(dir, NULL)) return 2;
    snprintf(path, sizeof(path), "%s\\s", dir);

    listener = bound_socket(path);
    out("bind_listen", "ok=%d err=%d", listener != INVALID_SOCKET, listener == INVALID_SOCKET ? WSAGetLastError() : 0);
    if (listener == INVALID_SOCKET) return 2;

    measure_createfile(path);
    measure_ntcreatefile(path);
    measure_security(path);

    client = socket(AF_UNIX, SOCK_STREAM, 0);
    set_addr(&addr, path);
    ret = connect(client, (struct sockaddr *)&addr, (int)(offsetof(SOCKADDR_UN, sun_path) + strlen(path) + 1));
    out("connect_after_setsecurity", "ret=%d err=%d", ret, ret ? WSAGetLastError() : 0);
    closesocket(client);
    closesocket(listener);

    ret = DeleteFileA(path);
    out("delete", "ret=%d err=%lu", ret, ret ? 0 : GetLastError());
    ret = RemoveDirectoryA(dir);
    out("cleanup", "remove_dir=%d", ret);
    WSACleanup();
    out("complete", "%d", !incomplete);
    return incomplete ? 2 : 0;
}
