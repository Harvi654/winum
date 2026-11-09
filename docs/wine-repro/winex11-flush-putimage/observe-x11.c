/* Original, AI-assisted (Codex), LGPL-2.1-or-later.
 * Optional Linux observer: logs real calls, adds no X requests/flushes.
 * Use only with this own-window fixture in a disposable Wine prefix.
 */
#define _GNU_SOURCE
#include <X11/Xlib.h>
#include <dlfcn.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

static int (*real_put)(Display *, Drawable, GC, XImage *, int, int, int, int, unsigned int, unsigned int);
static int (*real_flush)(Display *);
static pthread_once_t once = PTHREAD_ONCE_INIT;

static void initialize(void)
{
    void *module = dlopen("libX11.so.6", RTLD_NOW | RTLD_LOCAL);
    if (module)
    {
        *(void **)(&real_put) = dlsym(module, "XPutImage");
        *(void **)(&real_flush) = dlsym(module, "XFlush");
    }
    if (!real_put || !real_flush)
    {
        fputs("OBSERVER_ERROR resolve_Xlib=1\n", stderr);
        _Exit(2);
    }
}

static unsigned long long milliseconds(void)
{
    struct timespec time;
    if (clock_gettime(CLOCK_MONOTONIC, &time)) return 0;
    return (unsigned long long)time.tv_sec * 1000 + time.tv_nsec / 1000000;
}

int XPutImage(Display *display, Drawable drawable, GC gc, XImage *image,
              int sx, int sy, int dx, int dy, unsigned int width, unsigned int height)
{
    pthread_once(&once, initialize);
    fprintf(stderr, "XPUT pid=%ld display=%p tick_ms=%llu width=%u height=%u\n",
            (long)getpid(), (void *)display, milliseconds(), width, height);
    return real_put(display, drawable, gc, image, sx, sy, dx, dy, width, height);
}

int XFlush(Display *display)
{
    pthread_once(&once, initialize);
    fprintf(stderr, "XFLUSH pid=%ld display=%p tick_ms=%llu\n",
            (long)getpid(), (void *)display, milliseconds());
    return real_flush(display);
}
