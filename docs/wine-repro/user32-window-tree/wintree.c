/* Original, AI-assisted (Claude), LGPL-2.1-or-later.
 * Prints the window tree of a running process, to compare Windows with
 * Wine for the same program state (Inventor's Content Center editor, issue
 * Harvi654/Autodesk-Inventor-for-Linux#45). Observes from outside with
 * public user32 calls only.
 *
 * Usage: wintree.exe [process name, default Inventor.exe]
 * One line per window: depth, handle, class, style, ex style, GetParent,
 * GW_OWNER, GA_PARENT, GA_ROOT, visible, enabled, child id, and the first
 * 24 characters of the title (titles can name documents: mask before
 * sending). Top-level windows of the process first, each followed by its
 * children. */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <tlhelp32.h>
#include <stdio.h>
#include <wchar.h>

static DWORD target_pid;

static void print_window(HWND hwnd, int depth)
{
    WCHAR cls[96] = L"", title[25] = L"";
    int i;

    GetClassNameW(hwnd, cls, ARRAYSIZE(cls));
    GetWindowTextW(hwnd, title, ARRAYSIZE(title));
    for (i = 0; title[i]; i++) if (title[i] < 32 || title[i] == '\t') title[i] = ' ';
    printf("%d\t%p\t%ls\tstyle=%08lx\tex=%08lx\tparent=%p\towner=%p\tga_parent=%p\tga_root=%p\t"
           "visible=%d\tenabled=%d\tid=%ld\ttitle=%ls\n",
           depth, (void *)hwnd, cls, (unsigned long)GetWindowLongW(hwnd, GWL_STYLE),
           (unsigned long)GetWindowLongW(hwnd, GWL_EXSTYLE), (void *)GetParent(hwnd),
           (void *)GetWindow(hwnd, GW_OWNER), (void *)GetAncestor(hwnd, GA_PARENT),
           (void *)GetAncestor(hwnd, GA_ROOT), IsWindowVisible(hwnd), IsWindowEnabled(hwnd),
           (long)GetDlgCtrlID(hwnd), title);
}

static void print_children(HWND parent, int depth)
{
    HWND child = GetWindow(parent, GW_CHILD);

    for (; child; child = GetWindow(child, GW_HWNDNEXT))
    {
        print_window(child, depth);
        print_children(child, depth + 1);
    }
}

static BOOL CALLBACK top_proc(HWND hwnd, LPARAM lp)
{
    DWORD pid = 0;

    GetWindowThreadProcessId(hwnd, &pid);
    if (pid != target_pid) return TRUE;
    print_window(hwnd, 0);
    print_children(hwnd, 1);
    return TRUE;
}

int wmain(int argc, WCHAR **argv)
{
    const WCHAR *name = argc > 1 ? argv[1] : L"Inventor.exe";
    PROCESSENTRY32W pe;
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

    pe.dwSize = sizeof(pe);
    if (snap != INVALID_HANDLE_VALUE && Process32FirstW(snap, &pe))
    {
        do
        {
            if (!_wcsicmp(pe.szExeFile, name)) { target_pid = pe.th32ProcessID; break; }
        } while (Process32NextW(snap, &pe));
    }
    if (snap != INVALID_HANDLE_VALUE) CloseHandle(snap);
    if (!target_pid)
    {
        printf("error\tprocess %ls not found\n", name);
        return 1;
    }
    printf("columns\tdepth\thwnd\tclass\tstyle\tex\tparent\towner\tga_parent\tga_root\tvisible\tenabled\tid\ttitle\n");
    EnumWindows(top_proc, 0);
    printf("done\n");
    return 0;
}
