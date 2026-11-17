/*
 * MEASURE: deleting and renaming an AF_UNIX socket file, as Windows 10 does.
 *
 * Original AI-assisted (Claude), LGPL-2.1-or-later. Public API observations
 * only. CodeMeter removes a socket file left over from an earlier run before
 * it binds again; under Wine that removal fails with "Invalid parameter".
 * This prints what Windows returns for each way a program can delete or
 * rename a socket file (with no listener and while one listens), and whether
 * the file is gone afterwards. Files live in a new folder under %TEMP% and
 * are removed.
 *
 * Exit code: 0 when every observation ran, 2 when a setup step failed.
 */

#include <winsock2.h>
#include <windows.h>
#include <afunix.h>
#include <stddef.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

/* FileDispositionInfoEx, declared here so older headers build it too */
#define MEASURE_FILE_DISPOSITION_INFO_EX_CLASS 21
#define MEASURE_DISP_DELETE 0x1
#define MEASURE_DISP_POSIX 0x2
#define MEASURE_DISP_IGNORE_READONLY 0x10
typedef struct { DWORD Flags; } measure_disposition_ex;

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

static int addr_len(const char *path)
{
    return (int)(offsetof(SOCKADDR_UN, sun_path) + strlen(path) + 1);
}

static void set_addr(SOCKADDR_UN *addr, const char *path)
{
    size_t len = strlen(path);

    memset(addr, 0, sizeof(*addr));
    addr->sun_family = AF_UNIX;
    if (len >= sizeof(addr->sun_path)) { incomplete = 1; len = sizeof(addr->sun_path) - 1; }
    memcpy(addr->sun_path, path, len);
}

/* creates the socket file; returns the listening socket when keep is set */
static SOCKET make_socket_file(const char *path, int keep)
{
    SOCKADDR_UN addr;
    SOCKET s = socket(AF_UNIX, SOCK_STREAM, 0);

    if (s == INVALID_SOCKET) { incomplete = 1; return s; }
    set_addr(&addr, path);
    if (bind(s, (struct sockaddr *)&addr, addr_len(path)) || listen(s, 5))
    {
        closesocket(s);
        incomplete = 1;
        return INVALID_SOCKET;
    }
    if (keep) return s;
    closesocket(s);
    return INVALID_SOCKET;
}

static void report_exists(const char *key, const char *path)
{
    char k[96];
    DWORD attrs = GetFileAttributesA(path);

    snprintf(k, sizeof(k), "%s_after", key);
    out(k, "exists=%d attrs=%#lx err=%lu", attrs != INVALID_FILE_ATTRIBUTES,
        attrs == INVALID_FILE_ATTRIBUTES ? 0 : attrs,
        attrs == INVALID_FILE_ATTRIBUTES ? GetLastError() : 0);
    DeleteFileA(path); /* clean up whatever is left */
}

static void by_deletefile(const char *path, int listening)
{
    const char *key = listening ? "deletefile_listening" : "deletefile";
    SOCKET s = make_socket_file(path, listening);
    BOOL ret = DeleteFileA(path);

    out(key, "ret=%d err=%lu", ret, ret ? 0 : GetLastError());
    if (s != INVALID_SOCKET) closesocket(s);
    report_exists(key, path);
}

static void by_open(const char *key, const char *path, DWORD access, DWORD flags, int listening,
                    int disposition)
{
    SOCKET s = make_socket_file(path, listening);
    HANDLE h = CreateFileA(path, access, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                           NULL, OPEN_EXISTING, flags, NULL);
    char k[96];

    out(key, "open=%d err=%lu", h != INVALID_HANDLE_VALUE, h == INVALID_HANDLE_VALUE ? GetLastError() : 0);
    if (h != INVALID_HANDLE_VALUE)
    {
        BOOL ret = TRUE;

        if (disposition == 1)
        {
            FILE_DISPOSITION_INFO info = { TRUE };
            ret = SetFileInformationByHandle(h, FileDispositionInfo, &info, sizeof(info));
        }
        else if (disposition == 2)
        {
            measure_disposition_ex info = { MEASURE_DISP_DELETE | MEASURE_DISP_POSIX | MEASURE_DISP_IGNORE_READONLY };
            ret = SetFileInformationByHandle(h, (FILE_INFO_BY_HANDLE_CLASS)MEASURE_FILE_DISPOSITION_INFO_EX_CLASS,
                                             &info, sizeof(info));
        }
        if (disposition)
        {
            snprintf(k, sizeof(k), "%s_set", key);
            out(k, "ret=%d err=%lu", ret, ret ? 0 : GetLastError());
        }
        CloseHandle(h);
    }
    if (s != INVALID_SOCKET) closesocket(s);
    report_exists(key, path);
}

static void by_move(const char *path, const char *other, int listening)
{
    const char *key = listening ? "movefile_listening" : "movefile";
    SOCKET s = make_socket_file(path, listening);
    BOOL ret = MoveFileExA(path, other, 0);
    char k[96];
    DWORD attrs;

    out(key, "ret=%d err=%lu", ret, ret ? 0 : GetLastError());
    if (s != INVALID_SOCKET) closesocket(s);
    attrs = GetFileAttributesA(other);
    snprintf(k, sizeof(k), "%s_target", key);
    out(k, "exists=%d attrs=%#lx", attrs != INVALID_FILE_ATTRIBUTES,
        attrs == INVALID_FILE_ATTRIBUTES ? 0 : attrs);
    DeleteFileA(other);
    report_exists(key, path);
}

int main(void)
{
    static const DWORD reparse = FILE_FLAG_OPEN_REPARSE_POINT | FILE_FLAG_BACKUP_SEMANTICS;
    WSADATA wsa;
    char tmp[MAX_PATH], path[MAX_PATH], other[MAX_PATH];
    int ret, listening;

    out("measure", "ws2_32-afunix-del format=1 arch=%s", sizeof(void *) == 8 ? "x86_64" : "i686");
    if (WSAStartup(MAKEWORD(2, 2), &wsa)) return 2;
    GetTempPathA(sizeof(tmp), tmp);
    ret = snprintf(dir, sizeof(dir), "%.200safunix-del-%lu", tmp, GetCurrentProcessId());
    if (ret < 0 || ret >= MAX_PATH - 16 || !CreateDirectoryA(dir, NULL)) return 2;
    snprintf(path, sizeof(path), "%.240s\\s", dir);
    snprintf(other, sizeof(other), "%.240s\\t", dir);

    for (listening = 0; listening <= 1; listening++)
    {
        const char *l = listening ? "_listening" : "";
        char key[96];

        by_deletefile(path, listening);

        snprintf(key, sizeof(key), "disposition_reparse%s", l);
        by_open(key, path, DELETE, reparse, listening, 1);
        snprintf(key, sizeof(key), "disposition_ex_reparse%s", l);
        by_open(key, path, DELETE, reparse, listening, 2);
        snprintf(key, sizeof(key), "delete_on_close_reparse%s", l);
        by_open(key, path, DELETE, reparse | FILE_FLAG_DELETE_ON_CLOSE, listening, 0);
        snprintf(key, sizeof(key), "open_delete_backup%s", l);
        by_open(key, path, DELETE, FILE_FLAG_BACKUP_SEMANTICS, listening, 1);
        snprintf(key, sizeof(key), "open_delete_plain%s", l);
        by_open(key, path, DELETE, 0, listening, 1);

        by_move(path, other, listening);
    }

    ret = RemoveDirectoryA(dir);
    out("cleanup", "remove_dir=%d", ret);
    WSACleanup();
    out("complete", "%d", !incomplete);
    return incomplete ? 2 : 0;
}
