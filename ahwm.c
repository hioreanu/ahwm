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
#include <signal.h>

#include "ahwm.h"
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
#include "paint.h"
#include "mwm.h"
#include "colormap.h"

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
GC extra_gc4;
/* Give a nice default font.  Should be available almost all the time. */
char *ahwm_fontname = "-*-helvetica-bold-r-*-*-12-*-*-*-*-*-*-*";
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

static char *argv0;

static int already_running_windowmanager;
static int (*default_error_handler)(Display *, XErrorEvent *);

static int tmp_error_handler(Display *dpy, XErrorEvent *error);
static int error_handler(Display *dpy, XErrorEvent *error);
static void scan_windows();
static void remove_titlebars();
static void sigterm(int signo);

#ifdef DEBUG
static void mark(XEvent *e, struct _arglist *ignored);
#endif

int main(int argc, char **argv)
{
    XEvent event;
    int xfd, junk;
    XGCValues xgcv;
    XColor xcolor, junk2;
    
    argv0 = argv[0];
#ifdef DEBUG
    /* set non-buffered */
    setvbuf(stdout, NULL, _IONBF, 0);
#endif /* DEBUG */
    dpy = XOpenDisplay(NULL);
    if (dpy == NULL) {
        fprintf(stderr, "AHWM: Could not open display '%s'\n", XDisplayName(NULL));
        exit(1);
    }
    xfd = ConnectionNumber(dpy);
    fcntl(xfd, F_SETFD, FD_CLOEXEC);
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
    XSync(dpy, False);
    if (already_running_windowmanager) {
        fprintf(stderr,
                "AHWM: You're already running a window manager.  Quitting.\n");
        exit(1);
    }

    /* get the default error handler and set our error handler */
    XSetErrorHandler(NULL);
    default_error_handler = XSetErrorHandler(error_handler);

#if 0
    /* using this is a really bad idea, but this is where the call
     * goes if it's needed */
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

    /* call initialization functions of various modules (order matters) */

    colormap_init();
    client_init();
    cursor_init();
    paint_init();
    keyboard_init();

#ifdef DEBUG
    keyboard_bind("Control | Alt | Shift | l", KEYBOARD_DEPRESS,
                  mark, NULL);
#endif
    
    prefs_init();
    icccm_init();
    ewmh_init();
    mwm_init();
    focus_init();

    /* we need to set ahwm_fontname (in prefs_init())
     * before we load the font and create the GCs */
    fontstruct = XLoadQueryFont(dpy, ahwm_fontname);
    if (fontstruct == NULL) {
        fprintf(stderr, "AHWM: Could not load font \"%s\".  "
                "Using default font instead.\n", ahwm_fontname);
        /* now this font name should never fail */
        ahwm_fontname = "-*-*-*-*-*-*-*-*-*-*-*-*-*-*";
        fontstruct = XLoadQueryFont(dpy, ahwm_fontname);
        if (fontstruct == NULL) {
            /* Could not load any fonts.  Might happen if user uses
             * and misconfigures font server or something.  Might try
             * to continue without titlebars. */
            fprintf(stderr, "AHWM: Could not load any fonts at all.\n");
            fprintf(stderr, "AHWM: This is a fatal error, quitting.\n");
            exit(1);
        }
    }
    TITLE_HEIGHT = fontstruct->max_bounds.ascent + fontstruct->max_bounds.descent;
    paint_ascent = fontstruct->max_bounds.ascent;

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
    extra_gc4 = XCreateGC(dpy, root_window,
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

    atexit(remove_titlebars);
    atexit(focus_save_stacks);
    signal(SIGTERM, sigterm);
    
    scan_windows();
    focus_load_stacks();
    
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
            && error->error_code == BadDrawable)
        || (error->request_code == X_PolyFillRectangle
            && error->error_code == BadDrawable)
        || (error->request_code == X_PolySegment
            && error->error_code == BadDrawable)
        || (error->request_code == X_ConfigureWindow
            && error->error_code == BadMatch))
        return 0;
    fprintf(stderr, "AHWM: ");
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
void run_program(XEvent *e, struct _arglist *args)
{
    pid_t pid;
    char *progname;
    struct _arglist *p;

    fflush(stdout);
    fflush(stderr);
    for (p = args; p != NULL; p = p->arglist_next) {
        if (p->arglist_arg->type_type != STRING) {
            fprintf(stderr, "AHWM: type error\n"); /* FIXME */
            continue;
        }
        progname = p->arglist_arg->type_value.stringval;
        if ( (pid = fork()) == 0) {
            close(ConnectionNumber(dpy));
            if (fork() == 0) {
                execl("/bin/sh", "/bin/sh", "-c", progname, NULL);
            }
            _exit(0);
        } else if (pid > 0) {
            wait(NULL);
        } else {
            perror("AHWM: fork");
        }
    }
}

void ahwm_quit(XEvent *e, struct _arglist *ignored)
{
#ifdef DEBUG
    printf("AHWM: ahwm_quit called, quitting\n");
#endif
    exit(0);
}

void ahwm_restart(XEvent *e, struct _arglist *ignored)
{
    focus_save_stacks();
    remove_titlebars();
    fflush(stderr);
    fflush(stdout);
    execlp(argv0, argv0, NULL);
    fprintf(stderr,
            "AHWM:  Could not restart.  AHWM was not called with an\n"
            "absolute pathname and the ahwm binary is not in your PATH.\n");
    fflush(stderr);
    _exit(1);
}

static void remove_titlebars()
{
    static int exiting = 0;
    int i, n;
    Window *wins, junk;
    client_t *client;

    if (exiting == 1) _exit(1);
    exiting = 1;

    XQueryTree(dpy, root_window, &junk, &junk, &wins, &n);
    for (i = 0; i < n; i++) {
        client = client_find(wins[i]);
        if (client != NULL) {
            client_remove_titlebar(client);
        }
    }
    if (wins != NULL) XFree(wins); /* not needed, but oh well */
    XCloseDisplay(dpy);
    close(ConnectionNumber(dpy));
}

/*
 * call atexit() functions instead of terminating immediately on SIGTERM.
 * 
 * Problem is that we aren't supposed to call any Xlib functions in a
 * signal handler as Xlib isn't reentrant.  This way, we just do all
 * the Xlib stuff in the atexit functions instead of in the signal
 * handler.  We still get all sorts of X IO errors doing it like this,
 * however.  Not sure what's going on with that.
 */

static void sigterm(int signo)
{
    exit(0);
}

#ifdef DEBUG
/* make it easier to read debug output */
static void mark(XEvent *e, struct _arglist *ignored)
{
    printf("-------------------------------------");
    printf(" MARK ");
    printf("-------------------------------------\n");
}
#endif