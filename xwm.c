/* $Id$
 * This was written by Alex Hioreanu in 2001.
 * This code is in the public domain and the author disclaims all
 * copyright privileges.
 */

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include "xwm.h"
#include "event.h"
#include "client.h"

Display *dpy;
int scr;
int scr_height;
int scr_width;
Window root_window;

static int already_running_windowmanager;

static void scan_windows();

static int tmp_error_handler(Display *dpy, XErrorEvent *error)
{
    already_running_windowmanager = 1;
    return -1;
}

static int error_handler(Display *dpy, XErrorEvent *error)
{
    printf("Caught some sort of error.\n");
    return -1;
}

/*
 * 1.  Open a display
 * 2.  Set up some convenience global variables
 * 3.  Select the X events we want to see
 * 4.  Die if we can't do that because some other windowmanager is running
 * 5.  Set the X error handler
 * 6.  Define the root window cursor
 * 7.  Create an XContext for the window management functions
 * 8.  Scan already-created windows and manage them
 * 9.  Go into a select() loop waiting for events
 * 10. Dispatch events
 * 
 * Should also probably set some properties on the root window....
 */

int main(int argc, char **argv)
{
    XEvent event;
    int    xfd;
    
    dpy = XOpenDisplay(NULL);
    if (dpy == NULL) {
        fprintf(stderr, "Could not open display '%s'\n", XDisplayName(NULL));
        exit(1);
    }
    scr = DefaultScreen(dpy);
    root_window = DefaultRootWindow(dpy);
    scr_height = DisplayHeight(dpy, scr);
    scr_width = DisplayWidth(dpy, scr);

    already_running_windowmanager = 0;
    XSetErrorHandler(tmp_error_handler);
    XSelectInput(dpy, root_window,
                 PropertyChangeMask | SubstructureRedirectMask |
                 SubstructureNotifyMask | KeyPressMask |
                 KeyReleaseMask);
    XSync(dpy, 0);
    if (already_running_windowmanager) {
        fprintf(stderr, "You're already running a window manager, silly.\n");
        exit(1);
    }
        
//    XSetErrorHandler(error_handler);
    XSetErrorHandler(NULL);
    XSynchronize(dpy, True);

    XDefineCursor(dpy, root_window, XCreateFontCursor(dpy, XC_left_ptr));

    window_context = XUniqueContext(); /* client.c */
    
    scan_windows();

    printf("Setting root input focus...");
    XSetInputFocus(dpy, root_window, RevertToNone, CurrentTime);
    printf("ok\n");
    focus_ensure();             /* focus.c */
    
    xfd = ConnectionNumber(dpy);
    fcntl(xfd, F_SETFD, FD_CLOEXEC);

    XSync(dpy, 0);
    for (;;) {
        event_get(xfd, &event); /* event.c */
        event_dispatch(&event); /* event.c */
    }
    return 0;
}

/*
 * for all windows
 *    if window does not have the override redirect flag set
 *        call client_create upon that window
 */

static void scan_windows()
{
    int i, n;
    Window *wins, w1, w2;

    XQueryTree(dpy, root_window, &w1, &w2, &wins, &n);
    for (i = 0; i < n; i++)
        client_create(wins[i]);  /* client.c */
    XFree(wins);
}

/*
 * ctwm:
                 ColormapChangeMask | EnterWindowMask |
                 PropertyChangeMask | SubstructureRedirectMask |
                 KeyPressMask | ButtonPressMask | ButtonReleaseMask |
                 StructureNotifyMask
 * windowmaker:
                 LeaveWindowMask | EnterWindowMask |
                 PropertyChangeMask | SubstructureNotifyMask |
                 PointerMotionMask | SubstructureRedirectMask |
                 KeyPressMask | KeyReleaseMask
 * wm2, 9wm:
                 SubstructureRedirectMask | SubstructureNotifyMask |
                 ColormapChangeMask | ButtonPressMask |
                 ButtonReleaseMask | PropertyChangeMask
 */
