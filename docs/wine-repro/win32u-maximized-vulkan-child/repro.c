/* Original, AI-assisted (Codex)
 * SPDX-License-Identifier: LGPL-2.1-or-later
 * Public Win32/Vulkan APIs only; no application code or private interfaces. */
#define WIN32_LEAN_AND_MEAN
#define VK_USE_PLATFORM_WIN32_KHR
#define VK_NO_PROTOTYPES
#include <windows.h>
#include <commctrl.h>
#include <uxtheme.h>
#include <shlwapi.h>
#include <vulkan/vulkan.h>
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
static HWND top, child, status;
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

static void geometry(const char *event)
{
    RECT w, c, v, s;
    GetWindowRect(top, &w); GetClientRect(top, &c);
    GetWindowRect(child, &v); GetWindowRect(status, &s);
    printf("GEOMETRY event=%s phase=%d zoomed=%d window=%ld,%ld,%ld,%ld "
           "client=%ld,%ld,%ld,%ld child_screen=%ld,%ld,%ld,%ld "
           "status_screen=%ld,%ld,%ld,%ld extent=%u,%u frames=%llu\n",
           event, phase, IsZoomed(top), w.left,w.top,w.right,w.bottom,
           c.left,c.top,c.right,c.bottom,v.left,v.top,v.right,v.bottom,
           s.left,s.top,s.right,s.bottom,extent.width,extent.height,
           (unsigned long long)frames);
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
        if (!started)
        {
            RECT r; GetClientRect(hwnd, &r);
            FillRect(dc, &r, (HBRUSH)GetStockObject(DKGRAY_BRUSH));
            SetTextColor(dc, RGB(255,255,255)); SetBkMode(dc, TRANSPARENT);
            TextOutW(dc, 20, 20, L"GDI control / Vulkan not started", 31);
        }
        EndPaint(hwnd, &ps); return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

static LRESULT CALLBACK window_proc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    if (msg == WM_SIZE && status)
    {
        RECT c, s;
        SendMessageW(status, WM_SIZE, 0, 0);
        GetClientRect(hwnd, &c); GetWindowRect(status, &s);
        MoveWindow(child, 0, 0, c.right, c.bottom > s.bottom-s.top ? c.bottom-(s.bottom-s.top) : 0, TRUE);
        dirty = 1;
        return 0;
    }
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
            if (elapsed >= 30000) { DestroyWindow(hwnd); return 0; }
            if (next != phase)
            {
                phase = next;
                swprintf(title, 160, L"Synthetic repaint [%ls] %ls%ls", use_vulkan ? L"Vulkan" : L"GDI",
                         titles[phase], delayed ? L" (Vulkan starts at 10s)" : L"");
                SetWindowTextW(hwnd, title);
                ShowWindow(hwnd, (phase == 1 || phase == 3) ? SW_MAXIMIZE : SW_RESTORE);
                geometry("phase");
            }
            if (use_vulkan && !started && (!delayed || elapsed >= 10000))
                if (!init_vulkan()) { failed = 1; DestroyWindow(hwnd); return 0; }
            if (started && !IsIconic(hwnd) && !frame()) { failed = 1; DestroyWindow(hwnd); }
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
        else if (!strcmp(argv[i], "--delay-vulkan")) delayed = 1;
        else { puts("Usage: maximized-vulkan-child.exe (--none | --vulkan) [--delay-vulkan]"); return 2; }
    }
    if (!mode || (delayed && !use_vulkan)) { puts("ERROR choose one mode; delay requires Vulkan"); return 2; }
    if (!InitCommonControlsEx(&controls)) return 2;
    wc.hInstance = GetModuleHandleW(NULL); wc.hCursor = LoadCursorW(NULL, (LPCWSTR)IDC_ARROW);
    wc.lpfnWndProc = window_proc; wc.lpszClassName = L"SyntheticTop";
    if (!RegisterClassW(&wc)) return 2;
    wc.lpfnWndProc = child_proc; wc.lpszClassName = L"SyntheticChild";
    if (!RegisterClassW(&wc)) return 2;
    top = CreateWindowW(L"SyntheticTop", L"Synthetic repaint", WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
                        60,60,1000,700,NULL,NULL,wc.hInstance,NULL);
    if (!top) return 2;
    child = CreateWindowW(L"SyntheticChild", NULL, WS_CHILD | WS_VISIBLE, 0,0,1,1,top,NULL,wc.hInstance,NULL);
    status = CreateWindowW(STATUSCLASSNAMEW, NULL, WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP,
                          0,0,0,0,top,NULL,wc.hInstance,NULL);
    if (!child || !status || !SetWindowSubclass(status, status_proc, 1, 0)) return 2;
    {
        DLLVERSIONINFO version = {0};
        HMODULE module = (HMODULE)GetClassLongPtrW(status, GCLP_HMODULE);
        HRESULT (CALLBACK *get_version)(DLLVERSIONINFO *) =
            (void *)GetProcAddress(module, "DllGetVersion");
        version.cbSize = sizeof(version);
        if (!get_version || FAILED(get_version(&version)) || version.dwMajorVersion != 6)
        { puts("ERROR status class is not comctl32 v6"); DestroyWindow(top); return 2; }
        printf("CONTROL version=%lu.%lu.%lu themed=%d\n", version.dwMajorVersion,
               version.dwMinorVersion, version.dwBuildNumber, GetWindowTheme(status) != NULL);
    }
    SendMessageW(status, SB_SETTEXTW, 0, (LPARAM)L"Ready");
    puts("START duration_ms=30000 text_period_ms=250 render_timer_ms=16 visual_check_required=1");
    epoch = GetTickCount64();
    ShowWindow(top, SW_SHOWNORMAL);
    if (!SetTimer(top, 1, 16, NULL) || !SetTimer(top, 2, 250, NULL)) return 2;
    while ((result = GetMessageW(&msg, NULL, 0, 0)) > 0) { TranslateMessage(&msg); DispatchMessageW(&msg); }
    if (result == -1) failed = 1;
    cleanup();
    printf("SUMMARY failed=%d frames=%llu completed_timeline=%d\n", failed,
           (unsigned long long)frames, GetTickCount64()-epoch >= 30000);
    return failed || GetTickCount64()-epoch < 30000 ? 2 : 0;
}
