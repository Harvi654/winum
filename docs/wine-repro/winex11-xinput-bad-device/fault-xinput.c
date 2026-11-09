/* Original, AI-assisted (Codex), LGPL-2.1-or-later.
 * Linux x86_64/glibc fixture-only fault injection. No device is changed.
 * Wine resolves libXi through dlsym(handle,...), so interpose that lookup,
 * not just the exported XOpenDevice name in the global ELF namespace.
 */
#define _GNU_SOURCE
#include <X11/Xlib.h>
#include <X11/extensions/XInput.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

typedef void *(*lookup_fn)(void *, const char *);
typedef XDevice *(*open_fn)(Display *, XID);
typedef int (*map_fn)(Display *, XDevice *, unsigned char *, unsigned int);
static lookup_fn actual_lookup;
static int fixture_process;
static _Atomic(open_fn) actual_open;
static _Atomic(map_fn) actual_map;
static atomic_uint open_calls, map_calls;
static pthread_once_t once = PTHREAD_ONCE_INIT;

static void initialize(void)
{
    char args[8192];
    ssize_t length;
    int fd;
    size_t offset = 0;
    /* Inspect only this process's argv, print no paths/arguments, and do
     * not inject into wineserver, services or other Wine helper programs. */
    fd = open("/proc/self/cmdline", O_RDONLY | O_CLOEXEC);
    if (fd >= 0)
    {
        length = read(fd, args, sizeof(args));
        close(fd);
        while (length > 0 && offset < (size_t)length)
        {
            const char *item = args + offset;
            const char *end = memchr(item, 0, length - offset);
            const char *base = item, *p;
            if (!end) break;
            for (p = item; p < end; ++p)
                if (*p == '/' || *p == '\\') base = p + 1;
            if (!strcasecmp(base, "raw-input.exe")) fixture_process = 1;
            offset = end - args + 1;
        }
    }
    /* The build/README deliberately restrict this helper to x86_64 glibc. */
    actual_lookup = (lookup_fn)dlvsym(RTLD_NEXT, "dlsym", "GLIBC_2.2.5");
    if (!actual_lookup)
    {
        fputs("FAULT_ERROR resolve_dlsym=1\n", stderr);
        _Exit(2);
    }
}

static XDevice *fault_open(Display *display, XID device)
{
    const char *mode = getenv("WINUM_XI_FAULT");
    unsigned int call = atomic_fetch_add(&open_calls, 1) + 1;
    open_fn fn = atomic_load(&actual_open);
    fprintf(stderr, "XI_CALL pid=%ld fixture=1 operation=open call=%u source_id=%lu\n", (long)getpid(), call, device);
    if (mode && !strcmp(mode, "open"))
    {
        fprintf(stderr, "FAULT pid=%ld fixture=1 operation=open request_device=0\n", (long)getpid());
        return fn(display, 0); /* reserved XIAllDevices pseudo-id, not a legacy device */
    }
    return fn(display, device);
}

static int fault_map(Display *display, XDevice *device, unsigned char *buttons, unsigned int count)
{
    const char *mode = getenv("WINUM_XI_FAULT");
    unsigned int call = atomic_fetch_add(&map_calls, 1) + 1;
    map_fn fn = atomic_load(&actual_map);
    fprintf(stderr, "XI_CALL pid=%ld fixture=1 operation=map call=%u source_id=%lu\n", (long)getpid(), call, device->device_id);
    if (mode && !strcmp(mode, "map"))
    {
        /* Only alter a stack copy for this query. The original allocation
         * and device ID remain intact for Wine's real XCloseDevice. */
        XDevice invalid = *device;
        invalid.device_id = 0;
        fprintf(stderr, "FAULT pid=%ld fixture=1 operation=map request_device=0\n", (long)getpid());
        return fn(display, &invalid, buttons, count);
    }
    return fn(display, device, buttons, count);
}

void *dlsym(void *handle, const char *name)
{
    void *symbol;
    const char *mode;
    pthread_once(&once, initialize);
    symbol = actual_lookup(handle, name);
    mode = getenv("WINUM_XI_FAULT");
    if (!fixture_process || !symbol || !mode || (strcmp(mode, "open") && strcmp(mode, "map") && strcmp(mode, "observe")))
        return symbol;
    if (!strcmp(name, "XOpenDevice"))
    {
        atomic_store(&actual_open, (open_fn)symbol);
        fprintf(stderr, "XI_HOOK pid=%ld fixture=1 symbol=XOpenDevice mode=%s\n", (long)getpid(), mode);
        return (void *)fault_open;
    }
    if (!strcmp(name, "XGetDeviceButtonMapping"))
    {
        atomic_store(&actual_map, (map_fn)symbol);
        fprintf(stderr, "XI_HOOK pid=%ld fixture=1 symbol=XGetDeviceButtonMapping mode=%s\n", (long)getpid(), mode);
        return (void *)fault_map;
    }
    return symbol;
}
