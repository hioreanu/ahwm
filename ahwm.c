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
#include "prefs.h"

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
Atom _AHWM_MOVE_OFFSET;

#ifdef SHAPE
int shape_supported;
int shape_event_base;
#endif

static char *argv0;             /* used by segfault handler for exec() */

static int already_running_windowmanager;
static int (*default_error_handler)(Display *, XErrorEvent *);

static int tmp_error_handler(Display *dpy, XErrorEvent *error);
static int error_handler(Display *dpy, XErrorEvent *error);
static void scan_windows();
static void reposition(Window w);
static void sigterm(int signo);
static void sigsegv(int signo);
static void crash_handler();
static void crashwin_draw(GC gc, Window toplevel, Window button1,
                          Window button2, Window button3,
                          int height, int width);


#ifdef DEBUG
static void mark(XEvent *e, struct _arglist *ignored);
#endif

int main(int argc, char **argv)
{
    XEvent event;
    int xfd, junk;
    XGCValues xgcv;
    XColor xcolor, junk2;
    sigset_t set;

/* 
 * We may enter main() with SIGSEGV or SIGBUS blocked.  If we get a
 * SEGV, we jump into our segfault handler function.  Just before
 * jumping into this function, kernel blocks SIGSEGV to avoid signal
 * loops within signal handlers.  Blocked signals are inherited across
 * exec(), so we may be running with SEGV blocked.  We might also be
 * able to do the sigprocmask in the SEGV handler, but I don't like
 * that idea - we don't know what sigprocmask might do (it might try a
 * malloc when we just corrupted our entire heap).
 * 
 * Now we unblock SIGSEGV and SIGBUS.  Want to do very first thing in
 * program, just in case one of our initialization segfaults (and if
 * we're ignoring SIGSEGV when that happens, program simply "freezes"
 * at point of failure).
 * 
 * I lost a couple hours trying to figure out why my segfault handler
 * was only called on the first (original) invocation and the program
 * was freezing upon segfaulting in the second invocation (after the
 * exec).  Well, now you also know the trick, and you won't lose those
 * two hours.
 * 
 * Here's a nice discussion of the topic:
 * http://lists.community.tummy.com/pipermail/linux-ha-dev/ \
 * 2001-September/002464.html
 */

    sigemptyset(&set);
    sigprocmask(SIG_SETMASK, &set, NULL);
    argv0 = argv[0];
#ifdef DEBUG
    /* set non-buffered */
    setvbuf(stdout, NULL, _IONBF, 0);
#endif /* DEBUG */
    dpy = XOpenDisplay(NULL);
    if (dpy == NULL) {
        fprintf(stderr,
                "AHWM: Could not open display '%s'\n", XDisplayName(NULL));
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

    if (argc > 1 && strcmp(argv[1], "--segv") == 0) {
        crash_handler();
    }

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
    _AHWM_MOVE_OFFSET = XInternAtom(dpy, "_AHWM_MOVE_OFFSET", False);

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
    kill_init();

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

    atexit(focus_save_stacks);
    signal(SIGTERM, sigterm);
    signal(SIGSEGV, sigsegv);
    signal(SIGBUS, sigsegv);
    
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
 * 
 */

static void scan_windows()
{
    int i, n;
    Window *wins, junk;
    client_t *client;

    XQueryTree(dpy, root_window, &junk, &junk, &wins, &n);
    for (i = 0; i < n; i++) {
        reposition(wins[i]);
        client = client_create(wins[i]);
    }
    if (wins != NULL) XFree(wins);
}

/*
 * When AHWM exits, managed windows will be reparented to root;
 * however, they will not be moved, so the y coordinate will be a few
 * pixels higher, depending on the size of the titlebar.  We now check
 * if a previous invocation of AHWM moved the client window down at
 * all, and move it back if needed.  We store the amount we move a
 * client window in a property on the client window.  The size of the
 * titlebar may have changed in between invocations and the client
 * window may have not been moved at all depending on gravity, so
 * using an integer for this property is much better than a boolean.
 */

static void reposition(Window w)
{
    Window junk;
    int x, y;
    unsigned int junk2, height;
    Atom actual;
    int fmt;
    unsigned long nitems, bytes_after_return;
    int *offset;
    XWindowAttributes xwa;

    if (XGetGeometry(dpy, w, &junk, &x, &y,
                     &junk2, &height, &junk2, &junk2) == 0) {
        return;
    }
    if (XGetWindowProperty(dpy, w, _AHWM_MOVE_OFFSET, 0, 1, False,
                           XA_INTEGER, &actual, &fmt,
                           &nitems, &bytes_after_return,
                           (unsigned char **)&offset) != Success) {
        return;
    }
    if (offset == NULL || fmt != 32 || actual != XA_INTEGER || nitems != 1
        || bytes_after_return != 0 || nitems != 1) {

        if (offset != NULL) XFree(offset);
        return;
    }
    if (*offset == 0) {
        XFree(offset);
        return;
    }

    /* don't move offscreen unless window is already offscreen */
    if ((y <= scr_height && y >= 0)
        && (y + *offset > scr_height || y + *offset + height < 0)) {

        XFree(offset);
        return;
    }
    XMoveWindow(dpy, w, x, y - *offset);
    XFree(offset);
}

/* standard double fork trick, don't leave zombies */
/* anything that takes an 'arglist' argument is a bindable
 * function which appears in the config file; see prefs.c:fn_table */
void run_program(XEvent *e, struct _arglist *args)
{
    pid_t pid;
    char *progname;
    struct _arglist *p;

    fflush(stdout);
    fflush(stderr);
    for (p = args; p != NULL; p = p->arglist_next) {
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
    fflush(stderr);
    fflush(stdout);
    execlp(argv0, argv0, NULL);
    fprintf(stderr,
            "AHWM:  Could not restart.  AHWM was not called with an\n"
            "absolute pathname and the ahwm binary is not in your PATH.\n");
    fflush(stderr);
    _exit(1);
}

/*
 * call atexit() functions instead of terminating immediately on SIGTERM.
 * 
 * Problem is that we aren't supposed to call any Xlib functions in a
 * signal handler as Xlib isn't reentrant.  This might screw things up
 * royally.  We could simply set a flag for the event-dispatcher
 * function.  However, someone might have sent us SIGTERM because AHWM
 * is hung, and we may never get back to the dispatcher function.
 * Can't really come up with an optimal solution, but going ahead and
 * calling the Xlib functions works *some* of the time.
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

/*
 * Well, this is embarrasing.  If this function is called, AHWM
 * has crashed.
 * 
 * It would be most unfortunate if the user is actually working in
 * some window and our crash makes the user lose their work.  In fact,
 * that would really piss me off.
 * 
 * Instead, we pop up an extremely primitive "dialog" box asking the
 * user if they would like to restart.  Restarting generally restores
 * AHWM to the previous state.
 * 
 * Simply 'exec'ing AHWM again with a special hidden argument seems
 * like the most reliable way to ensure we have a sane memory map and
 * that we can use Xlib et al. again.
 * 
 * Hopefully, our libc's 'exec' function depends on very little.  Our
 * entire memory map may be corrupt.  A good example is FreeBSD's libc
 * version of execlp, which does only a couple of stack-based memory
 * allocations, examines the PATH (which may, in fact be corrupt) and
 * jumps into kernel fairly quickly.
 * 
 * Possible problems:
 * 
 * 1.  'argv0' is corrupted.
 * 2.  '*argv0' is corrupted.
 * 3.  heap is corrupted and libc's execlp needs to use heap.
 * 4.  PATH environment variable is corrupted.
 */
static void sigsegv(int signo)
{
    /* third arg points into ro-data segment */
    execlp(argv0, argv0, "--segv", NULL);
}

/*
 * depends on:
 * 
 * dpy
 * scr
 * root_window
 * scr_width
 * scr_height
 * white
 * black
 */

#define CRASHWIN_WIDTH 400
#define CRASHWIN_HEIGHT 500
#define CRASHBUTTON_WIDTH 100
#define CRASHBUTTON_HEIGHT 35
#define CRASHWIN_SPACING 20

/*
 * Not very elegant.
 * 
 * When AHWM crashes, it exec's itself with a special argument.  The
 * exec ensures we have a clean memory map, etc.  We then pop up a
 * dialog asking user if they want to continue (in which case we
 * simply exec again with the special argument), quit, or start TWM
 * instead.
 * 
 * In practice, I haven't seen any place where restarting does not
 * work.  AHWM is mostly stateless, so restarting is pretty
 * transparent.
 * 
 * This "dialog" is butt-ugly, but that's what you get when you want
 * to ensure you don't rely on any toolkit libraries.
 */

static void crash_handler()
{
    Window toplevel, button1, button2, button3;
    XSetWindowAttributes xswa;
    GC gc;
    XGCValues xgcv;
    XFontStruct *xfs;
    XEvent ev;
    
    xswa.background_pixel = white;
    xswa.border_pixel = black;
    xswa.event_mask = ButtonPressMask;
    toplevel = XCreateWindow(dpy, root_window,
                             scr_width / 2 - CRASHWIN_WIDTH / 2,
                             scr_height / 2 - CRASHWIN_HEIGHT / 2,
                             CRASHWIN_WIDTH, CRASHWIN_HEIGHT,
                             0, DefaultDepth(dpy, scr),
                             InputOutput, DefaultVisual(dpy, scr),
                             CWBackPixel, &xswa);
    button1 = XCreateWindow(dpy, toplevel,
                            CRASHWIN_WIDTH / 2 - CRASHBUTTON_WIDTH / 2,
                            CRASHWIN_HEIGHT - CRASHWIN_SPACING
                                            - CRASHBUTTON_HEIGHT,
                            CRASHBUTTON_WIDTH,
                            CRASHBUTTON_HEIGHT,
                            3, DefaultDepth(dpy, scr),
                            InputOutput, DefaultVisual(dpy, scr),
                            CWBackPixel | CWBorderPixel | CWEventMask,
                            &xswa);
    button2 = XCreateWindow(dpy, toplevel,
                            CRASHWIN_WIDTH / 2 - CRASHBUTTON_WIDTH / 2,
                            CRASHWIN_HEIGHT - CRASHWIN_SPACING * 2
                                            - CRASHBUTTON_HEIGHT * 2,
                            CRASHBUTTON_WIDTH,
                            CRASHBUTTON_HEIGHT,
                            3, DefaultDepth(dpy, scr),
                            InputOutput, DefaultVisual(dpy, scr),
                            CWBackPixel | CWBorderPixel | CWEventMask,
                            &xswa);
    button3 = XCreateWindow(dpy, toplevel,
                            CRASHWIN_WIDTH / 2 - CRASHBUTTON_WIDTH / 2,
                            CRASHWIN_HEIGHT - CRASHWIN_SPACING * 3
                                            - CRASHBUTTON_HEIGHT * 3,
                            CRASHBUTTON_WIDTH,
                            CRASHBUTTON_HEIGHT,
                            3, DefaultDepth(dpy, scr),
                            InputOutput, DefaultVisual(dpy, scr),
                            CWBackPixel | CWBorderPixel | CWEventMask,
                            &xswa);

    xfs = XLoadQueryFont(dpy, "fixed"); /* should always be safe */
    xgcv.function = GXcopy;
    xgcv.foreground = black;
    xgcv.background = white;
    xgcv.font = xfs->fid;
    gc = XCreateGC(dpy, toplevel,
                   GCForeground | GCBackground | GCFont | GCFunction, &xgcv);

    crashwin_draw(gc, toplevel, button1, button2, button3,
                  xfs->ascent + xfs->descent, xfs->max_bounds.width);
    XMapRaised(dpy, toplevel);
    XMapRaised(dpy, button1);
    XMapRaised(dpy, button2);
    XMapRaised(dpy, button3);
                                 
    for (;;) {
        crashwin_draw(gc, toplevel, button1, button2, button3,
                      xfs->ascent + xfs->descent, xfs->max_bounds.width);
        XNextEvent(dpy, &ev);

        if (ev.xany.type == ButtonPress) {
            if (ev.xbutton.window == button3) {
                execlp(argv0, argv0, NULL);
                _exit(1);
            } else if (ev.xbutton.window == button2) {
                _exit(1);
            } else if (ev.xbutton.window == button1) {
                execlp("twm", "twm", NULL);
                _exit(1);
            }
        }
    }
}

static void crashwin_draw(GC gc, Window toplevel, Window button1,
                          Window button2, Window button3,
                          int height, int width)
{
    int i;
    static char *message[] = {
        "Don't Panic!",
        "",
        "AHWM (your window manager) has crashed.",
        "",
        "Click on the top button to attempt to restart AHWM.",
        "This will hopefully allow you to continue where you left off.",
        "",
        "Click on the middle button to exit AHWM.",
        "This will allow you to restart your X session, but you",
        "may lose your work in any open applications.",
        "",
        "Click on the bottom button to start the TWM window manager.",
        "This will allow you to use TWM to save your work in",
        "any open applications and then exit your X session cleanly.",
        "",
        "Please send an email to hioreanu+ahwm@uchicago.edu.",
        "",
        "Seriously, please send me an email so I can fix this." 
    };
    
    XDrawString(dpy, button3, gc,
                CRASHBUTTON_WIDTH / 2 - strlen("Restart") * width / 2,
                CRASHBUTTON_HEIGHT / 2 + height / 2,
                "Restart", strlen("Restart"));
    XDrawString(dpy, button2, gc,
                CRASHBUTTON_WIDTH / 2 - strlen("Exit") * width / 2,
                CRASHBUTTON_HEIGHT / 2 + height / 2,
                "Exit", strlen("Exit"));
    XDrawString(dpy, button1, gc,
                CRASHBUTTON_WIDTH / 2 - strlen("TWM") * width / 2,
                CRASHBUTTON_HEIGHT / 2 + height / 2,
                "TWM", strlen("TWM"));
    for (i = 0; i < sizeof(message) / sizeof(char *); i++) {
        XDrawString(dpy, toplevel, gc, 5, 5 + height + i * height,
                    message[i], strlen(message[i]));
    }
}
