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

/*
 * If you're just starting here, you may want to read the header files
 * before the implementation files when reading my code.
 */

#include "config.h"

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xproto.h>
#include <X11/Xresource.h>      /* needed for XUniqueContext in Xutil.h */
#include <X11/Xutil.h>

#ifdef SHAPE
#include <X11/extensions/shape.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>

#include "xwm.h"
#include "event.h"
#include "client.h"
#include "keyboard-mouse.h"
#include "focus.h"
#include "cursor.h"
#include "move-resize.h"
#include "kill.h"
#include "workspace.h"
#include "icccm.h"
#include "ewmh.h"

Display *dpy;
int scr;
int scr_height;
int scr_width;
unsigned long black;
unsigned long white;
Window root_window;
GC root_white_fg_gc;
GC root_black_fg_gc;
GC root_invert_gc;
GC extra_gc1;
GC extra_gc2;
GC extra_gc3;
XFontStruct *fontstruct;
Atom WM_STATE;
Atom WM_CHANGE_STATE;
Atom WM_TAKE_FOCUS;
Atom WM_SAVE_YOURSELF;
Atom WM_DELETE_WINDOW;
Atom WM_PROTOCOLS;

#ifdef SHAPE
int shape_supported;
int shape_event_base;
#endif

void run_program(XEvent *e, void *arg);
#ifdef DEBUG
void mark(XEvent *e, void *arg);
#endif

static int already_running_windowmanager;
static int (*default_error_handler)(Display *, XErrorEvent *);
static int tmp_error_handler(Display *dpy, XErrorEvent *error);
static int error_handler(Display *dpy, XErrorEvent *error);
static void scan_windows();

/*
 * 1.  Open a display
 * 2.  Set up some convenience global variables
 * 3.  Select the X events we want to see
 * 4.  Die if we can't do that because some other windowmanager is running
 * 5.  Set the X error handler
 * 6.  Define the root window cursor and other cursors
 * 7.  Create a XContexts for the window management functions
 * 8.  Scan already-created windows and manage them
 * 9.  Go into a select() loop waiting for events
 * 10. Dispatch events
 * 
 * FIXME:  move everything not defined in this file into various
 * _init functions, document this in xwm.h
 */

int main(int argc, char **argv)
{
    XEvent event;
    int xfd, junk;
    XGCValues xgcv;
    XColor xcolor, junk2;
    
#ifdef DEBUG
    setvbuf(stdout, NULL, _IONBF, 0);
#endif /* DEBUG */
    dpy = XOpenDisplay(NULL);
    if (dpy == NULL) {
        fprintf(stderr, "XWM: Could not open display '%s'\n", XDisplayName(NULL));
        exit(1);
    }
    scr = DefaultScreen(dpy);
    root_window = DefaultRootWindow(dpy);
    scr_height = DisplayHeight(dpy, scr);
    scr_width = DisplayWidth(dpy, scr);
    black = BlackPixel(dpy, scr);
    white = WhitePixel(dpy, scr);

    already_running_windowmanager = 0;
    XSetErrorHandler(tmp_error_handler);
    /* this causes an error if some other window manager is running */
    XSelectInput(dpy, root_window, ROOT_EVENT_MASK);
    /* and we want to ensure the server processes the request now */
    XSync(dpy, 0);
    if (already_running_windowmanager) {
        fprintf(stderr, "XWM: You're already running a window manager, silly.\n");
        exit(1);
    }

    printf("--------------------------------");
    printf(" Welcome to XWM ");
    printf("--------------------------------\n");
    fflush(stdout);

    /* get the default error handler and set our error handler */
    XSetErrorHandler(NULL);
    default_error_handler = XSetErrorHandler(error_handler);
#ifdef DEBUG_BAD_IDEA
    XSynchronize(dpy, True);
#endif

    /* set up our global variables */
    WM_STATE = XInternAtom(dpy, "WM_STATE", False);
    WM_CHANGE_STATE = XInternAtom(dpy, "WM_CHANGE_STATE", False);
    WM_TAKE_FOCUS = XInternAtom(dpy, "WM_TAKE_FOCUS", False);
    WM_SAVE_YOURSELF = XInternAtom(dpy, "WM_SAVE_YOURSELF", False);
    WM_DELETE_WINDOW = XInternAtom(dpy, "WM_DELETE_WINDOW", False);
    WM_PROTOCOLS = XInternAtom(dpy, "WM_PROTOCOLS", False);

#ifdef SHAPE
    shape_supported = XShapeQueryExtension(dpy, &shape_event_base, &junk);
#endif
    
    fontstruct = XLoadQueryFont(dpy,
                                "-*-helvetica-bold-r-*-*-12-*-*-*-*-*-*-*");

    if (XAllocNamedColor(dpy, DefaultColormap(dpy, scr), "#E0E0E0",
                         &xcolor, &junk2) == 0) {
        xgcv.foreground = white;
    } else {
        xgcv.foreground = xcolor.pixel;
    }
    
    xgcv.function = GXcopy;
    xgcv.plane_mask = AllPlanes;
    xgcv.background = black;
    xgcv.line_width = 0;
    xgcv.line_style = LineSolid;
    xgcv.cap_style = CapButt;
    xgcv.join_style = JoinMiter;
    xgcv.font = fontstruct->fid;
    xgcv.subwindow_mode = IncludeInferiors;
    
    root_white_fg_gc = XCreateGC(dpy, root_window,
                                 GCForeground | GCBackground
                                 | GCLineWidth | GCLineStyle
                                 | GCCapStyle | GCJoinStyle
                                 | GCFont | GCFunction
                                 | GCPlaneMask | GCSubwindowMode,
                                 &xgcv);
    extra_gc1 = XCreateGC(dpy, root_window,
                          GCForeground | GCBackground
                          | GCLineWidth | GCLineStyle
                          | GCCapStyle | GCJoinStyle
                          | GCFont | GCFunction
                          | GCPlaneMask | GCSubwindowMode,
                          &xgcv);
    extra_gc2 = XCreateGC(dpy, root_window,
                          GCForeground | GCBackground
                          | GCLineWidth | GCLineStyle
                          | GCCapStyle | GCJoinStyle
                          | GCFont | GCFunction
                          | GCPlaneMask | GCSubwindowMode,
                          &xgcv);
    extra_gc3 = XCreateGC(dpy, root_window,
                          GCForeground | GCBackground
                          | GCLineWidth | GCLineStyle
                          | GCCapStyle | GCJoinStyle
                          | GCFont | GCFunction
                          | GCPlaneMask | GCSubwindowMode,
                          &xgcv);
    xgcv.function = GXxor;
    root_invert_gc = XCreateGC(dpy, root_window,
                                 GCForeground | GCBackground
                                 | GCLineWidth | GCLineStyle
                                 | GCCapStyle | GCJoinStyle
                                 | GCFont | GCFunction
                                 | GCPlaneMask | GCSubwindowMode,
                                 &xgcv);
    xgcv.function = GXcopy;
    xgcv.background = white;
    xgcv.foreground = black;
    root_black_fg_gc = XCreateGC(dpy, root_window,
                                 GCForeground | GCBackground
                                 | GCLineWidth | GCLineStyle
                                 | GCCapStyle | GCJoinStyle
                                 | GCFont | GCFunction
                                 | GCPlaneMask | GCSubwindowMode,
                                 &xgcv);

    window_context = XUniqueContext(); /* FIXME:  move to client_init */
    frame_context = XUniqueContext();
    title_context = XUniqueContext();

    cursor_init();
    icccm_init();
    ewmh_init();
    keyboard_init();
    workspace_update_color();

#ifdef DEBUG
    keyboard_bind("Control | Alt | Shift | l", KEYBOARD_DEPRESS,
                  mark, NULL);
#endif
    keyboard_bind("Control | Alt | Shift | t", KEYBOARD_DEPRESS,
                  run_program, "xterm");
    keyboard_bind("Control | Alt | Shift | n", KEYBOARD_DEPRESS,
                  run_program, "netscape");
    keyboard_bind("Control | Alt | Shift | k", KEYBOARD_DEPRESS,
                  run_program, "konqueror");
    keyboard_bind("Control | Alt | Shift | e", KEYBOARD_DEPRESS,
                  run_program, "emacs");
    keyboard_bind("Alt | Tab", KEYBOARD_DEPRESS, focus_alt_tab, NULL);
    keyboard_bind("Alt | Shift | Tab", KEYBOARD_DEPRESS,
                  focus_alt_tab, NULL);
    keyboard_bind("Control | Alt | Shift | m", KEYBOARD_DEPRESS,
                  move_client, NULL);
    keyboard_bind("Control | Alt | Shift | r", KEYBOARD_DEPRESS,
                  resize_client, NULL);
    keyboard_bind("Control | Alt | 1", KEYBOARD_DEPRESS,
                  workspace_client_moveto_bindable, (void *)1);
    keyboard_bind("Control | Alt | 2", KEYBOARD_DEPRESS,
                  workspace_client_moveto_bindable, (void *)2);
    keyboard_bind("Control | Alt | 3", KEYBOARD_DEPRESS,
                  workspace_client_moveto_bindable, (void *)3);
    keyboard_bind("Control | Alt | 4", KEYBOARD_DEPRESS,
                  workspace_client_moveto_bindable, (void *)4);
    keyboard_bind("Control | Alt | 5", KEYBOARD_DEPRESS,
                  workspace_client_moveto_bindable, (void *)5);
    keyboard_bind("Control | Alt | 6", KEYBOARD_DEPRESS,
                  workspace_client_moveto_bindable, (void *)6);
    keyboard_bind("Control | Alt | 7", KEYBOARD_DEPRESS,
                  workspace_client_moveto_bindable, (void *)7);
    keyboard_bind("Alt | 1", KEYBOARD_DEPRESS,
                  workspace_goto_bindable, (void *)1);
    keyboard_bind("Alt | 2", KEYBOARD_DEPRESS,
                  workspace_goto_bindable, (void *)2);
    keyboard_bind("Alt | 3", KEYBOARD_DEPRESS,
                  workspace_goto_bindable, (void *)3);
    keyboard_bind("Alt | 4", KEYBOARD_DEPRESS,
                  workspace_goto_bindable, (void *)4);
    keyboard_bind("Alt | 5", KEYBOARD_DEPRESS,
                  workspace_goto_bindable, (void *)5);
    keyboard_bind("Alt | 6", KEYBOARD_DEPRESS,
                  workspace_goto_bindable, (void *)6);
    keyboard_bind("Alt | 7", KEYBOARD_DEPRESS,
                  workspace_goto_bindable, (void *)7);
    keyboard_bind("Control | Alt | Shift | q", KEYBOARD_RELEASE,
                  keyboard_quote, NULL);
    mouse_bind("Alt | Button1", MOUSE_DEPRESS, MOUSE_FRAME,
               move_client, NULL);
    mouse_bind("Alt | Button3", MOUSE_DEPRESS, MOUSE_FRAME,
               resize_client, NULL);
    mouse_bind("Button1", MOUSE_DEPRESS, MOUSE_TITLEBAR,
               move_client, NULL);
    mouse_bind("Button2", MOUSE_DEPRESS, MOUSE_TITLEBAR,
               kill_nicely, NULL);
    mouse_bind("Control | Button2", MOUSE_DEPRESS, MOUSE_TITLEBAR,
               kill_with_extreme_prejudice, NULL);
    mouse_bind("Button3", MOUSE_RELEASE, MOUSE_TITLEBAR,
               resize_maximize, NULL);

    focus_init();
    printf("Start parsing\n");
    prefs_init();
    printf("Done parsing\n");
    scan_windows();
    
    xfd = ConnectionNumber(dpy);
    fcntl(xfd, F_SETFD, FD_CLOEXEC);

    XSync(dpy, 0);

    for (;;) {
        event_get(xfd, &event);
        event_dispatch(&event);
    }
    return 0;
}

/*
 * Error handler while we see if we have another
 * window manager already running
 */

static int tmp_error_handler(Display *dpy, XErrorEvent *error)
{
    already_running_windowmanager = 1;
    return -1;
}

/*
 * Xwm's error handler.  Sometimes we'll try to manipulate
 * a window that's just been destroyed.  There's no way
 * one can avoid this, so we simply ignore such errors.
 * Other types of errors call Xlib's default error handler.
 */

static int error_handler(Display *dpy, XErrorEvent *error)
{
    if (error->error_code == BadWindow
        || (error->request_code == X_SetInputFocus
            && error->error_code == BadMatch)
        || (error->request_code == X_PolyText8
            && error->error_code == BadDrawable))
        return 0;
    fprintf(stderr, "XWM: ");
    return default_error_handler(dpy, error); /* calls exit() */
}

/*
 * Set up all windows that were here before the windowmanager started
 */

static void scan_windows()
{
    int i, n;
    Window *wins, junk;
    client_t *client;

    XQueryTree(dpy, root_window, &junk, &junk, &wins, &n);
    for (i = 0; i < n; i++) {
        client = client_create(wins[i]);
    }
    if (wins != NULL) XFree(wins);
}

/* standard double fork trick, don't leave zombies */
void run_program(XEvent *e, void *arg)
{
    pid_t pid;
    
    if ( (pid = fork()) == 0) {
        close(ConnectionNumber(dpy));
        if (fork() == 0) {
            execl("/bin/sh", "/bin/sh", "-c", arg, NULL);
        }
        exit(0);
    } else if (pid > 0) {
        wait(NULL);
    } else {
        perror("XWM: fork");
    }
}

#ifdef DEBUG
/* make it easier to parse debug output */
void mark(XEvent *e, void *arg)
{
    printf("-------------------------------------");
    printf(" MARK ");
    printf("-------------------------------------\n");
}
#endif

#ifndef HAVE_STRDUP
/* 'autoscan' tells me 'strdup()' isn't portable (?) */
char *strdup(char *s)
{
    char *n;

    n = malloc(strlen(s));
    if (n == NULL) return NULL;
    strcpy(n, s);
    return n;
}
#endif /* ! HAVE_STRDUP */
