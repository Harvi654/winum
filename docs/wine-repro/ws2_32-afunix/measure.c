/*
 * MEASURE: AF_UNIX stream sockets in Winsock, as Windows 10 behaves.
 *
 * Original AI-assisted (Claude), LGPL-2.1-or-later. Public Winsock API
 * observations only; it prints what Windows returns and judges nothing.
 * All files live in a new folder under %TEMP% and are removed at the end;
 * paths are printed relative to that folder ("<dir>"), never with the
 * user's profile path.
 *
 * Exit code: 0 when every observation ran, 2 when a setup step failed
 * (the result is then incomplete).
 */

#include <winsock2.h>
#include <ws2tcpip.h>
#include <mswsock.h>
#include <windows.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#ifndef AF_UNIX
#define AF_UNIX 1
#endif

/* Our own definition of the address, with the layout of afunix.h. */
struct m_sockaddr_un
{
    ADDRESS_FAMILY sun_family;
    char sun_path[108];
};

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

static const char *rel(const char *path)
{
    static char buf[2][2 * MAX_PATH];
    static int n;
    char *b = buf[n++ & 1];
    size_t len = strlen(dir);
    if (!_strnicmp(path, dir, len))
        snprintf(b, 2 * MAX_PATH, "<dir>%.*s", MAX_PATH, path + len);
    else
        snprintf(b, 2 * MAX_PATH, "%.*s", MAX_PATH, path);
    return b;
}

static void make_addr(struct m_sockaddr_un *addr, const char *path)
{
    memset(addr, 0, sizeof(*addr));
    addr->sun_family = AF_UNIX;
    strncpy(addr->sun_path, path, sizeof(addr->sun_path) - 1);
}

static int addr_len(const char *path)
{
    return (int)(offsetof(struct m_sockaddr_un, sun_path) + strlen(path) + 1);
}

static void path_in_dir(char *buf, const char *name)
{
    int n = snprintf(buf, MAX_PATH, "%s\\%s", dir, name);
    if (n < 0 || n >= MAX_PATH) incomplete = 1;
}

static void file_info(const char *key, const char *path)
{
    WIN32_FIND_DATAA fd;
    HANDLE h;
    DWORD attrs = GetFileAttributesA(path);
    DWORD err = attrs == INVALID_FILE_ATTRIBUTES ? GetLastError() : 0;

    h = FindFirstFileA(path, &fd);
    if (h == INVALID_HANDLE_VALUE)
        out(key, "attrs=%#lx err=%lu find=none", attrs, err);
    else
    {
        out(key, "attrs=%#lx err=%lu find_attrs=%#lx reparse_tag=%#lx size=%lu",
            attrs, err, fd.dwFileAttributes, fd.dwReserved0, fd.nFileSizeLow);
        FindClose(h);
    }
}

static void print_addr(const char *key, int ret, const struct m_sockaddr_un *addr, int len)
{
    int err = ret ? WSAGetLastError() : 0;
    if (ret)
    {
        out(key, "ret=%d err=%d", ret, err);
        return;
    }
    if (len > (int)offsetof(struct m_sockaddr_un, sun_path))
        out(key, "ret=0 len=%d family=%u path_strlen=%u path=%s first=%#x",
            len, addr->sun_family, (unsigned)strnlen(addr->sun_path, sizeof(addr->sun_path)),
            rel(addr->sun_path), (unsigned char)addr->sun_path[0]);
    else
        out(key, "ret=0 len=%d family=%u", len, addr->sun_family);
}

static SOCKET new_socket(const char *key)
{
    SOCKET s = socket(AF_UNIX, SOCK_STREAM, 0);
    if (s == INVALID_SOCKET)
    {
        out(key, "socket failed err=%d", WSAGetLastError());
        incomplete = 1;
    }
    return s;
}

static void close_socket(SOCKET s)
{
    if (s != INVALID_SOCKET) closesocket(s);
}

/* A bound, listening socket at path; INVALID_SOCKET (and incomplete) on failure. */
static SOCKET listener(const char *key, const char *path)
{
    struct m_sockaddr_un addr;
    SOCKET s = new_socket(key);
    if (s == INVALID_SOCKET) return s;
    make_addr(&addr, path);
    if (bind(s, (struct sockaddr *)&addr, addr_len(path)) || listen(s, 5))
    {
        out(key, "setup failed err=%d", WSAGetLastError());
        incomplete = 1;
        closesocket(s);
        return INVALID_SOCKET;
    }
    return s;
}

static int do_connect(SOCKET s, const char *path)
{
    struct m_sockaddr_un addr;
    make_addr(&addr, path);
    return connect(s, (struct sockaddr *)&addr, addr_len(path));
}

static void measure_protocols(void)
{
    DWORD size = 0;
    int i, n, count = 0;
    WSAPROTOCOL_INFOW *info;

    n = WSAEnumProtocolsW(NULL, NULL, &size);
    out("enum_protocols_size_query", "ret=%d err=%d", n, WSAGetLastError());
    info = HeapAlloc(GetProcessHeap(), 0, size);
    if (!info) { incomplete = 1; return; }
    n = WSAEnumProtocolsW(NULL, info, &size);
    out("enum_protocols", "ret=%d", n);
    for (i = 0; i < n; i++)
    {
        char name[256];
        if (info[i].iAddressFamily != AF_UNIX) continue;
        count++;
        WideCharToMultiByte(CP_UTF8, 0, info[i].szProtocol, -1, name, sizeof(name), NULL, NULL);
        out("enum_protocols_unix", "type=%d protocol=%d flags1=%#lx provider_flags=%#lx "
            "max_addr=%d min_addr=%d version=%d name=%s",
            info[i].iSocketType, info[i].iProtocol, info[i].dwServiceFlags1,
            info[i].dwProviderFlags, info[i].iMaxSockAddr, info[i].iMinSockAddr,
            info[i].iVersion, name);
    }
    out("enum_protocols_unix_count", "%d", count);
    HeapFree(GetProcessHeap(), 0, info);
}

static void measure_socket_kinds(void)
{
    static const struct { const char *key; int type, protocol; } kinds[] =
    {
        { "socket_stream_0", SOCK_STREAM, 0 },
        { "socket_dgram_0", SOCK_DGRAM, 0 },
        { "socket_seqpacket_0", SOCK_SEQPACKET, 0 },
        { "socket_stream_tcp", SOCK_STREAM, IPPROTO_TCP },
    };
    unsigned i;
    SOCKET s;

    for (i = 0; i < sizeof(kinds) / sizeof(kinds[0]); i++)
    {
        s = socket(AF_UNIX, kinds[i].type, kinds[i].protocol);
        out(kinds[i].key, "valid=%d err=%d", s != INVALID_SOCKET, s == INVALID_SOCKET ? WSAGetLastError() : 0);
        close_socket(s);
    }
    s = WSASocketW(AF_UNIX, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
    out("wsasocket_stream_overlapped", "valid=%d err=%d", s != INVALID_SOCKET,
        s == INVALID_SOCKET ? WSAGetLastError() : 0);
    if (s != INVALID_SOCKET)
    {
        WSAPROTOCOL_INFOW pi;
        int type = 0, len = sizeof(type), ret;
        struct m_sockaddr_un addr;

        ret = getsockopt(s, SOL_SOCKET, SO_TYPE, (char *)&type, &len);
        out("sockopt_type", "ret=%d type=%d len=%d", ret, type, len);
        len = sizeof(pi);
        memset(&pi, 0, sizeof(pi));
        ret = getsockopt(s, SOL_SOCKET, SO_PROTOCOL_INFOW, (char *)&pi, &len);
        out("sockopt_protocol_info", "ret=%d family=%d type=%d protocol=%d flags1=%#lx max_addr=%d min_addr=%d",
            ret, pi.iAddressFamily, pi.iSocketType, pi.iProtocol, pi.dwServiceFlags1,
            pi.iMaxSockAddr, pi.iMinSockAddr);
        len = sizeof(addr);
        memset(&addr, 0xcc, sizeof(addr));
        ret = getsockname(s, (struct sockaddr *)&addr, &len);
        print_addr("getsockname_unbound", ret, &addr, len);
        closesocket(s);
    }
}

static void measure_bind_connect(void)
{
    char path[MAX_PATH], regular[MAX_PATH];
    struct m_sockaddr_un addr;
    SOCKET srv, cli, acc, other;
    int ret, len;
    char buf[16];
    HANDLE h;

    path_in_dir(path, "s1");
    out("path_length", "%u", (unsigned)strlen(path));
    file_info("file_before_bind", path);

    srv = new_socket("bind_socket");
    if (srv == INVALID_SOCKET) return;
    make_addr(&addr, path);
    ret = bind(srv, (struct sockaddr *)&addr, addr_len(path));
    out("bind", "ret=%d err=%d", ret, ret ? WSAGetLastError() : 0);
    if (ret) { incomplete = 1; closesocket(srv); return; }
    file_info("file_after_bind", path);

    len = sizeof(addr);
    memset(&addr, 0xcc, sizeof(addr));
    ret = getsockname(srv, (struct sockaddr *)&addr, &len);
    print_addr("getsockname_bound", ret, &addr, len);

    other = new_socket("bind_again_socket");
    if (other != INVALID_SOCKET)
    {
        make_addr(&addr, path);
        ret = bind(other, (struct sockaddr *)&addr, addr_len(path));
        out("bind_existing_socket_path", "ret=%d err=%d", ret, ret ? WSAGetLastError() : 0);
        closesocket(other);
    }

    cli = new_socket("connect_before_listen");
    if (cli != INVALID_SOCKET)
    {
        ret = do_connect(cli, path);
        out("connect_before_listen", "ret=%d err=%d", ret, ret ? WSAGetLastError() : 0);
        closesocket(cli);
    }

    ret = listen(srv, 5);
    out("listen", "ret=%d err=%d", ret, ret ? WSAGetLastError() : 0);
    if (ret) { incomplete = 1; closesocket(srv); return; }

    cli = new_socket("connect_socket");
    if (cli == INVALID_SOCKET) { closesocket(srv); return; }
    ret = do_connect(cli, path);
    out("connect", "ret=%d err=%d", ret, ret ? WSAGetLastError() : 0);
    if (ret) { incomplete = 1; closesocket(cli); closesocket(srv); return; }

    {
        fd_set rd;
        struct timeval tv = { 2, 0 };
        FD_ZERO(&rd);
        FD_SET(srv, &rd);
        ret = select(0, &rd, NULL, NULL, &tv);
        out("select_listener_readable", "ret=%d set=%d", ret, FD_ISSET(srv, &rd) ? 1 : 0);
    }

    len = sizeof(addr);
    memset(&addr, 0xcc, sizeof(addr));
    acc = accept(srv, (struct sockaddr *)&addr, &len);
    if (acc == INVALID_SOCKET)
    {
        out("accept", "valid=0 err=%d", WSAGetLastError());
        incomplete = 1;
        closesocket(cli);
        closesocket(srv);
        return;
    }
    print_addr("accept_addr", 0, &addr, len);

    len = sizeof(addr);
    memset(&addr, 0xcc, sizeof(addr));
    ret = getpeername(cli, (struct sockaddr *)&addr, &len);
    print_addr("getpeername_client", ret, &addr, len);
    len = sizeof(addr);
    memset(&addr, 0xcc, sizeof(addr));
    ret = getsockname(cli, (struct sockaddr *)&addr, &len);
    print_addr("getsockname_client", ret, &addr, len);
    len = sizeof(addr);
    memset(&addr, 0xcc, sizeof(addr));
    ret = getpeername(acc, (struct sockaddr *)&addr, &len);
    print_addr("getpeername_accepted", ret, &addr, len);
    len = sizeof(addr);
    memset(&addr, 0xcc, sizeof(addr));
    ret = getsockname(acc, (struct sockaddr *)&addr, &len);
    print_addr("getsockname_accepted", ret, &addr, len);

    ret = send(cli, "ping", 4, 0);
    out("send_client", "ret=%d err=%d", ret, ret < 0 ? WSAGetLastError() : 0);
    memset(buf, 0, sizeof(buf));
    ret = recv(acc, buf, sizeof(buf), 0);
    out("recv_accepted", "ret=%d data_ok=%d", ret, ret == 4 && !memcmp(buf, "ping", 4));
    ret = send(acc, "pong!", 5, 0);
    out("send_accepted", "ret=%d", ret);
    memset(buf, 0, sizeof(buf));
    ret = recv(cli, buf, sizeof(buf), 0);
    out("recv_client", "ret=%d data_ok=%d", ret, ret == 5 && !memcmp(buf, "pong!", 5));

    ret = shutdown(cli, SD_SEND);
    out("shutdown_client_send", "ret=%d", ret);
    ret = recv(acc, buf, sizeof(buf), 0);
    out("recv_after_shutdown", "ret=%d err=%d", ret, ret < 0 ? WSAGetLastError() : 0);

    closesocket(acc);
    closesocket(cli);
    closesocket(srv);
    file_info("file_after_close", path);

    cli = new_socket("connect_closed_socket");
    if (cli != INVALID_SOCKET)
    {
        ret = do_connect(cli, path);
        out("connect_after_listener_closed", "ret=%d err=%d", ret, ret ? WSAGetLastError() : 0);
        closesocket(cli);
    }

    other = new_socket("rebind_socket");
    if (other != INVALID_SOCKET)
    {
        make_addr(&addr, path);
        ret = bind(other, (struct sockaddr *)&addr, addr_len(path));
        out("bind_after_close_same_path", "ret=%d err=%d", ret, ret ? WSAGetLastError() : 0);
        closesocket(other);
    }

    ret = DeleteFileA(path);
    out("delete_socket_file", "ret=%d err=%lu", ret, ret ? 0 : GetLastError());
    file_info("file_after_delete", path);

    path_in_dir(path, "missing");
    cli = new_socket("connect_missing_socket");
    if (cli != INVALID_SOCKET)
    {
        ret = do_connect(cli, path);
        out("connect_missing_path", "ret=%d err=%d", ret, ret ? WSAGetLastError() : 0);
        closesocket(cli);
    }

    path_in_dir(regular, "regular");
    h = CreateFileA(regular, GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
    if (h == INVALID_HANDLE_VALUE) { incomplete = 1; return; }
    CloseHandle(h);
    cli = new_socket("connect_regular_socket");
    if (cli != INVALID_SOCKET)
    {
        ret = do_connect(cli, regular);
        out("connect_regular_file", "ret=%d err=%d", ret, ret ? WSAGetLastError() : 0);
        closesocket(cli);
    }
    other = new_socket("bind_regular_socket");
    if (other != INVALID_SOCKET)
    {
        make_addr(&addr, regular);
        ret = bind(other, (struct sockaddr *)&addr, addr_len(regular));
        out("bind_regular_file", "ret=%d err=%d", ret, ret ? WSAGetLastError() : 0);
        closesocket(other);
    }
    DeleteFileA(regular);
}

static void measure_bind_forms(void)
{
    char path[MAX_PATH], mixed[MAX_PATH], old_cwd[MAX_PATH];
    struct m_sockaddr_un addr;
    SOCKET s;
    int ret, len;
    char *p;

    /* empty path: only the family */
    s = new_socket("bind_empty_socket");
    if (s != INVALID_SOCKET)
    {
        make_addr(&addr, "");
        ret = bind(s, (struct sockaddr *)&addr, (int)offsetof(struct m_sockaddr_un, sun_path));
        out("bind_family_only", "ret=%d err=%d", ret, ret ? WSAGetLastError() : 0);
        len = sizeof(addr);
        memset(&addr, 0xcc, sizeof(addr));
        ret = getsockname(s, (struct sockaddr *)&addr, &len);
        print_addr("getsockname_family_only", ret, &addr, len);
        closesocket(s);
    }

    /* abstract name: leading NUL */
    s = new_socket("bind_abstract_socket");
    if (s != INVALID_SOCKET)
    {
        make_addr(&addr, "");
        memcpy(addr.sun_path + 1, "abstract-measure", 16);
        ret = bind(s, (struct sockaddr *)&addr, (int)offsetof(struct m_sockaddr_un, sun_path) + 17);
        out("bind_abstract", "ret=%d err=%d", ret, ret ? WSAGetLastError() : 0);
        closesocket(s);
    }

    /* full structure length instead of strlen + 1 */
    path_in_dir(path, "s-full");
    s = new_socket("bind_full_len_socket");
    if (s != INVALID_SOCKET)
    {
        make_addr(&addr, path);
        ret = bind(s, (struct sockaddr *)&addr, sizeof(addr));
        out("bind_full_struct_len", "ret=%d err=%d", ret, ret ? WSAGetLastError() : 0);
        len = sizeof(addr);
        ret = getsockname(s, (struct sockaddr *)&addr, &len);
        print_addr("getsockname_full_struct_len", ret, &addr, len);
        closesocket(s);
        DeleteFileA(path);
    }

    /* length without the terminating NUL */
    path_in_dir(path, "s-nonul");
    s = new_socket("bind_nonul_socket");
    if (s != INVALID_SOCKET)
    {
        make_addr(&addr, path);
        ret = bind(s, (struct sockaddr *)&addr, (int)(offsetof(struct m_sockaddr_un, sun_path) + strlen(path)));
        out("bind_len_without_nul", "ret=%d err=%d", ret, ret ? WSAGetLastError() : 0);
        file_info("file_len_without_nul", path);
        closesocket(s);
        DeleteFileA(path);
    }

    /* forward slashes after the drive (as in C:\ProgramData\x/y/z) */
    path_in_dir(path, "s-mixed");
    strcpy(mixed, path);
    p = strrchr(mixed, '\\');
    if (p) *p = '/';
    s = new_socket("bind_mixed_socket");
    if (s != INVALID_SOCKET)
    {
        make_addr(&addr, mixed);
        ret = bind(s, (struct sockaddr *)&addr, addr_len(mixed));
        out("bind_forward_slash", "ret=%d err=%d", ret, ret ? WSAGetLastError() : 0);
        file_info("file_forward_slash", path);
        len = sizeof(addr);
        ret = getsockname(s, (struct sockaddr *)&addr, &len);
        print_addr("getsockname_forward_slash", ret, &addr, len);
        closesocket(s);
        DeleteFileA(path);
    }

    /* relative to the current directory */
    GetCurrentDirectoryA(MAX_PATH, old_cwd);
    if (SetCurrentDirectoryA(dir))
    {
        s = new_socket("bind_relative_socket");
        if (s != INVALID_SOCKET)
        {
            make_addr(&addr, "s-rel");
            ret = bind(s, (struct sockaddr *)&addr, addr_len("s-rel"));
            out("bind_relative", "ret=%d err=%d", ret, ret ? WSAGetLastError() : 0);
            path_in_dir(path, "s-rel");
            file_info("file_relative", path);
            len = sizeof(addr);
            ret = getsockname(s, (struct sockaddr *)&addr, &len);
            print_addr("getsockname_relative", ret, &addr, len);
            closesocket(s);
            DeleteFileA(path);
        }
        SetCurrentDirectoryA(old_cwd);
    }
    else incomplete = 1;

    /* directory that does not exist */
    path_in_dir(path, "no-such-dir\\s");
    s = new_socket("bind_nodir_socket");
    if (s != INVALID_SOCKET)
    {
        make_addr(&addr, path);
        ret = bind(s, (struct sockaddr *)&addr, addr_len(path));
        out("bind_missing_directory", "ret=%d err=%d", ret, ret ? WSAGetLastError() : 0);
        closesocket(s);
    }
}

static void measure_nonblocking(void)
{
    char path[MAX_PATH];
    SOCKET srv, cli, acc;
    u_long one = 1;
    int ret, err;
    fd_set wr;
    struct timeval tv = { 2, 0 };

    path_in_dir(path, "s-nb");
    srv = listener("nonblocking_listener", path);
    if (srv == INVALID_SOCKET) return;
    ret = ioctlsocket(srv, FIONBIO, &one);
    acc = accept(srv, NULL, NULL);
    out("accept_nonblocking_empty", "valid=%d err=%d", acc != INVALID_SOCKET,
        acc == INVALID_SOCKET ? WSAGetLastError() : 0);
    close_socket(acc);

    cli = new_socket("nonblocking_client");
    if (cli != INVALID_SOCKET)
    {
        ret = ioctlsocket(cli, FIONBIO, &one);
        ret = do_connect(cli, path);
        err = ret ? WSAGetLastError() : 0;
        out("connect_nonblocking", "ret=%d err=%d", ret, err);
        FD_ZERO(&wr);
        FD_SET(cli, &wr);
        ret = select(0, NULL, &wr, NULL, &tv);
        out("select_nonblocking_writable", "ret=%d set=%d", ret, FD_ISSET(cli, &wr) ? 1 : 0);
        Sleep(100);
        acc = accept(srv, NULL, NULL);
        out("accept_nonblocking_pending", "valid=%d err=%d", acc != INVALID_SOCKET,
            acc == INVALID_SOCKET ? WSAGetLastError() : 0);
        close_socket(acc);
        closesocket(cli);
    }
    closesocket(srv);
    DeleteFileA(path);
}

static void measure_extensions(void)
{
    GUID acceptex_guid = WSAID_ACCEPTEX, connectex_guid = WSAID_CONNECTEX;
    GUID sockaddrs_guid = WSAID_GETACCEPTEXSOCKADDRS;
    LPFN_ACCEPTEX pAcceptEx = NULL;
    LPFN_CONNECTEX pConnectEx = NULL;
    LPFN_GETACCEPTEXSOCKADDRS pGetSockaddrs = NULL;
    char path[MAX_PATH], buffer[2 * (sizeof(struct m_sockaddr_un) + 16)];
    SOCKET srv, acc, cli;
    OVERLAPPED ov;
    DWORD bytes = 0, flags = 0;
    int ret, err;

    path_in_dir(path, "s-ex");
    srv = WSASocketW(AF_UNIX, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
    if (srv == INVALID_SOCKET) { incomplete = 1; return; }
    {
        struct m_sockaddr_un addr;
        make_addr(&addr, path);
        if (bind(srv, (struct sockaddr *)&addr, addr_len(path)) || listen(srv, 5))
        {
            out("extensions_setup", "failed err=%d", WSAGetLastError());
            incomplete = 1;
            closesocket(srv);
            return;
        }
    }

    ret = WSAIoctl(srv, SIO_GET_EXTENSION_FUNCTION_POINTER, &acceptex_guid, sizeof(acceptex_guid),
                   &pAcceptEx, sizeof(pAcceptEx), &bytes, NULL, NULL);
    out("acceptex_pointer", "ret=%d err=%d have=%d", ret, ret ? WSAGetLastError() : 0, pAcceptEx != NULL);
    ret = WSAIoctl(srv, SIO_GET_EXTENSION_FUNCTION_POINTER, &sockaddrs_guid, sizeof(sockaddrs_guid),
                   &pGetSockaddrs, sizeof(pGetSockaddrs), &bytes, NULL, NULL);
    out("getacceptexsockaddrs_pointer", "ret=%d err=%d have=%d", ret, ret ? WSAGetLastError() : 0,
        pGetSockaddrs != NULL);
    ret = WSAIoctl(srv, SIO_GET_EXTENSION_FUNCTION_POINTER, &connectex_guid, sizeof(connectex_guid),
                   &pConnectEx, sizeof(pConnectEx), &bytes, NULL, NULL);
    out("connectex_pointer", "ret=%d err=%d have=%d", ret, ret ? WSAGetLastError() : 0, pConnectEx != NULL);

    if (pAcceptEx)
    {
        acc = WSASocketW(AF_UNIX, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
        memset(&ov, 0, sizeof(ov));
        ov.hEvent = CreateEventW(NULL, TRUE, FALSE, NULL);
        bytes = 0;
        ret = pAcceptEx(srv, acc, buffer, 0, sizeof(struct m_sockaddr_un) + 16,
                        sizeof(struct m_sockaddr_un) + 16, &bytes, &ov);
        err = ret ? 0 : WSAGetLastError();
        out("acceptex_call", "ret=%d err=%d", ret, err);

        cli = new_socket("acceptex_client");
        if (cli != INVALID_SOCKET)
        {
            ret = do_connect(cli, path);
            out("acceptex_client_connect", "ret=%d err=%d", ret, ret ? WSAGetLastError() : 0);
            ret = WaitForSingleObject(ov.hEvent, 3000);
            out("acceptex_wait", "ret=%d", ret);
            bytes = 0;
            ret = WSAGetOverlappedResult(srv, &ov, &bytes, FALSE, &flags);
            out("acceptex_result", "ret=%d err=%d bytes=%lu", ret, ret ? 0 : WSAGetLastError(), bytes);
            ret = setsockopt(acc, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT, (char *)&srv, sizeof(srv));
            out("acceptex_update_context", "ret=%d err=%d", ret, ret ? WSAGetLastError() : 0);
            if (pGetSockaddrs)
            {
                struct sockaddr *local = NULL, *remote = NULL;
                int llen = 0, rlen = 0;
                pGetSockaddrs(buffer, 0, sizeof(struct m_sockaddr_un) + 16, sizeof(struct m_sockaddr_un) + 16,
                              &local, &llen, &remote, &rlen);
                if (local) print_addr("acceptex_local", 0, (struct m_sockaddr_un *)local, llen);
                if (remote) print_addr("acceptex_remote", 0, (struct m_sockaddr_un *)remote, rlen);
            }
            ret = send(cli, "x", 1, 0);
            ret = recv(acc, buffer, 1, 0);
            out("acceptex_data", "ret=%d", ret);
            closesocket(cli);
        }
        CloseHandle(ov.hEvent);
        close_socket(acc);
    }

    if (pConnectEx)
    {
        cli = WSASocketW(AF_UNIX, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
        if (cli != INVALID_SOCKET)
        {
            struct m_sockaddr_un addr;
            memset(&ov, 0, sizeof(ov));
            ov.hEvent = CreateEventW(NULL, TRUE, FALSE, NULL);
            make_addr(&addr, path);
            ret = pConnectEx(cli, (struct sockaddr *)&addr, addr_len(path), NULL, 0, NULL, &ov);
            out("connectex_unbound", "ret=%d err=%d", ret, ret ? 0 : WSAGetLastError());
            CloseHandle(ov.hEvent);
            closesocket(cli);
        }
    }
    closesocket(srv);
    DeleteFileA(path);
}

int main(void)
{
    WSADATA wsa;
    char tmp[MAX_PATH];
    OSVERSIONINFOW ver;
    int ret;
    typedef LONG (WINAPI *rtlgetversion_t)(OSVERSIONINFOW *);
    rtlgetversion_t pRtlGetVersion = (rtlgetversion_t)(void (*)(void))GetProcAddress(GetModuleHandleW(L"ntdll.dll"), "RtlGetVersion");

    memset(&ver, 0, sizeof(ver));
    ver.dwOSVersionInfoSize = sizeof(ver);
    if (pRtlGetVersion) pRtlGetVersion(&ver);
    out("measure", "ws2_32-afunix format=1 arch=%s", sizeof(void *) == 8 ? "x86_64" : "i686");
    out("windows", "%lu.%lu.%lu", ver.dwMajorVersion, ver.dwMinorVersion, ver.dwBuildNumber);

    ret = WSAStartup(MAKEWORD(2, 2), &wsa);
    out("wsastartup", "ret=%d", ret);
    if (ret) return 2;

    GetTempPathA(sizeof(tmp), tmp);
    ret = snprintf(dir, sizeof(dir), "%safunix-measure-%lu", tmp, GetCurrentProcessId());
    if (ret < 0 || ret >= MAX_PATH - 32)
    {
        out("tempdir", "path too long");
        return 2;
    }
    if (dir[strlen(dir) - 1] == '\\') dir[strlen(dir) - 1] = 0;
    if (!CreateDirectoryA(dir, NULL))
    {
        out("tempdir", "failed err=%lu", GetLastError());
        return 2;
    }

    measure_protocols();
    measure_socket_kinds();
    measure_bind_connect();
    measure_bind_forms();
    measure_nonblocking();
    measure_extensions();

    ret = RemoveDirectoryA(dir);
    out("cleanup", "remove_dir=%d err=%lu", ret, ret ? 0 : GetLastError());
    WSACleanup();
    out("complete", "%d", !incomplete);
    return incomplete ? 2 : 0;
}
