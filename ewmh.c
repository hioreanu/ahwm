/* $Id$
 * This was written by Alex Hioreanu in 2001.
 * This code is in the public domain and the author disclaims all
 * copyright privileges.
 */

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include "xwm.h"
#include "ewmh.h"

#define NO_SUPPORTED_HINTS 34

static Atom _NET_CURRENT_DESKTOP, _NET_SUPPORTED, _NET_CLIENT_LIST;
static Atom _NET_CLIENT_LIST_STACKING, _NET_NUMBER_OF_DESKTOPS;
static Atom _NET_DESKTOP_GEOMETRY, _NET_DESKTOP_VIEWPORT;
static Atom _NET_ACTIVE_WINDOW, _NET_WORKAREA, _NET_SUPPORTING_WM_CHECK;

static Atom _NET_CLOSE_WINDOW, _NET_WM_MOVERESIZE, _NET_WM_NAME;
static Atom _NET_WM_DESKTOP, _NET_WM_WINDOW_TYPE, _NET_WM_WINDOW_TYPE_DESKTOP;
static Atom _NET_WM_WINDOW_TYPE_DOCK, _NET_WM_WINDOW_TYPE_TOOLBAR;
static Atom _NET_WM_WINDOW_TYPE_MENU, _NET_WM_WINDOW_TYPE_DIALOG;
static Atom _NET_WM_WINDOW_TYPE_NORMAL, _NET_WM_STATE;
static Atom _NET_WM_STATE_MODAL, _NET_WM_STATE_STICKY;
static Atom _NET_WM_STATE_MAXIMIZED_VERT, _NET_WM_STATE_MAXIMIZED_HORZ;
static Atom _NET_WM_STATE_SHADED, _NET_WM_STATE_SKIP_TASKBAR;
static Atom _NET_WM_STATE_SKIP_PAGER, _NET_WM_STATE_REMOVE;
static Atom _NET_WM_STATE_ADD, _NET_WM_STATE_TOGGLE;
static Atom _NET_WM_STRUT, _NET_WM_PING;
static Atom UTF_8_STRING;

static Window ewmh_window;

void ewmh_init()
{
    XSetWindowAttributes xswa;
    Atom supported[NO_SUPPORTED_HINTS];

    _NET_CURRENT_DESKTOP = XInternAtom(dpy, "_NET_CURRENT_DESKTOP", False);
    _NET_SUPPORTED = XInternAtom(dpy, "_NET_SUPPORTED", False);
    _NET_CLIENT_LIST = XInternAtom(dpy, "_NET_CLIENT_LIST", False);
    _NET_CLIENT_LIST_STACKING =
        XInternAtom(dpy, "_NET_CLIENT_LIST_STACKING", False);
    _NET_NUMBER_OF_DESKTOPS =
        XInternAtom(dpy, "_NET_NUMBER_OF_DESKTOPS", False);
    _NET_DESKTOP_GEOMETRY = XInternAtom(dpy, "_NET_DESKTOP_GEOMETRY", False);
    _NET_DESKTOP_VIEWPORT = XInternAtom(dpy, "_NET_DESKTOP_VIEWPORT", False);
    _NET_ACTIVE_WINDOW = XInternAtom(dpy, "_NET_ACTIVE_WINDOW", False);
    _NET_WORKAREA = XInternAtom(dpy, "_NET_WORKAREA", False);
    _NET_SUPPORTING_WM_CHECK =
        XInternAtom(dpy, "_NET_SUPPORTING_WM_CHECK", False);
    _NET_CLOSE_WINDOW = XInternAtom(dpy, "_NET_CLOSE_WINDOW", False);
    _NET_WM_MOVERESIZE = XInternAtom(dpy, "_NET_WM_MOVERESIZE", False);
    _NET_WM_NAME = XInternAtom(dpy, "_NET_WM_NAME", False);
    _NET_WM_DESKTOP = XInternAtom(dpy, "_NET_WM_DESKTOP", False);
    _NET_WM_WINDOW_TYPE = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE", False);
    _NET_WM_WINDOW_TYPE_DESKTOP =
        XInternAtom(dpy, "_NET_WM_WINDOW_TYPE_DESKTOP", False);
    _NET_WM_WINDOW_TYPE_DOCK =
        XInternAtom(dpy, "_NET_WM_WINDOW_TYPE_DOCK", False);
    _NET_WM_WINDOW_TYPE_TOOLBAR =
        XInternAtom(dpy, "_NET_WM_WINDOW_TYPE_TOOLBAR", False);
    _NET_WM_WINDOW_TYPE_MENU =
        XInternAtom(dpy, "_NET_WM_WINDOW_TYPE_MENU", False);
    _NET_WM_WINDOW_TYPE_DIALOG =
        XInternAtom(dpy, "_NET_WM_WINDOW_TYPE_DIALOG", False);
    _NET_WM_WINDOW_TYPE_NORMAL =
        XInternAtom(dpy, "_NET_WM_WINDOW_TYPE_NORMAL", False);
    _NET_WM_STATE = XInternAtom(dpy, "_NET_WM_STATE", False);
    _NET_WM_STATE_MODAL = XInternAtom(dpy, "_NET_WM_STATE_MODAL", False);
    _NET_WM_STATE_STICKY = XInternAtom(dpy, "_NET_WM_STATE_STICKY", False);
    _NET_WM_STATE_MAXIMIZED_VERT =
        XInternAtom(dpy, "_NET_WM_STATE_MAXIMIZED_VERT", False);
    _NET_WM_STATE_MAXIMIZED_HORZ =
        XInternAtom(dpy, "_NET_WM_STATE_MAXIMIZED_HORZ", False);
    _NET_WM_STATE_SHADED = XInternAtom(dpy, "_NET_WM_STATE_SHADED", False);
    _NET_WM_STATE_SKIP_TASKBAR =
        XInternAtom(dpy, "_NET_WM_STATE_SKIP_TASKBAR", False);
    _NET_WM_STATE_SKIP_PAGER =
        XInternAtom(dpy, "_NET_WM_STATE_SKIP_PAGER", False);
    _NET_WM_STATE_REMOVE = XInternAtom(dpy, "_NET_WM_STATE_REMOVE", False);
    _NET_WM_STATE_ADD = XInternAtom(dpy, "_NET_WM_STATE_ADD", False);
    _NET_WM_STATE_TOGGLE = XInternAtom(dpy, "_NET_WM_STATE_TOGGLE", False);
    _NET_WM_STRUT = XInternAtom(dpy, "_NET_WM_STRUT", False);
    _NET_WM_PING = XInternAtom(dpy, "_NET_WM_PING", False);
    UTF_8_STRING = XInternAtom(dpy, "UTF_8_STRING", False);

    supported[0] = _NET_SUPPORTED;
    supported[1] = _NET_CLIENT_LIST;
    supported[2] = _NET_CLIENT_LIST_STACKING;
    supported[3] = _NET_NUMBER_OF_DESKTOPS;
    supported[4] = _NET_DESKTOP_GEOMETRY;
    supported[5] = _NET_DESKTOP_VIEWPORT;
    supported[6] = _NET_CURRENT_DESKTOP;
    supported[7] = _NET_ACTIVE_WINDOW;
    supported[8] = _NET_WORKAREA;
    supported[9] = _NET_SUPPORTING_WM_CHECK;
    supported[10] = _NET_CLOSE_WINDOW;
    supported[11] = _NET_WM_MOVERESIZE;
    supported[12] = _NET_WM_NAME;
    supported[13] = _NET_WM_DESKTOP;
    supported[14] = _NET_WM_WINDOW_TYPE;
    supported[15] = _NET_WM_WINDOW_TYPE_DESKTOP;
    supported[16] = _NET_WM_WINDOW_TYPE_DOCK;
    supported[17] = _NET_WM_WINDOW_TYPE_TOOLBAR;
    supported[18] = _NET_WM_WINDOW_TYPE_MENU;
    supported[19] = _NET_WM_WINDOW_TYPE_DIALOG;
    supported[20] = _NET_WM_WINDOW_TYPE_NORMAL;
    supported[21] = _NET_WM_STATE;
    supported[22] = _NET_WM_STATE_MODAL;
    supported[23] = _NET_WM_STATE_STICKY;
    supported[24] = _NET_WM_STATE_MAXIMIZED_VERT;
    supported[25] = _NET_WM_STATE_MAXIMIZED_HORZ;
    supported[26] = _NET_WM_STATE_SHADED;
    supported[27] = _NET_WM_STATE_SKIP_TASKBAR;
    supported[28] = _NET_WM_STATE_SKIP_PAGER;
    supported[29] = _NET_WM_STATE_REMOVE;
    supported[30] = _NET_WM_STATE_ADD;
    supported[31] = _NET_WM_STATE_TOGGLE;
    supported[32] = _NET_WM_STRUT;
    supported[33] = _NET_WM_PING;

    XChangeProperty(dpy, root_window, _NET_SUPPORTED,
                    XA_ATOM, 32, PropModeReplace,
                    (unsigned char *)supported, NO_SUPPORTED_HINTS);
    
    xswa.override_redirect = True;
    ewmh_window = XCreateWindow(dpy, root_window, 0, 0, 1, 1, 0,
                                DefaultDepth(dpy, scr), InputOutput,
                                DefaultVisual(dpy, scr),
                                CWOverrideRedirect, &xswa);
    XChangeProperty(dpy, root_window, _NET_SUPPORTING_WM_CHECK,
                    XA_WINDOW, 32, PropModeReplace,
                    (unsigned char *)ewmh_window, 1);
    XChangeProperty(dpy, ewmh_window, _NET_SUPPORTING_WM_CHECK,
                    XA_WINDOW, 32, PropModeReplace,
                    (unsigned char *)ewmh_window, 1);
    XChangeProperty(dpy, ewmh_window, _NET_WM_NAME,
                    UTF_8_STRING, 8, PropModeReplace,
                    (unsigned char *)"XWM", 4);
}

void ewmh_active_window_update()
{
    ;
}
