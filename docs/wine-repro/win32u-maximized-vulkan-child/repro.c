/* Original, AI-assisted (Codex)
 * SPDX-License-Identifier: LGPL-2.1-or-later
 * Public Win32/WGL/Vulkan/D3D12 APIs only; no application code or private interfaces. */
#define COBJMACROS
#define WIDL_C_INLINE_WRAPPERS
#define WIN32_LEAN_AND_MEAN
#define VK_USE_PLATFORM_WIN32_KHR
#define VK_NO_PROTOTYPES
#include <windows.h>
#include <commctrl.h>
#include <uxtheme.h>
#include <shlwapi.h>
#include <vulkan/vulkan.h>
#include <d3d12.h>
#include <dxgi1_4.h>
#include <GL/gl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define VK_FUNCTIONS(X) \
    X(vkDestroyInstance) X(vkCreateWin32SurfaceKHR) \
    X(vkDestroySurfaceKHR) X(vkEnumeratePhysicalDevices) \
    X(vkGetPhysicalDeviceProperties) X(vkGetPhysicalDeviceQueueFamilyProperties) X(vkGetPhysicalDeviceSurfaceSupportKHR) \
    X(vkCreateDevice) X(vkDestroyDevice) X(vkGetDeviceQueue) \
    X(vkGetPhysicalDeviceSurfaceCapabilitiesKHR) X(vkGetPhysicalDeviceSurfaceFormatsKHR) \
    X(vkCreateSwapchainKHR) X(vkDestroySwapchainKHR) X(vkGetSwapchainImagesKHR) \
    X(vkCreateCommandPool) X(vkDestroyCommandPool) X(vkAllocateCommandBuffers) \
    X(vkResetCommandBuffer) X(vkBeginCommandBuffer) X(vkEndCommandBuffer) \
    X(vkCmdPipelineBarrier) X(vkCmdClearColorImage) X(vkCreateSemaphore) \
    X(vkDestroySemaphore) X(vkAcquireNextImageKHR) X(vkQueueSubmit) \
    X(vkQueuePresentKHR) X(vkQueueWaitIdle) X(vkDeviceWaitIdle)
#define DECLARE(name) static PFN_##name name;
VK_FUNCTIONS(DECLARE)
static PFN_vkCreateInstance vkCreateInstance;
static PFN_vkGetInstanceProcAddr get_proc;
static HMODULE loader;
static HWND top, child, status, containers[2];
static int nesting, use_d3d12, use_gl, sibling12, placement_selected;
static HWND child12;
static int use_vulkan, delayed, started, failed, phase = -1, dirty = 1;
static ULONGLONG epoch, frames;
static VkInstance instance;
static VkSurfaceKHR surface;
static VkPhysicalDevice physical;
static VkDevice device;
static VkQueue queue;
static uint32_t family;
static VkSwapchainKHR swapchain;
static VkExtent2D extent;
static VkImage *images;
static VkCommandPool pool;
static VkCommandBuffer command;
static VkSemaphore acquired, *rendered;
static uint32_t image_count;

static int check(VkResult result, const char *operation)
{
    if (result == VK_SUCCESS) return 1;
    printf("ERROR operation=%s VkResult=%d\n", operation, (int)result);
    failed = 1;
    PostMessageW(top, WM_CLOSE, 0, 0);
    return 0;
}
#define CHECK(call) do { if (!check((call), #call)) return 0; } while (0)

/* Legacy WGL only: match the requested window format without extensions. */
static HMODULE gl_module;
static HGLRC (WINAPI *create_gl_context)(HDC);
static BOOL (WINAPI *delete_gl_context)(HGLRC);
static BOOL (WINAPI *make_gl_current)(HDC, HGLRC);
static const GLubyte *(APIENTRY *get_gl_string)(GLenum);
static void (APIENTRY *viewport_gl)(GLint, GLint, GLsizei, GLsizei);
static void (APIENTRY *clear_color_gl)(GLclampf, GLclampf, GLclampf, GLclampf);
static void (APIENTRY *clear_gl)(GLbitfield);
static GLenum (APIENTRY *get_gl_error)(void);
static HDC gl_dc;
static HGLRC gl_context;
static int gl_started;
static ULONGLONG gl_frames;

static int gl_win32_error(const char *operation)
{
    printf("ERROR operation=%s win32=%lu\n", operation, GetLastError());
    return 0;
}

static void cleanup_gl(void)
{
    if (gl_context)
    {
        if (!make_gl_current(NULL, NULL)) { gl_win32_error("wglMakeCurrent(NULL)"); failed = 1; }
        if (!delete_gl_context(gl_context)) { gl_win32_error("wglDeleteContext"); failed = 1; }
        gl_context = NULL;
    }
    if (gl_dc) { ReleaseDC(child, gl_dc); gl_dc = NULL; }
    gl_started = 0;
    if (gl_module) { FreeLibrary(gl_module); gl_module = NULL; }
}

static int init_gl(void)
{
    PIXELFORMATDESCRIPTOR requested = {0}, actual = {0};
    int format;
    gl_module = LoadLibraryW(L"opengl32.dll");
    if (!gl_module) return gl_win32_error("LoadLibrary(opengl32.dll)");
#define LOAD_GL(target, name) do { \
    target = (void *)GetProcAddress(gl_module, name); \
    if (!target) { printf("ERROR missing_gl_entry=%s\n", name); return 0; } \
} while (0)
    LOAD_GL(create_gl_context, "wglCreateContext");
    LOAD_GL(delete_gl_context, "wglDeleteContext");
    LOAD_GL(make_gl_current, "wglMakeCurrent");
    LOAD_GL(get_gl_string, "glGetString");
    LOAD_GL(viewport_gl, "glViewport");
    LOAD_GL(clear_color_gl, "glClearColor");
    LOAD_GL(clear_gl, "glClear");
    LOAD_GL(get_gl_error, "glGetError");
#undef LOAD_GL
    requested.nSize = sizeof(requested); requested.nVersion = 1;
    requested.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    requested.iPixelType = PFD_TYPE_RGBA;
    requested.cColorBits = 32; requested.cDepthBits = 24;
    /* Alpha/stencil remain zero; do not request an ARB format or set it twice. */
    gl_dc = GetDC(child);
    if (!gl_dc) return gl_win32_error("GetDC(GL child)");
    format = ChoosePixelFormat(gl_dc, &requested);
    if (!format) return gl_win32_error("ChoosePixelFormat");
    if (!DescribePixelFormat(gl_dc, format, sizeof(actual), &actual)) return gl_win32_error("DescribePixelFormat");
    printf("GL_FORMAT index=%d requested_color=32 requested_depth=24 requested_alpha=0 "
           "actual_color=%u alpha=%u depth=%u stencil=%u flags=%08lx\n", format,
           actual.cColorBits, actual.cAlphaBits, actual.cDepthBits, actual.cStencilBits, actual.dwFlags);
    if ((actual.dwFlags & requested.dwFlags) != requested.dwFlags || actual.iPixelType != PFD_TYPE_RGBA)
    { puts("ERROR chosen GL format lacks required window/OpenGL/double-buffer capabilities"); return 0; }
    if (!SetPixelFormat(gl_dc, format, &actual)) return gl_win32_error("SetPixelFormat");
    gl_context = create_gl_context(gl_dc);
    if (!gl_context) return gl_win32_error("wglCreateContext");
    if (!make_gl_current(gl_dc, gl_context)) return gl_win32_error("wglMakeCurrent");
    const GLubyte *vendor = get_gl_string(GL_VENDOR), *renderer = get_gl_string(GL_RENDERER), *version = get_gl_string(GL_VERSION);
    if (!vendor || !renderer || !version) { puts("ERROR GL identity unavailable"); return 0; }
    printf("GL_DEVICE vendor=%s renderer=%s version=%s\n", vendor, renderer, version);
    gl_started = 1;
    puts("GL started=1 context=legacy window_format=1 double_buffer=1");
    return 1;
}

static int frame_gl(void)
{
    RECT rect;
    GLenum error;
    GetClientRect(child, &rect);
    if (!rect.right || !rect.bottom) return 1;
    /* Keep this context current while D3D12 is created/presented on this thread. */
    if (!make_gl_current(gl_dc, gl_context)) return gl_win32_error("wglMakeCurrent(frame)");
    viewport_gl(0, 0, rect.right, rect.bottom);
    clear_color_gl(0.08f, 0.26f + (float)((gl_frames / 60) % 4) * 0.025f, 0.10f, 1.0f);
    clear_gl(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    error = get_gl_error();
    if (error != GL_NO_ERROR) { printf("ERROR operation=glClear/Viewport GLenum=%04x\n", error); return 0; }
    if (!SwapBuffers(gl_dc)) return gl_win32_error("SwapBuffers");
    ++gl_frames;
    return 1;
}

static void layout(void);

static void geometry(const char *event)
{
    RECT w, c, v, s;
    GetWindowRect(top, &w); GetClientRect(top, &c);
    GetWindowRect(child12 ? child12 : child, &v); GetWindowRect(status, &s);
    printf("GEOMETRY event=%s phase=%d zoomed=%d window=%ld,%ld,%ld,%ld "
           "client=%ld,%ld,%ld,%ld child_screen=%ld,%ld,%ld,%ld "
           "status_screen=%ld,%ld,%ld,%ld extent=%u,%u frames=%llu\n",
           event, phase, IsZoomed(top), w.left,w.top,w.right,w.bottom,
           c.left,c.top,c.right,c.bottom,v.left,v.top,v.right,v.bottom,
           s.left,s.top,s.right,s.bottom,extent.width,extent.height,
           (unsigned long long)frames);
    if (use_gl)
    {
        RECT g, gc;
        GetWindowRect(child, &g); GetClientRect(child, &gc);
        printf("GL_GEOMETRY phase=%d screen=%ld,%ld,%ld,%ld client=%ld,%ld gl_frames=%llu d3d12_child=%d\n",
               phase,g.left,g.top,g.right,g.bottom,gc.right,gc.bottom,
               (unsigned long long)gl_frames, child12 != NULL);
    }
    for (int i = 0; i < nesting; ++i)
    {
        RECT r;
        GetWindowRect(containers[i], &r);
        printf("NEST level=%d screen=%ld,%ld,%ld,%ld\n", i+1, r.left,r.top,r.right,r.bottom);
    }
}

static void cleanup(void)
{
    if (device) vkDeviceWaitIdle(device);
    if (swapchain) vkDestroySwapchainKHR(device, swapchain, NULL);
    free(images);
    if (acquired) vkDestroySemaphore(device, acquired, NULL);
    for (uint32_t i = 0; i < image_count; ++i)
        if (rendered[i]) vkDestroySemaphore(device, rendered[i], NULL);
    free(rendered);
    if (pool) vkDestroyCommandPool(device, pool, NULL);
    if (device) vkDestroyDevice(device, NULL);
    if (surface) vkDestroySurfaceKHR(instance, surface, NULL);
    if (instance) vkDestroyInstance(instance, NULL);
    if (loader) FreeLibrary(loader);
}

static int init_vulkan(void)
{
    const char *extensions[] = {VK_KHR_SURFACE_EXTENSION_NAME, VK_KHR_WIN32_SURFACE_EXTENSION_NAME};
    const char *device_extension = VK_KHR_SWAPCHAIN_EXTENSION_NAME;
    VkInstanceCreateInfo ic = {0};
    VkWin32SurfaceCreateInfoKHR sc = {0};
    VkDeviceQueueCreateInfo qc = {0};
    VkDeviceCreateInfo dc = {0};
    VkCommandPoolCreateInfo pc = {0};
    VkCommandBufferAllocateInfo ca = {0};
    VkSemaphoreCreateInfo sem = {0};
    VkPhysicalDevice *devices;
    uint32_t count, i, j;
    float priority = 1.0f;
    loader = LoadLibraryW(L"vulkan-1.dll");
    if (!loader) { printf("ERROR LoadLibrary vulkan-1.dll win32=%lu\n", GetLastError()); return 0; }
    get_proc = (PFN_vkGetInstanceProcAddr)(void *)GetProcAddress(loader, "vkGetInstanceProcAddr");
    if (!get_proc) return 0;
    vkCreateInstance = (PFN_vkCreateInstance)get_proc(NULL, "vkCreateInstance");
    if (!vkCreateInstance) return 0;
    ic.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    ic.enabledExtensionCount = 2; ic.ppEnabledExtensionNames = extensions;
    CHECK(vkCreateInstance(&ic, NULL, &instance));
#define LOAD(name) name = (PFN_##name)get_proc(instance, #name); \
    if (!name) { printf("ERROR missing_entry=%s\n", #name); return 0; }
    VK_FUNCTIONS(LOAD)
#undef LOAD
    sc.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    sc.hinstance = GetModuleHandleW(NULL); sc.hwnd = child;
    CHECK(vkCreateWin32SurfaceKHR(instance, &sc, NULL, &surface));
    CHECK(vkEnumeratePhysicalDevices(instance, &count, NULL));
    devices = calloc(count ? count : 1, sizeof(*devices));
    if (!devices) return 0;
    { VkResult r = vkEnumeratePhysicalDevices(instance, &count, devices);
      if (!check(r, "vkEnumeratePhysicalDevices")) { free(devices); return 0; } }
    for (i = 0; i < count && !physical; ++i)
    {
        uint32_t n = 0;
        VkQueueFamilyProperties *props;
        vkGetPhysicalDeviceQueueFamilyProperties(devices[i], &n, NULL);
        props = calloc(n ? n : 1, sizeof(*props));
        if (!props) { free(devices); return 0; }
        vkGetPhysicalDeviceQueueFamilyProperties(devices[i], &n, props);
        for (j = 0; j < n; ++j)
        {
            VkBool32 present = VK_FALSE;
            VkResult r = vkGetPhysicalDeviceSurfaceSupportKHR(devices[i], j, surface, &present);
            if (r != VK_SUCCESS) printf("DEVICE_PROBE VkResult=%d\n", (int)r);
            if (r == VK_SUCCESS && present && (props[j].queueFlags & VK_QUEUE_GRAPHICS_BIT))
            { physical = devices[i]; family = j; break; }
        }
        free(props);
    }
    free(devices);
    if (!physical) { puts("ERROR no graphics+present queue"); return 0; }
    {
        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceProperties(physical, &properties);
        printf("DEVICE name=%s vendor=%04x device=%04x queue_family=%u\n",
               properties.deviceName, properties.vendorID, properties.deviceID, family);
    }
    qc.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    qc.queueFamilyIndex = family; qc.queueCount = 1; qc.pQueuePriorities = &priority;
    dc.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    dc.queueCreateInfoCount = 1; dc.pQueueCreateInfos = &qc;
    dc.enabledExtensionCount = 1; dc.ppEnabledExtensionNames = &device_extension;
    CHECK(vkCreateDevice(physical, &dc, NULL, &device));
    vkGetDeviceQueue(device, family, 0, &queue);
    pc.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    pc.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT; pc.queueFamilyIndex = family;
    CHECK(vkCreateCommandPool(device, &pc, NULL, &pool));
    ca.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    ca.commandPool = pool; ca.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY; ca.commandBufferCount = 1;
    CHECK(vkAllocateCommandBuffers(device, &ca, &command));
    sem.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    CHECK(vkCreateSemaphore(device, &sem, NULL, &acquired));
    started = 1;
    puts("VULKAN started=1");
    return 1;
}

static int resize_swapchain(void)
{
    VkSurfaceCapabilitiesKHR caps;
    VkSurfaceFormatKHR *formats, format;
    VkSwapchainCreateInfoKHR info = {0};
    uint32_t n, count, alpha;
    RECT rect;
    GetClientRect(child, &rect);
    if (!rect.right || !rect.bottom) return 1;
    CHECK(vkDeviceWaitIdle(device));
    CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical, surface, &caps));
    if (!(caps.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT))
    { puts("ERROR surface does not support TRANSFER_DST clear"); return 0; }
    CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(physical, surface, &n, NULL));
    if (!n) return 0;
    formats = calloc(n, sizeof(*formats));
    if (!formats) return 0;
    { VkResult r = vkGetPhysicalDeviceSurfaceFormatsKHR(physical, surface, &n, formats);
      if (!check(r, "vkGetPhysicalDeviceSurfaceFormatsKHR")) { free(formats); return 0; } }
    format = formats[0]; free(formats);
    if (format.format == VK_FORMAT_UNDEFINED) format.format = VK_FORMAT_B8G8R8A8_UNORM;
    extent = caps.currentExtent;
    if (extent.width == UINT32_MAX)
    {
        extent.width = (uint32_t)rect.right; extent.height = (uint32_t)rect.bottom;
#define CLAMP(member) if (extent.member < caps.minImageExtent.member) extent.member = caps.minImageExtent.member; \
        if (extent.member > caps.maxImageExtent.member) extent.member = caps.maxImageExtent.member;
        CLAMP(width) CLAMP(height)
#undef CLAMP
    }
    if (!extent.width || !extent.height) return 1;
    if (swapchain) { vkDestroySwapchainKHR(device, swapchain, NULL); swapchain = VK_NULL_HANDLE; }
    free(images); images = NULL;
    for (uint32_t i = 0; i < image_count; ++i)
        if (rendered[i]) vkDestroySemaphore(device, rendered[i], NULL);
    free(rendered); rendered = NULL; image_count = 0;
    count = caps.minImageCount + 1;
    if (caps.maxImageCount && count > caps.maxImageCount) count = caps.maxImageCount;
    for (alpha = 1; !(alpha & caps.supportedCompositeAlpha); alpha <<= 1) {}
    info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    info.surface = surface; info.minImageCount = count; info.imageFormat = format.format;
    info.imageColorSpace = format.colorSpace; info.imageExtent = extent; info.imageArrayLayers = 1;
    info.imageUsage = VK_IMAGE_USAGE_TRANSFER_DST_BIT; info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    info.preTransform = caps.currentTransform; info.compositeAlpha = (VkCompositeAlphaFlagBitsKHR)alpha;
    info.presentMode = VK_PRESENT_MODE_FIFO_KHR; info.clipped = VK_TRUE;
    CHECK(vkCreateSwapchainKHR(device, &info, NULL, &swapchain));
    CHECK(vkGetSwapchainImagesKHR(device, swapchain, &n, NULL));
    images = calloc(n, sizeof(*images));
    if (!images) return 0;
    CHECK(vkGetSwapchainImagesKHR(device, swapchain, &n, images));
    rendered = calloc(n, sizeof(*rendered));
    if (!rendered) return 0;
    image_count = n;
    for (uint32_t i = 0; i < n; ++i)
    {
        VkSemaphoreCreateInfo sem = {0};
        sem.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        CHECK(vkCreateSemaphore(device, &sem, NULL, &rendered[i]));
    }
    dirty = 0; geometry("swapchain");
    return 1;
}

static int frame(void)
{
    uint32_t index;
    VkResult result;
    VkCommandBufferBeginInfo begin = {0};
    VkImageMemoryBarrier barrier = {0};
    VkClearColorValue color = {{0.06f, 0.17f, 0.28f, 1.0f}};
    VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    VkSubmitInfo submit = {0};
    VkPresentInfoKHR present = {0};
    if (dirty && !resize_swapchain()) return 0;
    if (dirty || !swapchain) return 1;
    result = vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, acquired, VK_NULL_HANDLE, &index);
    if (result == VK_ERROR_OUT_OF_DATE_KHR) { puts("VULKAN acquire=OUT_OF_DATE"); dirty = 1; return 1; }
    if (result != VK_SUBOPTIMAL_KHR && !check(result, "vkAcquireNextImageKHR")) return 0;
    if (result == VK_SUBOPTIMAL_KHR) { puts("VULKAN acquire=SUBOPTIMAL"); dirty = 1; }
    CHECK(vkResetCommandBuffer(command, 0));
    begin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    CHECK(vkBeginCommandBuffer(command, &begin));
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED; /* discard previous clear */
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.srcQueueFamilyIndex = barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = images[index]; barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.levelCount = barrier.subresourceRange.layerCount = 1;
    vkCmdPipelineBarrier(command, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                         0, 0, NULL, 0, NULL, 1, &barrier);
    /* Slow visible change confirms presents continue across state transitions. */
    color.float32[0] += (float)((frames / 60) % 4) * 0.025f;
    vkCmdClearColorImage(command, images[index], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                        &color, 1, &barrier.subresourceRange);
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT; barrier.dstAccessMask = 0;
    vkCmdPipelineBarrier(command, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                         0, 0, NULL, 0, NULL, 1, &barrier);
    CHECK(vkEndCommandBuffer(command));
    submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit.waitSemaphoreCount = 1; submit.pWaitSemaphores = &acquired; submit.pWaitDstStageMask = &wait_stage;
    submit.commandBufferCount = 1; submit.pCommandBuffers = &command;
    submit.signalSemaphoreCount = 1; submit.pSignalSemaphores = &rendered[index];
    CHECK(vkQueueSubmit(queue, 1, &submit, VK_NULL_HANDLE));
    present.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present.waitSemaphoreCount = 1; present.pWaitSemaphores = &rendered[index];
    present.swapchainCount = 1; present.pSwapchains = &swapchain; present.pImageIndices = &index;
    result = vkQueuePresentKHR(queue, &present);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
    { printf("VULKAN present VkResult=%d\n", (int)result); dirty = 1; }
    else if (!check(result, "vkQueuePresentKHR")) return 0;
    /* Queue idle protects command buffer and acquire semaphore reuse.
     * Present waits use per-image semaphores, reused after reacquisition. */
    CHECK(vkQueueWaitIdle(queue));
    ++frames;
    return 1;
}

/* One D3D12 frame in flight, no shaders or private interfaces. */
static HMODULE d3d_module, dxgi_module;
static IDXGIFactory2 *factory12;
static ID3D12Device *device12;
static ID3D12CommandQueue *queue12;
static IDXGISwapChain3 *swap12;
static ID3D12CommandAllocator *allocator12;
static ID3D12GraphicsCommandList *list12;
static ID3D12DescriptorHeap *heap12;
static ID3D12Resource *buffers12[2];
static ID3D12Fence *fence12;
static HANDLE event12;
static UINT64 serial12;
static UINT stride12;
static int occluded12;
static ULONGLONG occluded_frames12;

static int hrcheck(HRESULT hr, const char *operation)
{
    if (SUCCEEDED(hr)) return 1;
    printf("ERROR operation=%s HRESULT=%08lx\n", operation, (unsigned long)hr);
    if (device12)
        printf("D3D12 removed_reason=%08lx\n", (unsigned long)ID3D12Device_GetDeviceRemovedReason(device12));
    return 0;
}
#define HR(call) do { if (!hrcheck((call), #call)) return 0; } while (0)
#define RELEASE12(p) do { if (p) { IUnknown_Release((IUnknown *)(p)); (p) = NULL; } } while (0)

static int drain12(void)
{
    UINT64 done;
    HR(ID3D12CommandQueue_Signal(queue12, fence12, ++serial12));
    done = ID3D12Fence_GetCompletedValue(fence12);
    if (done == UINT64_MAX) { puts("ERROR D3D12 fence device removed"); return 0; }
    if (done < serial12)
    {
        DWORD wait;
        HR(ID3D12Fence_SetEventOnCompletion(fence12, serial12, event12));
        wait = WaitForSingleObject(event12, 5000);
        if (wait != WAIT_OBJECT_0)
        { printf("ERROR D3D12 fence_wait=%lu win32=%lu\n", wait, GetLastError()); return 0; }
    }
    return 1;
}

static void cleanup12(void)
{
    if (queue12 && fence12 && event12 && !drain12()) failed = 1;
    RELEASE12(list12); RELEASE12(allocator12);
    for (unsigned int i = 0; i < 2; ++i) RELEASE12(buffers12[i]);
    RELEASE12(heap12); RELEASE12(swap12); RELEASE12(fence12);
    RELEASE12(queue12); RELEASE12(device12); RELEASE12(factory12);
    if (event12) CloseHandle(event12);
    if (d3d_module) FreeLibrary(d3d_module);
    if (dxgi_module) FreeLibrary(dxgi_module);
}

static int targets12(void)
{
    D3D12_CPU_DESCRIPTOR_HANDLE handle = ID3D12DescriptorHeap_GetCPUDescriptorHandleForHeapStart(heap12);
    for (UINT i = 0; i < 2; ++i)
    {
        HR(IDXGISwapChain3_GetBuffer(swap12, i, &IID_ID3D12Resource, (void **)&buffers12[i]));
        ID3D12Device_CreateRenderTargetView(device12, buffers12[i], NULL, handle);
        handle.ptr += stride12;
    }
    return 1;
}

static int init12(void)
{
    HRESULT (WINAPI *create_device)(IUnknown *, D3D_FEATURE_LEVEL, REFIID, void **);
    HRESULT (WINAPI *create_factory)(REFIID, void **);
    D3D12_COMMAND_QUEUE_DESC q = {0};
    D3D12_DESCRIPTOR_HEAP_DESC h = {0};
    DXGI_SWAP_CHAIN_DESC1 desc = {0};
    IDXGISwapChain1 *chain;
    RECT rect;
    if (use_gl)
    {
        child12 = CreateWindowW(L"SyntheticChild", NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS,
                                0,0,1,1,sibling12 ? GetParent(child) : child,NULL,GetModuleHandleW(NULL),NULL);
        if (!child12) return gl_win32_error("CreateWindow(D3D12 child)");
        layout();
        printf("D3D12_WINDOW placement=%s created_at_ms=%llu\n", sibling12 ? "sibling" : "child",
               (unsigned long long)(GetTickCount64() - epoch));
    }
    else child12 = child;
    dxgi_module = LoadLibraryW(L"dxgi.dll");
    d3d_module = LoadLibraryW(L"d3d12.dll");
    if (!dxgi_module || !d3d_module)
    { printf("ERROR D3D12 LoadLibrary win32=%lu\n", GetLastError()); return 0; }
    create_device = (void *)GetProcAddress(d3d_module, "D3D12CreateDevice");
    create_factory = (void *)GetProcAddress(dxgi_module, "CreateDXGIFactory1");
    if (!create_device || !create_factory) { puts("ERROR D3D12 missing entry point"); return 0; }
    HR(create_factory(&IID_IDXGIFactory2, (void **)&factory12));
    for (UINT i = 0; !device12; ++i)
    {
        IDXGIAdapter1 *adapter = NULL;
        DXGI_ADAPTER_DESC1 adapter_desc;
        HRESULT hr = IDXGIFactory2_EnumAdapters1(factory12, i, &adapter);
        if (hr == DXGI_ERROR_NOT_FOUND) break;
        HR(hr);
        hr = IDXGIAdapter1_GetDesc1(adapter, &adapter_desc);
        if (FAILED(hr)) { RELEASE12(adapter); return hrcheck(hr, "GetDesc1"); }
        if (!(adapter_desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE))
        {
            hr = create_device((IUnknown *)adapter, D3D_FEATURE_LEVEL_11_0,
                               &IID_ID3D12Device, (void **)&device12);
            printf("D3D12 adapter=%u vendor=%04x device=%04x create_hresult=%08lx\n",
                   i, adapter_desc.VendorId, adapter_desc.DeviceId, (unsigned long)hr);
            if (device12)
            {
                char name[256];
                if (!WideCharToMultiByte(CP_UTF8, 0, adapter_desc.Description, -1, name, sizeof(name), NULL, NULL))
                    strcpy(name, "unavailable");
                printf("DEVICE api=d3d12 name=%s vendor=%04x device=%04x\n",
                       name, adapter_desc.VendorId, adapter_desc.DeviceId);
            }
        }
        RELEASE12(adapter);
    }
    if (!device12) { puts("ERROR no hardware D3D12 device (no WARP fallback)"); return 0; }
    q.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    HR(ID3D12Device_CreateCommandQueue(device12, &q, &IID_ID3D12CommandQueue, (void **)&queue12));
    HR(ID3D12Device_CreateCommandAllocator(device12, q.Type, &IID_ID3D12CommandAllocator, (void **)&allocator12));
    HR(ID3D12Device_CreateCommandList(device12, 0, q.Type, allocator12, NULL,
                                   &IID_ID3D12GraphicsCommandList, (void **)&list12));
    HR(ID3D12GraphicsCommandList_Close(list12));
    h.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV; h.NumDescriptors = 2;
    HR(ID3D12Device_CreateDescriptorHeap(device12, &h, &IID_ID3D12DescriptorHeap, (void **)&heap12));
    stride12 = ID3D12Device_GetDescriptorHandleIncrementSize(device12, h.Type);
    HR(ID3D12Device_CreateFence(device12, 0, D3D12_FENCE_FLAG_NONE, &IID_ID3D12Fence, (void **)&fence12));
    event12 = CreateEventW(NULL, FALSE, FALSE, NULL);
    if (!event12) { printf("ERROR D3D12 CreateEvent win32=%lu\n", GetLastError()); return 0; }
    GetClientRect(child12, &rect);
    desc.Width = rect.right; desc.Height = rect.bottom;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; desc.SampleDesc.Count = 1;
    desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT; desc.BufferCount = 2;
    desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    desc.Scaling = DXGI_SCALING_STRETCH; desc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
    HR(IDXGIFactory2_CreateSwapChainForHwnd(factory12, (IUnknown *)queue12, child12, &desc, NULL, NULL, &chain));
    HRESULT hr = IDXGISwapChain1_QueryInterface(chain, &IID_IDXGISwapChain3, (void **)&swap12);
    IDXGISwapChain1_Release(chain);
    HR(hr);
    HR(IDXGIFactory2_MakeWindowAssociation(factory12, top, DXGI_MWA_NO_ALT_ENTER));
    if (!targets12()) return 0;
    extent.width = desc.Width; extent.height = desc.Height;
    dirty = 0; started = 1;
    puts("D3D12 started=1 buffers=2 swap_effect=FLIP_DISCARD sync_interval=1");
    geometry("swapchain");
    return 1;
}

static int frame12(void)
{
    RECT rect;
    D3D12_RESOURCE_BARRIER barrier = {0};
    D3D12_CPU_DESCRIPTOR_HANDLE handle;
    ID3D12CommandList *commands;
    FLOAT color[4] = {0.06f, 0.17f, 0.28f, 1.0f};
    UINT index;
    HRESULT hr;
    GetClientRect(child12, &rect);
    if (!rect.right || !rect.bottom) return 1;
    if (dirty)
    {
        if (!drain12()) return 0;
        /* Drop recorded references before ResizeBuffers. */
        HR(ID3D12CommandAllocator_Reset(allocator12));
        HR(ID3D12GraphicsCommandList_Reset(list12, allocator12, NULL));
        HR(ID3D12GraphicsCommandList_Close(list12));
        for (UINT i = 0; i < 2; ++i) RELEASE12(buffers12[i]);
        HR(IDXGISwapChain3_ResizeBuffers(swap12, 2, rect.right, rect.bottom, DXGI_FORMAT_R8G8B8A8_UNORM, 0));
        if (!targets12()) return 0;
        extent.width = rect.right; extent.height = rect.bottom;
        dirty = 0; geometry("swapchain");
    }
    index = IDXGISwapChain3_GetCurrentBackBufferIndex(swap12);
    HR(ID3D12CommandAllocator_Reset(allocator12));
    HR(ID3D12GraphicsCommandList_Reset(list12, allocator12, NULL));
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = buffers12[index];
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
    ID3D12GraphicsCommandList_ResourceBarrier(list12, 1, &barrier);
    handle = ID3D12DescriptorHeap_GetCPUDescriptorHandleForHeapStart(heap12);
    handle.ptr += index * stride12;
    color[0] += (FLOAT)((frames / 60) % 4) * 0.025f;
    ID3D12GraphicsCommandList_ClearRenderTargetView(list12, handle, color, 0, NULL);
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
    ID3D12GraphicsCommandList_ResourceBarrier(list12, 1, &barrier);
    HR(ID3D12GraphicsCommandList_Close(list12));
    commands = (ID3D12CommandList *)list12;
    ID3D12CommandQueue_ExecuteCommandLists(queue12, 1, &commands);
    hr = IDXGISwapChain3_Present(swap12, 1, 0);
    HR(hr);
    if (!drain12()) return 0;
    if (hr == DXGI_STATUS_OCCLUDED)
    {
        if (!occluded12) puts("D3D12 present=OCCLUDED visual_check_incomplete=1");
        occluded12 = 1; ++occluded_frames12;
    }
    else
    {
        if (occluded12) puts("D3D12 present=VISIBLE");
        occluded12 = 0;
        ++frames;
    }
    return 1;
}

static LRESULT CALLBACK status_proc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp,
                                    UINT_PTR id, DWORD_PTR data)
{
    (void)id; (void)data;
    if (msg == WM_ERASEBKGND)
    {
        RECT r;
        HDC dc = (HDC)wp;
        COLORREF old;
        GetClientRect(hwnd, &r);
        /* light, so the theme's dark status text stays readable */
        old = SetBkColor(dc, RGB(200,208,216));
        ExtTextOutW(dc, 0, 0, ETO_OPAQUE, &r, NULL, 0, NULL);
        SetBkColor(dc, old);
        return 1;
    }
    return DefSubclassProc(hwnd, msg, wp, lp);
}

static LRESULT CALLBACK child_proc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    if (msg == WM_PAINT)
    {
        PAINTSTRUCT ps;
        HDC dc = BeginPaint(hwnd, &ps);
        if (hwnd != child12 && !(hwnd == child && (started || gl_started)))
        {
            RECT r; GetClientRect(hwnd, &r);
            FillRect(dc, &r, (HBRUSH)GetStockObject(DKGRAY_BRUSH));
            SetTextColor(dc, RGB(255,255,255)); SetBkMode(dc, TRANSPARENT);
            TextOutW(dc, 20, 20, L"GDI / renderer not started", 25);
        }
        EndPaint(hwnd, &ps); return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

static void layout(void)
{
    RECT c, s;
    int x, y, width, height;
    SendMessageW(status, WM_SIZE, 0, 0);
    GetClientRect(top, &c); GetWindowRect(status, &s);
    width = c.right; height = c.bottom > s.bottom-s.top ? c.bottom-(s.bottom-s.top) : 0;
    for (int i = 0; i < nesting; ++i)
    {
        MoveWindow(containers[i], i ? 7 : 0, i ? 9 : 0, width, height, TRUE);
        width = width > 14 ? width - 14 : 0;
        height = height > 18 ? height - 18 : 0;
    }
    x = nesting ? 7 : 0; y = nesting ? 9 : 0;
    if (use_gl && use_d3d12 && sibling12)
    {
        /* Reserve the right half before D3D12 starts: GL size stays stable at 10s. */
        MoveWindow(child, x, y, width/2, height, TRUE);
        if (child12) MoveWindow(child12, x+width/2, y, width-width/2, height, TRUE);
    }
    else
    {
        MoveWindow(child, x, y, width, height, TRUE);
        if (use_gl && child12) MoveWindow(child12, width/4, height/4, width/2, height/2, TRUE);
    }
    dirty = 1;
}

static void close_window(HWND hwnd)
{
    /* WGL owns the child DC; release it before DestroyWindow destroys children. */
    cleanup_gl();
    DestroyWindow(hwnd);
}

static LRESULT CALLBACK window_proc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    if (msg == WM_CLOSE) { close_window(hwnd); return 0; }
    if (msg == WM_SIZE && status) { layout(); return 0; }
    if (msg == WM_TIMER)
    {
        if (wp == 2)
        {
            static unsigned int text;
            SendMessageW(status, SB_SETTEXTW, 0, (LPARAM)((text++ & 1) ? L"Ready" :
                         L"Synthetic long status text: move, maximize, restore; old letters must disappear."));
        }
        else
        {
            ULONGLONG elapsed = GetTickCount64() - epoch;
            int next = elapsed < 5000 ? 0 : elapsed < 15000 ? 1 : elapsed < 20000 ? 2 : 3;
            const wchar_t *titles[] = {L"restored 0-5s", L"maximized 5-15s", L"restored 15-20s", L"maximized 20-30s"};
            wchar_t title[160];
            if (elapsed >= 30000) { close_window(hwnd); return 0; }
            if (next != phase)
            {
                phase = next;
                swprintf(title, 160, L"Synthetic repaint [%ls] %ls%ls", use_gl ? (use_d3d12 ? L"GL+D3D12" : L"GL") : use_d3d12 ? L"D3D12" : use_vulkan ? L"Vulkan" : L"GDI",
                         titles[phase], delayed ? (use_d3d12 ? L" (D3D12 starts at 10s)" : L" (Vulkan starts at 10s)") : L"");
                SetWindowTextW(hwnd, title);
                ShowWindow(hwnd, (phase == 1 || phase == 3) ? SW_MAXIMIZE : SW_RESTORE);
                geometry("phase");
            }
            if (!IsIconic(hwnd) && gl_started && !frame_gl())
            { failed = 1; close_window(hwnd); return 0; }
            if ((use_vulkan || use_d3d12) && !started && (!delayed || elapsed >= 10000))
                if (!(use_d3d12 ? init12() : init_vulkan())) { failed = 1; close_window(hwnd); return 0; }
            if (started && !IsIconic(hwnd) && !(use_d3d12 ? frame12() : frame())) { failed = 1; close_window(hwnd); }
        }
        return 0;
    }
    if (msg == WM_DESTROY) { PostQuitMessage(0); return 0; }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

int main(int argc, char **argv)
{
    WNDCLASSW wc = {0};
    INITCOMMONCONTROLSEX controls = {sizeof(controls), ICC_BAR_CLASSES};
    MSG msg;
    int i, mode = 0, result;
    setvbuf(stdout, NULL, _IONBF, 0);
    for (i = 1; i < argc; ++i)
    {
        if (!strcmp(argv[i], "--none") && !mode) { mode = 1; use_vulkan = 0; }
        else if (!strcmp(argv[i], "--vulkan") && !mode) { mode = 1; use_vulkan = 1; }
        else if (!strcmp(argv[i], "--d3d12") && !mode) { mode = 1; use_d3d12 = 1; }
        else if (!strcmp(argv[i], "--gl") && !use_gl) use_gl = 1;
        else if (!strcmp(argv[i], "--d3d12-placement=child") && !placement_selected) { placement_selected = 1; sibling12 = 0; }
        else if (!strcmp(argv[i], "--d3d12-placement=sibling") && !placement_selected) { placement_selected = 1; sibling12 = 1; }
        else if (!strcmp(argv[i], "--delay")) delayed = 1;
        else if (!strcmp(argv[i], "--delay-vulkan")) delayed = 2;
        else if (!strcmp(argv[i], "--nest=0")) nesting = 0;
        else if (!strcmp(argv[i], "--nest=1")) nesting = 1;
        else if (!strcmp(argv[i], "--nest=2")) nesting = 2;
        else { puts("Usage: maximized-vulkan-child.exe (--none | --vulkan | --d3d12 | --gl [--d3d12]) [--d3d12-placement=child|sibling] [--delay] [--nest=0|1|2]"); return 2; }
    }
    if ((!mode && !use_gl) || (use_gl && mode && !use_d3d12) ||
        (placement_selected && !(use_gl && use_d3d12)) ||
        (delayed && !use_vulkan && !use_d3d12) || (delayed == 2 && !use_vulkan))
    { puts("ERROR invalid mode combination: GL combines only with D3D12; placement requires both; delay requires Vulkan/D3D12"); return 2; }
    if (!InitCommonControlsEx(&controls)) return 2;
    wc.hInstance = GetModuleHandleW(NULL); wc.hCursor = LoadCursorW(NULL, (LPCWSTR)IDC_ARROW);
    wc.hbrBackground = use_gl ? (HBRUSH)(COLOR_APPWORKSPACE+1) : NULL;
    wc.lpfnWndProc = window_proc; wc.lpszClassName = L"SyntheticTop";
    if (!RegisterClassW(&wc)) return 2;
    wc.lpfnWndProc = child_proc; wc.lpszClassName = L"SyntheticChild";
    if (!RegisterClassW(&wc)) return 2;
    wc.style = CS_OWNDC; wc.lpszClassName = L"SyntheticGL";
    if (!RegisterClassW(&wc)) return 2;
    top = CreateWindowW(L"SyntheticTop", L"Synthetic repaint", WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
                        60,60,1000,700,NULL,NULL,wc.hInstance,NULL);
    if (!top) return 2;
    HWND parent = top;
    for (i = 0; i < nesting; ++i)
    {
        containers[i] = CreateWindowW(L"SyntheticChild", NULL, WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN,
                                      0,0,1,1,parent,NULL,wc.hInstance,NULL);
        if (!containers[i]) return 2;
        parent = containers[i];
    }
    child = CreateWindowW(use_gl ? L"SyntheticGL" : L"SyntheticChild", NULL,
                          WS_CHILD | WS_VISIBLE | (use_gl ? WS_CLIPCHILDREN | WS_CLIPSIBLINGS : 0), 0,0,1,1,parent,NULL,wc.hInstance,NULL);
    status = CreateWindowW(STATUSCLASSNAMEW, NULL, WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP,
                          0,0,0,0,top,NULL,wc.hInstance,NULL);
    if (!child || !status || !SetWindowSubclass(status, status_proc, 1, 0)) return 2;
    {
        DLLVERSIONINFO version = {0};
        HMODULE module = NULL;
        HRESULT (CALLBACK *get_version)(DLLVERSIONINFO *) = NULL;
        /* A global class may have no class HINSTANCE (Wine): resolve the
           module of the class procedure, as the status bar MEASURE does. */
        if (GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                               (LPCWSTR)(ULONG_PTR)GetClassLongPtrW(status, GCLP_WNDPROC), &module))
            get_version = (void *)GetProcAddress(module, "DllGetVersion");
        version.cbSize = sizeof(version);
        if (!get_version || FAILED(get_version(&version)) || version.dwMajorVersion != 6)
        { puts("ERROR status class is not comctl32 v6"); DestroyWindow(top); return 2; }
        printf("CONTROL version=%lu.%lu.%lu themed=%d\n", version.dwMajorVersion,
               version.dwMinorVersion, version.dwBuildNumber, GetWindowTheme(status) != NULL);
    }
    SendMessageW(status, SB_SETTEXTW, 0, (LPARAM)L"Ready");
    printf("CONFIG mode=%s delay=%d nesting=%d placement=%s\n",
           use_gl ? (use_d3d12 ? "gl+d3d12" : "gl") : use_d3d12 ? "d3d12" : use_vulkan ? "vulkan" : "none",
           !!delayed, nesting, use_gl && use_d3d12 ? (sibling12 ? "sibling" : "child") : "none");
    puts("START duration_ms=30000 text_period_ms=250 render_timer_ms=16 visual_check_required=1");
    ShowWindow(top, SW_SHOWNORMAL);
    if (use_gl && !init_gl()) { close_window(top); return 2; }
    epoch = GetTickCount64();
    if (!SetTimer(top, 1, 16, NULL) || !SetTimer(top, 2, 250, NULL))
    { gl_win32_error("SetTimer"); close_window(top); return 2; }
    while ((result = GetMessageW(&msg, NULL, 0, 0)) > 0) { TranslateMessage(&msg); DispatchMessageW(&msg); }
    if (result == -1) failed = 1;
    cleanup_gl();
    cleanup12();
    cleanup();
    if (use_d3d12) printf("D3D12 occluded_presents=%llu\n", (unsigned long long)occluded_frames12);
    if (use_gl) printf("GL frames=%llu\n", (unsigned long long)gl_frames);
    if (use_gl && !gl_frames) failed = 1;
    if ((use_vulkan || use_d3d12) && !frames) failed = 1;
    printf("SUMMARY failed=%d frames=%llu gl_frames=%llu completed_timeline=%d\n", failed,
           (unsigned long long)frames, (unsigned long long)gl_frames, GetTickCount64()-epoch >= 30000);
    return failed || GetTickCount64()-epoch < 30000 ? 2 : 0;
}
