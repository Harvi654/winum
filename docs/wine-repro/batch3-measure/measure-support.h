/* Original, AI-assisted (Codex), LGPL-2.1-or-later.
 * Shared process bounds and own temporary-file helpers; no native oracle. */
#ifndef WINUM_BATCH3_MEASURE_SUPPORT_H
#define WINUM_BATCH3_MEASURE_SUPPORT_H
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0601
#endif
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <wchar.h>
static LONG b3_errors;
static HANDLE b3_cancel, b3_watchdog;
static const char *b3_topic;
static DWORD b3_timeout;
static inline void b3_error(const char *operation, DWORD error)
{
    InterlockedIncrement(&b3_errors);
    printf("ERROR operation=%s code=%08lx\n", operation, error);
}
static inline BOOL b3_hr(HRESULT hr, const char *operation)
{
    printf("API operation=%s hr=%08lx\n", operation, (DWORD)hr);
    if(FAILED(hr)) b3_error(operation, (DWORD)hr);
    return SUCCEEDED(hr);
}
static DWORD WINAPI b3_watch(void *event)
{
    if(WaitForSingleObject(event, b3_timeout) == WAIT_OBJECT_0) return 0;
    TerminateProcess(GetCurrentProcess(), 2);
    ExitProcess(2);
}
static inline BOOL b3_start(const char *topic, DWORD timeout)
{
    b3_topic = topic;
    b3_timeout = timeout;
    setvbuf(stdout, NULL, _IONBF, 0);
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
    printf("MEASURE topic=%s schema=1 bits=%u\n", topic, (unsigned int)(8 * sizeof(void *)));
    b3_cancel = CreateEventW(NULL, TRUE, FALSE, NULL);
    if(!b3_cancel) { b3_error("CreateEventW", GetLastError()); return FALSE; }
    b3_watchdog = CreateThread(NULL, 0, b3_watch, b3_cancel, 0, NULL);
    if(!b3_watchdog) {
        b3_error("CreateThread_watchdog", GetLastError());
        CloseHandle(b3_cancel);
        return FALSE;
    }
    return TRUE;
}
static inline int b3_finish(void)
{
    if(!SetEvent(b3_cancel)) { TerminateProcess(GetCurrentProcess(), 2); return 2; }
    if(WaitForSingleObject(b3_watchdog, INFINITE) != WAIT_OBJECT_0) b3_error("JoinWatchdog", GetLastError());
    if(!CloseHandle(b3_watchdog)) b3_error("CloseWatchdog", GetLastError());
    if(!CloseHandle(b3_cancel)) b3_error("CloseCancel", GetLastError());
    printf("SUMMARY topic=%s api_errors=%ld complete=1 expectations=none\n", b3_topic, b3_errors);
    return b3_errors ? 1 : 0;
}
static inline BOOL b3_tempdir(WCHAR *directory)
{
    WCHAR parent[MAX_PATH];
    DWORD n = GetTempPathW(MAX_PATH, parent);
    if(!n || n >= MAX_PATH) { b3_error("GetTempPathW", GetLastError()); return FALSE; }
    if(!GetTempFileNameW(parent, L"w3m", 0, directory)) {
        b3_error("GetTempFileNameW", GetLastError()); return FALSE;
    }
    if(!DeleteFileW(directory) || !CreateDirectoryW(directory, NULL)) {
        b3_error("CreateOwnDirectory", GetLastError()); return FALSE;
    }
    return TRUE;
}
static inline BOOL b3_path(WCHAR *path, const WCHAR *directory, const WCHAR *leaf)
{
    int n = swprintf(path, MAX_PATH, L"%ls\\%ls", directory, leaf);
    if(n < 0 || n >= MAX_PATH) { b3_error("OwnPathLength", ERROR_BUFFER_OVERFLOW); return FALSE; }
    return TRUE;
}
static inline BOOL b3_write_file(const WCHAR *path, const void *data, DWORD size)
{
    HANDLE file = CreateFileW(path, GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
    DWORD written = 0;
    BOOL ok;
    if(file == INVALID_HANDLE_VALUE) { b3_error("CreateOwnFile", GetLastError()); return FALSE; }
    ok = WriteFile(file, data, size, &written, NULL);
    if(!ok || written != size) b3_error("WriteOwnFile", ok ? ERROR_WRITE_FAULT : GetLastError());
    if(!CloseHandle(file)) { b3_error("CloseOwnFile", GetLastError()); return FALSE; }
    return ok && written == size;
}
static inline void b3_delete_file(const WCHAR *path)
{
    if(!DeleteFileW(path) && GetLastError() != ERROR_FILE_NOT_FOUND) b3_error("DeleteOwnFile", GetLastError());
}
static inline void b3_remove_dir(const WCHAR *path)
{
    if(!RemoveDirectoryW(path)) b3_error("RemoveOwnDirectory", GetLastError());
}
#endif
