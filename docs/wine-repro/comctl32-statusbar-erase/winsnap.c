/*
 * winsnap: a snapshot of a running program's window geometry and of its
 * status bar, observed from outside through public Win32 calls only.
 * Changes nothing in the observed program.
 *
 * Usage (in the program's prefix, with the same Wine, while it runs):
 *   wine winsnap.exe <label> [title substring]
 * The title substring defaults to "Autodesk Inventor". Writes
 *   status-<label>.bmp  the status bar as its window DC holds it
 *   screen-<label>.bmp  the same screen area read through the screen DC
 * in the current directory and prints the geometry to stdout. No window
 * texts are printed (they may contain file names).
 *
 * Build: x86_64-w64-mingw32-gcc -O2 -Wall -o winsnap.exe winsnap.c -lgdi32 -luser32
 *
 * Original, AI-assisted (Claude), LGPL-2.1-or-later.
 */
#define _WIN32_WINNT 0x0A00
#include <windows.h>
#include <stdio.h>
#include <string.h>

static HWND top, status;
static const char *want = "Autodesk Inventor";

static BOOL CALLBACK find_top(HWND hwnd, LPARAM lp)
{
    char title[512];
    (void)lp;
    if (!IsWindowVisible(hwnd) || GetParent(hwnd)) return TRUE;
    if (!GetWindowTextA(hwnd, title, sizeof(title))) return TRUE;
    if (strstr(title, want)) { top = hwnd; return FALSE; }
    return TRUE;
}

static BOOL CALLBACK find_status(HWND hwnd, LPARAM lp)
{
    char cls[64];
    RECT r, best;
    (void)lp;
    if (!IsWindowVisible(hwnd)) return TRUE;
    GetClassNameA(hwnd, cls, sizeof(cls));
    if (_stricmp(cls, "msctls_statusbar32")) return TRUE;
    GetWindowRect(hwnd, &r);
    if (!status) { status = hwnd; return TRUE; }
    GetWindowRect(status, &best);
    if (r.right - r.left > best.right - best.left) status = hwnd;
    return TRUE;
}

static int depth_of(HWND hwnd)
{
    int d = 0;
    while (hwnd && hwnd != top) { hwnd = GetParent(hwnd); d++; }
    return d;
}

static void describe(const char *tag, HWND hwnd, int indent)
{
    char cls[64];
    RECT w, c, rgn;
    POINT org = {0, 0};
    int region;
    GetClassNameA(hwnd, cls, sizeof(cls));
    GetWindowRect(hwnd, &w);
    GetClientRect(hwnd, &c);
    ClientToScreen(hwnd, &org);
    region = GetWindowRgnBox(hwnd, &rgn);
    printf("%s %*s%p %-26s win=%ld,%ld,%ld,%ld client=%ldx%ld org=%ld,%ld style=%08lx ex=%08lx dpi=%u region=%s\n",
           tag, indent, "", (void *)hwnd, cls, w.left, w.top, w.right, w.bottom, c.right, c.bottom,
           org.x, org.y, (unsigned long)GetWindowLongW(hwnd, GWL_STYLE),
           (unsigned long)GetWindowLongW(hwnd, GWL_EXSTYLE), GetDpiForWindow(hwnd),
           region == ERROR ? "none" : region == NULLREGION ? "empty" : "set");
}

static BOOL CALLBACK tree(HWND hwnd, LPARAM lp)
{
    (void)lp;
    if (IsWindowVisible(hwnd)) describe("TREE", hwnd, depth_of(hwnd) * 2);
    return TRUE;
}

static int save_bmp(HDC src, int x, int y, int w, int h, const char *name)
{
    BITMAPINFO bi;
    BITMAPFILEHEADER fh;
    void *bits = NULL;
    HDC mem;
    HBITMAP bmp, old;
    FILE *f;
    int ok = 0;

    memset(&bi, 0, sizeof(bi));
    bi.bmiHeader.biSize = sizeof(bi.bmiHeader);
    bi.bmiHeader.biWidth = w;
    bi.bmiHeader.biHeight = h;  /* bottom-up, as BMP files store it */
    bi.bmiHeader.biPlanes = 1;
    bi.bmiHeader.biBitCount = 32;
    bi.bmiHeader.biCompression = BI_RGB;
    mem = CreateCompatibleDC(src);
    bmp = CreateDIBSection(mem, &bi, DIB_RGB_COLORS, &bits, NULL, 0);
    if (!mem || !bmp || !bits) { printf("ERROR capture_setup %s\n", name); goto done; }
    old = SelectObject(mem, bmp);
    GdiFlush();
    if (!BitBlt(mem, 0, 0, w, h, src, x, y, SRCCOPY)) printf("ERROR bitblt %s win32=%lu\n", name, GetLastError());
    GdiFlush();
    SelectObject(mem, old);
    memset(&fh, 0, sizeof(fh));
    fh.bfType = 0x4d42;
    fh.bfOffBits = sizeof(fh) + sizeof(bi.bmiHeader);
    fh.bfSize = fh.bfOffBits + (DWORD)w * h * 4;
    if (!(f = fopen(name, "wb"))) { printf("ERROR open %s\n", name); goto done; }
    fwrite(&fh, sizeof(fh), 1, f);
    fwrite(&bi.bmiHeader, sizeof(bi.bmiHeader), 1, f);
    fwrite(bits, 4, (size_t)w * h, f);
    fclose(f);
    ok = 1;
done:
    if (bmp) DeleteObject(bmp);
    if (mem) DeleteDC(mem);
    return ok;
}

int main(int argc, char **argv)
{
    char name[300];
    RECT sr, sc;
    POINT dcorg;
    HDC dc;

    if (argc < 2) { puts("usage: winsnap.exe <label> [title substring]"); return 2; }
    if (argc > 2) want = argv[2];
    /* Physical pixels, so the numbers are not virtualised for this observer. */
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    EnumWindows(find_top, 0);
    if (!top) { printf("ERROR no visible top-level window with \"%s\" in its title\n", want); return 2; }
    EnumChildWindows(top, find_status, 0);
    printf("SNAP label=%s zoomed=%d iconic=%d foreground=%d\n", argv[1], IsZoomed(top), IsIconic(top),
           GetForegroundWindow() == top);
    describe("TOP", top, 0);
    if (!status) { puts("ERROR no visible status bar"); return 2; }
    {
        HWND a = status;
        while (a && a != top) { describe("CHAIN", a, depth_of(a) * 2); a = GetParent(a); }
    }
    GetWindowRect(status, &sr);
    GetClientRect(status, &sc);
    dc = GetDC(status);
    if (dc)
    {
        GetDCOrgEx(dc, &dcorg);
        printf("STATUS_DC org=%ld,%ld (window top-left %ld,%ld)\n", dcorg.x, dcorg.y, sr.left, sr.top);
        snprintf(name, sizeof(name), "status-%s.bmp", argv[1]);
        if (save_bmp(dc, 0, 0, sc.right, sc.bottom, name)) printf("SAVED %s %ldx%ld\n", name, sc.right, sc.bottom);
        ReleaseDC(status, dc);
    }
    dc = GetDC(NULL);
    if (dc)
    {
        snprintf(name, sizeof(name), "screen-%s.bmp", argv[1]);
        if (save_bmp(dc, sr.left, sr.top, sr.right - sr.left, sr.bottom - sr.top, name))
            printf("SAVED %s %ldx%ld\n", name, sr.right - sr.left, sr.bottom - sr.top);
        ReleaseDC(NULL, dc);
    }
    EnumChildWindows(top, tree, 0);
    return 0;
}
