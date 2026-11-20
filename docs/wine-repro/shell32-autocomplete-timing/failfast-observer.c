/* Original, AI-assisted (Codex), LGPL-2.1-or-later. */
#define _WIN32_WINNT 0x0601
#include <windows.h>
#include <dbghelp.h>
#include <intrin.h>
#include <psapi.h>
#include <tlhelp32.h>
#include <stdio.h>
#include <string.h>
#include <wchar.h>

#define FAIL_FAST_STATUS 0xc0000409u
#define CLR_EXCEPTION_STATUS 0xe0434352u
#define AUTOCAD_TIMEOUT_MS 120000u
#define EXCEPTION_HISTORY_SIZE 128u
#define STACK_FRAME_LIMIT 12u

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

static void report_unresolved_frame(HANDLE process, LPCVOID address)
{
    MEMORY_BASIC_INFORMATION memory;
    const char *kind;
    DWORD access;
    BOOL executable;

    if (!VirtualQueryEx(process, address, &memory, sizeof(memory)))
    {
        printf(" memory=query_failed error=%lu\n", (unsigned long)GetLastError());
        return;
    }
    if (memory.State != MEM_COMMIT)
    {
        printf(" memory=%s\n", memory.State == MEM_FREE ? "free" : "reserved");
        return;
    }
    switch (memory.Type)
    {
        case MEM_IMAGE: kind = "image"; break;
        case MEM_MAPPED: kind = "mapped"; break;
        case MEM_PRIVATE: kind = "private"; break;
        default: kind = "unknown"; break;
    }
    access = memory.Protect & 0xff;
    executable = !(memory.Protect & PAGE_GUARD) &&
                 (access == PAGE_EXECUTE || access == PAGE_EXECUTE_READ ||
                  access == PAGE_EXECUTE_READWRITE || access == PAGE_EXECUTE_WRITECOPY);
    printf(" memory=%s executable=%d allocation_offset=0x%llx", kind, executable,
           (unsigned long long)((ULONG_PTR)address - (ULONG_PTR)memory.AllocationBase));
    if (memory.Type == MEM_IMAGE || memory.Type == MEM_MAPPED)
    {
        WCHAR path[MAX_PATH];
        DWORD length = K32GetMappedFileNameW(process, (LPVOID)address, path, MAX_PATH);

        if (length && length < MAX_PATH)
        {
            const WCHAR *basename = path, *cursor;

            path[length] = 0;
            for (cursor = path; *cursor; ++cursor)
                if (*cursor == '\\' || *cursor == '/') basename = cursor + 1;
            if (*basename && wcsspn(basename, L"abcdefghijklmnopqrstuvwxyz"
                                 L"ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_.-") == wcslen(basename))
                printf(" mapped_file=%ls", basename);
        }
    }
    putchar('\n');
}

static void report_frame(HANDLE process, const char *record, unsigned int index, DWORD64 pc)
{
    IMAGEHLP_MODULE64 module = {0};

    module.SizeOfStruct = sizeof(module);
    if (SymGetModuleInfo64(process, pc, &module) && module.BaseOfImage <= pc &&
        pc - module.BaseOfImage < module.ImageSize &&
        module.ModuleName[0] && strspn(module.ModuleName, "abcdefghijklmnopqrstuvwxyz"
                                      "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_.-") == strlen(module.ModuleName))
        printf("observer_stack_%s=%u module=%s offset=0x%llx\n", record, index, module.ModuleName,
               (unsigned long long)(pc - module.BaseOfImage));
    else
    {
        printf("observer_stack_%s=%u module=unresolved", record, index);
        report_unresolved_frame(process, (LPCVOID)(ULONG_PTR)pc);
    }
}

static unsigned int report_stack(HANDLE process, DWORD thread_id)
{
    STACKFRAME64 frame = {0};
    CONTEXT context = {0};
    DWORD machine;
    DWORD64 pc, sp, next_return = 0;
    unsigned int attempts, count = 0;
    HANDLE thread = OpenThread(THREAD_GET_CONTEXT | THREAD_QUERY_INFORMATION, FALSE, thread_id);

    if (!thread)
    {
        printf("observer_stack_status=thread_error error=%lu\n", (unsigned long)GetLastError());
        return 0;
    }
    context.ContextFlags = CONTEXT_CONTROL | CONTEXT_INTEGER;
    if (!GetThreadContext(thread, &context))
    {
        printf("observer_stack_status=context_error error=%lu\n", (unsigned long)GetLastError());
        CloseHandle(thread);
        return 0;
    }
    SymSetOptions(SYMOPT_DEFERRED_LOADS | SYMOPT_FAIL_CRITICAL_ERRORS | SYMOPT_NO_PROMPTS);
    if (!SymInitialize(process, ".", TRUE))
    {
        printf("observer_stack_status=modules_error error=%lu\n", (unsigned long)GetLastError());
        CloseHandle(thread);
        return 0;
    }
#ifdef _WIN64
    machine = IMAGE_FILE_MACHINE_AMD64;
    frame.AddrPC.Offset = context.Rip;
    frame.AddrFrame.Offset = context.Rbp;
    frame.AddrStack.Offset = context.Rsp;
#else
    machine = IMAGE_FILE_MACHINE_I386;
    frame.AddrPC.Offset = context.Eip;
    frame.AddrFrame.Offset = context.Ebp;
    frame.AddrStack.Offset = context.Esp;
#endif
    frame.AddrPC.Mode = frame.AddrFrame.Mode = frame.AddrStack.Mode = AddrModeFlat;
    pc = frame.AddrPC.Offset;
    sp = frame.AddrStack.Offset;
    report_frame(process, "frame", count++, pc);
    for (attempts = 0; count < STACK_FRAME_LIMIT && attempts < STACK_FRAME_LIMIT; ++attempts)
    {
        if (!StackWalk64(machine, process, thread, &frame, &context, NULL, SymFunctionTableAccess64,
                         SymGetModuleBase64, NULL) || !frame.AddrPC.Offset || frame.AddrStack.Offset < sp)
            break;
        next_return = frame.AddrReturn.Offset;
        if (frame.AddrPC.Offset == pc && frame.AddrStack.Offset == sp) continue;
        pc = frame.AddrPC.Offset;
        sp = frame.AddrStack.Offset;
        report_frame(process, "frame", count++, pc);
    }
    if (next_return && next_return != pc)
    {
        char methods[2];

        report_frame(process, "next_return", count, next_return);
        if (GetEnvironmentVariableA("ACAD_EVENTPIPE_METHODS", methods, sizeof(methods)) == 1 && methods[0] == '1')
            printf("observer_return_pc=0x%llx\n", (unsigned long long)next_return);
    }
    printf("observer_stack_status=%s frames=%u\n",
           count == STACK_FRAME_LIMIT ? "limit" : count > 1 ? "walk_ended" : "fault_frame_only", count);
    SymCleanup(process);
    CloseHandle(thread);
    return count;
}

static int observe(DWORD pid, DWORD timeout, BOOL self_test)
{
    ULONGLONG end = GetTickCount64() + timeout, now;
    struct exception_event preceding[EXCEPTION_HISTORY_SIZE] = {0};
    unsigned int count = 0;
    BOOL initial_breakpoint = TRUE, saw_termination = FALSE, saw_test_reason = FALSE;
    HANDLE process = NULL;
    unsigned int stack_frames = 0;

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
                process = event.u.CreateProcessInfo.hProcess;
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
                        if (process) stack_frames = report_stack(process, event.dwThreadId);
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
            return !(self_test ? saw_test_reason && stack_frames > 1 && event.u.ExitProcess.dwExitCode == FAIL_FAST_STATUS
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

static __declspec(noinline) void self_test_child(void)
{
    /* Deliberately terminate only the synthetic child. */
    __fastfail(FAST_FAIL_FATAL_APP_EXIT);
}

int main(int argc, char **argv)
{
    DWORD pid;

    setvbuf(stdout, NULL, _IONBF, 0);
    if (argc == 1) return self_test();
    if (argc == 2 && !strcmp(argv[1], "--self-test-child")) self_test_child();
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
