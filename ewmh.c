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

#include "xwm.h"
#include "ewmh.h"
#include "malloc.h"
#include "debug.h"
#include "focus.h"
#include "kill.h"

/*
 * TODO:
 * 
 * - figure out if _NET_CLIENT_LIST[_STACKING] has windows on all workspaces
 * - use _NET_WORKAREA for move/resize/placement
 * - _NET_WM_MOVERESIZE, should be easy
 * x honor _NET_WM_DESKTOP, also omnipresence
 * x _NET_WM_WINDOW_TYPE:
 *   DESKTOP:  sticky, no title, omnipresent, SkipCycle, AlwaysOnBottom,
 *             don't bind mouse or keyboard, force click-to-focus
 *   DOCK:  sticky, no title, omnipresent?, SkipCycle, AlwaysOnTop
 *   TOOLBAR:  SkipCycle, modal?
 *   MENU:  SkipCycle, modal?
 *   DIALOG:  nothing special
 *   NORMAL:  nothing special
 * - _NET_WM_STATE:  
 *   MODAL:  nothing special
 *   STICKY:  sticky
 *   MAXMIZED_HORIZ, VERT:  separate maximization states
 *   SHADED:  add a shaded state, only for clients with titlebars
 *   SKIP_TASKBAR:  skip alt-tab
 *   SKIP_PAGER:  nothing special
 * - _NET_WM_STRUT:  recalculate _NET_WORKAREA
 * - _NET_WM_PING, _NET_WM_PID
 * - kicker has a KEEP_ON_TOP WM_STATE not mentioned in EWMH 1.1
 * - some properties must be updated on window if set by ahwm
 * - proxy clicks for GNOME
 * 
 * Need to implement horiz, and vert max., sticky windows.
 */

/* bitmasks for _WIN_STATE: */
#define WIN_STATE_STICKY          (1<<0) /*everyone knows sticky*/
#define WIN_STATE_MINIMIZED       (1<<1) /*Reserved - definition is unclear*/
#define WIN_STATE_MAXIMIZED_VERT  (1<<2) /*window in maximized V state*/
#define WIN_STATE_MAXIMIZED_HORIZ (1<<3) /*window in maximized H state*/  
#define WIN_STATE_HIDDEN          (1<<4) /*not on taskbar but window visible*/
#define WIN_STATE_SHADED          (1<<5) /*shaded (MacOS / Afterstep style)*/
#define WIN_STATE_HID_WORKSPACE   (1<<6) /*not on current desktop*/
#define WIN_STATE_HID_TRANSIENT   (1<<7) /*owner of transient is hidden*/
#define WIN_STATE_FIXED_POSITION  (1<<8) /*window is fixed in position even*/
#define WIN_STATE_ARRANGE_IGNORE  (1<<9) /*ignore for auto arranging*/

/* bitmasks for _WIN_HINTS: */
#define WIN_HINTS_SKIP_FOCUS      (1<<0) /*"alt-tab" skips this win*/
#define WIN_HINTS_SKIP_WINLIST    (1<<1) /*do not show in window list*/
#define WIN_HINTS_SKIP_TASKBAR    (1<<2) /*do not show on taskbar*/
#define WIN_HINTS_GROUP_TRANSIENT (1<<3) /*Reserved - definition is unclear*/
#define WIN_HINTS_FOCUS_ON_CLICK  (1<<4) /*app only accepts focus if clicked*/

/* values for _WIN_LAYER: */
#define WIN_LAYER_DESKTOP                0
#define WIN_LAYER_BELOW                  2
#define WIN_LAYER_NORMAL                 4
#define WIN_LAYER_ONTOP                  6
#define WIN_LAYER_DOCK                   8
#define WIN_LAYER_ABOVE_DOCK             10 
#define WIN_LAYER_MENU                   12

/*
 * EWMH 1.1 does not clearly state which atoms are "hints" and belong
 * in _NET_SUPPORTED_HINTS.  Not much of a specification.  I'm putting
 * every atom EWMH mentions into _NET_SUPPORTED_HINTS, even those
 * which don't really look like "hints."
 */

#define NO_SUPPORTED_HINTS 35

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
static Atom _NET_WM_PING;

static Atom _WIN_SUPPORTING_WM_CHECK, _WIN_PROTOCOLS, _WIN_LAYER;
static Atom _WIN_HINTS, _WIN_APP_STATE, _WIN_EXPANDED_SIZE, _WIN_ICONS;
static Atom _WIN_WORKSPACE, _WIN_WORKSPACE_COUNT, _WIN_STATE;
static Atom _WIN_WORKSPACE_NAMES, _WIN_CLIENT_LIST, _WIN_SUPPORTED;

Atom _NET_WM_STRUT, _NET_WM_STATE, _NET_WM_WINDOW_TYPE, _NET_WM_DESKTOP;

/* EWMH 1.1 claims this is supposed to be "UTF-8_STRING" (which
 * is a perfectly OK atom identifier), but it's actually
 * "UTF8_STRING" (NB, no dash).  We have to choose to support
 * one or the other until they choose to fix EWMH, so I'll
 * choose the correct one and completely ignore the broken
 * "specification."  This is an really inexcusable mistake. */
static Atom UTF8_STRING;

static Window ewmh_window;

static void no_titlebar(client_t *client);
static void click_to_focus(client_t *client);
static void skip_cycle(client_t *client);
void ewmh_to_desktop(client_t *client);
void ewmh_to_dock(client_t *client);
void ewmh_to_normal(client_t *client);
void ewmh_to_dialog(client_t *client);
void ewmh_to_menu(client_t *client);
void ewmh_to_toolbar(client_t *client);
Bool ewmh_handle_clientmessage(XClientMessageEvent *xevent);

void ewmh_init()
{
    long *l;
    int i;
    XSetWindowAttributes xswa;
    Atom supported[NO_SUPPORTED_HINTS];
    char workspace_name[32];

    l = malloc(4 * nworkspaces * sizeof(long));
    if (l == NULL) {
        perror("XWM: ewmh_init: malloc");
        fprintf(stderr, "XWM: this is a fatal error, quitting.\n");
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
    _NET_WM_STATE_STICKY = XInternAtom(dpy, "FIXME_NET_WM_STATE_STICKY", False);
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
    _NET_WM_STRUT = XInternAtom(dpy, "_NET_WM_STRUT", False);

    _WIN_SUPPORTING_WM_CHECK =
        XInternAtom(dpy, "_WIN_SUPPORTING_WM_CHECK", False);
    _WIN_SUPPORTING_WM_CHECK =
        XInternAtom(dpy, "_WIN_SUPPORTING_WM_CHECK", False);
    _WIN_PROTOCOLS = XInternAtom(dpy, "_WIN_PROTOCOLS", False);
    _WIN_LAYER = XInternAtom(dpy, "_WIN_LAYER", False);
    _WIN_HINTS = XInternAtom(dpy, "_WIN_HINTS", False);
    _WIN_APP_STATE = XInternAtom(dpy, "_WIN_APP_STATE", False);
    _WIN_EXPANDED_SIZE = XInternAtom(dpy, "_WIN_EXPANDED_SIZE", False);
    _WIN_ICONS = XInternAtom(dpy, "_WIN_ICONS", False);
    _WIN_WORKSPACE = XInternAtom(dpy, "_WIN_WORKSPACE", False);
    _WIN_WORKSPACE_COUNT = XInternAtom(dpy, "_WIN_WORKSPACE_COUNT", False);
    _WIN_STATE = XInternAtom(dpy, "_WIN_STATE", False);
    _WIN_WORKSPACE_NAMES = XInternAtom(dpy, "_WIN_WORKSPACE_NAMES", False);
    _WIN_CLIENT_LIST = XInternAtom(dpy, "_WIN_CLIENT_LIST", False);
    _WIN_SUPPORTED = XInternAtom(dpy, "_WIN_SUPPORTED", False);

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
    supported[34] = _NET_DESKTOP_NAMES;
    supported[35] = _NET_WM_WINDOW_TYPE;
    supported[36] = _NET_WM_STATE;
    supported[37] = _NET_WM_STRUT;

    XChangeProperty(dpy, root_window, _NET_SUPPORTED,
                    XA_ATOM, 32, PropModeReplace,
                    (unsigned char *)supported, NO_SUPPORTED_HINTS);

    supported[0] = _WIN_LAYER;
    supported[1] = _WIN_STATE;
    supported[2] = _WIN_HINTS;
    supported[3] = _WIN_APP_STATE;
    supported[4] = _WIN_EXPANDED_SIZE;
    supported[5] = _WIN_ICONS;
    supported[6] = _WIN_WORKSPACE;
    supported[7] = _WIN_WORKSPACE_COUNT;
    supported[8] = _WIN_WORKSPACE_NAMES;
    supported[9] = _WIN_CLIENT_LIST;

    XChangeProperty(dpy, root_window, _WIN_SUPPORTED,
                    XA_ATOM, 32, PropModeReplace,
                    (unsigned char *)supported, 10);
    
    xswa.override_redirect = True;
    ewmh_window = XCreateWindow(dpy, root_window, 0, 0, 1, 1, 0,
                                DefaultDepth(dpy, scr), InputOutput,
                                DefaultVisual(dpy, scr),
                                CWOverrideRedirect, &xswa);
    XChangeProperty(dpy, root_window, _NET_SUPPORTING_WM_CHECK,
                    XA_WINDOW, 32, PropModeReplace,
                    (unsigned char *)&ewmh_window, 1);
    XChangeProperty(dpy, root_window, _WIN_SUPPORTING_WM_CHECK,
                    XA_CARDINAL, 32, PropModeReplace,
                    (unsigned char *)&ewmh_window, 1);
    XChangeProperty(dpy, ewmh_window, _NET_SUPPORTING_WM_CHECK,
                    XA_WINDOW, 32, PropModeReplace,
                    (unsigned char *)&ewmh_window, 1);
    XChangeProperty(dpy, ewmh_window, _WIN_SUPPORTING_WM_CHECK,
                    XA_CARDINAL, 32, PropModeReplace,
                    (unsigned char *)&ewmh_window, 1);
    XChangeProperty(dpy, ewmh_window, _NET_WM_NAME,
                    UTF8_STRING, 8, PropModeReplace,
                    (unsigned char *)"XWM", 4);
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
    XChangeProperty(dpy, root_window, _NET_WORKAREA,
                    XA_CARDINAL, 32, PropModeReplace,
                    (unsigned char *)l, nworkspaces * 4);
    l[0] = 0;
    XChangeProperty(dpy, root_window, _NET_CURRENT_DESKTOP,
                    XA_CARDINAL, 32, PropModeReplace,
                    (unsigned char *)l, 1);
    XChangeProperty(dpy, root_window, _WIN_WORKSPACE,
                    XA_CARDINAL, 32, PropModeReplace,
                    (unsigned char *)l, 1);
    /* We don't really have a system for naming workspaces, so I'll
     * just name them "Workspace 1", etc. so the workspace names will
     * appear in pagers.  I would like to keep any workspace names
     * that are already there, but that would mean I have to ensure
     * the property contains the correct number of strings.  We can't
     * simply look for NULs: we need a full UTF8 parser, which is a
     * non-trivial task.  There is no standard UTF8 library and I
     * refuse to make this window manager rely on nonstandard
     * libraries (does your window manager work out of the box on Irix
     * 10 and SunOS 4?).  Thus, we simply overwrite any property
     * that's already there */
    for (i = 0; i < nworkspaces; i++) {
        /* EWMH is vague about this.  There is no such thing
         * as a list of arrays of some property type.
         * _NET_DESKTOP_NAMES is simply a big array that
         * contains all the desktop names, separated by
         * NULs. */
        snprintf(workspace_name, 32, "Workspace %d", i+1);
        XChangeProperty(dpy, root_window, _NET_DESKTOP_NAMES,
                        UTF8_STRING, 8,
                        i == 0 ? PropModeReplace : PropModeAppend,
                        workspace_name, strlen(workspace_name) + 1);
        XChangeProperty(dpy, root_window, _WIN_WORKSPACE_NAMES,
                        XA_STRING, 8,
                        i == 0 ? PropModeReplace : PropModeAppend,
                        workspace_name, strlen(workspace_name) + 1);
    }
    l[0] = nworkspaces;
    XChangeProperty(dpy, root_window, _WIN_WORKSPACE_COUNT, XA_CARDINAL, 32,
                    PropModeReplace, (unsigned char *)l, 1);
    
}

void ewmh_wm_state_apply(client_t *client)
{
    Atom actual, *states;
    int fmt, i;
    long bytes_after_return, nitems;

    if (XGetWindowProperty(dpy, client->window, _NET_WM_STATE, 0,
                           sizeof(Atom), False, XA_ATOM,
                           &actual, &fmt, &nitems, &bytes_after_return,
                           (unsigned char **)&states) != Success) {
        debug(("XGetWindowProperty(_NET_WM_STATE) FAILED!\n"));
        return;
    }

    if (nitems == 0 || fmt != 32 || actual != XA_ATOM) {
        debug(("nitems = %d, fmt = %d\n", nitems, fmt));
        return;
    }

    for (i = 0; i < nitems; i++) {
        if (states[i] == _NET_WM_STATE_SKIP_PAGER) {
            if (client->cycle_behaviour_set <= HintSet) {
                client->cycle_behaviour = SkipCycle;
                client->cycle_behaviour_set = HintSet;
            }
        }
    }
}

void ewmh_win_state_apply(client_t *client)
{
    Atom actual;
    int fmt;
    unsigned long *hint, bytes_after_return, nitems;

    if (XGetWindowProperty(dpy, client->window, _WIN_STATE, 0,
                           sizeof(Atom), False, XA_CARDINAL,
                           &actual, &fmt, &nitems, &bytes_after_return,
                           (unsigned char **)&hint) != Success) {
        debug(("XGetWindowProperty(_WIN_STATE) FAILED!\n"));
        return;
    }
    if (nitems == 0 || fmt != 32 || actual != XA_CARDINAL) {
        debug(("nitems = %d, fmt = %d\n", nitems, fmt));
        return;
    }
    /* FIXME:  must implement */
}

void ewmh_win_hints_apply(client_t *client)
{
    Atom actual;
    int fmt;
    unsigned long *hint, bytes_after_return, nitems;

    if (XGetWindowProperty(dpy, client->window, _WIN_HINTS, 0,
                           sizeof(Atom), False, XA_CARDINAL,
                           &actual, &fmt, &nitems, &bytes_after_return,
                           (unsigned char **)&hint) != Success) {
        debug(("XGetWindowProperty(_WIN_HINTS) FAILED!\n"));
        return;
    }
    if (nitems == 0 || fmt != 32 || actual != XA_CARDINAL) {
        debug(("nitems = %d, fmt = %d\n", nitems, fmt));
        return;
    }
    if (*hint & WIN_HINTS_SKIP_FOCUS) {
        skip_cycle(client);
    }
    if (*hint & WIN_HINTS_FOCUS_ON_CLICK) {
        click_to_focus(client);
    }
}

void ewmh_win_workspace_apply(client_t *client)
{
    ;
}

void ewmh_win_layer_apply(client_t *client)
{
    ;
}

void ewmh_wm_strut_apply(client_t *client)
{
    
}

void ewmh_wm_desktop_apply(client_t *client)
{
    Atom actual;
    int fmt;
    unsigned long *ws, bytes_after_return, nitems;
    
    if (XGetWindowProperty(dpy, client->window, _NET_WM_DESKTOP, 0,
                           sizeof(Atom), False, XA_CARDINAL,
                           &actual, &fmt, &nitems, &bytes_after_return,
                           (unsigned char **)&ws) != Success) {
        debug(("XGetWindowProperty(_NET_WM_DESKTOP) FAILED!\n"));
        return;
    }
    if (nitems == 0 || fmt != 32 || actual != XA_CARDINAL) {
        debug(("nitems = %d, fmt = %d\n", nitems, fmt));
        return;
    }
    if (*ws == 0xFFFFFFFF) {
        if (client->omnipresent_set <= HintSet) {
            client->omnipresent = 1;
            client->omnipresent_set = HintSet;
        }
        return;
    }
    if (client->workspace_set <= HintSet) {
        client->workspace = *ws + 1;
        client->workspace_set = HintSet;
    }
}

void ewmh_window_type_apply(client_t *client)
{
    Atom actual, *types;
    int fmt, i;
    unsigned long bytes_after_return, nitems;

    if (XGetWindowProperty(dpy, client->window, _NET_WM_WINDOW_TYPE, 0,
                           sizeof(Atom), False, XA_ATOM,
                           &actual, &fmt, &nitems, &bytes_after_return,
                           (unsigned char **)&types) != Success) {
        debug(("XGetWindowProperty(_NET_WM_WINDOW_TYPE) FAILED!\n"));
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
}

static void no_titlebar(client_t *client)
{
    if (client->has_titlebar_set <= HintSet) {
        client_remove_titlebar(client);
        client->has_titlebar_set = HintSet;
    }
}

static void click_to_focus(client_t *client)
{
    if (client->focus_policy_set <= HintSet) {
        if (client->focus_policy != ClickToFocus) {
            focus_policy_to_click(client);
        }
        client->focus_policy = ClickToFocus;
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

/*
 * make a window a "desktop" window.  No titlebar, omnipresent, Skip
 * alt-tab, force click-to-focus, force pass-through-click,
 * always-on-bottom.  Of course, user can override any of these using
 * unconditional option settings.
 * 
 * FIXME:  also sticky, don't bind anything.
 */

void ewmh_to_desktop(client_t *client)
{
    no_titlebar(client);
    skip_cycle(client);
    click_to_focus(client);
    if (client->omnipresent_set <= HintSet) {
        client->omnipresent = 1;
        client->omnipresent_set = HintSet;
    }
    if (client->always_on_bottom_set <= HintSet) {
        if (client->always_on_bottom == 0) {
            stacking_remove(client);
            client->always_on_bottom = 1;
            client->always_on_bottom_set = HintSet;
            stacking_add(client);
        }
    }
}

/* Remove titlebar, always-on-top, click-to-focus
 * Could also make omnipresent, but client
 * can specify that separately using _NET_WM_DESKTOP
 */
void ewmh_to_dock(client_t *client)
{
    no_titlebar(client);
    click_to_focus(client);
    skip_cycle(client);
    if (client->always_on_top_set <= HintSet) {
        stacking_remove(client);
        client->always_on_top = 1;
        client->always_on_top_set = HintSet;
        stacking_add(client);
    }
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
 * implies that these hints may only be set once before mapping and
 * cannot change (and EWMH should make this explicit if this is so).
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

/* these two might be better with no focus: need experimentation, examples */
void ewmh_to_menu(client_t *client)
{
    no_titlebar(client);
    skip_cycle(client);
    click_to_focus(client);
    if (client->always_on_top_set <= HintSet) {
        stacking_remove(client);
        client->always_on_top = 1;
        client->always_on_top_set = HintSet;
        stacking_add(client);
    }
}

void ewmh_to_toolbar(client_t *client)
{
    no_titlebar(client);
    skip_cycle(client);
    click_to_focus(client);
}

Bool ewmh_handle_clientmessage(XClientMessageEvent *xevent)
{
    client_t *client;
    long data;
    
    if (xevent->message_type == _NET_CURRENT_DESKTOP) {
        data = xevent->data.l[0] + 1;
        workspace_goto((unsigned int)data);
        return True;
    } else if (xevent->message_type == _NET_ACTIVE_WINDOW) {
        client = client_find(xevent->window);
        if (client != NULL) {
            focus_set(client, CurrentTime);
        }
        return True;
    } else if (xevent->message_type == _NET_CLOSE_WINDOW) {
        kill_nicely((XEvent *)xevent, NULL); /* ugly but works ok */
        return True;
    } else if (xevent->message_type == _NET_WM_DESKTOP) {
        client = client_find(xevent->window);
        data = xevent->data.l[0];
        if (data == 0xFFFFFFFF) {
            if (client->omnipresent_set <= HintSet) {
                client->omnipresent = 1;
                client->omnipresent_set = HintSet;
            }
        } else {
            if (client->workspace_set <= HintSet) {
                client->workspace_set = HintSet;
                workspace_client_moveto(client, (unsigned int)(data + 1));
            }
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
    XChangeProperty(dpy, root_window, _WIN_WORKSPACE,
                    XA_CARDINAL, 32, PropModeReplace,
                    (unsigned char *)&l, 1);
}

void ewmh_active_window_update()
{
    XChangeProperty(dpy, root_window, _NET_ACTIVE_WINDOW,
                    XA_WINDOW, 32, PropModeReplace,
                    (unsigned char *)&(focus_current->window), 1);
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
                perror("XWM: malloc EWMH client list");
                return;
            }
            nwindows_allocated = nclients + 1;
        } else {
            tmp = Realloc(ewmh_client_list, 2*nwindows_allocated*sizeof(Window));
            if (tmp == NULL) {
                perror("XWM: realloc EWMH client list");
                return;
            }
            ewmh_client_list = tmp;
            nwindows_allocated *= 2;
        }
    }
    ewmh_client_list[nclients++] = client->window;
    debug(("\tAdding window %#lx to _NET_CLIENT_LIST, %d clients\n",
           client->window, nclients));
    XChangeProperty(dpy, root_window, _NET_CLIENT_LIST,
                    XA_WINDOW, 32, PropModeReplace,
                    (unsigned char *)ewmh_client_list, nclients);
    XChangeProperty(dpy, root_window, _WIN_CLIENT_LIST,
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
            debug(("\tRemoving window %#lx from _NET_CLIENT_LIST, %d clients\n",
                   client->window, nclients));
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
    debug(("Updating _NET_CLIENT_LIST_STACKING, i=%d\n", nwindows));
    XChangeProperty(dpy, root_window, _NET_CLIENT_LIST_STACKING,
                    XA_WINDOW, 32, PropModeReplace,
                    (unsigned char *)w, nwindows);
}
