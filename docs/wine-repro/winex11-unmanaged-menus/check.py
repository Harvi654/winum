#!/usr/bin/env python3
"""Reads menu.exe's output on stdin and asks the X server about each window:
override-redirect (not managed) or not, and whether the window manager lists
it as a client (_NET_CLIENT_LIST). Needs libX11 only."""
import ctypes, ctypes.util, sys

X = ctypes.CDLL(ctypes.util.find_library("X11"))
Window = ctypes.c_ulong

class Attrs(ctypes.Structure):          # XWindowAttributes, Xlib.h
    _fields_ = [("x", ctypes.c_int), ("y", ctypes.c_int), ("width", ctypes.c_int),
                ("height", ctypes.c_int), ("border_width", ctypes.c_int), ("depth", ctypes.c_int),
                ("visual", ctypes.c_void_p), ("root", Window), ("class_", ctypes.c_int),
                ("bit_gravity", ctypes.c_int), ("win_gravity", ctypes.c_int),
                ("backing_store", ctypes.c_int), ("backing_planes", ctypes.c_ulong),
                ("backing_pixel", ctypes.c_ulong), ("save_under", ctypes.c_int),
                ("colormap", ctypes.c_ulong), ("map_installed", ctypes.c_int),
                ("map_state", ctypes.c_int), ("all_event_masks", ctypes.c_long),
                ("your_event_mask", ctypes.c_long), ("do_not_propagate_mask", ctypes.c_long),
                ("override_redirect", ctypes.c_int), ("screen", ctypes.c_void_p)]

X.XOpenDisplay.restype = ctypes.c_void_p
X.XOpenDisplay.argtypes = [ctypes.c_char_p]
X.XGetWindowAttributes.argtypes = [ctypes.c_void_p, Window, ctypes.POINTER(Attrs)]
X.XDefaultRootWindow.restype = Window
X.XDefaultRootWindow.argtypes = [ctypes.c_void_p]
X.XInternAtom.restype = ctypes.c_ulong
X.XInternAtom.argtypes = [ctypes.c_void_p, ctypes.c_char_p, ctypes.c_int]
X.XGetWindowProperty.argtypes = [ctypes.c_void_p, Window, ctypes.c_ulong, ctypes.c_long,
                                 ctypes.c_long, ctypes.c_int, ctypes.c_ulong,
                                 ctypes.POINTER(ctypes.c_ulong), ctypes.POINTER(ctypes.c_int),
                                 ctypes.POINTER(ctypes.c_ulong), ctypes.POINTER(ctypes.c_ulong),
                                 ctypes.POINTER(ctypes.c_void_p)]

dpy = X.XOpenDisplay(None)
if not dpy:
    sys.exit("cannot open the X display")

def client_list():
    typ, fmt, n, after, data = ctypes.c_ulong(), ctypes.c_int(), ctypes.c_ulong(), ctypes.c_ulong(), ctypes.c_void_p()
    X.XGetWindowProperty(dpy, X.XDefaultRootWindow(dpy), X.XInternAtom(dpy, b"_NET_CLIENT_LIST", 0),
                         0, 4096, 0, 0, ctypes.byref(typ), ctypes.byref(fmt), ctypes.byref(n),
                         ctypes.byref(after), ctypes.byref(data))
    if not data.value:
        return set()
    return set(ctypes.cast(data, ctypes.POINTER(ctypes.c_ulong))[:n.value])

rows = [line.split() for line in sys.stdin if line.strip()]
clients = client_list()
for row in rows:
    name, wid = row[0], int(row[1], 16)
    a = Attrs()
    if not X.XGetWindowAttributes(dpy, wid, ctypes.byref(a)):
        print(f"{name:13} no such X window")
        continue
    state = "override-redirect" if a.override_redirect else "managed"
    print(f"{name:13} {state:17} wm-client={'yes' if wid in clients else 'no'}  ({' '.join(row[2:])})")
