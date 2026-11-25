/* Pixel-counting approach and diagnosis from @krokosik's tahoma-ppem.c,
 * Autodesk-Inventor-for-Linux PR #18. Original extended harness, AI-assisted
 * (Codex); LGPL-2.1-or-later. */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>

enum { width = 290, height = 38, first_px = 8, last_px = 24 };
static const WCHAR sample[] = L"HIllm Aa xy 012";
static const WCHAR *const files[] = {L"probe-tahoma.ttf", L"probe-sans.ttf"};
static const WCHAR *const families[] = {L"WinumProbeTahoma", L"Tahoma", L"WinumProbeSans"};
static const char *const ids[] = {"wine_tahoma", "system_tahoma", "open_sans_control"};
static const struct { BYTE value; const char *name; } qualities[] = {
    {DEFAULT_QUALITY, "default"}, {ANTIALIASED_QUALITY, "aa"},
    {CLEARTYPE_QUALITY, "ct"}, {CLEARTYPE_NATURAL_QUALITY, "ct_natural"},
    {NONANTIALIASED_QUALITY, "nonaa"},
    {DRAFT_QUALITY, "draft"}, {PROOF_QUALITY, "proof"}
};

struct settings { BOOL enabled; UINT type, orientation; };
struct drawing
{
    HDC dc;
    HBITMAP bitmap;
    BITMAPINFO info;
    void *surface;
    BYTE pixels[width * height * 4];
};

static LONG interrupted;

static BOOL check(BOOL ok, const char *operation)
{
    if (!ok) fprintf(stderr, "error=%s win32=%lu\n", operation, GetLastError());
    return ok;
}

static BOOL WINAPI control_handler(DWORD event)
{
    if (event != CTRL_C_EVENT && event != CTRL_BREAK_EVENT) return FALSE;
    InterlockedExchange(&interrupted, 1);
    return TRUE;
}

static BOOL read_settings(struct settings *state)
{
    BOOL ok = check(SystemParametersInfoW(SPI_GETFONTSMOOTHING, 0, &state->enabled, 0), "get_smoothing");
    ok &= check(SystemParametersInfoW(SPI_GETFONTSMOOTHINGTYPE, 0, &state->type, 0), "get_type");
    ok &= check(SystemParametersInfoW(SPI_GETFONTSMOOTHINGORIENTATION, 0, &state->orientation, 0),
                "get_orientation");
    return ok;
}

static void print_state(const char *phase, const struct settings *state)
{
    printf("state=%s on=%d type=%u orientation=%u\n", phase, !!state->enabled, state->type, state->orientation);
}

static BOOL write_settings(const struct settings *state)
{
    const UINT flags = SPIF_UPDATEINIFILE | SPIF_SENDCHANGE;
    BOOL ok = check(SystemParametersInfoW(SPI_SETFONTSMOOTHINGTYPE, 0, (void *)(UINT_PTR)state->type, flags),
                    "set_type");
    ok &= check(SystemParametersInfoW(SPI_SETFONTSMOOTHINGORIENTATION, 0,
                                     (void *)(UINT_PTR)state->orientation, flags), "set_orientation");
    ok &= check(SystemParametersInfoW(SPI_SETFONTSMOOTHING, !!state->enabled, NULL, flags), "set_smoothing");
    return ok;
}

static BOOL same_settings(const struct settings *left, const struct settings *right)
{
    return !!left->enabled == !!right->enabled && left->type == right->type &&
           left->orientation == right->orientation;
}

static void print_face(const WCHAR *face)
{
    for (; *face; ++face) putchar(*face >= 33 && *face <= 126 ? (char)*face : '_');
}

static BOOL draw_case(struct drawing *draw, LOGFONTW *lf, const char *id, const char *quality,
                      const struct settings *state)
{
    HGDIOBJ oldfont, oldbitmap;
    HFONT font = CreateFontIndirectW(lf);
    WCHAR face[LF_FACESIZE] = {0};
    TEXTMETRICW tm;
    unsigned full = 0, gray = 0, colored = 0, levels = 0, i;
    BYTE seen[256] = {0};
    DWORD rgb = 2166136261u, bgr = 2166136261u;
    BOOL ok;

    if (!check(!!font, "CreateFontIndirect")) return FALSE;
    oldfont = SelectObject(draw->dc, font);
    if (!check(!!oldfont, "SelectObject(font)")) { DeleteObject(font); return FALSE; }
    oldbitmap = SelectObject(draw->dc, draw->bitmap);
    if (!check(!!oldbitmap, "SelectObject(bitmap)"))
    {
        SelectObject(draw->dc, oldfont);
        DeleteObject(font);
        return FALSE;
    }

    ZeroMemory(draw->surface, sizeof(draw->pixels));
    ok = check(TextOutW(draw->dc, 3, 3, sample, ARRAYSIZE(sample) - 1), "TextOut");
    GdiFlush();
    ok &= check(GetTextFaceW(draw->dc, ARRAYSIZE(face), face) != 0, "GetTextFace");
    ok &= check(GetTextMetricsW(draw->dc, &tm), "GetTextMetrics");
    SelectObject(draw->dc, oldbitmap);
    ok &= check(GetDIBits(draw->dc, draw->bitmap, 0, height, draw->pixels,
                          &draw->info, DIB_RGB_COLORS) == height, "GetDIBits");
    SelectObject(draw->dc, oldfont);
    DeleteObject(font);
    if (!ok) return FALSE;

    for (i = 0; i < sizeof(draw->pixels); i += 4)
    {
        unsigned b = draw->pixels[i], g = draw->pixels[i + 1], r = draw->pixels[i + 2];
        if (r != g || g != b) ++colored;
        else if (r == 255) ++full;
        else if (r) { ++gray; seen[r] = 1; }
        rgb = (rgb ^ r) * 16777619u;
        rgb = (rgb ^ g) * 16777619u;
        rgb = (rgb ^ b) * 16777619u;
        bgr = (bgr ^ b) * 16777619u;
        bgr = (bgr ^ g) * 16777619u;
        bgr = (bgr ^ r) * 16777619u;
    }
    for (i = 1; i < 255; ++i) levels += seen[i];
    printf("case=%s height=%ld quality=%s on=%d type=%u orientation=%u requested=", id, lf->lfHeight,
           quality, !!state->enabled, state->type, state->orientation);
    print_face(lf->lfFaceName);
    printf(" actual=");
    print_face(face);
    printf(" match=%d ppem=%ld mode=%s ink=%u gray=%u levels=%u color=%u rgb=%08lx bgr=%08lx\n",
           !lstrcmpiW(lf->lfFaceName, face), tm.tmHeight - tm.tmInternalLeading,
           colored ? "subpixel" : gray ? "gray" : full ? "bilevel" : "empty",
           full + gray + colored, gray, levels, colored, rgb, bgr);
    return TRUE;
}

static BOOL render(const struct settings *requested)
{
    static struct drawing draw;
    struct settings state;
    NONCLIENTMETRICSW metrics = {0};
    LOGFONTW stock;
    unsigned loaded = 0, f, px, q, count = 0, contrast;
    BOOL ok = FALSE;

    if (!read_settings(&state)) return FALSE;
    print_state("render", &state);
    if (!check(same_settings(requested, &state), "child_settings_readback")) return FALSE;
    if (!check(SystemParametersInfoW(SPI_GETFONTSMOOTHINGCONTRAST, 0, &contrast, 0), "get_contrast"))
        return FALSE;
    for (; loaded < ARRAYSIZE(files); ++loaded)
        if (!check(AddFontResourceExW(files[loaded], FR_PRIVATE, NULL) != 0, "AddFontResourceEx"))
            goto cleanup;
    draw.dc = CreateCompatibleDC(NULL);
    if (!check(!!draw.dc, "CreateCompatibleDC")) goto cleanup;
    draw.info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    draw.info.bmiHeader.biWidth = width;
    draw.info.bmiHeader.biHeight = -height;
    draw.info.bmiHeader.biPlanes = 1;
    draw.info.bmiHeader.biBitCount = 32;
    draw.info.bmiHeader.biCompression = BI_RGB;
    draw.bitmap = CreateDIBSection(draw.dc, &draw.info, DIB_RGB_COLORS, &draw.surface, NULL, 0);
    if (!check(!!draw.bitmap, "CreateDIBSection")) goto cleanup;
    printf("device_dpi=%d contrast=%u\n", GetDeviceCaps(draw.dc, LOGPIXELSY), contrast);
    SetBkMode(draw.dc, TRANSPARENT);
    SetBkColor(draw.dc, RGB(0, 0, 0));
    SetTextColor(draw.dc, RGB(255, 255, 255));

    for (f = 0; f < ARRAYSIZE(families); ++f)
        for (px = first_px; px <= last_px; ++px)
            for (q = 0; q < ARRAYSIZE(qualities); ++q)
            {
                LOGFONTW lf = {0};
                lf.lfHeight = -(LONG)px;
                lf.lfWeight = FW_NORMAL;
                lf.lfQuality = qualities[q].value;
                lf.lfCharSet = DEFAULT_CHARSET;
                lstrcpynW(lf.lfFaceName, families[f], LF_FACESIZE);
                if (!draw_case(&draw, &lf, ids[f], qualities[q].name, &state)) goto cleanup;
                ++count;
            }
    metrics.cbSize = sizeof(metrics);
    if (!check(SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, metrics.cbSize, &metrics, 0),
               "SPI_GETNONCLIENTMETRICS")) goto cleanup;
    if (!check(GetObjectW(GetStockObject(DEFAULT_GUI_FONT), sizeof(stock), &stock) == sizeof(stock),
               "GetObject(DEFAULT_GUI_FONT)")) goto cleanup;
    metrics.lfMessageFont.lfQuality = metrics.lfMenuFont.lfQuality = stock.lfQuality = DEFAULT_QUALITY;
    ok = draw_case(&draw, &metrics.lfMessageFont, "ui_message", "default", &state) &&
         draw_case(&draw, &metrics.lfMenuFont, "ui_menu", "default", &state) &&
         draw_case(&draw, &stock, "ui_stock", "default", &state);
    if (ok) printf("matrix_cases=%u\n", count + 3);

cleanup:
    if (draw.bitmap) DeleteObject(draw.bitmap);
    if (draw.dc) DeleteDC(draw.dc);
    while (loaded) RemoveFontResourceExW(files[--loaded], FR_PRIVATE, NULL);
    return ok;
}

static BOOL run_child(const WCHAR *exe, const struct settings *state)
{
    STARTUPINFOW startup = {0};
    PROCESS_INFORMATION process;
    WCHAR command[MAX_PATH + 60];
    DWORD code = 1, wait;

    startup.cb = sizeof(startup);
    swprintf(command, ARRAYSIZE(command), L"\"%ls\" --render %d %u %u",
             exe, !!state->enabled, state->type, state->orientation);
    fflush(NULL);
    if (!check(CreateProcessW(exe, command, NULL, NULL, TRUE, 0, NULL, NULL, &startup, &process),
               "CreateProcess")) return FALSE;
    wait = WaitForSingleObject(process.hProcess, 60000);
    if (wait == WAIT_OBJECT_0) GetExitCodeProcess(process.hProcess, &code);
    else
    {
        fprintf(stderr, "error=render_wait result=%lu\n", wait);
        TerminateProcess(process.hProcess, 1);
        WaitForSingleObject(process.hProcess, 5000);
    }
    CloseHandle(process.hThread);
    CloseHandle(process.hProcess);
    return code == 0;
}

int main(int argc, char **argv)
{
    struct settings original, requested, actual;
    WCHAR exe[MAX_PATH];
    DWORD length;
    unsigned on, type, order, matrices = 0;
    BOOL ok = TRUE;

    SetProcessDPIAware();
    if (argc == 5 && !lstrcmpA(argv[1], "--render"))
    {
        requested.enabled = atoi(argv[2]);
        requested.type = atoi(argv[3]);
        requested.orientation = atoi(argv[4]);
        return render(&requested) ? 0 : 1;
    }
    if (argc != 1) return 2;
    length = GetModuleFileNameW(NULL, exe, ARRAYSIZE(exe));
    if (!check(length && length < ARRAYSIZE(exe), "GetModuleFileName") || !read_settings(&original))
        return 1;
    printf("measure=font_smoothing pointer_bits=%u\n", (unsigned)(sizeof(void *) * 8));
    print_state("initial", &original);
    if (!check(SetConsoleCtrlHandler(control_handler, TRUE), "SetConsoleCtrlHandler")) return 1;
    for (on = 0; on <= 1 && ok; ++on)
        for (type = 1; type <= 2 && ok; ++type)
            for (order = 0; order <= 1 && ok; ++order)
            {
                requested.enabled = on;
                requested.type = type;
                requested.orientation = order;
                ok = !InterlockedCompareExchange(&interrupted, 0, 0) && write_settings(&requested) &&
                     read_settings(&actual) && check(same_settings(&requested, &actual), "parent_settings_readback") &&
                     run_child(exe, &requested);
                if (ok) ++matrices;
            }
    if (write_settings(&original) && read_settings(&actual))
    {
        print_state("restored", &actual);
        ok &= check(same_settings(&original, &actual), "restore_readback");
    }
    else ok = FALSE;
    printf("measure_complete matrices=%u restored=%d\n", matrices, ok);
    return ok ? 0 : 1;
}
