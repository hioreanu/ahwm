/* $Id$
 * This was written by Alex Hioreanu in 2001.
 * This code is in the public domain and the author disclaims all
 * copyright privileges.
 */

#ifndef XWM_H
#define XWM_H

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#define ROOT_EVENT_MASK PropertyChangeMask | SubstructureRedirectMask | \
                        SubstructureNotifyMask | KeyPressMask |         \
                        KeyReleaseMask

/* these globals are used throughout - only one screen is supported */

extern Display *dpy;
extern int scr;
extern int scr_height;
extern int scr_width;
extern unsigned long black;
extern unsigned long white;
extern Window root_window;
extern GC root_white_fg_gc;
extern GC root_black_fg_gc;
extern GC root_invert_gc;
extern XFontStruct *fontstruct;
extern Atom WM_STATE;
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
