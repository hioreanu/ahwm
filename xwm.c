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
#include "keyboard.h"
#include "focus.h"

Display *dpy;
int scr;
int scr_height;
int scr_width;
unsigned long black;
unsigned long white;
Window root_window;
Cursor cursor_normal;
Cursor cursor_moving;
Cursor cursor_sizing;

int alt_tab(Window win, Window subwindow, Time t,
            int x, int y, int root_x, int root_y);
int alt_shift_tab(Window win, Window subwindow, Time t,
                  int x, int y, int root_x, int root_y);

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
 * 6.  Define the root window cursor and other cursors
 * 7.  Create a XContexts for the window management functions
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
    black = BlackPixel(dpy, scr);
    white = WhitePixel(dpy, scr);

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

    printf("--------------------------------");
    printf(" Welcome to xwm ");
    printf("--------------------------------\n");
        
//    XSetErrorHandler(error_handler);
    XSetErrorHandler(NULL);
    XSynchronize(dpy, True);

//    keyboard_grab_keys(root_window);
    cursor_normal = XCreateFontCursor(dpy, XC_left_ptr);
    cursor_moving = XCreateFontCursor(dpy, XC_fleur);
    cursor_sizing = XCreateFontCursor(dpy, XC_sizing);
    XDefineCursor(dpy, root_window, cursor_normal);

    window_context = XUniqueContext(); /* client.c */
    frame_context = XUniqueContext();

    keyboard_set_function("Alt | Tab", KEYBOARD_DEPRESS, alt_tab);
    keyboard_set_function("Alt | Shift | Tab", KEYBOARD_DEPRESS,
                          alt_shift_tab);

    scan_windows();

    printf("\tSetting root input focus...");
    XSetInputFocus(dpy, root_window, RevertToPointerRoot, CurrentTime);
    printf("ok\n");
    focus_ensure();             /* focus.c */
    
    xfd = ConnectionNumber(dpy);
    fcntl(xfd, F_SETFD, FD_CLOEXEC);

    XSync(dpy, 0);
    if (fork() == 0) {
        sleep(1);
        execlp("/usr/X11R6/bin/xterm", "xterm", NULL);
    }
    printf("\tInitialization complete\n");
    for (;;) {
        event_get(xfd, &event); /* event.c */
        event_dispatch(&event); /* event.c */
    }
    return 0;
}

/*
 * Set up all windows that were here before the windowmanager started
 */

static void scan_windows()
{
    int i, n;
    Window *wins, w1, w2;
    client_t *client;

    XQueryTree(dpy, root_window, &w1, &w2, &wins, &n);
    for (i = 0; i < n; i++) {
        client = client_create(wins[i]);  /* client.c */
        if (client != NULL && client->state == MAPPED) {
            if (client->frame != None) {
                keyboard_grab_keys(client->frame); /* keyboard.c */
            } else {
                keyboard_grab_keys(wins[i]); /* keyboard.c */
            }
        }
    }
    XFree(wins);
}

int alt_tab(Window win, Window subwindow, Time t,
            int x, int y, int root_x, int root_y)
{
    focus_next();
    focus_ensure();
    return 0;
}

int alt_shift_tab(Window win, Window subwindow, Time t,
                  int x, int y, int root_x, int root_y)
{
    focus_prev();
    focus_ensure();
    return 0;
}
