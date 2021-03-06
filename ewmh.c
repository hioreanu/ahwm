/* $Id$ */
/* Copyright (c) 2001 Alex Hioreanu.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "config.h"

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <stdio.h>

#include "compat.h"
#include "ahwm.h"
#include "ewmh.h"
#include "malloc.h"
#include "debug.h"
#include "focus.h"
#include "kill.h"
#include "move-resize.h"
#include "stacking.h"
#include "keyboard-mouse.h"

/*
 * TODO:
 * 
 * - use _NET_WORKAREA for move/resize/placement
 * - _NET_WM_STRUT:  recalculate _NET_WORKAREA
 * - _NET_WM_PING, _NET_WM_PID
 * - some properties must be updated on window if set by ahwm:
 *   x _NET_WM_DESKTOP
 *   - _NET_WM_STATE
 * x kicker has a KEEP_ON_TOP WM_STATE not mentioned in EWMH 1.1
 * x proxy clicks for GNOME
 * x figure out if _NET_CLIENT_LIST[_STACKING] has windows on all workspaces
 * x _NET_WM_MOVERESIZE, should be easy
 * x honor _NET_WM_DESKTOP, also omnipresence
 * x _NET_WM_WINDOW_TYPE:
 *   DESKTOP:  sticky, no title, omnipresent, SkipCycle, AlwaysOnBottom,
 *             don't bind mouse or keyboard, force click-to-focus
 *   DOCK:  sticky, no title, omnipresent?, SkipCycle, AlwaysOnTop
 *   TOOLBAR:  SkipCycle, modal?
 *   MENU:  SkipCycle, modal?
 *   DIALOG:  nothing special
 *   NORMAL:  nothing special
 * x _NET_WM_STATE:  
 *   MODAL:  nothing special
 *   STICKY:  sticky
 *   MAXMIZED_HORIZ, VERT:  separate maximization states
 *   SHADED:  add a shaded state, only for clients with titlebars
 *   SKIP_TASKBAR:  nothing special
 *   SKIP_PAGER:  nothing special
 * x add horiz, and vert max.
 */

/* move/resize direction for _NET_WM_MOVERESIZE */
#define _NET_WM_MOVERESIZE_SIZE_TOPLEFT      0
#define _NET_WM_MOVERESIZE_SIZE_TOP          1
#define _NET_WM_MOVERESIZE_SIZE_TOPRIGHT     2
#define _NET_WM_MOVERESIZE_SIZE_RIGHT        3
#define _NET_WM_MOVERESIZE_SIZE_BOTTOMRIGHT  4
#define _NET_WM_MOVERESIZE_SIZE_BOTTOM       5
#define _NET_WM_MOVERESIZE_SIZE_BOTTOMLEFT   6
#define _NET_WM_MOVERESIZE_SIZE_LEFT         7
#define _NET_WM_MOVERESIZE_MOVE              8   /* Movement only */

/*
 * EWMH 1.1 does not clearly state which atoms are "hints" and belong
 * in _NET_SUPPORTED_HINTS.  I'm putting every atom EWMH mentions into
 * _NET_SUPPORTED_HINTS, even those which don't really look like
 * "hints."
 */

#define NO_SUPPORTED_HINTS 33

static Atom _NET_CURRENT_DESKTOP, _NET_SUPPORTED, _NET_CLIENT_LIST;
static Atom _NET_CLIENT_LIST_STACKING, _NET_NUMBER_OF_DESKTOPS;
static Atom _NET_DESKTOP_GEOMETRY, _NET_DESKTOP_VIEWPORT;
static Atom _NET_ACTIVE_WINDOW, _NET_WORKAREA, _NET_SUPPORTING_WM_CHECK;
static Atom _NET_DESKTOP_NAMES;
static Atom _NET_CLOSE_WINDOW, _NET_WM_MOVERESIZE, _NET_WM_NAME;
static Atom _NET_WM_WINDOW_TYPE_DESKTOP;
static Atom _NET_WM_WINDOW_TYPE_DOCK, _NET_WM_WINDOW_TYPE_TOOLBAR;
static Atom _NET_WM_WINDOW_TYPE_MENU, _NET_WM_WINDOW_TYPE_DIALOG;
static Atom _NET_WM_WINDOW_TYPE_NORMAL;
static Atom _NET_WM_STATE_MODAL, _NET_WM_STATE_STICKY;
static Atom _NET_WM_STATE_MAXIMIZED_VERT, _NET_WM_STATE_MAXIMIZED_HORZ;
static Atom _NET_WM_STATE_SHADED, _NET_WM_STATE_SKIP_TASKBAR;
static Atom _NET_WM_STATE_SKIP_PAGER, _NET_WM_STATE_REMOVE;
static Atom _NET_WM_STATE_ADD, _NET_WM_STATE_TOGGLE;
static Atom _NET_WM_PING, _NET_WM_STATE_STAYS_ON_TOP;

Atom _NET_WM_STRUT, _NET_WM_STATE, _NET_WM_WINDOW_TYPE, _NET_WM_DESKTOP;

/* EWMH 1.1 claims this is supposed to be "UTF-8_STRING" (which
 * is a perfectly good atom identifier), but it's actually
 * "UTF8_STRING" (NB, no dash). */
static Atom UTF8_STRING;

/* used for both _NET and _WIN stuff: */
static Window ewmh_window;

/* Used by emwh_wm_strut_apply(): */
static client_t **left_client, **right_client, **top_client, **bottom_client;
unsigned long *top, *left, *right, *bottom, *width, *height;

static void no_titlebar(client_t *client);
static void unfocusable(client_t *client);
static void skip_cycle(client_t *client);
static void sticky(client_t *client);
static void on_top(client_t *client);
static void on_bottom(client_t *client);
static void update_wm_workarea();
void ewmh_to_desktop(client_t *client);
void ewmh_to_dock(client_t *client);
void ewmh_to_normal(client_t *client);
void ewmh_to_dialog(client_t *client);
void ewmh_to_menu(client_t *client);
void ewmh_to_toolbar(client_t *client);
Bool ewmh_handle_clientmessage(XClientMessageEvent *xevent);

void ewmh_init()
{
    unsigned long *l, *l2, bytes_after_return, nitems;
    int i, fmt;
    XSetWindowAttributes xswa;
    Atom supported[NO_SUPPORTED_HINTS];
    Atom actual;
    char workspace_name[32];

    l = malloc(4 * nworkspaces * sizeof(long));
    if (l == NULL) {
        perror("AHWM: ewmh_init: malloc");
        fprintf(stderr, "AHWM: this is a fatal error, quitting.\n");
        exit(1);
    }
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
    UTF8_STRING = XInternAtom(dpy, "UTF8_STRING", False);
    _NET_DESKTOP_NAMES = XInternAtom(dpy, "_NET_DESKTOP_NAMES", False);
    _NET_WM_WINDOW_TYPE = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE", False);
    _NET_WM_STATE = XInternAtom(dpy, "_NET_WM_STATE", False);
    _NET_WM_STATE_STAYS_ON_TOP =
        XInternAtom(dpy, "_NET_WM_STATE_STAYS_ON_TOP", False);

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
    supported[32] = _NET_DESKTOP_NAMES;
    /* supported[33] = _NET_WM_STRUT; */
    /* supported[34] = _NET_WM_PING; */

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
                    (unsigned char *)&ewmh_window, 1);
    XChangeProperty(dpy, ewmh_window, _NET_SUPPORTING_WM_CHECK,
                    XA_WINDOW, 32, PropModeReplace,
                    (unsigned char *)&ewmh_window, 1);
    XChangeProperty(dpy, ewmh_window, _NET_WM_NAME,
                    UTF8_STRING, 8, PropModeReplace,
                    (unsigned char *)"AHWM", 4);
    l[0] = nworkspaces;
    XChangeProperty(dpy, root_window, _NET_NUMBER_OF_DESKTOPS,
                    XA_CARDINAL, 32, PropModeReplace,
                    (unsigned char *)l, 1);
    l[0] = scr_width;
    l[1] = scr_height;
    XChangeProperty(dpy, root_window, _NET_DESKTOP_GEOMETRY,
                    XA_CARDINAL, 32, PropModeReplace,
                    (unsigned char *)l, 2);
    for (i = 0; i < nworkspaces * 2; i++) {
        l[i] = 0;
    }
    XChangeProperty(dpy, root_window, _NET_DESKTOP_VIEWPORT,
                    XA_CARDINAL, 32, PropModeReplace,
                    (unsigned char *)l, nworkspaces * 2);
    for (i = 0; i < nworkspaces * 4; i += 4) {
        l[i] = 0;
        l[i+1] = 0;
        l[i+2] = scr_width;
        l[i+3] = scr_height;
    }
    /* We don't really have a system for naming workspaces, so I'll
     * just name them "Workspace 1", etc. so the workspace names will
     * appear in pagers.  I would like to keep any workspace names
     * that are already there, but that would mean I have to ensure
     * the property contains the correct number of strings.  We can't
     * simply look for NULs: we need a full UTF8 parser, which is a
     * non-trivial task.  Thus, we simply overwrite any property
     * that's already there. */
    for (i = 0; i < nworkspaces; i++) {
        /* _NET_DESKTOP_NAMES is simply a big array that
         * contains all the desktop names, separated by
         * NULs. */
        snprintf(workspace_name, 32, "Workspace %d", i+1);
        XChangeProperty(dpy, root_window, _NET_DESKTOP_NAMES,
                        UTF8_STRING, 8,
                        i == 0 ? PropModeReplace : PropModeAppend,
                        (unsigned char *)workspace_name,
                        strlen(workspace_name) + 1);
    }
    /*
     * This function is called whenever AHWM is started.  We try to
     * read _NET_CURRENT_DESKTOP to see if we are taking over for
     * another EWMH-compliant window manager (or perhaps we even
     * restarted ourselves).
     */
    if (XGetWindowProperty(dpy, root_window, _NET_CURRENT_DESKTOP, 0, 1,
                           False, XA_CARDINAL, &actual, &fmt, &nitems,
                           &bytes_after_return,
                           (void *)&l2) == Success) {
        if (l2 != NULL) {
            if (nitems == 1 && fmt == 32 && actual == XA_CARDINAL &&
                *l2 > 0 && *l2 + 1 <= nworkspaces) {
                
                workspace_current = *l2 + 1;
            }
            XFree(l2);
        }
    }
    
    l[0] = workspace_current - 1;
    XChangeProperty(dpy, root_window, _NET_CURRENT_DESKTOP,
                    XA_CARDINAL, 32, PropModeReplace,
                    (unsigned char *)l, 1);

    top_client = malloc(sizeof(client_t *) * nworkspaces);
    bottom_client = malloc(sizeof(client_t *) * nworkspaces);
    left_client = malloc(sizeof(client_t *) * nworkspaces);
    right_client = malloc(sizeof(client_t *) * nworkspaces);
    top = malloc(sizeof(unsigned long) * nworkspaces);
    bottom = malloc(sizeof(unsigned long) * nworkspaces);
    left = malloc(sizeof(unsigned long) * nworkspaces);
    right = malloc(sizeof(unsigned long) * nworkspaces);
    width = malloc(sizeof(unsigned long) * nworkspaces);
    height = malloc(sizeof(unsigned long) * nworkspaces);

    if (top_client == NULL || bottom_client == NULL ||
        left_client == NULL || right_client == NULL ||
        top == NULL || bottom == NULL || left == NULL ||
        right == NULL || width == NULL || height == NULL) {

        perror("AHWM: ewmh_init: malloc");
        fprintf(stderr, "AHWM: this is a fatal error.  Quitting.\n");
        exit(1);
    }
    
    for (i = 0; i < nworkspaces; i++) {
        top_client[i] = NULL;
        bottom_client[i] = NULL;
        left_client[i] = NULL;
        right_client[i] = NULL;
        top[i] = 0;
        bottom[i] = scr_height;
        left[i] = 0;
        right[i] = scr_width;
    }
    
    free(l);
    
    update_wm_workarea();
}

/*
 * EWMH does not specify the reason for using both ClientMessages and
 * setting properties on the root window.
 * 
 * Some properties on the client window must be kept updated by the
 * window manager, and we can't have both the client and the window
 * manager writing to this property at the same time.  For these
 * properties, the way a client can change the property is by sending
 * a ClientMessage.  However, the client may simply set the property
 * directly before the client maps the window or after the client
 * unmaps the window.
 * 
 * Some other properties on the client window are not kept updated by
 * the window manager.  For these properties, the client can change
 * the property at any time by simply using XChangeProperty().
 * 
 * The whole reason for the ClientMessage thing is probably because it
 * is much more work to try to figure out if a PropertyChanged event
 * originated from the client or the window manager.
 */

void ewmh_wm_state_apply(client_t *client)
{
    Atom actual, *states;
    int fmt, i;
    unsigned long bytes_after_return, nitems;
    int max_v, max_h;

    if (client->state != WithdrawnState) return;

    states = NULL;
    if (XGetWindowProperty(dpy, client->window, _NET_WM_STATE, 0,
                           sizeof(Atom), False, XA_ATOM,
                           &actual, &fmt, &nitems, &bytes_after_return,
                           (void *)&states) != Success) {
        debug(("\tXGetWindowProperty(_NET_WM_STATE) failed\n"));
        return;
    }

    if (nitems == 0 || fmt != 32 || actual != XA_ATOM) {
        debug(("\tnitems = %d, fmt = %d\n", nitems, fmt));
        if (states != NULL) XFree(states);
        return;
    }

    max_v = max_h = 0;
    for (i = 0; i < nitems; i++) {
        if (states[i] == _NET_WM_STATE_STICKY) {
            sticky(client);
        } else if (states[i] == _NET_WM_STATE_STAYS_ON_TOP) {
            on_top(client);
        } else if (states[i] == _NET_WM_STATE_MAXIMIZED_HORZ) {
            max_h = 1;
        } else if (states[i] == _NET_WM_STATE_MAXIMIZED_VERT) {
            max_v = 1;
        }
    }
    if (max_h && max_v) {
        /* do both atomically instead of one at a time */
        resize_maximize_client(client, MAX_BOTH, MAX_MAXED);
    } else if (max_h) {
        resize_maximize_client(client, MAX_HORIZ, MAX_MAXED);
    } else if (max_v) {
        resize_maximize_client(client, MAX_VERT, MAX_MAXED);
    }
    if (states != NULL) XFree(states);
}

static void update_wm_workarea()
{
    static unsigned long *values = NULL;
    int i;

    if (values == NULL) {
        values = malloc(sizeof(unsigned long) * 4 * nworkspaces);
        if (values == NULL) {
            perror("AHWM: update_wm_workarea: malloc");
            return;
        }
    }
    for (i = 0; i < nworkspaces; i++) {
        width[i] = right[i] - left[i];
        height[i] = bottom[i] - top[i];
        values[i * 4] = left[i];
        values[i * 4 + 1] = top[i];
        values[i * 4 + 2] = width[i];
        values[i * 4 + 3] = height[i];
    }

    XChangeProperty(dpy, root_window, _NET_WORKAREA, XA_CARDINAL,
                    32, PropModeReplace, (unsigned char *)values,
                    nworkspaces * 4);
}

static Bool reset(client_t *client, int ws)
{
    Bool changed = False;

    if (client == left_client[ws]) {
        left[ws] = 0;
        left_client[ws] = NULL;
        changed = True;
    }
    if (client == right_client[ws]) {
        right[ws] = scr_width;
        right_client[ws] = NULL;
        changed = True;
    }
    if (client == top_client[ws]) {
        top[ws] = 0;
        top_client[ws] = NULL;
        changed = True;
    }
    if (client == bottom_client[ws]) {
        bottom[ws] = scr_height;
        bottom_client[ws] = NULL;
        changed = True;
    }
    return changed;
}

/* no ClientMessage for _NET_WM_STRUT, just XChangeProperty() */
void ewmh_wm_strut_apply(client_t *client)
{
    Atom actual;
    int fmt, i;
    unsigned long bytes_after_return, nitems;
    unsigned long *values;
    Bool changed = False;

    if (client->omnipresent == True) {
        for (i = 0; i < nworkspaces; i++) {
            if (reset(client, i)) {
                changed = True;
            }
        }
    } else {
        changed = reset(client, client->workspace);
    }
    if (changed) {
        /* FIXME: need to check all others except "client" */
    }
    
    if (XGetWindowProperty(dpy, client->window, _NET_WM_STRUT, 0,
                           sizeof(Atom), False, XA_ATOM,
                           &actual, &fmt, &nitems, &bytes_after_return,
                           (void *)&values) != Success) {
        debug(("\tXGetWindowProperty(_NET_WM_STRUT) failed\n"));
        if (changed)
            update_wm_workarea();
        return;
    }

    if (actual != _NET_WM_STRUT || fmt != 32 || nitems != 4) {
        debug(("\tatom = %d, expected = %d, fmt = %d, nitems = %d\n",
               actual, _NET_WM_STRUT, fmt, nitems));
        if (changed)
            update_wm_workarea();
        return;
    }

    /* FIXMENOW: HERE */
#if 0
    /* I suppose one could use this property to set a full-screen
     * "presentation" type window, so I'll allow resetting the
     * workarea to zero by zero pixels (but no less). */
    if (values[0] > left && values[0] <= scr_width) {
        left = values[0];
        changed = True;
        left_client = client;
    }
    if (scr_width - values[1] < right && values[1] <= scr_width) {
        right = scr_width - values[1];
        changed = True;
        right_client = client;
    }
    if (values[2] > top && values[2] <= scr_height) {
        top = values[2];
        changed = True;
        top_client = client;
    }
    if (scr_height - values[3] < bottom && values[3] <= scr_height) {
        bottom = scr_height - values[3];
        changed = True;
        bottom_client = client;
    }

    if (left + right > scr_width || top + bottom > scr_height) {
        fprintf(stderr,
                "AHWM: client '%s' is abusing _NET_WM_STRUT.\n"
                "Ignoring client, resetting _NET_WORKAREA.\n",
                client->name);
        top = 0;
        left = 0;
        right = scr_width;
        bottom = scr_height;
        top_client = bottom_client = NULL;
        left_client = right_client = NULL;
        changed = True;
    }

    if (changed)
        update_wm_workarea();
#endif
}

void ewmh_wm_desktop_apply(client_t *client)
{
    Atom actual;
    int fmt;
    unsigned long *ws, bytes_after_return, nitems;

    if (client->state != WithdrawnState) return;

    ws = NULL;
    if (XGetWindowProperty(dpy, client->window, _NET_WM_DESKTOP, 0,
                           sizeof(Atom), False, XA_CARDINAL,
                           &actual, &fmt, &nitems, &bytes_after_return,
                           (void *)&ws) != Success) {
        debug(("\tXGetWindowProperty(_NET_WM_DESKTOP) failed\n"));
        return;
    }
    if (nitems == 0 || fmt != 32 || actual != XA_CARDINAL) {
        debug(("\tnitems = %d, fmt = %d\n", nitems, fmt));
        if (ws != NULL) XFree(ws);
        return;
    }
    if (*ws == 0xFFFFFFFF) {
        if (client->omnipresent_set <= HintSet) {
            client->omnipresent = 1;
            client->omnipresent_set = HintSet;
        }
        if (ws != NULL) XFree(ws);
        return;
    }
    if (client->workspace_set <= HintSet) {
        client->workspace = *ws + 1;
        client->workspace_set = HintSet;
        ewmh_desktop_update(client);
        prefs_apply(client);
    }
    if (ws != NULL) XFree(ws);
}

/*
 * EWMH does not specify how the application can change
 * _NET_WINDOW_TYPE.  We will assume the application can change this
 * property at any time.  We won't change this property.
 */

void ewmh_window_type_apply(client_t *client)
{
    Atom actual, *types;
    int fmt, i;
    unsigned long bytes_after_return, nitems;

    types = NULL;
    if (XGetWindowProperty(dpy, client->window, _NET_WM_WINDOW_TYPE, 0,
                           sizeof(Atom), False, XA_ATOM,
                           &actual, &fmt, &nitems, &bytes_after_return,
                           (void *)&types) != Success) {
        debug(("\tXGetWindowProperty(_NET_WM_WINDOW_TYPE) failed\n"));
        return;
    }
    if (fmt != 32 || nitems == 0) return;

    for (i = 0; i < nitems; i++) {
        if (types[i] ==  _NET_WM_WINDOW_TYPE_DESKTOP) {
            ewmh_to_desktop(client);
        } else if (types[i] == _NET_WM_WINDOW_TYPE_DOCK) {
            ewmh_to_dock(client);
        } else if (types[i] ==  _NET_WM_WINDOW_TYPE_NORMAL) {
            ewmh_to_normal(client);
        } else if (types[i] == _NET_WM_WINDOW_TYPE_DIALOG) {
            ewmh_to_dialog(client);
        } else if (types[i] == _NET_WM_WINDOW_TYPE_MENU) {
            ewmh_to_menu(client);
        } else if (types[i] == _NET_WM_WINDOW_TYPE_TOOLBAR) {
            ewmh_to_toolbar(client);
        }
    }
    if (types != NULL) XFree(types);
}

static void no_titlebar(client_t *client)
{
    if (client->has_titlebar_set <= HintSet) {
        client_remove_titlebar(client);
        client->has_titlebar_set = HintSet;
    }
}

static void unfocusable(client_t *client)
{
    if (client->focus_policy_set <= HintSet) {
        if (client->focus_policy == ClickToFocus) {
			focus_policy_from_click(client);
		}
        client->focus_policy = DontFocus;
        client->focus_policy_set = HintSet;
    }
    if (client->pass_focus_click_set <= HintSet) {
        client->pass_focus_click = 1;
        client->pass_focus_click_set = HintSet;
    }
}

static void skip_cycle(client_t *client)
{
    if (client->cycle_behaviour_set <= HintSet) {
        client->cycle_behaviour = SkipCycle;
        client->cycle_behaviour_set = HintSet;
    }
}

static void sticky(client_t *client)
{
    if (client->sticky_set <= HintSet) {
        client->sticky = 1;
        client->sticky_set = HintSet;
    }
}

static void on_top(client_t *client)
{
    if (client->always_on_bottom_set <= HintSet) {
        client->always_on_bottom = 0;
        client->always_on_bottom_set = HintSet;
    }
    if (client->always_on_top_set <= HintSet) {
        client->always_on_top = 1;
        client->always_on_top_set = HintSet;
    }
    stacking_restack(client);
}

static void on_bottom(client_t *client)
{
    if (client->always_on_top_set <= HintSet) {
        client->always_on_top = 0;
        client->always_on_top_set = HintSet;
    }
    if (client->always_on_bottom_set <= HintSet) {
        client->always_on_bottom = 1;
        client->always_on_bottom_set = HintSet;
    }
    stacking_restack(client);
}

/*
 * make a window a "desktop" window.  No titlebar, omnipresent, Skip
 * alt-tab, force click-to-focus, force pass-through-click,
 * always-on-bottom.  Of course, user can override any of these using
 * unconditional option settings.
 */

void ewmh_to_desktop(client_t *client)
{
    no_titlebar(client);
    skip_cycle(client);
    unfocusable(client);
    sticky(client);
    on_bottom(client);
    if (client->omnipresent_set <= HintSet) {
        client->omnipresent = 1;
        client->omnipresent_set = HintSet;
    }
    if (client->dont_bind_mouse_set <= HintSet) {
        if (client->dont_bind_mouse == 0) {
            mouse_ungrab_buttons(client);
        }
        client->dont_bind_mouse = 1;
        client->dont_bind_mouse_set = HintSet;
    }
    if (stacking_desktop_window != None
        || stacking_desktop_frame != None) {
        fprintf(stderr, "AHWM: Client '%s' wants to be a desktop,\n"
                "but you already have a desktop window.\n", client->name);
    } else {
        stacking_desktop_window = client->window;
        stacking_desktop_frame = client->frame;
        stacking_remove(client);
    }
}

/* Remove titlebar, always-on-top, click-to-focus
 * Could also make omnipresent, but client
 * can specify that separately using _NET_WM_DESKTOP
 */
void ewmh_to_dock(client_t *client)
{
    no_titlebar(client);
	unfocusable(client);
    skip_cycle(client);
    sticky(client);
    on_top(client);
}

/* small problem here:
 * 
 * 1. client may have been desktop window and then becomes normal
 * window.  Need to change stuff set in ewmh_to_desktop()
 * 2. client may simply be normal window from start.  Shouldn't
 * change any of the stuff set in ewmh_to_desktop(), since that
 * would override regular user prefs.
 * 
 * Could keep another tag with each option indicating where option was
 * set.  This would solve (1) and (2).  However, in practice, problem
 * (1) never happens but problem (2) happens often, so I just take
 * care of problem (2) by not doing anything here.  In addition, EWMH
 * does not state how _NET_WINDOW_TYPE can be changed by the
 * application, so (1) shouldn't happen.
 */

void ewmh_to_normal(client_t *client)
{
    /* do nothing */
}

/* Can't really do anything here with current framework */
void ewmh_to_dialog(client_t *client)
{
    /* do nothing */
}

void ewmh_to_menu(client_t *client)
{
    no_titlebar(client);
    skip_cycle(client);
	unfocusable(client);
    on_top(client);
}

void ewmh_to_toolbar(client_t *client)
{
    no_titlebar(client);
    skip_cycle(client);
	unfocusable(client);
}

Bool ewmh_handle_clientmessage(XClientMessageEvent *xevent)
{
    client_t *client;
    long data, data2, data3;
    
    client = client_find(xevent->window);
    data = xevent->data.l[0];
    data2 = xevent->data.l[1];
    data3 = xevent->data.l[2];
    
    if (xevent->message_type == _NET_CURRENT_DESKTOP) {
        debug(("\tChanging workspace due to EWMH message\n"));
        workspace_goto((unsigned int)(data + 1));
        return True;
    } else if (xevent->message_type == _NET_ACTIVE_WINDOW) {
        if (client != NULL) {
            debug(("\tChanging active window due to EWMH message\n"));
            focus_set(client, CurrentTime);
            stacking_raise(client);
        }
        return True;
    } else if (xevent->message_type == _NET_CLOSE_WINDOW) {
        debug(("\tKilling window due to EWMH message\n"));
        kill_nicely((XEvent *)xevent, NULL); /* ugly but works ok */
        return True;
    } else if (xevent->message_type == _NET_WM_DESKTOP) {
        if (data == 0xFFFFFFFF) {
            if (client->omnipresent_set <= HintSet) {
                debug(("\tSetting %s omnipresent due to EWMH message\n",
                       client_dbg(client)));
                client->omnipresent = 1;
                client->omnipresent_set = HintSet;
            }
        } else {
            if (client->workspace_set <= HintSet) {
                debug(("\tMoving client %s to workspace %d due to EWMH message\n",
                       client_dbg(client), (unsigned int)(data + 1)));
                client->workspace_set = HintSet;
                workspace_client_moveto(client, (unsigned int)(data + 1));
            }
        }
        return True;
    } else if (xevent->message_type == _NET_WM_STATE) {
        /* "data" is:
         * 0: toggle state
         * 1: ensure state ON
         * 2: ensure state OFF
         * "data2" and "data3" are atoms to change */

        /* ensure maximizing in both directions happens atomically: */
        if ((data2 == _NET_WM_STATE_MAXIMIZED_HORZ &&
             data3 == _NET_WM_STATE_MAXIMIZED_VERT) ||
            (data2 == _NET_WM_STATE_MAXIMIZED_VERT &&
             data3 == _NET_WM_STATE_MAXIMIZED_HORZ)) {
            if (data == 2) {
                resize_maximize_client(client, MAX_BOTH, MAX_TOGGLE);
            } else if (data == 1) {
                resize_maximize_client(client, MAX_BOTH, MAX_MAXED);
            } else if (data == 0) {
                resize_maximize_client(client, MAX_BOTH, MAX_UNMAXED);
            }
            return True;
        }
        if (data2 == _NET_WM_STATE_MAXIMIZED_HORZ ||
            data3 == _NET_WM_STATE_MAXIMIZED_HORZ) {

            if (data == 2) {
                resize_maximize_client(client, MAX_HORIZ, MAX_TOGGLE);
            } else if (data == 1) {
                resize_maximize_client(client, MAX_HORIZ, MAX_MAXED);
            } else if (data == 0) {
                resize_maximize_client(client, MAX_HORIZ, MAX_UNMAXED);
            }
        }
        if (data2 == _NET_WM_STATE_MAXIMIZED_VERT ||
            data3 == _NET_WM_STATE_MAXIMIZED_VERT) {

            if (data == 2) {
                resize_maximize_client(client, MAX_VERT, MAX_TOGGLE);
            } else if (data == 1) {
                resize_maximize_client(client, MAX_VERT, MAX_MAXED);
            } else if (data == 0) {
                resize_maximize_client(client, MAX_VERT, MAX_UNMAXED);
            }
        }
        if (data2 == _NET_WM_STATE_STICKY ||
            data3 == _NET_WM_STATE_STICKY) {

            if (client->sticky_set <= HintSet) {
                client->sticky_set = HintSet;
                if (data == 2) {
                    client->sticky = client->sticky ? 0 : 1;
                } else if (data == 1) {
                    client->sticky = 1;
                } else if (data == 0) {
                    client->sticky = 0;
                }
            }
        }
        if (data2 == _NET_WM_STATE_STAYS_ON_TOP ||
            data3 == _NET_WM_STATE_STAYS_ON_TOP) {

            if (client->always_on_top_set <= HintSet) {
                client->always_on_top_set = HintSet;
                if (data == 2) {
                    client->always_on_top = client->always_on_top ? 0 : 1;
                } else if (data == 1) {
                    client->always_on_top = 1;
                } else if (data == 0) {
                    client->always_on_top = 0;
                }
            }
        }

        return True;
    } else if (xevent->message_type == _NET_WM_MOVERESIZE) {
        /* We create a fake XEvent and send that off to the
         * move/resize code to deal with it as if it were
         * normal button event */
        XEvent ev;
        arglist al;
        type typ;

		/* FIXME: this assumes MOVERESIZE_TOPLEFT */
        
        ev.type = ButtonPress;
        ev.xbutton.window = xevent->window;
        ev.xbutton.x_root = client->x;
        ev.xbutton.y_root = client->y;
        ev.xbutton.x = 0;
        ev.xbutton.y = 0;
		ev.xbutton.button = Button1;
		XWarpPointer(dpy, None, client->frame, 0, 0, 0, 0, 0, 0);
        /* and that's all move/resize code should ever need */

        if (data3 == _NET_WM_MOVERESIZE_MOVE) {
			/* FIXME: untested */
            move_client(&ev, NULL);
        } else {
            al.arglist_next = NULL;
            al.arglist_arg = &typ;
            typ.type_type = RESIZE_ENUM;
            typ.type_value.resize_enum = data3;
            resize_client(&ev, &al);
        }
        
        return True;
    }
    
    return False;
}

void ewmh_current_desktop_update()
{
    long l = workspace_current - 1;
    
    XChangeProperty(dpy, root_window, _NET_CURRENT_DESKTOP,
                    XA_CARDINAL, 32, PropModeReplace,
                    (unsigned char *)&l, 1);
}

void ewmh_active_window_update()
{
    XChangeProperty(dpy, root_window, _NET_ACTIVE_WINDOW,
                    XA_WINDOW, 32, PropModeReplace,
                    (unsigned char *)&(focus_current->window), 1);
}

void ewmh_desktop_update(client_t *client)
{
    int i;

    if (client->workspace == 0) return;
    i = client->workspace - 1;
    
    XChangeProperty(dpy, client->window, _NET_WM_DESKTOP,
                    XA_CARDINAL, 32, PropModeReplace,
                    (unsigned char *)&i, 1);
}

void ewmh_proxy_buttonevent(XEvent *e)
{
    XUngrabPointer(dpy, CurrentTime);
    XSendEvent(dpy, ewmh_window, False, SubstructureNotifyMask, e);
}

static Window *ewmh_client_list = NULL;
static unsigned int nclients = 0;
static unsigned int nwindows_allocated = 0;

void ewmh_client_list_add(client_t *client)
{
    Window *tmp;
    
    if (nclients == nwindows_allocated) {
        if (ewmh_client_list == NULL) {
            ewmh_client_list = Malloc((nclients + 1) * sizeof(Window));
            if (ewmh_client_list == NULL) {
                perror("AHWM: malloc EWMH client list");
                return;
            }
            nwindows_allocated = nclients + 1;
        } else {
            tmp = Realloc(ewmh_client_list, 2*nwindows_allocated*sizeof(Window));
            if (tmp == NULL) {
                perror("AHWM: realloc EWMH client list");
                return;
            }
            ewmh_client_list = tmp;
            nwindows_allocated *= 2;
        }
    }
    ewmh_client_list[nclients++] = client->window;
    debug(("\tAdding window %s to _NET_CLIENT_LIST, %d clients\n",
           client_dbg(client), nclients));
    XChangeProperty(dpy, root_window, _NET_CLIENT_LIST,
                    XA_WINDOW, 32, PropModeReplace,
                    (unsigned char *)ewmh_client_list, nclients);
}

void ewmh_client_list_remove(client_t *client)
{
    int i;

    for (i = 0; i < nclients; i++) {
        if (ewmh_client_list[i] == client->window) {
            while (++i < nclients) {
                ewmh_client_list[i - 1] = ewmh_client_list[i];
            }
            nclients--;
            debug(("\tRemoving window %s from _NET_CLIENT_LIST, %d clients\n",
                   client_dbg(client), nclients));
            XChangeProperty(dpy, root_window, _NET_CLIENT_LIST,
                            XA_WINDOW, 32, PropModeReplace,
                            (unsigned char *)ewmh_client_list, nclients);
            return;
        }
    }
    debug(("\tClient not found in ewmh_client_list_remove\n"));
    return;
}

void ewmh_stacking_list_update(Window *w, int nwindows)
{
    debug(("\tUpdating _NET_CLIENT_LIST_STACKING, i=%d\n", nwindows));
    XChangeProperty(dpy, root_window, _NET_CLIENT_LIST_STACKING,
                    XA_WINDOW, 32, PropModeReplace,
                    (unsigned char *)w, nwindows);
}
