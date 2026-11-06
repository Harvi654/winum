/* MEASURE: how ChoosePixelFormat, wglChoosePixelFormatARB and a second
 * SetPixelFormat on the same window behave when alpha is requested.
 *
 * Original AI-assisted (Claude), LGPL-2.1-or-later. Own program; it loads
 * no vendor DLLs beyond the system's OpenGL driver and prints numbers only.
 *
 * Background: Inventor's GPU ray tracing capability check (see README.md)
 * chooses a format with ChoosePixelFormat, sets it, creates a legacy
 * context, then chooses again with wglChoosePixelFormatARB and sets that
 * format on the same window. Wine picks different formats in the two steps
 * and the second SetPixelFormat fails. This program records what Windows
 * does in the same steps, on the real driver of the VM's GPU.
 */
#include <windows.h>
#include <GL/gl.h>
#include <stdio.h>

#define WGL_DRAW_TO_WINDOW_ARB    0x2001
#define WGL_ACCELERATION_ARB      0x2003
#define WGL_SUPPORT_OPENGL_ARB    0x2010
#define WGL_DOUBLE_BUFFER_ARB     0x2011
#define WGL_PIXEL_TYPE_ARB        0x2013
#define WGL_COLOR_BITS_ARB        0x2014
#define WGL_ALPHA_BITS_ARB        0x201b
#define WGL_DEPTH_BITS_ARB        0x2022
#define WGL_STENCIL_BITS_ARB      0x2023
#define WGL_FULL_ACCELERATION_ARB 0x2027
#define WGL_TYPE_RGBA_ARB         0x202b

typedef BOOL (WINAPI *choose_arb_fn)(HDC, const int *, const FLOAT *, UINT, int *, UINT *);

static const char class_name[] = "winum_pf_measure";

static HWND new_window(void)
{
    return CreateWindowA(class_name, "pf", WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
                         0, 0, 64, 64, NULL, NULL, GetModuleHandleA(NULL), NULL);
}

static void print_format(const char *tag, HDC dc, int index)
{
    PIXELFORMATDESCRIPTOR pfd;

    memset(&pfd, 0, sizeof(pfd));
    if (index <= 0 || !DescribePixelFormat(dc, index, sizeof(pfd), &pfd))
    {
        printf("%s index=%d describe=0 error=%lu\n", tag, index, GetLastError());
        return;
    }
    printf("%s index=%d flags=%#lx type=%u color=%u rgba=%u/%u/%u/%u depth=%u stencil=%u accum=%u aux=%u\n",
           tag, index, pfd.dwFlags, pfd.iPixelType, pfd.cColorBits, pfd.cRedBits, pfd.cGreenBits,
           pfd.cBlueBits, pfd.cAlphaBits, pfd.cDepthBits, pfd.cStencilBits, pfd.cAccumBits, pfd.cAuxBuffers);
}

static void init_request(PIXELFORMATDESCRIPTOR *pfd, BYTE color, BYTE alpha)
{
    memset(pfd, 0, sizeof(*pfd));
    pfd->nSize = sizeof(*pfd);
    pfd->nVersion = 1;
    pfd->dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd->iPixelType = PFD_TYPE_RGBA;
    pfd->cColorBits = color;
    pfd->cRedBits = pfd->cGreenBits = pfd->cBlueBits = 8;
    pfd->cAlphaBits = alpha;
    pfd->cDepthBits = 24;
    pfd->cStencilBits = 8;
    pfd->iLayerType = PFD_MAIN_PLANE;
}

/* Part 1: the formats the driver offers for an OpenGL window. */
static void list_formats(void)
{
    PIXELFORMATDESCRIPTOR pfd;
    HWND wnd = new_window();
    HDC dc = GetDC(wnd);
    int count, i, listed = 0;

    count = DescribePixelFormat(dc, 1, sizeof(pfd), &pfd);
    printf("FORMATS count=%d\n", count);
    for (i = 1; i <= count; i++)
    {
        if (!DescribePixelFormat(dc, i, sizeof(pfd), &pfd)) continue;
        if ((pfd.dwFlags & (PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER))
                != (PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER)) continue;
        if (pfd.iPixelType != PFD_TYPE_RGBA) continue;
        print_format("LIST", dc, i);
        listed++;
    }
    printf("FORMATS listed=%d\n", listed);
    ReleaseDC(wnd, dc);
    DestroyWindow(wnd);
}

/* Part 2: ChoosePixelFormat alone, for four requests. */
static void choose_variants(void)
{
    static const BYTE variants[][2] = {{24, 8}, {24, 0}, {32, 8}, {32, 0}};
    PIXELFORMATDESCRIPTOR req;
    unsigned int v;

    for (v = 0; v < ARRAYSIZE(variants); v++)
    {
        HWND wnd = new_window();
        HDC dc = GetDC(wnd);
        char tag[64];
        int index;

        init_request(&req, variants[v][0], variants[v][1]);
        SetLastError(0xdeadbeef);
        index = ChoosePixelFormat(dc, &req);
        sprintf(tag, "CHOOSE color=%u alpha=%u ->", variants[v][0], variants[v][1]);
        if (!index) printf("%s index=0 error=%lu\n", tag, GetLastError());
        else print_format(tag, dc, index);
        ReleaseDC(wnd, dc);
        DestroyWindow(wnd);
    }
}

/* Part 3: the sequence of the capability check, on one window. */
static void sequence(BYTE color, BYTE alpha, int arb_color)
{
    PIXELFORMATDESCRIPTOR req;
    HWND wnd = new_window();
    HDC dc = GetDC(wnd);
    int first, second = 0, other, count, attrs[32], n = 0;
    UINT found = 0;
    choose_arb_fn choose_arb;
    HGLRC rc;
    BOOL ret;

    printf("SEQUENCE legacy color=%u alpha=%u, ARB color=%d alpha=8\n", color, alpha, arb_color);
    init_request(&req, color, alpha);
    first = ChoosePixelFormat(dc, &req);
    print_format("SEQ first", dc, first);
    SetLastError(0xdeadbeef);
    ret = SetPixelFormat(dc, first, &req);
    printf("SEQ first_set=%d error=%lu getpixelformat=%d\n", ret, ret ? 0 : GetLastError(), GetPixelFormat(dc));
    if (!ret) goto done;

    rc = wglCreateContext(dc);
    if (!rc || !wglMakeCurrent(dc, rc))
    {
        printf("SEQ legacy_context=0 error=%lu\n", GetLastError());
        if (rc) wglDeleteContext(rc);
        goto done;
    }
    printf("GL vendor=\"%s\" renderer=\"%s\" version=\"%s\"\n", (const char *)glGetString(GL_VENDOR),
           (const char *)glGetString(GL_RENDERER), (const char *)glGetString(GL_VERSION));

    choose_arb = (choose_arb_fn)wglGetProcAddress("wglChoosePixelFormatARB");
    printf("SEQ wglChoosePixelFormatARB=%s\n", choose_arb ? "yes" : "no");
    if (choose_arb)
    {
        attrs[n++] = WGL_DRAW_TO_WINDOW_ARB; attrs[n++] = 1;
        attrs[n++] = WGL_SUPPORT_OPENGL_ARB; attrs[n++] = 1;
        attrs[n++] = WGL_DOUBLE_BUFFER_ARB;  attrs[n++] = 1;
        attrs[n++] = WGL_ACCELERATION_ARB;   attrs[n++] = WGL_FULL_ACCELERATION_ARB;
        attrs[n++] = WGL_PIXEL_TYPE_ARB;     attrs[n++] = WGL_TYPE_RGBA_ARB;
        attrs[n++] = WGL_COLOR_BITS_ARB;     attrs[n++] = arb_color;
        attrs[n++] = WGL_ALPHA_BITS_ARB;     attrs[n++] = 8;
        attrs[n++] = WGL_DEPTH_BITS_ARB;     attrs[n++] = 24;
        attrs[n++] = WGL_STENCIL_BITS_ARB;   attrs[n++] = 8;
        attrs[n] = 0;
        ret = choose_arb(dc, attrs, NULL, 1, &second, &found);
        printf("SEQ arb_choose=%d found=%u\n", ret, found);
        if (ret && found) print_format("SEQ second", dc, second);
    }
    wglMakeCurrent(NULL, NULL);
    wglDeleteContext(rc);

    if (second > 0)
    {
        printf("SEQ same_index=%d\n", second == first);
        SetLastError(0xdeadbeef);
        ret = SetPixelFormat(dc, second, &req);
        printf("SEQ second_set=%d error=%lu getpixelformat=%d\n", ret, ret ? 0 : GetLastError(), GetPixelFormat(dc));
    }

    /* A different format on a window that has one, whatever the above chose. */
    count = DescribePixelFormat(dc, 1, sizeof(req), NULL);
    other = first == 1 ? 2 : 1;
    if (other <= count)
    {
        SetLastError(0xdeadbeef);
        ret = SetPixelFormat(dc, other, &req);
        printf("SEQ other_set index=%d ret=%d error=%lu getpixelformat=%d\n", other, ret,
               ret ? 0 : GetLastError(), GetPixelFormat(dc));
    }

done:
    ReleaseDC(wnd, dc);
    DestroyWindow(wnd);
}

int main(void)
{
    WNDCLASSA cls;

    memset(&cls, 0, sizeof(cls));
    cls.style = CS_OWNDC;
    cls.lpfnWndProc = DefWindowProcA;
    cls.hInstance = GetModuleHandleA(NULL);
    cls.lpszClassName = class_name;
    if (!RegisterClassA(&cls))
    {
        printf("RegisterClass failed %lu\n", GetLastError());
        return 2;
    }
    printf("MEASURE opengl32 pixel format alpha, %d-bit\n", (int)(sizeof(void *) * 8));
    list_formats();
    choose_variants();
    sequence(24, 8, 32);
    sequence(24, 8, 24);
    sequence(32, 8, 32);
    printf("DONE\n");
    return 0;
}
