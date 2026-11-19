/* Original, AI-assisted (Codex), LGPL-2.1-or-later. */
#define _WIN32_WINNT 0x0601
#include <windows.h>
#include <intrin.h>
#include <tlhelp32.h>
#include <stdio.h>
#include <string.h>

#define FAIL_FAST_STATUS 0xc0000409u
#define CLR_EXCEPTION_STATUS 0xe0434352u
#define AUTOCAD_TIMEOUT_MS 120000u
#define EXCEPTION_HISTORY_SIZE 128u

struct exception_event
{
    DWORD thread, code;
    BOOL has_parameter;
    ULONG_PTR parameter;
};

static DWORD autocad_pid(void)
{
    PROCESSENTRY32W entry = {0};
    DWORD pid = 0;
    unsigned int matches = 0;
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

    if (snapshot == INVALID_HANDLE_VALUE) return 0;
    entry.dwSize = sizeof(entry);
    if (Process32FirstW(snapshot, &entry))
    {
        do
        {
            if (!lstrcmpiW(entry.szExeFile, L"acad.exe"))
            {
                pid = entry.th32ProcessID;
                ++matches;
            }
        } while (Process32NextW(snapshot, &entry));
    }
    CloseHandle(snapshot);
    printf("acad_processes=%u\n", matches);
    return matches == 1 ? pid : 0;
}

static void report_preceding(const struct exception_event *history, unsigned int count, DWORD thread)
{
    const struct exception_event *selected[8];
    unsigned int available = min(count, EXCEPTION_HISTORY_SIZE);
    unsigned int i, size = 0;

    for (i = 0; i < available && size < sizeof(selected) / sizeof(*selected); ++i)
    {
        const struct exception_event *entry = &history[(count - i - 1) % EXCEPTION_HISTORY_SIZE];

        if (entry->thread == thread) selected[size++] = entry;
    }
    printf("same_thread_exceptions=%u\n", size);
    while (size)
    {
        const struct exception_event *entry = selected[--size];

        printf("preceding_exception=0x%08lx", (unsigned long)entry->code);
        if (entry->code == CLR_EXCEPTION_STATUS && entry->has_parameter)
            printf(" parameter0=0x%llx", (unsigned long long)entry->parameter);
        putchar('\n');
    }
}

static int observe(DWORD pid, DWORD timeout, BOOL self_test)
{
    ULONGLONG end = GetTickCount64() + timeout, now;
    struct exception_event preceding[EXCEPTION_HISTORY_SIZE] = {0};
    unsigned int count = 0;
    BOOL initial_breakpoint = TRUE, saw_termination = FALSE, saw_test_reason = FALSE;

    while ((now = GetTickCount64()) < end)
    {
        DEBUG_EVENT event;
        DWORD disposition = DBG_CONTINUE;

        if (!WaitForDebugEvent(&event, (DWORD)(end - now)))
        {
            printf("observer_wait_error=%lu\n", (unsigned long)GetLastError());
            break;
        }

        switch (event.dwDebugEventCode)
        {
            case CREATE_PROCESS_DEBUG_EVENT:
                if (event.u.CreateProcessInfo.hFile) CloseHandle(event.u.CreateProcessInfo.hFile);
                break;
            case LOAD_DLL_DEBUG_EVENT:
                if (event.u.LoadDll.hFile) CloseHandle(event.u.LoadDll.hFile);
                break;
            case EXIT_THREAD_DEBUG_EVENT:
            {
                unsigned int i;

                for (i = 0; i < EXCEPTION_HISTORY_SIZE; ++i)
                    if (preceding[i].thread == event.dwThreadId) preceding[i].thread = 0;
                break;
            }
            case EXCEPTION_DEBUG_EVENT:
            {
                const EXCEPTION_RECORD *rec = &event.u.Exception.ExceptionRecord;
                DWORD code = rec->ExceptionCode;
                BOOL first_chance = event.u.Exception.dwFirstChance;

                if (initial_breakpoint && first_chance && code == EXCEPTION_BREAKPOINT)
                {
                    initial_breakpoint = FALSE;
                    if (!self_test) puts("observer_ready=1");
                    break;
                }
                disposition = DBG_EXCEPTION_NOT_HANDLED;
                if (first_chance)
                {
                    struct exception_event *entry = &preceding[count++ % EXCEPTION_HISTORY_SIZE];

                    entry->thread = event.dwThreadId;
                    entry->code = code;
                    entry->has_parameter = rec->NumberParameters != 0;
                    entry->parameter = entry->has_parameter ? rec->ExceptionInformation[0] : 0;
                }
                else
                {
                    saw_termination = TRUE;
                    report_preceding(preceding, count, event.dwThreadId);
                    printf("last_chance=0x%08lx flags=0x%lx tid=0x%lx address=0x%llx parameters=%lu\n",
                           (unsigned long)code, (unsigned long)rec->ExceptionFlags,
                           (unsigned long)event.dwThreadId,
                           (unsigned long long)(ULONG_PTR)rec->ExceptionAddress,
                           (unsigned long)rec->NumberParameters);
                    if (code == FAIL_FAST_STATUS && rec->NumberParameters)
                    {
                        unsigned long long reason = rec->ExceptionInformation[0];

                        printf("fast_fail_reason=%llu\n", reason);
                        saw_test_reason = reason == FAST_FAIL_FATAL_APP_EXIT;
                    }
                }
                break;
            }
            default:
                break;
        }

        if (!ContinueDebugEvent(event.dwProcessId, event.dwThreadId, disposition))
        {
            printf("observer_continue_error=%lu\n", (unsigned long)GetLastError());
            break;
        }
        if (event.dwDebugEventCode == EXIT_PROCESS_DEBUG_EVENT)
        {
            printf("target_exit=0x%08lx\n", (unsigned long)event.u.ExitProcess.dwExitCode);
            return !(self_test ? saw_test_reason && event.u.ExitProcess.dwExitCode == FAIL_FAST_STATUS
                               : saw_termination);
        }
    }

    if (!DebugActiveProcessStop(pid))
        printf("observer_detach_error=%lu\n", (unsigned long)GetLastError());
    return 1;
}

static int self_test(void)
{
    STARTUPINFOW startup = {0};
    PROCESS_INFORMATION process;
    WCHAR executable[MAX_PATH], command[] = L"observer --self-test-child";
    DWORD length = GetModuleFileNameW(NULL, executable, MAX_PATH);
    int result;

    if (!length || length >= MAX_PATH) return 2;
    startup.cb = sizeof(startup);
    if (!CreateProcessW(executable, command, NULL, NULL, FALSE, DEBUG_ONLY_THIS_PROCESS,
                        NULL, NULL, &startup, &process))
    {
        printf("self_test_start_error=%lu\n", (unsigned long)GetLastError());
        return 2;
    }
    CloseHandle(process.hThread);
    CloseHandle(process.hProcess);
    result = observe(process.dwProcessId, 15000, TRUE);
    printf("self_test=%s\n", result ? "failed" : "ok");
    return result;
}

int main(int argc, char **argv)
{
    DWORD pid;

    setvbuf(stdout, NULL, _IONBF, 0);
    if (argc == 1) return self_test();
    if (argc == 2 && !strcmp(argv[1], "--self-test-child"))
    {
        /* Deliberately terminate only the synthetic child. */
        __fastfail(FAST_FAIL_FATAL_APP_EXIT);
    }
    if (argc != 2 || strcmp(argv[1], "--autocad")) return 2;

    if (!(pid = autocad_pid())) return 2;
    if (!DebugActiveProcess(pid))
    {
        printf("observer_attach_error=%lu\n", (unsigned long)GetLastError());
        return 2;
    }
    if (!DebugSetProcessKillOnExit(FALSE))
    {
        printf("observer_setup_error=%lu\n", (unsigned long)GetLastError());
        DebugActiveProcessStop(pid);
        return 2;
    }
    return observe(pid, AUTOCAD_TIMEOUT_MS, FALSE);
}
