/* Original AI-assisted (Codex), LGPL-2.1-or-later. */
#include <winsock2.h>
#include <mswsock.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#ifndef AF_UNIX
#define AF_UNIX 1
#endif

static int connect_overlapped(SOCKET sock, const struct sockaddr *target, int length)
{
    const GUID connect_ex_id = WSAID_CONNECTEX;
    const struct sockaddr local = {.sa_family = AF_UNIX};
    LPFN_CONNECTEX connect_ex;
    OVERLAPPED overlapped = {0};
    DWORD count, flags = 0, wait;
    int err;

    if (bind(sock, &local, sizeof(local.sa_family)) ||
        WSAIoctl(sock, SIO_GET_EXTENSION_FUNCTION_POINTER, (void *)&connect_ex_id, sizeof(connect_ex_id),
                 &connect_ex, sizeof(connect_ex), &count, NULL, NULL))
    {
        printf("connectex_setup=%d\n", WSAGetLastError());
        return 1;
    }
    overlapped.hEvent = CreateEventW(NULL, TRUE, FALSE, NULL);
    if (!overlapped.hEvent) return 1;

    if (connect_ex(sock, target, length, NULL, 0, &count, &overlapped))
    {
        puts("connectex did not return pending");
        CloseHandle(overlapped.hEvent);
        return 1;
    }
    err = WSAGetLastError();
    if (err != WSA_IO_PENDING)
    {
        printf("connectex=%d\n", err);
        CloseHandle(overlapped.hEvent);
        return 1;
    }
    wait = WaitForSingleObject(overlapped.hEvent, 3000);
    if (wait != WAIT_OBJECT_0)
    {
        printf("connectex_wait=%lu\n", wait);
        fflush(stdout);
        ExitProcess(1);
    }
    err = !WSAGetOverlappedResult(sock, &overlapped, &count, FALSE, &flags) || count ||
          setsockopt(sock, SOL_SOCKET, SO_UPDATE_CONNECT_CONTEXT, NULL, 0);
    if (err) printf("connectex_completion=%d\n", WSAGetLastError());
    CloseHandle(overlapped.hEvent);
    return err;
}

int main(int argc, char **argv)
{
    struct
    {
        ADDRESS_FAMILY family;
        char path[108];
    } addr = {0};
    static const char prefix[] = "\\\\?\\unix";
    WSADATA wsa;
    SOCKET sock;
    size_t len;
    DWORD timeout = 3000;
    char reply[4];
    BOOL extended = argc == 3 && !strcmp(argv[2], "--connectex");
    int i, offset, ret, err = 0;

    if ((argc != 2 && !extended) || argv[1][0] != '/' ||
        strlen(argv[1]) + sizeof(prefix) > sizeof(addr.path))
    {
        fputs("Usage: host-peer.exe <absolute Linux socket path> [--connectex]\n", stderr);
        return 2;
    }
    addr.family = AF_UNIX;
    strcpy(addr.path, prefix);
    strcat(addr.path, argv[1]);
    len = strlen(addr.path);
    for (i = (int)sizeof(prefix) - 1; i < (int)len; ++i)
        if (addr.path[i] == '/') addr.path[i] = '\\';

    ret = WSAStartup(MAKEWORD(2, 2), &wsa);
    if (ret)
    {
        printf("WSAStartup=%d\n", ret);
        return 1;
    }

    sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET)
    {
        printf("socket=%d\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char *)&timeout, sizeof(timeout));
    if (extended)
        ret = connect_overlapped(sock, (const struct sockaddr *)&addr, (int)(sizeof(addr.family) + len + 1));
    else
        ret = connect(sock, (const struct sockaddr *)&addr, (int)(sizeof(addr.family) + len + 1));
    if (ret)
    {
        printf("connect=%d\n", WSAGetLastError());
        err = 1;
    }
    else if (!extended && send(sock, "PING", 4, 0) != 4)
    {
        printf("send incomplete\n");
        err = 1;
    }
    else
    {
        for (offset = 0; offset < (int)sizeof(reply); offset += ret)
        {
            ret = recv(sock, reply + offset, (int)sizeof(reply) - offset, 0);
            if (ret <= 0) break;
        }
        err = offset != (int)sizeof(reply) || memcmp(reply, "PONG", sizeof(reply));
        if (extended)
        {
            int peer_len = sizeof(addr);
            char byte;

            ret = recv(sock, &byte, 1, 0);
            if (ret)
            {
                printf("recv_after_close=%d error=%d\n", ret, ret < 0 ? WSAGetLastError() : 0);
                err = 1;
            }

            ret = getpeername(sock, (struct sockaddr *)&addr, &peer_len);
            if (ret || addr.family != AF_UNIX || peer_len != sizeof(addr))
            {
                printf("peer_after_close=%d error=%d length=%d\n", ret, ret ? WSAGetLastError() : 0, peer_len);
                err = 1;
            }
        }
        printf("%s=%s\n", extended ? "host_connectex_peer_closed" : "host_exchange", err ? "failed" : "ok");
    }
    closesocket(sock);
    WSACleanup();
    return err;
}
