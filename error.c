/* $Id$
 * This was written by Alex Hioreanu in 2001.
 * This code is in the public domain and the author disclaims all
 * copyright privileges.
 */

#include <stdio.h>
#include "error.h"
#include "xwm.h"

/* table describing whether or not to ignore a specific error */
static unsigned char table[BadImplementation * X_NoOperation] = { 0 };

int (*error_default_handler)(Display *, XErrorEvent *);

int error_handler(Display *dpy, XErrorEvent *error)
{
    if (error->request_code > X_NoOperation ||
        error->error_code > BadImplementation ||
        table[BadImplementation*error->request_code+error->error_code] == 0) {
        fprintf(stderr, "XWM: ");
        fflush(stderr);
        (*error_default_handler)(dpy, error);
    }
#ifdef DEBUG
    printf("IGNORING X ERROR:  code = %d, request = %d\n",
           error->error_code, error->request_code);
    fflush(stdout);
#endif
    return 0;
}

void error_ignore(unsigned char error_code, unsigned char request_code)
{
    table[BadImplementation * request_code + error_code] = 1;
}

void error_unignore(unsigned char error_code, unsigned char request_code)
{
    table[BadImplementation * request_code + error_code] = 0;
}

