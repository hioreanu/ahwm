/* $Id$
 * This was written by Alex Hioreanu in 2001.
 * This code is in the public domain and the author disclaims all
 * copyright privileges.
 */

#ifndef XWM_H
#define XWM_H

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/cursorfont.h>
/* #include <X11/extensions/shape.h> */

/* these globals are used throughout - only one screen is supported */

extern Display *dpy;
extern int scr;
extern int scr_height;
extern int scr_width;
extern unsigned long black;
extern unsigned long white;
extern Window root_window;

extern Cursor cursor_normal;
extern Cursor cursor_moving;
extern Cursor cursor_sizing;

#endif /* XWM_H */
