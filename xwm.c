/* $Id$
 * This was written by Alex Hioreanu in 2001.
 * This code is in the public domain and the author disclaims all
 * copyright privileges.
 */

/*
 * If you're just starting here, you may want to read the header files
 * before the implementation files when reading my code.
 */

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#include "xwm.h"
#include "event.h"
#include "client.h"
#include "keyboard.h"
#include "focus.h"
#include "cursor.h"
#include "mouse.h"
#include "move-resize.h"
#include "error.h"
#include "kill.h"
#include "workspace.h"
#include "icccm.h"
#include "ewmh.h"

#ifdef SHAPE
#include <X11/extensions/shape.h>
#endif

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

void alt_tab(XEvent *, void *arg);
void alt_shift_tab(XEvent *, void *arg);
void run_program(XEvent *e, void *arg);
void mark(XEvent *e, void *arg);

static int already_running_windowmanager;

static void scan_windows();

static int tmp_error_handler(Display *dpy, XErrorEvent *error)
{
    already_running_windowmanager = 1;
    return -1;
}

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
    XEvent    event;
    int       xfd, junk;
    XGCValues xgcv;
    XColor    xcolor, junk2;
    
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
    XSelectInput(dpy, root_window, ROOT_EVENT_MASK);
    XSync(dpy, 0);
    if (already_running_windowmanager) {
        fprintf(stderr, "XWM: You're already running a window manager, silly.\n");
        exit(1);
    }

    printf("--------------------------------");
    printf(" Welcome to XWM ");
    printf("--------------------------------\n");

    /* get the default error handler and set the error handler */
    XSetErrorHandler(NULL);
    error_default_handler = XSetErrorHandler(error_handler);
#ifdef DEBUG
    XSynchronize(dpy, True);
#endif

    cursor_init();
    /* I would prefer not changing the root cursor at all (user can
     * already do that with 'xsetroot'), but it's extremely
     * distracting to have the cursor change when it simply hovers
     * over something, and we need to define a cursor for some of our
     * windows.  X does not provide any mechanism for getting a
     * window's cursor, so we define the root cursor for
     * consistency.
     * FIXME:  move into cursor_init() */
    XDefineCursor(dpy, root_window, cursor_normal);

    workspace_update_color();

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

    window_context = XUniqueContext();
    frame_context = XUniqueContext();
    title_context = XUniqueContext();

    icccm_init();
    ewmh_init();
    keyboard_init();

    keyboard_set_function("Control | Alt | Shift | l", KEYBOARD_DEPRESS,
                          mark, NULL);
    keyboard_set_function("Control | Alt | Shift | t", KEYBOARD_DEPRESS,
                          run_program, "xterm");
    keyboard_set_function("Control | Alt | Shift | n", KEYBOARD_DEPRESS,
                          run_program, "netscape");
    keyboard_set_function("Control | Alt | Shift | k", KEYBOARD_DEPRESS,
                          run_program, "konqueror");
    keyboard_set_function("Control | Alt | Shift | e", KEYBOARD_DEPRESS,
                          run_program, "emacs");
    keyboard_set_function("Alt | Tab", KEYBOARD_DEPRESS, focus_alt_tab, NULL);
    keyboard_set_function("Alt | Shift | Tab", KEYBOARD_DEPRESS,
                          focus_alt_tab, NULL);
    keyboard_set_function("Control | Alt | Shift | m", KEYBOARD_DEPRESS,
                          move_client, NULL);
    keyboard_set_function("Control | Alt | Shift | r", KEYBOARD_DEPRESS,
                          resize_client, NULL);
    keyboard_set_function("Control | Alt | 1", KEYBOARD_DEPRESS,
                          workspace_client_moveto, (void *)1);
    keyboard_set_function("Control | Alt | 2", KEYBOARD_DEPRESS,
                          workspace_client_moveto, (void *)2);
    keyboard_set_function("Control | Alt | 3", KEYBOARD_DEPRESS,
                          workspace_client_moveto, (void *)3);
    keyboard_set_function("Control | Alt | 4", KEYBOARD_DEPRESS,
                          workspace_client_moveto, (void *)4);
    keyboard_set_function("Control | Alt | 5", KEYBOARD_DEPRESS,
                          workspace_client_moveto, (void *)5);
    keyboard_set_function("Control | Alt | 6", KEYBOARD_DEPRESS,
                          workspace_client_moveto, (void *)6);
    keyboard_set_function("Control | Alt | 7", KEYBOARD_DEPRESS,
                          workspace_client_moveto, (void *)7);
    keyboard_set_function("Alt | 1", KEYBOARD_DEPRESS,
                          workspace_goto, (void *)1);
    keyboard_set_function("Alt | 2", KEYBOARD_DEPRESS,
                          workspace_goto, (void *)2);
    keyboard_set_function("Alt | 3", KEYBOARD_DEPRESS,
                          workspace_goto, (void *)3);
    keyboard_set_function("Alt | 4", KEYBOARD_DEPRESS,
                          workspace_goto, (void *)4);
    keyboard_set_function("Alt | 5", KEYBOARD_DEPRESS,
                          workspace_goto, (void *)5);
    keyboard_set_function("Alt | 6", KEYBOARD_DEPRESS,
                          workspace_goto, (void *)6);
    keyboard_set_function("Alt | 7", KEYBOARD_DEPRESS,
                          workspace_goto, (void *)7);
    mouse_set_function("Alt | Button1", MOUSE_DEPRESS, MOUSE_FRAME,
                       move_client, NULL);
    mouse_set_function("Alt | Button3", MOUSE_DEPRESS, MOUSE_FRAME,
                       resize_client, NULL);
    mouse_set_function("Button1", MOUSE_DEPRESS, MOUSE_TITLEBAR,
                       move_client, NULL);
    mouse_set_function("Button2", MOUSE_DEPRESS, MOUSE_TITLEBAR,
                       kill_nicely, NULL);
    mouse_set_function("Control | Button2", MOUSE_DEPRESS, MOUSE_TITLEBAR,
                       kill_with_extreme_prejudice, NULL);
    mouse_set_function("Button3", MOUSE_RELEASE, MOUSE_TITLEBAR,
                       resize_maximize, NULL);
    keyboard_set_function("Control | Alt | Shift | q", KEYBOARD_RELEASE,
                          keyboard_quote, NULL);

    WM_STATE = XInternAtom(dpy, "WM_STATE", False);
    WM_CHANGE_STATE = XInternAtom(dpy, "WM_CHANGE_STATE", False);
    WM_TAKE_FOCUS = XInternAtom(dpy, "WM_TAKE_FOCUS", False);
    WM_SAVE_YOURSELF = XInternAtom(dpy, "WM_SAVE_YOURSELF", False);
    WM_DELETE_WINDOW = XInternAtom(dpy, "WM_DELETE_WINDOW", False);
    WM_PROTOCOLS = XInternAtom(dpy, "WM_PROTOCOLS", False);

#ifdef SHAPE
    shape_supported = XShapeQueryExtension(dpy, &shape_event_base, &junk);
#endif
    
    scan_windows();

    XSetInputFocus(dpy, root_window, RevertToPointerRoot, CurrentTime);
    focus_ensure(CurrentTime);
    
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

void run_program(XEvent *e, void *arg)
{
    /* FIXME: reap children */
    if (fork() == 0) {
/*        execlp(arg, arg, NULL); */
        system((char *)arg);
        _exit(0);
    }
}

/* make it easier to parse debug output */
void mark(XEvent *e, void *arg)
{
    printf("-------------------------------------");
    printf(" MARK ");
    printf("-------------------------------------\n");
}
