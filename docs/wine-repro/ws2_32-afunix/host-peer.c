/* Original AI-assisted (Codex), LGPL-2.1-or-later. */
#include <winsock2.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#ifndef AF_UNIX
#define AF_UNIX 1
#endif

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
    int i, offset, ret, err = 0;

    if (argc != 2 || argv[1][0] != '/' ||
        strlen(argv[1]) + sizeof(prefix) > sizeof(addr.path))
    {
        fputs("Usage: host-peer.exe <absolute Linux socket path>\n", stderr);
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
    ret = connect(sock, (const struct sockaddr *)&addr, (int)(sizeof(addr.family) + len + 1));
    if (ret)
    {
        printf("connect=%d\n", WSAGetLastError());
        err = 1;
    }
    else if (send(sock, "PING", 4, 0) != 4)
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
        printf("host_exchange=%s\n", err ? "failed" : "ok");
    }
    closesocket(sock);
    WSACleanup();
    return err;
}
