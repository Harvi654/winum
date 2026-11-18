/*
 * AF_UNIX v4 measurements for the offline Windows VM.
 * Original AI-assisted (Codex), LGPL-2.1-or-later.
 */
#define _WIN32_WINNT 0x0601
#include <winsock2.h>
#include <windows.h>
#include <mswsock.h>
#include <winioctl.h>
#include <winternl.h>
#include <aclapi.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <wchar.h>

#ifndef AF_UNIX
#define AF_UNIX 1
#endif

struct unix_address
{
    ADDRESS_FAMILY family;
    char path[108];
};

struct file_stat_information
{
    LARGE_INTEGER id, creation, access, write, change, allocation, size;
    ULONG attributes, tag, links, effective_access;
};

struct probe_reparse_data
{
    DWORD tag;
    WORD size, reserved;
    GUID guid;
    BYTE payload[4];
};

static const struct probe_reparse_data probe_tag =
{
    0x42, 4, 0, {0x5566d97b, 0x6554, 0x4f67, {0xa9, 0xb5, 0xa2, 0x11, 0x47, 0x7a, 0xee, 0xa8}}, {1, 2, 3, 4}
};

static int make_address(struct unix_address *addr, const char *path)
{
    size_t len = strlen(path) + 1;

    memset(addr, 0, sizeof(*addr));
    addr->family = AF_UNIX;
    memcpy(addr->path, path, len);
    return (int)(offsetof(struct unix_address, path) + len);
}

static SOCKET make_socket(const char *label)
{
    SOCKET sock = WSASocketW(AF_UNIX, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);

    if (sock == INVALID_SOCKET)
        printf("%s socket_error=%d\n", label, WSAGetLastError());
    return sock;
}

static void print_address(const char *label, int ret, const struct unix_address *addr, int len)
{
    const char *path = "other";

    if (ret)
    {
        printf("%s ret=%d err=%d\n", label, ret, WSAGetLastError());
        return;
    }
    if (len <= (int)offsetof(struct unix_address, path) || !addr->path[0])
        path = "empty";
    else if (!strncmp(addr->path, "listener", sizeof(addr->path)))
        path = "listener";
    else if (!strncmp(addr->path, "bound-client", sizeof(addr->path)))
        path = "bound-client";
    printf("%s ret=0 len=%d family=%u path=%s\n", label, len, addr->family, path);
}

static void print_names(const char *left, const char *right, SOCKET sock)
{
    struct unix_address addr;
    int ret, len = sizeof(addr);

    memset(&addr, 0, sizeof(addr));
    ret = getsockname(sock, (struct sockaddr *)&addr, &len);
    print_address(left, ret, &addr, len);
    memset(&addr, 0, sizeof(addr));
    len = sizeof(addr);
    ret = getpeername(sock, (struct sockaddr *)&addr, &len);
    print_address(right, ret, &addr, len);
}

static void measure_handle_tags(void)
{
    typedef NTSTATUS (WINAPI *query_info_fn)(HANDLE, PIO_STATUS_BLOCK, PVOID, ULONG, FILE_INFORMATION_CLASS);
    query_info_fn query = (query_info_fn)(void (*)(void))GetProcAddress(
        GetModuleHandleW(L"ntdll.dll"), "NtQueryInformationFile");
    struct file_stat_information stat_info = {0};
    FILE_ATTRIBUTE_TAG_INFO tag_info = {0};
    IO_STATUS_BLOCK io = {0};
    HANDLE file;
    NTSTATUS status;
    BOOL ret;

    file = CreateFileW(L"listener", FILE_READ_ATTRIBUTES, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                       NULL, OPEN_EXISTING, FILE_FLAG_OPEN_REPARSE_POINT | FILE_FLAG_BACKUP_SEMANTICS, NULL);
    if (file == INVALID_HANDLE_VALUE)
    {
        printf("socket_handle open_error=%lu\n", GetLastError());
        return;
    }
    ret = GetFileInformationByHandleEx(file, FileAttributeTagInfo, &tag_info, sizeof(tag_info));
    printf("socket_handle FileAttributeTagInfo ret=%d err=%lu attributes=%#lx tag=%#lx\n",
           ret, ret ? 0 : GetLastError(), tag_info.FileAttributes, tag_info.ReparseTag);
    if (query)
    {
        /* FileStatInformation, as declared by the NT file information API. */
        status = query(file, &io, &stat_info, sizeof(stat_info), (FILE_INFORMATION_CLASS)68);
        printf("socket_handle FileStatInformation status=%#lx attributes=%#lx tag=%#lx\n",
               (unsigned long)status, stat_info.attributes, stat_info.tag);
    }
    CloseHandle(file);
}

static void measure_short_lengths(void)
{
    struct unix_address addr;
    int len, ret;
    SOCKET sock;

    make_address(&addr, "short-address");
    for (len = 0; len < (int)offsetof(struct unix_address, path); ++len)
    {
        sock = make_socket("short_bind");
        if (sock == INVALID_SOCKET) return;
        ret = bind(sock, (struct sockaddr *)&addr, len);
        printf("short_bind len=%d ret=%d err=%d\n", len, ret, ret ? WSAGetLastError() : 0);
        closesocket(sock);

        sock = make_socket("short_connect");
        if (sock == INVALID_SOCKET) return;
        ret = connect(sock, (struct sockaddr *)&addr, len);
        printf("short_connect len=%d ret=%d err=%d\n", len, ret, ret ? WSAGetLastError() : 0);
        closesocket(sock);
    }
}

static void measure_explicit_bind(SOCKET listener, const struct unix_address *target,
                                  int target_len, BOOL extended)
{
    static const GUID connectex_id = WSAID_CONNECTEX;
    struct unix_address addr;
    SOCKET client = make_socket("bound_client"), accepted;
    HANDLE event = NULL;
    DWORD count = 0, flags = 0, wait;
    OVERLAPPED overlap = {0};
    LPFN_CONNECTEX connectex = NULL;
    int len, ret, err;
    BOOL connected;
    fd_set readable;
    struct timeval timeout = {2, 0};

    if (client == INVALID_SOCKET)
        return;
    len = make_address(&addr, "bound-client");
    ret = bind(client, (struct sockaddr *)&addr, len);
    printf("client_bind extended=%d ret=%d err=%d\n", extended, ret, ret ? WSAGetLastError() : 0);
    if (ret) goto done;
    if (extended)
    {
        ret = WSAIoctl(client, SIO_GET_EXTENSION_FUNCTION_POINTER, (void *)&connectex_id,
                       sizeof(connectex_id), &connectex, sizeof(connectex), &count, NULL, NULL);
        printf("bound_ConnectEx pointer ret=%d err=%d available=%d\n",
               ret, ret ? WSAGetLastError() : 0, connectex != NULL);
        if (ret || !connectex) goto done;
        event = CreateEventW(NULL, TRUE, FALSE, NULL);
        if (!event) goto done;
        overlap.hEvent = event;
        connected = connectex(client, (const struct sockaddr *)target, target_len, NULL, 0, &count, &overlap);
        err = connected ? 0 : WSAGetLastError();
        printf("bound_ConnectEx ret=%d err=%d\n", connected, err);
        if (!connected && err == WSA_IO_PENDING)
        {
            wait = WaitForSingleObject(event, 2000);
            printf("bound_ConnectEx wait=%lu\n", wait);
            if (wait == WAIT_OBJECT_0)
            {
                connected = WSAGetOverlappedResult(client, &overlap, &count, FALSE, &flags);
                printf("bound_ConnectEx completion=%d err=%d bytes=%lu\n",
                       connected, connected ? 0 : WSAGetLastError(), count);
            }
            else
            {
                CancelIoEx((HANDLE)client, &overlap);
                if (WaitForSingleObject(event, 2000) != WAIT_OBJECT_0)
                {
                    puts("bound_ConnectEx did not finish after cancellation");
                    fflush(stdout);
                    ExitProcess(2);
                }
            }
        }
        if (connected)
        {
            ret = setsockopt(client, SOL_SOCKET, SO_UPDATE_CONNECT_CONTEXT, NULL, 0);
            printf("bound_ConnectEx update_context ret=%d err=%d\n", ret, ret ? WSAGetLastError() : 0);
        }
    }
    else
    {
        ret = connect(client, (const struct sockaddr *)target, target_len);
        printf("bound_connect ret=%d err=%d\n", ret, ret ? WSAGetLastError() : 0);
        connected = !ret;
    }
    if (!connected) goto done;

    print_names("bound_client getsockname", "bound_client getpeername", client);
    FD_ZERO(&readable);
    FD_SET(listener, &readable);
    ret = select(0, &readable, NULL, NULL, &timeout);
    if (ret != 1)
    {
        printf("bound_accept select=%d err=%d\n", ret, ret < 0 ? WSAGetLastError() : 0);
        goto done;
    }
    memset(&addr, 0, sizeof(addr));
    len = sizeof(addr);
    accepted = accept(listener, (struct sockaddr *)&addr, &len);
    print_address("bound_accept", accepted == INVALID_SOCKET ? SOCKET_ERROR : 0, &addr, len);
    if (accepted != INVALID_SOCKET)
    {
        print_names("bound_server getsockname", "bound_server getpeername", accepted);
        closesocket(accepted);
    }

done:
    closesocket(client);
    if (event) CloseHandle(event);
    DeleteFileW(L"bound-client");
}

static void measure_other_reparse_tag(void)
{
    const WCHAR name[] = L"synthetic-reparse";
    struct probe_reparse_data data = probe_tag;
    HANDLE file, ordinary;
    PSECURITY_DESCRIPTOR direct_sd = NULL, named_sd = NULL;
    PACL direct_acl = NULL, named_acl = NULL;
    DWORD size, err;
    BOOL ret;

    file = CreateFileW(name, GENERIC_WRITE | READ_CONTROL | WRITE_DAC,
                       FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                       NULL, CREATE_NEW, FILE_FLAG_OPEN_REPARSE_POINT, NULL);
    if (file == INVALID_HANDLE_VALUE)
    {
        printf("synthetic_reparse create_error=%lu\n", GetLastError());
        return;
    }
    ret = DeviceIoControl(file, FSCTL_SET_REPARSE_POINT, &data,
                          offsetof(struct probe_reparse_data, payload) + data.size, NULL, 0, &size, NULL);
    printf("synthetic_reparse set_tag=%d err=%lu\n", ret, ret ? 0 : GetLastError());
    if (ret)
    {
        ordinary = CreateFileW(name, READ_CONTROL, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                               NULL, OPEN_EXISTING, 0, NULL);
        printf("synthetic_reparse ordinary_open=%d err=%lu\n",
               ordinary != INVALID_HANDLE_VALUE, ordinary == INVALID_HANDLE_VALUE ? GetLastError() : 0);
        if (ordinary != INVALID_HANDLE_VALUE) CloseHandle(ordinary);
        err = GetSecurityInfo(file, SE_FILE_OBJECT, DACL_SECURITY_INFORMATION,
                              NULL, NULL, &direct_acl, NULL, &direct_sd);
        printf("synthetic_reparse GetSecurityInfo=%lu\n", err);
        printf("synthetic_reparse GetNamedSecurityInfo=%lu\n",
               GetNamedSecurityInfoW((LPWSTR)name, SE_FILE_OBJECT, DACL_SECURITY_INFORMATION,
                                     NULL, NULL, &named_acl, NULL, &named_sd));
        if (!err && direct_acl)
            printf("synthetic_reparse SetNamedSecurityInfo_same_dacl=%lu\n",
                   SetNamedSecurityInfoW((LPWSTR)name, SE_FILE_OBJECT, DACL_SECURITY_INFORMATION,
                                         NULL, NULL, direct_acl, NULL));
        data.size = 0;
        ret = DeviceIoControl(file, FSCTL_DELETE_REPARSE_POINT, &data,
                              offsetof(struct probe_reparse_data, payload), NULL, 0, &size, NULL);
        printf("synthetic_reparse remove_tag=%d err=%lu\n", ret, ret ? 0 : GetLastError());
    }
    LocalFree(named_sd);
    LocalFree(direct_sd);
    CloseHandle(file);
    DeleteFileW(name);
}

int wmain(int argc, WCHAR **argv)
{
    WCHAR previous[MAX_PATH];
    struct unix_address addr;
    WSADATA wsa;
    SOCKET listener;
    DWORD len;
    int address_len, ret, exit_code = 1;

    printf("arch=%u\n", (unsigned int)(8 * sizeof(void *)));
    len = GetCurrentDirectoryW(ARRAYSIZE(previous), previous);
    if (argc != 2 || !len || len >= ARRAYSIZE(previous) || !SetCurrentDirectoryW(argv[1]))
    {
        puts("Could not use the new sample directory.");
        return 2;
    }
    measure_other_reparse_tag();

    ret = WSAStartup(MAKEWORD(2, 2), &wsa);
    printf("WSAStartup ret=%d\n", ret);
    if (ret) goto restore;

    measure_short_lengths();
    listener = make_socket("listener");
    if (listener != INVALID_SOCKET)
    {
        address_len = make_address(&addr, "listener");
        ret = bind(listener, (struct sockaddr *)&addr, address_len);
        printf("listener_bind ret=%d err=%d\n", ret, ret ? WSAGetLastError() : 0);
        if (!ret)
        {
            measure_handle_tags();
            ret = listen(listener, 4);
            printf("listener_listen ret=%d err=%d\n", ret, ret ? WSAGetLastError() : 0);
            if (!ret)
            {
                measure_explicit_bind(listener, &addr, address_len, FALSE);
                measure_explicit_bind(listener, &addr, address_len, TRUE);
                exit_code = 0;
            }
        }
        closesocket(listener);
        DeleteFileW(L"listener");
    }
    WSACleanup();

restore:
    SetCurrentDirectoryW(previous);
    return exit_code;
}
