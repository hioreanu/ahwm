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

#ifndef XWM_H
#define XWM_H

#include "config.h"

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

#ifndef HAVE_STRDUP
char *strdup(char *s);
#endif

/* FIXME:  wrong place */
struct _arglist;
void run_program(XEvent *e, struct _arglist *args);
void xwm_quit(XEvent *e, struct _arglist *ignored);

#endif /* XWM_H */
