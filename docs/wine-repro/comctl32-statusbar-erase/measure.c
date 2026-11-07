/* Original AI-assisted (Codex), LGPL-2.1-or-later. Public-API MEASURE only.
 * No expected Windows painting results and no changes to comctl32. */
#define UNICODE
#define _UNICODE
#define _WIN32_WINNT 0x0601
#include <windows.h>
#include <commctrl.h>
#include <uxtheme.h>
#include <shlwapi.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef STATUSBAR_VERSION
#error Build with STATUSBAR_VERSION=5 or 6 and the corresponding manifest.
#endif

static const WCHAR old_text[] = L"MMMMMMMMMMMMMMMM";
static const WCHAR short_text[] = L"I";
static const COLORREF dark = RGB(24, 32, 40), marker = RGB(203, 17, 139);
static const uint32_t marker_pixel = 0x00cb118b;
static unsigned errors, rows;

struct counters { unsigned paint, erase, nested_erase, drawitem, paint_update; };
struct fixture
{
    HWND host, bar;
    WNDPROC original;
    HBRUSH brush;
    HFONT font;
    HDC capture;
    HBITMAP bitmap;
    HGDIOBJ old_bitmap;
    uint32_t *dib;
    int width, height, depth;
    UINT style;
    BOOL custom_erase;
    struct counters counts;
};

static void error(const char *where)
{
    printf("ERROR where=%s code=%lu\n", where, GetLastError());
    errors++;
}

static LRESULT CALLBACK bar_proc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    struct fixture *f = (void *)GetWindowLongPtrW(hwnd, GWLP_USERDATA);
    LRESULT result;
    if (msg == WM_ERASEBKGND)
    {
        RECT rect;
        f->counts.erase++;
        if (f->depth) f->counts.nested_erase++;
        if (f->custom_erase)
        {
            GetClientRect(hwnd, &rect);
            if (!FillRect((HDC)wp, &rect, f->brush)) error("erase_fill");
            return 1;
        }
    }
    if (msg != WM_PAINT) return CallWindowProcW(f->original, hwnd, msg, wp, lp);
    f->counts.paint++;
    if (GetUpdateRect(hwnd, NULL, FALSE)) f->counts.paint_update++;
    f->depth++;
    result = CallWindowProcW(f->original, hwnd, msg, wp, lp);
    f->depth--;
    return result;
}

static LRESULT CALLBACK host_proc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    if (msg == WM_DRAWITEM)
    {
        DRAWITEMSTRUCT *dis = (void *)lp;
        struct fixture *f = (void *)GetWindowLongPtrW(dis->hwndItem, GWLP_USERDATA);
        if (f && (f->style & SBT_OWNERDRAW))
        {
            RECT rect = dis->rcItem;
            HGDIOBJ font = SelectObject(dis->hDC, f->font);
            int mode = SetBkMode(dis->hDC, TRANSPARENT);
            COLORREF color = SetTextColor(dis->hDC, RGB(255, 255, 255));
            f->counts.drawitem++;
            /* Deliberately no fill: the owner draws text on the existing background. */
            DrawTextW(dis->hDC, (const WCHAR *)dis->itemData, -1, &rect,
                      DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
            SetTextColor(dis->hDC, color);
            SetBkMode(dis->hDC, mode);
            SelectObject(dis->hDC, font);
            return TRUE;
        }
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

static BOOL snapshot(struct fixture *f, uint32_t *pixels)
{
    HDC dc = GetDC(f->bar);
    BOOL ok;
    if (!dc) { error("capture_dc"); return FALSE; }
    GdiFlush();
    ok = BitBlt(f->capture, 0, 0, f->width, f->height, dc, 4, 4, SRCCOPY);
    GdiFlush();
    ReleaseDC(f->bar, dc);
    if (!ok) { error("capture_blt"); return FALSE; }
    for (int i = 0; i < f->width * f->height; i++) pixels[i] = f->dib[i] & 0x00ffffff;
    return TRUE;
}

static unsigned difference(const uint32_t *a, const uint32_t *b, int count)
{
    unsigned result = 0;
    for (int i = 0; i < count; i++) if (a[i] != b[i]) result++;
    return result;
}

static unsigned markers(const uint32_t *pixels, int count)
{
    unsigned result = 0;
    for (int i = 0; i < count; i++) if (pixels[i] == marker_pixel) result++;
    return result;
}

static BOOL set_text(struct fixture *f, const WCHAR *text)
{
    if (!SendMessageW(f->bar, SB_SETTEXTW, f->style, (LPARAM)text))
    { error("settext"); return FALSE; }
    return TRUE;
}

static void stage_text(struct fixture *f, const WCHAR *text)
{
    SendMessageW(f->bar, WM_SETREDRAW, FALSE, 0);
    set_text(f, text);
    SendMessageW(f->bar, WM_SETREDRAW, TRUE, 0);
    /* Clear SB_SETTEXT's pending invalidation/erase, retaining old screen pixels. */
    if (!RedrawWindow(f->bar, NULL, NULL, RDW_VALIDATE | RDW_NOERASE | RDW_NOINTERNALPAINT))
        error("stage_validate");
}

static void clean_text(struct fixture *f, const WCHAR *text)
{
    RECT rect;
    HDC dc;
    stage_text(f, text);
    GetClientRect(f->bar, &rect);
    dc = GetDC(f->bar);
    if (!dc) { error("clean_dc"); return; }
    if (!FillRect(dc, &rect, f->brush)) error("clean_fill");
    ReleaseDC(f->bar, dc);
    if (!RedrawWindow(f->bar, NULL, NULL, RDW_INVALIDATE | RDW_ERASE | RDW_UPDATENOW))
        error("clean_redraw");
}

static BOOL create_fixture(struct fixture *f, UINT style, BOOL custom_erase, BOOL classic)
{
    typedef HRESULT (CALLBACK *version_fn)(DLLVERSIONINFO *);
    DLLVERSIONINFO version = {sizeof(version), 0, 0, 0, 0};
    BITMAPINFO bi = {0};
    HMODULE module;
    version_fn get_version;
    RECT rect;
    int edge = -1;
    HRESULT theme_hr = S_OK;
    memset(f, 0, sizeof(*f));
    f->style = style;
    f->custom_erase = custom_erase;
    f->brush = CreateSolidBrush(dark);
    f->font = CreateFontW(-20, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                          OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, NONANTIALIASED_QUALITY,
                          DEFAULT_PITCH, L"Tahoma");
    f->host = CreateWindowExW(WS_EX_TOPMOST, L"winum_statusbar_measure", L"Status bar MEASURE",
                              WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_VISIBLE,
                              40, 40, 660, 180, NULL, NULL, GetModuleHandleW(NULL), NULL);
    if (!f->host || !f->brush || !f->font) { error("fixture_host"); return FALSE; }
    UpdateWindow(f->host);
    f->bar = CreateWindowExW(0, STATUSCLASSNAMEW, NULL,
                             WS_CHILD | WS_VISIBLE | CCS_NOPARENTALIGN | CCS_NORESIZE | CCS_NODIVIDER,
                             12, 24, 600, 44, f->host, (HMENU)(UINT_PTR)1, GetModuleHandleW(NULL), NULL);
    if (!f->bar) { error("fixture_bar"); return FALSE; }
    if (classic) theme_hr = SetWindowTheme(f->bar, L"", L"");
    SetWindowLongPtrW(f->bar, GWLP_USERDATA, (LONG_PTR)f);
    f->original = (WNDPROC)SetWindowLongPtrW(f->bar, GWLP_WNDPROC, (LONG_PTR)bar_proc);
    if (!f->original) { error("subclass"); return FALSE; }
    SendMessageW(f->bar, WM_SETFONT, (WPARAM)f->font, FALSE);
    if (!SendMessageW(f->bar, SB_SETPARTS, 1, (LPARAM)&edge)) { error("parts"); return FALSE; }
    GetClientRect(f->bar, &rect);
    f->width = rect.right - 8;
    f->height = rect.bottom - 8;
    if (f->width < 200 || f->width > 1000 || f->height < 8 || f->height > 200)
    { error("capture_size"); return FALSE; }
    bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bi.bmiHeader.biWidth = f->width;
    bi.bmiHeader.biHeight = -f->height;
    bi.bmiHeader.biPlanes = 1;
    bi.bmiHeader.biBitCount = 32;
    bi.bmiHeader.biCompression = BI_RGB;
    f->capture = CreateCompatibleDC(NULL);
    f->bitmap = CreateDIBSection(f->capture, &bi, DIB_RGB_COLORS, (void **)&f->dib, NULL, 0);
    if (!f->capture || !f->bitmap) { error("capture_create"); return FALSE; }
    f->old_bitmap = SelectObject(f->capture, f->bitmap);
    if (!f->old_bitmap || f->old_bitmap == HGDI_ERROR)
    { f->old_bitmap = NULL; error("capture_select"); return FALSE; }
    /* Global classes may have no class HINSTANCE. Resolve the class procedure's
     * module instead; never print its path. */
    module = NULL;
    if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                           (LPCWSTR)(ULONG_PTR)GetClassLongPtrW(f->bar, GCLP_WNDPROC), &module))
    { error("control_module"); return FALSE; }
    get_version = module ? (void *)GetProcAddress(module, "DllGetVersion") : NULL;
    if (!get_version) { error("control_version_export"); return FALSE; }
    {
        HRESULT hr = get_version(&version);
        if (FAILED(hr)) { printf("ERROR where=control_version hr=%08lx\n", (unsigned long)hr); errors++; return FALSE; }
    }
    printf("CONFIG target=%d actual=%lu.%lu.%lu theme=%d classic=%d theme_hr=%08lx custom_erase=%d style=%u width=%d height=%d\n",
           STATUSBAR_VERSION, version.dwMajorVersion, version.dwMinorVersion, version.dwBuildNumber,
           GetWindowTheme(f->bar) != NULL, classic, (unsigned long)theme_hr, custom_erase, style, f->width, f->height);
    if (version.dwMajorVersion != STATUSBAR_VERSION) { error("control_version_mismatch"); return FALSE; }
    return TRUE;
}

static void destroy_fixture(struct fixture *f)
{
    if (f->host) DestroyWindow(f->host);
    if (f->capture && f->old_bitmap) SelectObject(f->capture, f->old_bitmap);
    if (f->bitmap) DeleteObject(f->bitmap);
    if (f->capture) DeleteDC(f->capture);
    if (f->font) DeleteObject(f->font);
    if (f->brush) DeleteObject(f->brush);
}

enum operation { SETTEXT, INVALIDATE_FALSE, INVALIDATE_TRUE, REDRAW_NOERASE, REDRAW_ERASE, PAINT_NO_UPDATE };

static void run_cases(struct fixture *f)
{
    static const char *const names[] = {"settext", "invalidate_false", "invalidate_true", "redraw_noerase", "redraw_erase", "paint_no_update"};
    int count = f->width * f->height;
    uint32_t *all = calloc((size_t)count * 5, sizeof(uint32_t));
    uint32_t *old, *empty, *wanted, *before, *after;
    if (!all) { error("pixel_alloc"); return; }
    old = all; empty = old + count; wanted = empty + count; before = wanted + count; after = before + count;
    clean_text(f, old_text); if (!snapshot(f, old)) goto done;
    clean_text(f, L""); if (!snapshot(f, empty)) goto done;
    for (int blank = 0; blank < 2; blank++)
    {
        const WCHAR *text = blank ? L"" : short_text;
        clean_text(f, text); if (!snapshot(f, wanted)) goto done;
        for (int op = SETTEXT; op <= PAINT_NO_UPDATE; op++)
        {
            struct counters prep, sent;
            RECT mark = {f->width - 40, 10, f->width - 16, 18};
            HDC dc;
            HBRUSH brush;
            unsigned prep_diff, old_pixels = 0, old_retained = 0, old_dirty = 0;
            int sample = -1;
            BOOL update_before;
            clean_text(f, old_text);
            memset(&f->counts, 0, sizeof(f->counts));
            if (op != SETTEXT) stage_text(f, text);
            prep = f->counts;
            if (!snapshot(f, before)) goto done;
            prep_diff = difference(before, old, count);
            dc = GetDC(f->bar); brush = CreateSolidBrush(marker);
            if (!dc || !brush) { if (dc) ReleaseDC(f->bar, dc); if (brush) DeleteObject(brush); error("marker_create"); goto done; }
            if (!FillRect(dc, &mark, brush)) error("marker_fill");
            DeleteObject(brush); ReleaseDC(f->bar, dc);
            if (!snapshot(f, before)) goto done;
            update_before = GetUpdateRect(f->bar, NULL, FALSE);
            memset(&f->counts, 0, sizeof(f->counts));
            memset(&sent, 0, sizeof(sent));
            if (op == SETTEXT)
            {
                set_text(f, text);
                sent = f->counts;
                UpdateWindow(f->bar);
            }
            else if (op == INVALIDATE_FALSE || op == INVALIDATE_TRUE)
            {
                if (!InvalidateRect(f->bar, NULL, op == INVALIDATE_TRUE)) error("invalidate");
                UpdateWindow(f->bar);
            }
            else if (op == PAINT_NO_UPDATE) SendMessageW(f->bar, WM_PAINT, 0, 0);
            else if (!RedrawWindow(f->bar, NULL, NULL,
                                   RDW_INVALIDATE | RDW_UPDATENOW | (op == REDRAW_ERASE ? RDW_ERASE : 0)))
                error("redraw");
            if (!snapshot(f, after)) goto done;
            for (int i = 0; i < count; i++)
            {
                if (old[i] == empty[i] || wanted[i] != empty[i]) continue;
                if (sample < 0) sample = i;
                old_pixels++;
                if (after[i] == old[i]) old_retained++;
                if (after[i] != wanted[i]) old_dirty++;
            }
            printf("MEASURE op=%s blank=%d prep_diff=%u prep_paint=%u prep_erase=%u update_before=%d "
                   "send_paint=%u send_erase=%u paint=%u paint_update=%u erase=%u nested_erase=%u outside_erase=%u drawitem=%u "
                   "marker_before=%u marker_after=%u old_pixels=%u old_retained=%u old_dirty=%u diff_clean=%u update_after=%d "
                   "sample_x=%d sample_y=%d old_rgb=%06lx new_rgb=%06lx after_rgb=%06lx\n",
                   names[op], blank, prep_diff, prep.paint, prep.erase, update_before,
                   sent.paint, sent.erase, f->counts.paint, f->counts.paint_update, f->counts.erase,
                   f->counts.nested_erase, f->counts.erase - f->counts.nested_erase, f->counts.drawitem,
                   markers(before, count), markers(after, count), old_pixels, old_retained, old_dirty,
                   difference(after, wanted, count), GetUpdateRect(f->bar, NULL, FALSE),
                   sample < 0 ? -1 : sample % f->width + 4, sample < 0 ? -1 : sample / f->width + 4,
                   (unsigned long)(sample < 0 ? 0 : old[sample]), (unsigned long)(sample < 0 ? 0 : wanted[sample]),
                   (unsigned long)(sample < 0 ? 0 : after[sample]));
            rows++;
        }
    }
done:
    free(all);
}

int main(void)
{
    const UINT styles[] = {0, SBT_NOBORDERS, SBT_OWNERDRAW | SBT_NOBORDERS};
    INITCOMMONCONTROLSEX init = {sizeof(init), ICC_BAR_CLASSES};
    WNDCLASSW cls = {0};
    SetProcessDPIAware();
    setvbuf(stdout, NULL, _IOLBF, 0);
    printf("STATUSBAR_MEASURE schema=1 arch_bits=%u target=%d wine=%d theme_active=%d app_themed=%d\n",
           (unsigned)(8 * sizeof(void *)), STATUSBAR_VERSION,
           GetProcAddress(GetModuleHandleW(L"ntdll.dll"), "wine_get_version") != NULL,
           IsThemeActive(), IsAppThemed());
    cls.lpfnWndProc = host_proc;
    cls.hInstance = GetModuleHandleW(NULL);
    cls.lpszClassName = L"winum_statusbar_measure";
    cls.hbrBackground = GetSysColorBrush(COLOR_3DFACE);
    if (!InitCommonControlsEx(&init) || !RegisterClassW(&cls)) { error("init"); return 2; }
    for (int classic = 0; classic < 2; classic++)
        for (int custom = 0; custom < 2; custom++)
            for (unsigned style = 0; style < sizeof(styles) / sizeof(styles[0]); style++)
            {
                struct fixture f;
                if (create_fixture(&f, styles[style], custom, classic)) run_cases(&f);
                destroy_fixture(&f);
            }
    UnregisterClassW(cls.lpszClassName, cls.hInstance);
    printf("SUMMARY rows=%u errors=%u\n", rows, errors);
    return errors || rows != 144 ? 2 : 0;
}
