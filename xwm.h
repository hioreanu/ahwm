/* $Id$
 * This was written by Alex Hioreanu in 2001.
 * This code is in the public domain and the author disclaims all
 * copyright privileges.
 */

#ifndef XWM_H
#define XWM_H

#include <X11/Xlib.h>
#include <X11/Xutil.h>

/* the events we usually listen for on the root window */
#define ROOT_EVENT_MASK PropertyChangeMask | SubstructureRedirectMask | \
                        SubstructureNotifyMask | KeyPressMask |         \
                        KeyReleaseMask

extern Display *dpy;            /* standard in any X program */
extern int scr;                 /* only one screen supported */
extern int scr_height;          /* from DisplayHeight() */
extern int scr_width;           /* from DisplayWidth() */
extern unsigned long black;     /* black pixel */
extern unsigned long white;     /* white pixel */
extern Window root_window;      /* root window (only one screen supported) */
extern GC root_white_fg_gc;     /* GC with white foreground */
extern GC root_black_fg_gc;     /* GC with black foreground */
extern GC root_invert_gc;       /* GC with GXxor as function */
extern GC extra_gc1;            /* GC which changes */
extern GC extra_gc2;            /* GC which changes */
extern GC extra_gc3;            /* GC which changes */
extern XFontStruct *fontstruct; /* our font */
extern Atom WM_STATE;           /* various atoms used throughout */
extern Atom WM_CHANGE_STATE;
extern Atom WM_TAKE_FOCUS;
extern Atom WM_SAVE_YOURSELF;
extern Atom WM_DELETE_WINDOW;
extern Atom WM_PROTOCOLS;

#ifdef SHAPE
extern int shape_supported;
extern int shape_event_base;
#endif

/*
 * xwm.c also contains main(), which does the following:
 * 1.  Parse command line
 * 2.  Open X display
 * 3.  Select X event mask on root window
 * 4.  Barf if some other window manager is running
 * 5.  Initialize globals declared above
 * 6.  Call the various _init functions, in correct order
 * 7.  Create static key/mouse bindings
 * 8.  Scan already-mapped windows and manage them
 * 9.  Do the main event loop
 */

#endif /* XWM_H */
