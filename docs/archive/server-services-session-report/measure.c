/* Original, AI-assisted (Codex), LGPL-2.1-or-later. */
#include <windows.h>
#include <winternl.h>
#include <stdio.h>

int main(void)
{
    NTSTATUS (NTAPI *query)(SYSTEM_INFORMATION_CLASS, void *, ULONG, ULONG *);
    const DWORD own_pid = GetCurrentProcessId();
    const DWORD mismatch = 0xc0000004;
    SYSTEM_PROCESS_INFORMATION *process;
    ULONG size = 0;
    void *buffer;
    NTSTATUS status = (NTSTATUS)mismatch;
    unsigned int attempt;
    BOOL found_manager = FALSE, found_self = FALSE;

    query = (void *)GetProcAddress(GetModuleHandleW(L"ntdll.dll"), "NtQuerySystemInformation");
    if (!query) return 1;

    for (attempt = 0; attempt < 6; ++attempt)
    {
        ULONG needed = 0;

        size += 65536;
        buffer = HeapAlloc(GetProcessHeap(), 0, size);
        if (!buffer) return 2;
        status = query(SystemProcessInformation, buffer, size, &needed);
        if (!status) break;
        HeapFree(GetProcessHeap(), 0, buffer);
        if ((DWORD)status != mismatch) break;
        if (needed > size) size = needed;
    }
    if (status)
    {
        printf("query_status=0x%08lx\n", (unsigned long)(DWORD)status);
        return 3;
    }

    printf("probe=services-session pointer_bytes=%u\n", (unsigned int)sizeof(void *));
    process = buffer;
    for (;;)
    {
        const WCHAR manager_name[] = L"services.exe";
        const DWORD pid = (DWORD)(ULONG_PTR)process->UniqueProcessId;
        const BOOL manager = process->ImageName.Length == sizeof(manager_name) - sizeof(WCHAR) &&
                             !wcsnicmp(process->ImageName.Buffer, manager_name, ARRAYSIZE(manager_name) - 1);

        if (manager || pid == own_pid)
        {
            DWORD direct;

            printf("process=%s list_session=%lu", manager ? "services.exe" : "probe",
                   (unsigned long)process->SessionId);
            if (ProcessIdToSessionId(pid, &direct)) printf(" direct_session=%lu", (unsigned long)direct);
            printf("\n");
            found_manager |= manager;
            found_self |= pid == own_pid;
        }
        if (!process->NextEntryOffset) break;
        process = (SYSTEM_PROCESS_INFORMATION *)((char *)process + process->NextEntryOffset);
    }
    HeapFree(GetProcessHeap(), 0, buffer);
    return !(found_manager && found_self);
}
