/* $Id$
 * This was written by Alex Hioreanu in 2001.
 * This code is in the public domain and the author disclaims all
 * copyright privileges.
 */

#ifndef ERROR_H
#define ERROR_H

#include <X11/Xlib.h>
#include <X11/Xproto.h>

/*
 * The default Xlib error handler (the one which pretty-prints the
 * error and calls exit(1)), defined in error.c, initialized in xwm.c
 */

extern int (*error_default_handler)(Display *, XErrorEvent *);

/*
 * Our X error handler - it can ignore certain errors depending upon
 * the error code and the request code, or it can simply call the
 * default handler (which will quit the application).  The default is
 * to call error_default_handler.
 */

int error_handler(Display *dpy, XErrorEvent *error);

/*
 * use to ignore a specific error code when sending a specific X
 * request
 */

void error_ignore(unsigned char error_code, unsigned char request_code);

/*
 * used to undo the above
 */

void error_unignore(unsigned char error_code, unsigned char request_code);

#endif /* ERROR_H */
