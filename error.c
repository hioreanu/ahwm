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

static void error_print(XErrorEvent *error)
{
    char error_text[128];
    char database_text[128];
    char request[16];

    XGetErrorText(dpy, error->error_code, error_text, sizeof(error_text));
    snprintf(request, 16, "%d", error->request_code);
    XGetErrorDatabaseText(dpy, "XRequest", request, "???",
                          database_text, sizeof(database_text));
    fprintf(stderr, "XWM: Received X Error: %s on %s for resource 0x%08X\n",
            error_text, database_text, (unsigned int)error->resourceid);
}

int error_handler(Display *dpy, XErrorEvent *error)
{
    if (error->request_code > X_NoOperation ||
        error->error_code > BadImplementation ||
        table[BadImplementation*error->request_code+error->error_code] == 0) {
        fflush(stderr);
        error_print(error);
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
