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
extern GC root_white_fg_gc;     /* a GC with white foreground */
extern GC root_black_fg_gc;     /* GC with black foreground */
extern GC root_invert_gc;       /* GC with GXxor as function */
extern GC extra_gc;             /* GC which is changed */
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

#endif /* XWM_H */
