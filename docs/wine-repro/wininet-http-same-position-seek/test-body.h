/* Original, AI-assisted (Codex), LGPL-2.1-or-later.
 * Same generated bodies, framing, flags and API calls as our Windows
 * MEASURE. Only the private URL suffix adds a per-run cache namespace. */
#define SAMEPOS_BODY_SIZE 16384
struct samepos_server { SOCKET listener; HANDLE stop; LONG requests, faults; DWORD started; };
static BYTE samepos_body[SAMEPOS_BODY_SIZE];
static unsigned int samepos_completed;
static LONG samepos_total_requests;

static BOOL samepos_budget(const struct samepos_server *server)
{
    return GetTickCount() - server->started < 45000;
}

static BOOL samepos_send(SOCKET client, const void *data, int length)
{
    const char *p = data;
    while(length)
    {
        int n = send(client, p, length, 0);
        if(n <= 0) return FALSE;
        p += n;
        length -= n;
    }
    return TRUE;
}

static BOOL samepos_respond(SOCKET client, const char *request)
{
    char header[256];
    unsigned int offset;
    int length;
    if(!strncmp(request, "GET /chunk/", 11))
    {
        static const char chunk[] = "HTTP/1.1 200 OK\r\nContent-Type: application/octet-stream\r\nTransfer-Encoding: chunked\r\nConnection: close\r\n\r\n";
        if(!samepos_send(client, chunk, sizeof(chunk) - 1)) return FALSE;
        for(offset = 0; offset < SAMEPOS_BODY_SIZE; offset += 4096)
            if(!samepos_send(client, "1000\r\n", 6) || !samepos_send(client, samepos_body + offset, 4096)
               || !samepos_send(client, "\r\n", 2)) return FALSE;
        return samepos_send(client, "0\r\n\r\n", 5);
    }
    if(!strncmp(request, "GET /close/", 11))
    {
        static const char close_header[] = "HTTP/1.0 200 OK\r\nContent-Type: application/octet-stream\r\nConnection: close\r\n\r\n";
        return samepos_send(client, close_header, sizeof(close_header) - 1)
               && samepos_send(client, samepos_body, SAMEPOS_BODY_SIZE);
    }
    length = snprintf(header, sizeof(header), "HTTP/1.1 200 OK\r\nContent-Type: application/octet-stream\r\nContent-Length: %u\r\nConnection: close\r\n\r\n", SAMEPOS_BODY_SIZE);
    return length > 0 && (size_t)length < sizeof(header) && samepos_send(client, header, length)
           && samepos_send(client, samepos_body, SAMEPOS_BODY_SIZE);
}

static DWORD WINAPI samepos_serve(void *parameter)
{
    struct samepos_server *server = parameter;
    while(WaitForSingleObject(server->stop, 0) != WAIT_OBJECT_0 && samepos_budget(server))
    {
        fd_set set;
        struct timeval timeout = {0, 100000};
        SOCKET client;
        char request[8192];
        int length = 0, selected;
        DWORD socket_timeout = 2000;
        BOOL complete = FALSE;
        FD_ZERO(&set);
        FD_SET(server->listener, &set);
        selected = select(0, &set, NULL, NULL, &timeout);
        if(!selected) continue;
        if(selected == SOCKET_ERROR) { InterlockedIncrement(&server->faults); break; }
        client = accept(server->listener, NULL, NULL);
        if(client == INVALID_SOCKET) { InterlockedIncrement(&server->faults); continue; }
        if(setsockopt(client, SOL_SOCKET, SO_RCVTIMEO, (const char *)&socket_timeout, sizeof(socket_timeout))
           || setsockopt(client, SOL_SOCKET, SO_SNDTIMEO, (const char *)&socket_timeout, sizeof(socket_timeout)))
            InterlockedIncrement(&server->faults);
        else while(length < (int)sizeof(request) - 1 && samepos_budget(server))
        {
            int n = recv(client, request + length, sizeof(request) - 1 - length, 0);
            if(n <= 0) break;
            length += n;
            request[length] = 0;
            if(strstr(request, "\r\n\r\n"))
            {
                InterlockedIncrement(&server->requests);
                complete = samepos_respond(client, request);
                break;
            }
        }
        if(!complete) InterlockedIncrement(&server->faults);
        if(closesocket(client)) InterlockedIncrement(&server->faults);
    }
    return 0;
}

static void samepos_case(struct samepos_server *server, unsigned short port,
                         DWORD nonce, const char *framing, unsigned int mode)
{
    char url[160];
    HINTERNET session = NULL, request = NULL;
    BYTE data[997];
    DWORD count = 0, offset = 0, calls = 0, position, error, timeout = 4000;
    LONG before = InterlockedCompareExchange(&server->requests, 0, 0), requests;
    BOOL matches = TRUE, read_ok = TRUE, eof = FALSE;
    DWORD flags = INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_COOKIES | INTERNET_FLAG_NO_AUTH | INTERNET_FLAG_NO_UI
                  | (mode == 3 ? INTERNET_FLAG_NO_CACHE_WRITE : 0);

    if(!samepos_budget(server)) { ok(FALSE, "HTTP fixture time budget exhausted\n"); return; }
    snprintf(url, sizeof(url), "http://127.0.0.1:%u/%s/%u/%lu-%lu", port, framing, mode, GetCurrentProcessId(), nonce);
    session = InternetOpenA("Winum native fixture", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
    ok(session != NULL, "%s/%u: InternetOpen failed, error %lu\n", framing, mode, GetLastError());
    if(!session) goto done;
    ok(InternetSetOptionW(session, INTERNET_OPTION_CONNECT_TIMEOUT, &timeout, sizeof(timeout)), "Connect timeout setup failed\n");
    ok(InternetSetOptionW(session, INTERNET_OPTION_RECEIVE_TIMEOUT, &timeout, sizeof(timeout)), "Receive timeout setup failed\n");
    ok(InternetSetOptionW(session, INTERNET_OPTION_SEND_TIMEOUT, &timeout, sizeof(timeout)), "Send timeout setup failed\n");
    request = InternetOpenUrlA(session, url, NULL, 0, flags, 0);
    ok(request != NULL, "%s/%u: loopback request failed, error %lu\n", framing, mode, GetLastError());
    if(!request) goto done;
    while(offset < 17 && samepos_budget(server))
    {
        count = 0;
        if(!InternetReadFile(request, data, 17 - offset, &count) || !count || count > 17 - offset)
        {
            read_ok = FALSE;
            break;
        }
        if(memcmp(data, samepos_body + offset, count)) matches = FALSE;
        offset += count;
    }
    ok(read_ok && offset == 17, "%s/%u: incomplete prefix\n", framing, mode);
    ok(matches, "%s/%u: incorrect prefix\n", framing, mode);
    if(!read_ok || offset != 17 || !matches) goto done;
    if(mode)
    {
        SetLastError(0xdeadbeef);
        position = InternetSetFilePointer(request, mode == 2 ? 17 : 0, NULL, mode == 2 ? FILE_BEGIN : FILE_CURRENT, 0);
        error = GetLastError();
        if(!strcmp(framing, "length") && mode != 3)
        {
            /* Target contract: these must remain ordinary assertions. */
            ok(position == 17, "%s/%u: seek returned %lu\n", framing, mode, position);
            ok(!error, "%s/%u: seek error %lu\n", framing, mode, error);
        }
        else
        {
            /* Wine does not yet reject cached chunked/close-delimited seeks.
             * NO_CACHE_WRITE rejection already matches the measured result. */
            todo_wine_if(mode != 3)
            {
                ok(position == INVALID_SET_FILE_POINTER, "%s/%u: unsupported seek returned %lu\n", framing, mode, position);
                ok(error == ERROR_INTERNET_INVALID_OPERATION, "%s/%u: unsupported seek error %lu\n", framing, mode, error);
            }
        }
    }
    while(calls++ < 40 && samepos_budget(server))
    {
        count = 0;
        if(!InternetReadFile(request, data, sizeof(data), &count) || count > sizeof(data)) { read_ok = FALSE; break; }
        if(!count) { eof = TRUE; break; }
        if(offset > SAMEPOS_BODY_SIZE || count > SAMEPOS_BODY_SIZE - offset
           || memcmp(data, samepos_body + offset, count)) matches = FALSE;
        offset += count;
    }
    ok(read_ok, "%s/%u: body read failed\n", framing, mode);
    ok(eof, "%s/%u: no EOF within bound\n", framing, mode);
    ok(offset == SAMEPOS_BODY_SIZE, "%s/%u: body size %lu\n", framing, mode, offset);
    ok(matches, "%s/%u: body bytes changed across seek\n", framing, mode);
    ++samepos_completed;
done:
    if(request) ok(InternetCloseHandle(request), "%s/%u: request close failed\n", framing, mode);
    if(session) ok(InternetCloseHandle(session), "%s/%u: session close failed\n", framing, mode);
    DeleteUrlCacheEntryA(url); /* Best effort, only the unique own fixture URL. */
    requests = InterlockedCompareExchange(&server->requests, 0, 0) - before;
    ok(requests == 1, "%s/%u: %ld requests, expected one\n", framing, mode, requests);
}

static void test_same_position_seek(void)
{
    static const char *const framings[] = {"length", "chunk", "close"};
    WSADATA data;
    struct sockaddr_in address = {0};
    struct samepos_server server = {INVALID_SOCKET, NULL, 0, 0, GetTickCount()};
    HANDLE thread = NULL;
    DWORD nonce = GetTickCount();
    int size = sizeof(address), result;
    unsigned int i, mode;

    samepos_completed = 0;
    samepos_total_requests = 0;
    result = WSAStartup(MAKEWORD(2, 2), &data);
    ok(!result, "WSAStartup returned %d\n", result);
    if(result) return;
    for(i = 0; i < SAMEPOS_BODY_SIZE; ++i) samepos_body[i] = (BYTE)((i * 13 + (i >> 8) * 7 + 5) & 255);
    server.listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    ok(server.listener != INVALID_SOCKET, "Listener allocation failed\n");
    if(server.listener == INVALID_SOCKET) goto done;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    result = bind(server.listener, (const struct sockaddr *)&address, sizeof(address));
    ok(!result, "Loopback bind returned %d\n", result);
    if(result) goto done;
    result = listen(server.listener, 8);
    ok(!result, "Listen returned %d\n", result);
    if(result) goto done;
    result = getsockname(server.listener, (struct sockaddr *)&address, &size);
    ok(!result, "Listener address query returned %d\n", result);
    if(result) goto done;
    server.stop = CreateEventW(NULL, TRUE, FALSE, NULL);
    ok(server.stop != NULL, "Server stop event allocation failed\n");
    if(!server.stop) goto done;
    thread = CreateThread(NULL, 0, samepos_serve, &server, 0, NULL);
    ok(thread != NULL, "Server thread allocation failed\n");
    if(!thread) goto done;
    for(i = 0; i < ARRAY_SIZE(framings); ++i)
        for(mode = 0; mode < 4; ++mode) samepos_case(&server, ntohs(address.sin_port), nonce, framings[i], mode);
    ok(samepos_completed == 12, "Completed %u cases, expected 12\n", samepos_completed);
done:
    if(thread)
    {
        if(!SetEvent(server.stop) || WaitForSingleObject(thread, 6000) != WAIT_OBJECT_0)
        {
            ok(FALSE, "Server did not stop within bound\n");
            TerminateProcess(GetCurrentProcess(), 2);
            ExitProcess(2);
        }
        ok(CloseHandle(thread), "Server thread close failed\n");
    }
    samepos_total_requests = InterlockedCompareExchange(&server.requests, 0, 0);
    ok(!server.faults, "Server fixture faults %ld\n", server.faults);
    if(thread) ok(samepos_total_requests == 12, "Total request count %ld, expected 12\n", samepos_total_requests);
    if(server.stop) ok(CloseHandle(server.stop), "Server stop event close failed\n");
    if(server.listener != INVALID_SOCKET) ok(!closesocket(server.listener), "Listener close failed\n");
    ok(!WSACleanup(), "WSACleanup failed\n");
}
